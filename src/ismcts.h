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

typedef struct Node {
    int8 action;
    int8 current_player;

    int rewards, visits;

    struct Node *parent;
    struct Node *left_child;
    struct Node *right_sibling;

    int height;
    int8 unvisited_children, children;
} Node;

int8* act(State* state);

#endif //NARSET_ISMCTS_H
