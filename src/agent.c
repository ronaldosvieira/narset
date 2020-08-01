#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#pragma ide diagnostic ignored "cert-err34-c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "agent.h"
#include "engine.h"
#include "ismcts.h"

void state_from_native_input(State* state) {
    Player *pl = state->current_player;
    Player *en = state->opposing_player;

    // read player info
    scanf("%d%d%d%d%*d", &pl->health, &pl->base_mana, &pl->deck_size,
            &pl->next_rune);
    pl->mana = pl->base_mana;

    // read opponent info
    scanf("%d%d%d%d%d", &en->health, &en->base_mana, &en->deck_size,
          &en->next_rune, &en->bonus_draw);
    en->bonus_draw -= 1; // adjust to our meaning of bonus draw
    en->mana = en->base_mana;

    // read amount of cards in opponent's hand and past actions
    int op_actions;
    scanf("%d%d", &en->hand_size, &op_actions); fgetc(stdin);
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

        scanf("%d%d%d%d%d%d%d%s%d%d%d%d", &card.id,
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

char* action_index_to_native_action(State* state, int action_index) {
    Action action = decode_action(action_index);

    char *type;
    int origin, target;
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
            origin = state->player_board[action.origin].instance_id;

            if (action.target == NONE) target = action.target;
            else target = state->opp_board[action.target].instance_id;

            break;
    }

    char* native_action = malloc(25 * sizeof(char));
    snprintf(native_action, 25, "%s %d %d", type, origin, target);

    return native_action;
}

double* encode_state(State* state, double* input_matrix) {
    for (int i = 0; i < 3; i++) {
        Card* card = &state->player_hand[i];

        int card_defense = card->defense >= -12? card->defense : -12;

        input_matrix[i * 16 + 0] = card->type == 0? 1. : 0.;
        input_matrix[i * 16 + 1] = card->type == 1? 1. : 0.;
        input_matrix[i * 16 + 2] = card->type == 2? 1. : 0.f;
        input_matrix[i * 16 + 3] = card->type == 3? 1. : 0.;
        input_matrix[i * 16 + 4] = (float) card->cost / 12.;
        input_matrix[i * 16 + 5] = (float) card->attack / 12.;
        input_matrix[i * 16 + 6] = (float) card_defense / 12.;
        input_matrix[i * 16 + 7] = (float) card->player_hp / 12.;
        input_matrix[i * 16 + 8] = (float) card->enemy_hp / 12.;
        input_matrix[i * 16 + 9] = (float) card->card_draw / 2.;
        input_matrix[i * 16 + 10] = (float) has_keyword(*card, BREAKTHROUGH);
        input_matrix[i * 16 + 11] = (float) has_keyword(*card, CHARGE);
        input_matrix[i * 16 + 12] = (float) has_keyword(*card, DRAIN);
        input_matrix[i * 16 + 13] = (float) has_keyword(*card, GUARD);
        input_matrix[i * 16 + 14] = (float) has_keyword(*card, LETHAL);
        input_matrix[i * 16 + 15] = (float) has_keyword(*card, WARD);
    }

    return input_matrix;
}

int main() {
    srandom(clock());

    State *state;

    while (TRUE) {
        // keep track of time
        clock_t start_time = clock();

        // read game state
        state = new_state();
        state_from_native_input(state);

        //printf("Received state:\n");
        //state_to_native_input(state);

        if (state->current_player->base_mana == 0) { // if it's draft
            printf("PICK %d\n", (int) random() % 3); fflush(stdout);
        } else { // if it's battle
            int *actions = act(state);

            int i;
            for (i = 0; i < MAX_ACTIONS && actions[i] != 0; i++) {
                char *native_action = action_index_to_native_action(state, actions[i]);
                printf("%s; ", native_action); fflush(stdout);
                free(native_action);

                act_on_state(state, actions[i]);
            }

            if (i == 0) {
                printf("PASS"); fflush(stdout);
            }

            printf("\n"); fflush(stdout);
        }

        debug_print("Total time elapsed: %.3f seg\n",
                    (double) (clock() - start_time) / CLOCKS_PER_SEC);

        free(state);
    }
}
#pragma clang diagnostic pop