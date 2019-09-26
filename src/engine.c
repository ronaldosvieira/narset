#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

int instance_counter = 0;
Card _cards[160];

/* Card methods */
int next_instance_id() {
    return ++instance_counter;
}

void load_cards() {
    FILE* card_list;

    card_list = fopen("../cardlist.txt", "r");

    if (card_list == NULL) {
        printf("Error: cardlist.txt could not be opened.\n");
        exit(EXIT_FAILURE);
    }

    char* format = "%d ; %d ; %d ; %d ; %d ; %6s ; %d ; %d ; %d\n";

    for (int i = 0; i < 160; i++) {
        char card_keywords[7];

        _cards[i].instance_id = -1;

        fscanf(card_list, format, &_cards[i].id, &_cards[i].type, &_cards[i].cost,
                &_cards[i].attack, &_cards[i].defense, card_keywords,
                &_cards[i].player_hp, &_cards[i].enemy_hp, &_cards[i].card_draw);

        _cards[i].keywords = 0;

        for (int j = 0; j < 6; j++) {
            if (card_keywords[j] != '-') {
                _cards[i].keywords += 1 << j; // NOLINT(hicpp-signed-bitwise)
            }
        }
    }

    fclose(card_list);
}

int has_keyword(Card card, Keyword keyword) {
    return card.keywords >> keyword & 1; // NOLINT(hicpp-signed-bitwise)
}

void add_keyword(Card* card, Keyword keyword) {
    card->keywords |= 1 << keyword; // NOLINT(hicpp-signed-bitwise)
}

void remove_keyword(Card* card, Keyword keyword) {
    card->keywords &= ~(1 << keyword); // NOLINT(hicpp-signed-bitwise)
}

Card* copy_card(Card card) {
    Card *copied_card = malloc(sizeof(Card));

    memcpy(copied_card, &card, sizeof(Card));

    return copied_card;
}

/* Player methods */
void init_player(Player* player, int id) {
    player->id = 0;
    player->health = 30;
    player->base_mana = 0;
    player->bonus_mana = 0;
    player->mana = 0;
    player->next_rune = 25;
    player->bonus_draw = 0;
}

void damage_player(Player* player, int amount) {
    player->health -= amount;

    while (player->health <= player->next_rune) {
        player->next_rune -= 5;
        player->bonus_draw += 1;
    }
}

void new_draft(State* state) {
    int all_cards[160];

    // fill array with numbers from 0 to 159
    for (int i = 0; i < 160; i++) all_cards[i] = i;

    // shuffle array
    for (int i = 0; i < 160; i++) {
        int temp = all_cards[i];
        int random_pos = rand() % 160;

        all_cards[i] = all_cards[random_pos];
        all_cards[random_pos] = temp;
    }

    int pool[60];

    // get first 60 elements
    memcpy(pool, all_cards, sizeof(int) * 60);

    // fill all draft choices
    for (int i = 0; i < 30; i++) {
        int first = rand() % 60;
        int second = rand() % 60;
        int third = rand() % 60;

        while (second == first)
            second = rand() % 60;

        while (third == first || third == second)
            third = rand() % 60;

        state->draft[i][0] = _cards[pool[first]];
        state->draft[i][1] = _cards[pool[second]];
        state->draft[i][2] = _cards[pool[third]];
    }
}

int main() {
    struct timeval time;
    gettimeofday(&time, NULL);

    srand((time.tv_sec * 1000) + (time.tv_usec / 1000));

    load_cards();
}