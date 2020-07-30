#pragma clang diagnostic push
#pragma ide diagnostic ignored "cert-err34-c"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "agent.h"
#include "engine.h"

State* state_from_native_input() {
    // initialize the state
    State *state = malloc(sizeof(State));
    init_state(state);

    Player *pl = state->current_player;
    Player *en = state->opposing_player;
    Card *player_hand = &state->cards[pl->id == 0? P0_HAND : P1_HAND];
    Card *player_board = &state->cards[pl->id == 0? P0_BOARD : P1_BOARD];
    Card *opp_hand = &state->cards[pl->id == 0? P1_HAND : P0_HAND];
    Card *opp_board = &state->cards[pl->id == 0? P1_BOARD : P0_BOARD];

    // read player info
    scanf("%hhd%hhd%hhd%hhd%*d", &pl->health, &pl->base_mana, &pl->deck,
            &pl->next_rune);
    pl->mana = pl->base_mana;

    // read opponent info
    scanf("%hhd%hhd%hhd%hhd%hhd", &en->health, &en->base_mana, &en->deck,
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

    // add fake cards to opponent's hand
    for (int i = 0; i < en->hand_size; i++)
        opp_hand[i] = (Card) {.id = UNKNOWN, .instance_id = UNKNOWN, .cost = 1};

    // read cards
    int card_count;
    scanf("%d", &card_count);
    for (int i = 0; i < card_count; i++) {
        Card *card = malloc(sizeof(Card));
        char abilities[7];

        scanf("%d%d%hhd%hhd%hhd%hhd%hhd%s%hhd%hhd%hhd%hhd", &card->id,
                &card->instance_id, &card->location, &card->type, &card->cost,
                &card->attack, &card->defense, abilities, &card->player_hp,
                &card->enemy_hp, &card->card_draw, &card->lane);

        for (int j = 0; j < 6; j++)
            if (abilities[j] != '-')
                add_keyword(card, j);

        // add new card and increment appropriate counter
        if (card->location == 0)
            player_hand[state->current_player->hand_size++] = *card;
        else if (card->location == 1) {
            card->can_attack = TRUE;

            if (card->lane == 0)
                player_board[LEFT_LANE + state->current_player->left_lane_size++] = *card;
            else if (card->lane == 1)
                player_board[RIGHT_LANE + state->current_player->right_lane_size++] = *card;
        } else if (card->location == -1) {
            card->can_attack = TRUE;

            if (card->lane == 0)
                opp_board[LEFT_LANE + state->opposing_player->left_lane_size++] = *card;
            else if (card->lane == 1)
                opp_board[RIGHT_LANE + state->opposing_player->right_lane_size++] = *card;
        }
    }

    return state;
}

void state_to_native_input(State* state) {
    // initialize shortcuts
    Player *pl = state->current_player;
    Player *op = state->opposing_player;
    Card *player_hand = &state->cards[pl->id == 0 ? P0_HAND : P1_HAND];
    Card *player_board = &state->cards[pl->id == 0 ? P0_BOARD : P1_BOARD];
    Card *opp_board = &state->cards[pl->id == 0 ? P1_BOARD : P0_BOARD];

    // print players info
    printf("%d %d %d %d %d\n", pl->health, pl->base_mana + pl->bonus_mana,
           pl->deck, pl->next_rune, 1 + pl->bonus_draw);
    printf("%d %d %d %d %d\n", op->health, op->base_mana + op->bonus_mana,
           op->deck, op->next_rune, 1 + op->bonus_draw);

    // print size of opponent hand and no previous actions
    printf("%d 0\n", op->hand_size);

    // print amount of cards
    printf("%d\n", pl->hand_size + pl->left_lane_size + pl->right_lane_size +
            op->hand_size + op->left_lane_size + op->right_lane_size);

    // print cards in current player's hand
    for (int i = 0; i < pl->hand_size; i++) {
        Card *card = &player_hand[i];

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
        Card *card = &player_board[i];

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
        Card *card = &opp_board[i];

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

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    State *state = state_from_native_input();

    state_to_native_input(state);

    calculate_valid_actions(state);

    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = USE_START_INDEX; i < USE_START_INDEX + 56; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = ATTACK_START_INDEX; i < ATTACK_START_INDEX + 24; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("%ld\n", sizeof(State));

    act_on_state(state, 1);

    state_to_native_input(state);

    calculate_valid_actions(state);

    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = USE_START_INDEX; i < USE_START_INDEX + 56; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = ATTACK_START_INDEX; i < ATTACK_START_INDEX + 24; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("%ld\n", sizeof(State));

    act_on_state(state, 73);

    state_to_native_input(state);

    calculate_valid_actions(state);

    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = USE_START_INDEX; i < USE_START_INDEX + 56; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = ATTACK_START_INDEX; i < ATTACK_START_INDEX + 24; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("%ld\n", sizeof(State));

    act_on_state(state, 0);

    state_to_native_input(state);

    calculate_valid_actions(state);

    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = USE_START_INDEX; i < USE_START_INDEX + 56; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = ATTACK_START_INDEX; i < ATTACK_START_INDEX + 24; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    printf("%ld\n", sizeof(State));
}
#pragma clang diagnostic pop