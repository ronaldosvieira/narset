#include "engine.h"

#include <stdio.h>
#include <stdlib.h>

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

        _cards[i].instance_id = next_instance_id();

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