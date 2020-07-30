#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "cert-err34-c"
#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

Bool has_keyword(Card card, Keyword keyword) {
    return (card.keywords >> keyword) & (unsigned char) 1;
}

void add_keyword(Card* card, Keyword keyword) {
    card->keywords |= (unsigned char) 1 << keyword;
}

void remove_keyword(Card* card, Keyword keyword) {
    card->keywords &= ~((unsigned int) 1 << keyword);
}

int8 damage_creature(Card* creature, int8 amount) {
    if (amount <= 0) return (int8) 0;

    if (has_keyword(*creature, WARD)) {
        remove_keyword(creature, WARD);

        return (int8) 0;
    }

    creature->defense -= amount;

    return amount;
}

Card* copy_card(Card *card) {
    Card *copied_card = malloc(sizeof(Card));

    memcpy(copied_card, card, sizeof(Card));

    return copied_card;
}

/* Player methods */
void init_player(Player* player, int id) {
    player->id = id;
    player->health = 30;
    player->base_mana = 0;
    player->bonus_mana = 0;
    player->mana = 0;
    player->deck = 30;
    player->next_rune = 25;
    player->bonus_draw = 0;
}

int8 damage_player(Player* player, int8 amount) {
    player->health -= amount;

    while (player->health <= player->next_rune
            && player->next_rune > 0) {
        player->next_rune -= 5;
        player->bonus_draw += 1;
    }

    return amount;
}

void init_state(State* state) {
    state->turn = 1;

    init_player(&state->players[0], 0);
    init_player(&state->players[1], 1);

    state->current_player = &state->players[0];
    state->opposing_player = &state->players[1];

    for (int i = 0; i < CARDS_IN_STATE; i++)
        state->cards[i].id = -1;

    state->valid_actions[0] = NONE;

    state->winner = -1;
}

void calculate_valid_actions(State* state) {
    // initialize shortcuts
    Player *player = state->current_player;
    Player *opponent = state->opposing_player;
    Bool *actions = state->valid_actions;
    Card *player_hand = &state->cards[player->id == 0? P0_HAND : P1_HAND];
    Card *player_board = &state->cards[player->id == 0? P0_BOARD : P1_BOARD];
    Card *opp_board = &state->cards[player->id == 0? P1_BOARD : P0_BOARD];

    // valid actions have already been calculated
    if (actions[0] != NONE) return;

    // passing is always allowed
    actions[0] = TRUE;

    // control variables
    int left_lane_is_full = player->left_lane >= MAX_CARDS_LANE;
    int right_lane_is_full = player->right_lane >= MAX_CARDS_LANE;

    // check actions available for each card in hand
    for (int i = 0; i < player->hand; i++) {
        Card *card = &player_hand[i];

        // check if player has mana to cast it
        if (card->cost > player->mana)
            continue; // if not, ignores the card

        // enable appropriated actions for the card's type
        switch (card->type) {
            case CREATURE:
                if (!left_lane_is_full)
                    actions[SUMMON_START_INDEX + i * 2] = TRUE;

                if (!right_lane_is_full)
                    actions[SUMMON_START_INDEX + i * 2 + 1] = TRUE;

                break;

            case GREEN_ITEM:
                for (int j = 0; j < player->left_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 1 + j] = TRUE;

                for (int j = 0; j < player->right_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 4 + j] = TRUE;

                break;

            case BLUE_ITEM:
                actions[USE_START_INDEX + i * 13 + 0] = TRUE;

            case RED_ITEM:
                for (int j = 0; j < opponent->left_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 7 + j] = TRUE;

                for (int j = 0; j < opponent->right_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 10 + j] = TRUE;

                break;
        }
    }

    int has_guard_in_left_lane = FALSE;
    int has_guard_in_right_lane = FALSE;

    // check if there are guards in the left lane
    for (int j = 0; j < opponent->left_lane; j++)
        if (has_keyword(opp_board[LEFT_LANE + j], GUARD))
            has_guard_in_left_lane = TRUE;

    // check if there are guards in the right lane
    for (int j = 0; j < opponent->right_lane; j++)
        if (has_keyword(opp_board[RIGHT_LANE + j], GUARD))
            has_guard_in_right_lane = TRUE;

    // check attacks available for each creature in the left lane
    for (int i = 0; i < player->left_lane; i++) {
        if (!player_board[LEFT_LANE + i].can_attack)
            continue;

        for (int j = 0; j < opponent->left_lane; j++) {
            Card *creature = &opp_board[LEFT_LANE + j];

            if (!has_guard_in_left_lane || has_keyword(*creature, GUARD))
                actions[ATTACK_START_INDEX + i * 4 + 1 + j] = TRUE;
        }

            if (!has_guard_in_left_lane)
                actions[ATTACK_START_INDEX + i * 4 + 0] = TRUE;
    }

    // check attacks available for each creature in the right lane
    for (int i = 0; i < player->right_lane; i++) {
        if (!player_board[RIGHT_LANE + i].can_attack)
            continue;

        for (int j = 0; j < opponent->right_lane; j++) {
            Card *creature = &opp_board[RIGHT_LANE + j];

            if (!has_guard_in_right_lane || has_keyword(*creature, GUARD))
                actions[ATTACK_START_INDEX + (i + 3) * 4 + 1 + j] = TRUE;
        }

        if (!has_guard_in_right_lane)
            actions[ATTACK_START_INDEX + (i + 3) * 4 + 0] = TRUE;
    }
}

