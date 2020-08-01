#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "agent.h"
#include "engine.h"
#include "ismcts.h"

void state_from_native_input(State* state) {
    Player *pl = state->current_player;
    Player *en = state->opposing_player;

    // read player info
    scanf("%hhd%hhd%hhd%hhd%*d", &pl->health, &pl->base_mana, &pl->deck_size,
            &pl->next_rune);
    pl->mana = pl->base_mana;

    // read opponent info
    scanf("%hhd%hhd%hhd%hhd%hhd", &en->health, &en->base_mana, &en->deck_size,
          &en->next_rune, &en->bonus_draw);
    en->bonus_draw -= 1; // adjust to our meaning of bonus draw
    en->mana = en->base_mana;

    // read amount of cards in opponent's hand and past actions
    int op_actions;
    scanf("%hhd%d", &en->hand_size, &op_actions); fgetc(stdin);
    for (int i = 0; i < op_actions; i++) {
        char card_number_and_action[21];
        fgets(card_number_and_action, 21, stdin);
    }

    // populate opponent's hand with fake cards
    for (int i = 0; i < en->hand_size; i++)
        state->opp_hand[i] = (Card) {.id = UNKNOWN, .instance_id = UNKNOWN,
                                     .cost = 1, .attack = 1, .defense = 1};

    // populate decks with fake cards
    for (int i = 0; i < pl->deck_size; i++)
        state->decks[pl->id][i] = (Card) {.id = UNKNOWN, .instance_id = UNKNOWN,
                                          .cost = 1, .attack = 1, .defense = 1};

    for (int i = 0; i < en->deck_size; i++)
        state->decks[en->id][i] = (Card) {.id = UNKNOWN, .instance_id = UNKNOWN,
                                          .cost = 1, .attack = 1, .defense = 1};

    // read cards
    int card_count;
    scanf("%d", &card_count);
    for (int i = 0; i < card_count; i++) {
        Card card = {};
        char abilities[7];

        scanf("%d%d%hhd%hhd%hhd%hhd%hhd%s%hhd%hhd%hhd%hhd", &card.id,
                &card.instance_id, &card.location, &card.type, &card.cost,
                &card.attack, &card.defense, abilities, &card.player_hp,
                &card.enemy_hp, &card.card_draw, &card.lane);

        for (int j = 0; j < 6; j++)
            if (abilities[j] != '-')
                add_keyword(&card, j);

        // add new card and increment appropriate counter
        if (card.location == 0)
            state->player_hand[pl->hand_size++] = card;
        else if (card.location == 1) {
            card.can_attack = TRUE;

            if (card.lane == 0)
                state->player_board[LEFT_LANE + pl->left_lane_size++] = card;
            else if (card.lane == 1)
                state->player_board[RIGHT_LANE + pl->right_lane_size++] = card;
        } else if (card.location == -1) {
            card.can_attack = TRUE;

            if (card.lane == 0)
                state->opp_board[LEFT_LANE + en->left_lane_size++] = card;
            else if (card.lane == 1)
                state->opp_board[RIGHT_LANE + en->right_lane_size++] = card;
        }
    }
}

