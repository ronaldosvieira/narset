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
#define P0_BOARD 8
#define P1_BOARD 14
#define P1_HAND 20

typedef int Location;

#define LEFT_LANE 0
#define RIGHT_LANE 3

typedef int Lane;

#define BREAKTHROUGH 0
#define CHARGE 1
#define DRAIN 2
#define GUARD 3
#define LETHAL 4
#define WARD 5

typedef unsigned int Keyword;

typedef struct Card {
    int id, instance_id;
    CardType type;
    int cost, attack, defense, player_hp, enemy_hp, card_draw;
    unsigned int keywords;
    Location location; Lane lane;
    int can_attack;
} Card;

typedef struct Player {
    int id;
    int health;
    int base_mana;
    int bonus_mana;
    int mana;
    int deck;
    int next_rune;
    int bonus_draw;
} Player;

# define MAX_CARDS_HAND 8
# define MAX_CARDS_LANE 6
# define CARDS_IN_STATE (MAX_CARDS_HAND + MAX_CARDS_LANE * 4)

typedef struct State {
    int turn;
    int current_player;

    Player player;
    Player opponent;

    Card cards[CARDS_IN_STATE];

    int winner;
} State;

/* Card methods */
unsigned int has_keyword(Card card, Keyword keyword);
void add_keyword(Card* card, Keyword keyword);
void remove_keyword(Card* card, Keyword keyword);
Card* copy_card(Card *card);

/* Player methods */
void init_player(Player* player, int id);
void damage_player(Player* player, int amount);

/* State methods */
void init_state(State* state);
Action* get_available_actions(State* state);
void act_on_state(State* state, Action* action);
State* copy_state(State* state);
State* state_from_native_input(char* input);

#endif //NARSET_ENGINE_H