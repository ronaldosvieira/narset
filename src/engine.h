#ifndef NARSET_ENGINE_H
#define NARSET_ENGINE_H

#define PASS 0
#define PICK 1
#define SUMMON 2
#define USE 3
#define ATTACK 4

typedef int ActionType;

typedef struct Action {
    ActionType type;
    int origin, target;
} Action;

#define CREATURE 0
#define GREEN_ITEM 1
#define RED_ITEM 2
#define BLUE_ITEM 3

typedef int CardType;

#define OUTSIDE -1
#define P0_HAND 0
#define P0_BOARD 1
#define P1_BOARD 2
#define P1_HAND 3

typedef int Location;

#define LEFT_LANE 0
#define RIGHT_LANE 0

typedef int Lane;

#define BREAKTHROUGH 0
#define CHARGE 1
#define DRAIN 2
#define GUARD 3
#define LETHAL 4
#define WARD 5

typedef int Keyword;

typedef struct Card {
    int id, instance_id;
    CardType type;
    int cost, attack, defense, player_hp, enemy_hp, card_draw;
    int keywords;
    Location location; Lane lane;
} Card;

typedef struct Player {
    int id;
    int health;
    int base_mana;
    int bonus_mana;
    int mana;
    int next_rune;
    int bonus_draw;
} Player;

typedef struct State {
    Player player;
    Player enemy;

    Card draft[3][30];
    Card cards[];
} State;

/* Card methods */
int next_instance_id();
void load_cards();

int has_keyword(Card card, Keyword keyword);
void add_keyword(Card* card, Keyword keyword);
void remove_keyword(Card* card, Keyword keyword);
Card* copy_card(Card card);

/* Player methods */
void init_player(Player* player, int id);
void damage_player(Player* player, int amount);

/* State methods */
State* new_state();

#endif //NARSET_ENGINE_H