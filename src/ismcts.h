#ifndef NARSET_ISMCTS_H
#define NARSET_ISMCTS_H

#include <stdint.h>
#include <math.h>
#include "engine.h"

#define DEBUG 1

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define EXPLORATION_WEIGHT 0.5
#define MAX_ACTIONS 15
#define LOGS_TO_PRECOMPUTE 30000
#define NODES_TO_PREALLOCATE 50000
#define STATES_TO_PREALLOCATE 50000

float logs[LOGS_TO_PRECOMPUTE];

typedef struct Node {
    int action;
    int current_player;

    int rewards, visits;

    struct Node *parent;
    struct Node *left_child;
    struct Node *right_sibling;

    int height;
    int unvisited_children, children;

    State* state;
} Node;

Node* preallocated_nodes;
State* preallocated_states;
int amount_of_nodes, amount_of_states;
int next_node, next_state;

int* act(State* state, Card* draft_options, int* player_choices);

#endif //NARSET_ISMCTS_H