Action decode_action(uint8 action) {
    ActionType type = PASS;
    int8 origin = NONE;
    int8 target = NONE;

    if (action >= ATTACK_START_INDEX) {
        action -= ATTACK_START_INDEX;

        type = ATTACK;
        origin = action / 4;
        target = (action % 4) - 1; // if zero, then NONE (-1)
    } else if (action >= USE_START_INDEX) {
        action -= USE_START_INDEX;

        type = USE;
        origin = action / 13;
        target = (action % 13) - 1; // if zero, then NONE (-1)
    } else if (action >= SUMMON_START_INDEX) {
        action -= SUMMON_START_INDEX;

        type = SUMMON;
        origin = action / 2;
        target = action % 2;
    }

    return (Action) {.type = type, .origin = origin, .target = target};
}

void do_summon(State* state, int8 origin, int8 lane) {
    // initialize shortcut
    Player *player = state->current_player;
    Player *opponent = state->opposing_player;
    Card *player_hand = &state->cards[player->id == 0? P0_HAND : P1_HAND];
    Card *player_board = &state->cards[player->id == 0? P0_BOARD : P1_BOARD];

    // copy card
    Card creature = player_hand[origin];

    // spend mana
    state->current_player->mana -= creature.cost;

    // remove card from hand
    for (int i = origin + 1; i < player->hand; i++)
        player_hand[i - 1] = player_hand[i];

    player_hand[player->hand - 1].id = NONE;
    player->hand--;

    // if the creature has charge, let it attack immediately
    if (has_keyword(creature, CHARGE))
        creature.can_attack = TRUE;

    // add creature to board
    if (lane == 0)
        player_board[LEFT_LANE + player->left_lane++] = creature;
    else
        player_board[RIGHT_LANE + player->right_lane++] = creature;

    // trigger enter-the-board abilities
    player->bonus_draw += creature.card_draw;
    damage_player(player, -creature.player_hp);
    damage_player(opponent, -creature.enemy_hp);
}

void do_use(State* state, int8 origin, int8 target) {
    // initialize shortcuts
    Player *player = state->current_player;
    Player *opponent = state->opposing_player;
    Card *player_hand = &state->cards[player->id == 0? P0_HAND : P1_HAND];
    Card *player_board = &state->cards[player->id == 0? P0_BOARD : P1_BOARD];
    Card *opp_board = &state->cards[player->id == 0? P1_BOARD : P0_BOARD];

    // copy card
    Card item = player_hand[origin];

    // spend mana
    player->mana -= item.cost;

    // remove card from hand
    for (int i = origin + 1; i < player->hand; i++)
        player_hand[i - 1] = player_hand[i];

    player_hand[player->hand - 1].id = NONE;
    player->hand--;

    Card *creature;

    switch (item.type) {
        case GREEN_ITEM:
            creature = &player_board[target];

            creature->attack += item.attack; // increase attack
            creature->defense += item.defense; // increase defense
            creature->keywords |= item.keywords; // add keywords

            break;
        case BLUE_ITEM:
            if (target == NONE) {
                damage_player(opponent, -item.defense); // damage opponent

                break;
            } // otherwise treat as a red item
        case RED_ITEM:
            creature = &opp_board[target];

            creature->attack -= item.attack; // decrease attack
            damage_creature(creature, -item.defense); // decrease defense
            creature->keywords &= ~(creature->keywords & item.keywords); // remove keywords

            // ensure that the attack attribute is non-negative
            if (creature->attack < 0)
                creature->attack = 0;

            break;
    }

    // trigger enter-the-board abilities
    state->current_player->bonus_draw += item.card_draw;
    damage_player(player, -item.player_hp);
    damage_player(opponent, -item.enemy_hp);
}

