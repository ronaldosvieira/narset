#ifndef NARSET_ISMCTS_H
#define NARSET_ISMCTS_H

#include <stdint.h>
#include <math.h>
#include "engine.h"

#define DEBUG 0

#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define EXPLORATION_WEIGHT 0.5
#define MAX_ACTIONS 15
#define LOGS_TO_PRECOMPUTE 1000
#define NODES_TO_PREALLOCATE 100000

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
} Node;

Node* preallocated_nodes;
int amount_of_nodes;
int next_node;

int* act(State* state);

#endif //NARSET_ISMCTS_H
