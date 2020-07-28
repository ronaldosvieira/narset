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

    Player *pl = &state->players[0];
    Player *en = &state->players[1];

    // read player info
    scanf("%d%d%d%d%*d", &pl->health, &pl->mana, &pl->deck,
            &pl->next_rune);

    // read opponent info
    scanf("%d%d%d%d%d", &en->health, &en->mana, &en->deck,
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

        scanf("%d%d%d%d%d%d%d%s%d%d%d%d", &card->id, &card->instance_id,
              &card->location, &card->type, &card->cost, &card->attack,
              &card->defense, abilities, &card->player_hp, &card->enemy_hp,
              &card->card_draw, &card->lane);

        for (int j = 0; j < 6; j++)
            if (abilities[j] != '-')
                add_keyword(card, j);

        int card_position = -1;

        // get position to add new card and increment appropriate counter
        if (card->location == 0)
            card_position = P0_HAND + state->cards_in_hand++;
        else if (card->location == 1) {
            if (card->lane == 0)
                card_position = P0_BOARD + LEFT_LANE + state->cards_in_left_lane++;
            else if (card->lane == 1)
                card_position = P0_BOARD + RIGHT_LANE + state->cards_in_right_lane++;
        } else if (card->location == -1) {
            if (card->lane == 0)
                card_position = P1_BOARD + LEFT_LANE + state->cards_in_opp_left_lane++;
            else if (card->lane == 1)
                card_position = P1_BOARD + RIGHT_LANE + state->cards_in_opp_right_lane++;
        }

        state->cards[card_position] = *card;
    }

    return state;
}

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    State *state = state_from_native_input();

    Player *p = &state->players[0];
    Player *op = &state->players[1];

    printf("%d %d\n", state->turn, state->current_player);
    printf("%d %d %d %d %d\n", state->cards_in_hand, state->cards_in_left_lane,
            state->cards_in_right_lane, state->cards_in_opp_left_lane,
            state->cards_in_opp_right_lane);
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
}
#pragma clang diagnostic pop