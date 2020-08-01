#ifndef NARSET_ENGINE_H
#define NARSET_ENGINE_H

#define FALSE 0
#define TRUE 1
#define NONE -1
#define UNKNOWN -99

typedef int Bool;

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
#define LEFT_LANE 0
#define RIGHT_LANE 3

typedef int Location;

#define BREAKTHROUGH 0
#define CHARGE 1
#define DRAIN 2
#define GUARD 3
#define LETHAL 4
#define WARD 5

typedef unsigned char Keyword;

typedef struct Card {
    int id, instance_id;
    CardType type;
    int cost, attack, defense, player_hp, enemy_hp, card_draw, lane;
    unsigned char keywords;
    Location location;
    Bool can_attack;
} Card;

typedef struct Player {
    int id;
    int health, base_mana, bonus_mana, mana, next_rune, bonus_draw;
    int deck_size, hand_size, left_lane_size, right_lane_size;
} Player;

# define MAX_CARDS_HAND 8
# define MAX_CARDS_LANE 3
# define MAX_CARDS_BOARD 6
# define CARDS_IN_STATE ((MAX_CARDS_HAND + MAX_CARDS_BOARD) * 2)
# define CARDS_IN_DECK 30

#define SUMMON_START_INDEX 1
#define USE_START_INDEX 17
#define ATTACK_START_INDEX 73

typedef struct State {
    int round;
    Player *current_player, *opposing_player;

    Player players[2];

    Card cards[CARDS_IN_STATE];
    Card decks[2][CARDS_IN_DECK];

    Card *player_hand, *opp_hand;
    Card *player_board, *opp_board;

    Bool valid_actions[97];

    int winner;
} State;

/* Card functions */
Bool has_keyword(Card card, Keyword keyword);
void add_keyword(Card* card, Keyword keyword);
void remove_keyword(Card* card, Keyword keyword);
int damage_creature(Card* creature, int amount);

/* Player functions */
void init_player(Player* player, int id);
int damage_player(Player* player, int amount);

/* State functions */
State* new_state();
int calculate_valid_actions(State* state);
void act_on_state(State* state, int action_index);
State* copy_state(State* state, State* copied_state);

/* Util functions */
Action decode_action(int action);

#endif //NARSET_ENGINE_H