void do_attack(State* state, int8 origin, int8 target) {
    // initialize shortcuts
    Player *player = state->current_player;
    Player *opponent = state->opposing_player;
    Card *player_board = &state->cards[player->id == 0? P0_BOARD : P1_BOARD];
    Card *opp_board = &state->cards[player->id == 0? P1_BOARD : P0_BOARD];
    Card *attacker = &player_board[origin];

    int8 damage_dealt;

    if (target == NONE) { // if target is the opponent
        damage_dealt = damage_player(opponent, attacker->attack);
    } else { // if target is a creature
        Card *defender = &opp_board[target];
        int8 old_target_defense = defender->defense;

        // deal damage to defender
        damage_dealt = damage_creature(defender, attacker->attack);
        if (has_keyword(*attacker, LETHAL)) defender->defense = 0;

        // deal damage to attacker
        damage_creature(attacker, defender->attack);
        if (has_keyword(*defender, LETHAL)) attacker->defense = 0;

        // deal damage to opponent if attacker has breakthrough
        int8 excess_damage = damage_dealt - old_target_defense;

        if (has_keyword(*attacker, BREAKTHROUGH) && excess_damage > 0)
            damage_player(opponent, excess_damage);
    }

    // heal player if attacker has drain
    if (has_keyword(*attacker, DRAIN))
        player->health += damage_dealt;

    // prevent the creature from attacking again this turn
    attacker->can_attack = FALSE;
}

void do_pass(State* state) {
    // todo: implement
}

int remove_dead_creatures(Card lane[], int board_size) {
    int dead_creatures = 0;
    int last_alive_creature = -1;

    for (int i = 0; i < board_size; i++) {
        if (lane[i].id == NONE)
            continue;

        if (lane[i].defense <= 0) {
            lane[i].id = NONE;
            dead_creatures++;
        } else {
            if (last_alive_creature != i - 1) {
                lane[last_alive_creature + 1] = lane[i];
                lane[i].id = NONE;
            }

            last_alive_creature = i;
        }
    }

    return dead_creatures;
}

void act_on_state(State* state, uint8 action_index) {
    // initialize shortcuts
    Player *player = state->current_player;
    Player *opponent = state->opposing_player;
    Card *player_board = &state->cards[player->id == 0? P0_BOARD : P1_BOARD];
    Card *opp_board = &state->cards[player->id == 0? P1_BOARD : P0_BOARD];

    // find action type, origin and target from index
    Action action = decode_action(action_index);

    // execute the action appropriately
    switch (action.type) {
        case SUMMON: do_summon(state, action.origin, action.target); break;
        case USE: do_use(state, action.origin, action.target); break;
        case ATTACK: do_attack(state, action.origin, action.target); break;
        default: do_pass(state); break;
    }

    // nullify valid actions array
    memset(state->valid_actions, (Bool) 0, sizeof(state->valid_actions));
    state->valid_actions[0] = NONE;

    // remove any dead creatures
    player->left_lane -= remove_dead_creatures(
            &player_board[LEFT_LANE], MAX_CARDS_LANE);
    player->right_lane -= remove_dead_creatures(
            &player_board[RIGHT_LANE], MAX_CARDS_LANE);
    opponent->left_lane -= remove_dead_creatures(
            &opp_board[LEFT_LANE], MAX_CARDS_LANE);
    opponent->right_lane -= remove_dead_creatures(
            &opp_board[RIGHT_LANE], MAX_CARDS_LANE);

    // declare a winner, if there's any
    if (state->players[0].health <= 0)
        state->winner = 1;
    else if (state->players[1].health <= 0)
        state->winner = 0;
}

State* copy_state(State* state) {
    State *copied_state = malloc(sizeof(State));

    memcpy(copied_state, state, sizeof(State));

    // update pointers
    int current_player_id = state->current_player->id;

    copied_state->current_player = &copied_state->players[current_player_id];
    copied_state->opposing_player = &copied_state->players[(current_player_id + 1) % 2];

    return copied_state;
}

#pragma clang diagnostic pop