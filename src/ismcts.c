#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "ismcts.h"

void precompute_log10s() {
    log10s[0] = 1;

    for (int i = 1; i < LOG10_TO_PRECOMPUTE; i++)
        log10s[i] = log10f((float) i);
}

double uct_score(Node* node, double exploration_weight) {
    float exploitation = (float) node->rewards / (float) node->visits;

    int parent_visits = node->parent != NULL? node->parent->visits : 0;
    float log_10_parent_visits = parent_visits < LOG10_TO_PRECOMPUTE?
            log10s[parent_visits] : log10f((float) parent_visits);

    float exploration = sqrtf(log_10_parent_visits / (float) node->visits);

    return exploitation + exploration_weight * exploration;
}

Node* select_with_uct(Node* node, double exploration_weight) {
    Node* best_child = NULL;
    double best_score = -1.0;

    Node* child = node->left_child;

    while (child != NULL) {
        double score = uct_score(child, exploration_weight);

        if (score > best_score) {
            best_score = score;
            best_child = child;
        }

        if (exploration_weight == 0)
            debug_print("stats %d -> %d: %d / %d = %.3f (height %d)\n",
                   node->action, child->action,
                   child->rewards, child->visits, score, child->height);

        child = child->right_sibling;
    }

    return best_child;
}

Node* select_node(Node* root, State* state) {
    Node* node = root;

    // while is not terminal and have no unexplored children, descend
    while (node->children > 0 && node->unvisited_children == 0) {
        node = select_with_uct(node, EXPLORATION_WEIGHT);
        act_on_state(state, node->action);
    }

    return node;
}

Node* expand(Node* node, int8 action) {
    Node* new_leaf = malloc(sizeof(Node));

    new_leaf->parent = node;
    new_leaf->height = node->height + 1;
    new_leaf->right_sibling = node->left_child;
    node->left_child = new_leaf;

    if (action != 0)
        new_leaf->current_player = node->current_player;
    else
        new_leaf->current_player = (node->current_player + 1) % 2;

    new_leaf->action = action;

    return new_leaf;
}

int simulate(State* state) {
    while (state->winner == NONE) {
        // chose a random action
        int8 valid_actions = calculate_valid_actions(state);
        int8 ith_action = random() % valid_actions;

        // find the i-th action
        int action = 0; int counter = 0;
        while (counter != ith_action)
            if (state->valid_actions[++action] == TRUE)
                counter++;

        // apply the action
        act_on_state(state, action);
    }

    return state->winner;
}

void backpropagate(Node* node, int reward, int current_player) {
    // update the visits and rewards of each node
    // until we reach the top of the tree
    while (node != NULL) {
        node->visits++;

        if (node->parent == NULL || node->parent->current_player == current_player)
            node->rewards += reward;
        else
            node->rewards += 1 - reward;

        node = node->parent;
    }
}

void do_rollout(Node* root, State* state) {
    // select a leaf and forwards the state accordingly
    Node* leaf = select_node(root, state);

    // if the leaf is not terminal, expand
    if (leaf->children > 0) {
        // find the i-th action
        calculate_valid_actions(state);
        int action = 0;

        if (leaf->unvisited_children-- > 1) {
            int counter = 0;
            while (counter != leaf->children - leaf->unvisited_children)
                if (state->valid_actions[++action] == TRUE)
                    counter++;
        }

        // expand the leaf, creating a new leaf
        leaf = expand(leaf, action);

        // apply the action
        act_on_state(state, action);

        int8 valid_actions = calculate_valid_actions(state);

        leaf->children = valid_actions;
        leaf->unvisited_children = valid_actions;
    }

    // simulate the remainder of the match and get the result
    int8 winner = simulate(state);
    int8 reward = winner == root->current_player? 1 : 0;

    // backpropagate the simulation result upwards in the tree
    backpropagate(leaf, reward, root->current_player);
}

void choose_best(Node* root, int8* actions) {
    Node* node = root;
    int i = 0;

    node = select_with_uct(node, 0);

    // while action is not to pass
    while (node != NULL && node->action != 0) {
        actions[i++] = node->action;
        node = select_with_uct(node, 0);
    }

    if (node != NULL)
        debug_print("stats: %d / %d \n", node->rewards, node->visits);

    actions[i] = 0;
}

int8* act(State* state) {
    if (log10s[0] != 1) precompute_log10s();

    int8 valid_actions = calculate_valid_actions(state);

    Node root = {.action = -1,
                 .current_player = state->current_player->id,
                 .children = valid_actions,
                 .unvisited_children = valid_actions,
                 .height = 0};

    for (int i = 0; i < 15000; i++) {
        State *state_copy = copy_state(state);

        // todo: determinize the state

        do_rollout(&root, state_copy);

        free(state_copy);
    }

    int8 *actions = calloc(0, MAX_ACTIONS * sizeof(int8));

    choose_best(&root, actions);

    return actions;
}