void state_to_native_input(State* state) {
    // initialize shortcuts
    Player *pl = state->current_player;
    Player *op = state->opposing_player;

    // print players info
    printf("%d %d %d %d %d\n", pl->health, pl->base_mana + pl->bonus_mana,
           pl->deck_size, pl->next_rune, 1 + pl->bonus_draw);
    printf("%d %d %d %d %d\n", op->health, op->base_mana + op->bonus_mana,
           op->deck_size, op->next_rune, 1 + op->bonus_draw);

    // print size of opponent hand and no previous actions
    printf("%d 0\n", op->hand_size);

    // print amount of cards
    printf("%d\n", pl->hand_size + pl->left_lane_size + pl->right_lane_size +
            op->left_lane_size + op->right_lane_size);

    // print cards in current player's hand
    for (int i = 0; i < pl->hand_size; i++) {
        Card *card = &state->player_hand[i];

        char abilities[7] = "BCDGLW\0";

        for (int j = 0; j < 6; j++)
            if (!has_keyword(*card, j))
                abilities[j] = '-';

        printf("%d %d 0 %d %d %d %d %s %d %d %d -1 \n", card->id, card->instance_id,
               card->type, card->cost, card->attack, card->defense, abilities,
               card->player_hp, card->enemy_hp, card->card_draw);
    }

    // print cards in current player's board
    for (int i = 0; i < 6; i++) {
        Card *card = &state->player_board[i];

        if (card->id == NONE)
            continue;

        char abilities[7] = "BCDGLW\0";

        for (int j = 0; j < 6; j++)
            if (!has_keyword(*card, j))
                abilities[j] = '-';

        printf("%d %d 1 %d %d %d %d %s %d %d %d %d \n", card->id, card->instance_id,
               card->type, card->cost, card->attack, card->defense, abilities,
               card->player_hp, card->enemy_hp, card->card_draw, card->lane);
    }

    // print cards in opposing player's board
    for (int i = 0; i < 6; i++) {
        Card *card = &state->opp_board[i];

        if (card->id == NONE)
            continue;

        char abilities[7] = "BCDGLW\0";

        for (int j = 0; j < 6; j++)
            if (!has_keyword(*card, j))
                abilities[j] = '-';

        printf("%d %d -1 %d %d %d %d %s %d %d %d %d \n", card->id, card->instance_id,
               card->type, card->cost, card->attack, card->defense, abilities,
               card->player_hp, card->enemy_hp, card->card_draw, card->lane);
    }
}

char* action_index_to_native_action(State* state, uint8 action_index) {
    // initialize shortcut
    Player *player = state->current_player;

    Action action = decode_action(action_index);

    char *type;
    int8 origin, target;
    switch (action.type) {
        case PASS: return "PASS";
        case SUMMON:
            type = "SUMMON";
            origin = state->player_hand[action.origin].instance_id;
            target = action.target;
            break;
        case USE:
            type = "USE";
            origin = state->player_hand[action.origin].instance_id;

            if (action.target == NONE) target = action.target;
            else {
                switch (state->player_hand[action.origin].type) {
                    case GREEN_ITEM:
                        target = state->player_board[action.target].instance_id;
                        break;
                    case RED_ITEM:
                    case BLUE_ITEM:
                        target = state->opp_board[action.target].instance_id;
                        break;
                }
            }
            break;
        case ATTACK:
            type = "ATTACK";
            origin = state->player_board[action.target].instance_id;

            if (action.target == NONE) target = action.target;
            else target = state->opp_board[action.target].instance_id;

            break;
    }

    char* native_action = malloc(25 * sizeof(char));
    snprintf(native_action, 25, "%s %d %d", type, origin, target);

    return native_action;
}

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srandom((time.tv_sec * 1000) + (time.tv_usec / 1000));
    //srandom(5);

    // initialize state
    State *state = new_state();
    state_from_native_input(state);
    calculate_valid_actions(state);

    printf("Received state:\n");
    state_to_native_input(state);

    printf("\nValid actions:\n");
    printf("PASS: %d\n", state->valid_actions[0]);

    printf("SUMMON: ");
    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("USE: ");
    for (int i = USE_START_INDEX; i < USE_START_INDEX + 56; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("ATTACK: ");
    for (int i = ATTACK_START_INDEX; i < ATTACK_START_INDEX + 24; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    int8 *actions = act(state);

    printf("Chosen actions: ");

    for (int i = 0; i < MAX_ACTIONS && actions[i] != 0; i++) {
        char *native_action = action_index_to_native_action(state, actions[i]);
        printf("%s; ", native_action);
        act_on_state(state, actions[i]);

        free(native_action);
    }

    printf("\n");

    free(state);
}
#pragma clang diagnostic pop