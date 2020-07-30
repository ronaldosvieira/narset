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
    Card *opp_board = &state->cards[pl->id == 0? P1_BOARD : P0_BOARD];

    // read player info
    scanf("%hhd%hhd%hhd%hhd%*d", &pl->health, &pl->mana, &pl->deck,
            &pl->next_rune);

    // read opponent info
    scanf("%hhd%hhd%hhd%hhd%hhd", &en->health, &en->mana, &en->deck,
          &en->next_rune, &en->bonus_draw);
    en->bonus_draw -= 1; // adjust to our meaning of bonus draw

    // read amount of cards in opponent's hand and past actions
    int op_hand, op_actions;
    scanf("%d%d", &op_hand, &op_actions); fgetc(stdin);
    for (int i = 0; i < op_actions; i++) {
        char card_number_and_action[21];
        fgets(card_number_and_action, 21, stdin);
    }

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
            player_hand[state->current_player->hand++] = *card;
        else if (card->location == 1) {
            card->can_attack = TRUE;

            if (card->lane == 0)
                player_board[LEFT_LANE + state->current_player->left_lane++] = *card;
            else if (card->lane == 1)
                player_board[RIGHT_LANE + state->current_player->right_lane++] = *card;
        } else if (card->location == -1) {
            card->can_attack = TRUE;

            if (card->lane == 0)
                opp_board[LEFT_LANE + state->opposing_player->left_lane++] = *card;
            else if (card->lane == 1)
                opp_board[RIGHT_LANE + state->opposing_player->right_lane++] = *card;
        }
    }

    return state;
}

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    State *state = state_from_native_input();

    Player *p = state->current_player;
    Player *op = state->opposing_player;

    printf("%d %d\n", state->round, state->current_player->id);
    printf("%d %d %d %d %d\n", p->hand, p->left_lane, p->right_lane,
           op->left_lane, op->right_lane);
    printf("%d %d %d %d %d\n", p->health, p->mana, p->deck,
            p->next_rune, p->bonus_draw);
    printf("%d %d %d %d %d\n", op->health, op->mana, op->deck,
            op->next_rune, op->bonus_draw);
    printf("%d %d %d\n", has_keyword(state->cards[1], GUARD),
            has_keyword(state->cards[1], CHARGE), state->cards[1].id);
    printf("%d\n", state->winner);

    calculate_valid_actions(state);

    for (int i = SUMMON_START_INDEX; i < SUMMON_START_INDEX + 16; ++i) {
        printf("%d ", state->valid_actions[i]);
    }
    printf("\n");

    for (int i = USE_START_INDEX; i < USE_START_INDEX + 104; ++i) {
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