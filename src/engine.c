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

void damage_player(Player* player, int amount) {
    player->health -= amount;

    while (player->health <= player->next_rune
            && player->next_rune > 0) {
        player->next_rune -= 5;
        player->bonus_draw += 1;
    }
}

void init_state(State* state) {
    state->turn = 1;
    state->current_player = 0;

    init_player(&state->players[0], 0);
    init_player(&state->players[1], 1);

    for (int i = 0; i < CARDS_IN_STATE; i++)
        state->cards[i].instance_id = -1;

    state->valid_actions[0] = NONE;

    state->winner = -1;
}

void calculate_valid_actions(State* state) {
    Bool *actions = state->valid_actions;

    // valid actions have already been calculated
    if (actions[0] != NONE) return;

    // passing is always allowed
    actions[0] = TRUE;

    // control variables
    int left_lane_is_full = state->cards_in_left_lane >= MAX_CARDS_SINGLE_LANE;
    int right_lane_is_full = state->cards_in_right_lane >= MAX_CARDS_SINGLE_LANE;
    int current_player = state->current_player;

    // check actions available for each card in hand
    for (int i = 0; i < state->cards_in_hand; i++) {
        Card *card = &state->cards[P0_HAND + i];

        // check if player has mana to cast it
        if (card->cost > state->players[current_player].mana)
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
                for (int j = 0; j < state->cards_in_left_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 1 + j] = TRUE;

                for (int j = 0; j < state->cards_in_right_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 4 + j] = TRUE;

                break;

            case BLUE_ITEM:
                actions[USE_START_INDEX + i * 13 + 0] = TRUE;

            case RED_ITEM:
                for (int j = 0; j < state->cards_in_opp_left_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 7 + j] = TRUE;

                for (int j = 0; j < state->cards_in_opp_right_lane; j++)
                    actions[USE_START_INDEX + i * 13 + 10 + j] = TRUE;

                break;
        }
    }

    int has_guard_in_left_lane = FALSE;
    int has_guard_in_right_lane = FALSE;

    // check if there are guards in the left lane
    for (int j = 0; j < state->cards_in_opp_left_lane; j++)
        if (has_keyword(state->cards[P1_BOARD + LEFT_LANE + j], GUARD))
            has_guard_in_left_lane = TRUE;

    // check if there are guards in the right lane
    for (int j = 0; j < state->cards_in_opp_right_lane; j++)
        if (has_keyword(state->cards[P1_BOARD + RIGHT_LANE + j], GUARD))
            has_guard_in_right_lane = TRUE;

    // check attacks available for each creature in the left lane
    for (int i = 0; i < state->cards_in_left_lane; i++) {
        if (!state->cards[P0_BOARD + LEFT_LANE + i].can_attack)
            continue;

        for (int j = 0; j < state->cards_in_opp_left_lane; j++) {
            Card *creature = &state->cards[P1_BOARD + LEFT_LANE + j];

            if (!has_guard_in_left_lane || has_keyword(*creature, GUARD))
                actions[ATTACK_START_INDEX + i * 4 + 1 + j] = TRUE;
        }

            if (!has_guard_in_left_lane)
                actions[ATTACK_START_INDEX + i * 4 + 0] = TRUE;
    }

    // check attacks available for each creature in the right lane
    for (int i = 0; i < state->cards_in_right_lane; i++) {
        if (!state->cards[P0_BOARD + RIGHT_LANE + i].can_attack)
            continue;

        for (int j = 0; j < state->cards_in_opp_right_lane; j++) {
            Card *creature = &state->cards[P1_BOARD + RIGHT_LANE + j];

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
    // todo: implement
}

void do_use(State* state, int8 origin, int8 target) {
    // todo: implement
}
void do_attack(State* state, int8 origin, int8 target) {
    // todo: implement
}

void do_pass(State* state) {
    // todo: implement
}

void act_on_state(State* state, uint8 action_index) {
    Action action = decode_action(action_index);

    switch (action.type) {
        case SUMMON: do_summon(state, action.origin, action.target); break;
        case USE: do_use(state, action.origin, action.target); break;
        case ATTACK: do_attack(state, action.origin, action.target); break;
        default: do_pass(state); break;
    }

    state->valid_actions[0] = NONE;
}

State* copy_state(State* state) {
    State *copied_state = malloc(sizeof(State));

    memcpy(copied_state, state, sizeof(State));

    return copied_state;
}

#pragma clang diagnostic pop