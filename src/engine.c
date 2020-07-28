#pragma clang diagnostic push
#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

unsigned int has_keyword(Card card, Keyword keyword) {
    return card.keywords >> keyword & (unsigned int) 1;
}

void add_keyword(Card* card, Keyword keyword) {
    card->keywords |= (unsigned int) 1 << keyword;
}

void remove_keyword(Card* card, Keyword keyword) {
    card->keywords &= ~((unsigned int) 1 << keyword);
}

Card* copy_card(Card card) {
    Card *copied_card = malloc(sizeof(Card));

    memcpy(copied_card, &card, sizeof(Card));

    return copied_card;
}

/* Player methods */
void init_player(Player* player, int id) {
    player->id = id;
    player->health = 30;
    player->base_mana = 0;
    player->bonus_mana = 0;
    player->mana = 0;
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

    init_player(&state->player, 0);
    init_player(&state->enemy, 1);

    for (int i = 0; i < CARDS_IN_STATE; i++)
        state->cards[i].instance_id = -1;

    state->winner = -1;
}

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    State state;
    init_state(&state);
}
#pragma clang diagnostic pop