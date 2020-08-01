#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ismcts.h"

int LOGS_ARE_PRECOMPUTED = FALSE;
int NODES_ARE_PREALLOCATED = FALSE;
State *state_copy = NULL;

void precompute_logs() {
    logs[0] = 0;

    for (int i = 1; i < LOGS_TO_PRECOMPUTE; i++)
        logs[i] = logf((float) i);

    LOGS_ARE_PRECOMPUTED = TRUE;
}

Node* get_next_node() {
    if (NODES_ARE_PREALLOCATED == FALSE) {
        preallocated_nodes = malloc(NODES_TO_PREALLOCATE * sizeof(Node));
        amount_of_nodes = NODES_ARE_PREALLOCATED;
        next_node = 0;
    } else if (next_node >= NODES_TO_PREALLOCATE) {
        preallocated_nodes = realloc(preallocated_nodes,
                                     2 * amount_of_nodes * sizeof(Node));
    }

    return &preallocated_nodes[next_node++];
}

double uct_score(Node* node, double exploration_weight) {
    float exploitation = (float) node->rewards / (float) node->visits;

    int parent_visits = node->parent != NULL? node->parent->visits : 0;
    float log_parent_visits = parent_visits < LOGS_TO_PRECOMPUTE ?
                              logs[parent_visits] : logf((float) parent_visits);

    float exploration = sqrtf(log_parent_visits / (float) node->visits);

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
    Node* new_leaf = get_next_node();

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
    clock_t start_time = clock();

    if (LOGS_ARE_PRECOMPUTED == FALSE) precompute_logs();

    next_node = 0;

    int8 valid_actions = calculate_valid_actions(state);

    Node *root = get_next_node();
    root->action = -1;
    root->current_player = state->current_player->id;
    root->children = valid_actions;
    root->unvisited_children = valid_actions;
    root->height = 0;

    if (state_copy == NULL)
        state_copy = malloc(sizeof(State));

    for (int i = 1; TRUE; i++) {
         state_copy = copy_state(state, state_copy);

        // todo: determinize the state

        do_rollout(root, state_copy);

        if (i % 100 == 0) {
            double time_elapsed = (double) (clock() - start_time) / CLOCKS_PER_SEC;

            if (time_elapsed > 0.150) {
                debug_print("rollouts: %d\n", i);
                debug_print("time spent searching: %.3f\n", time_elapsed);
                break;
            }
        }
    }

    int8 *actions = calloc(0, MAX_ACTIONS * sizeof(int8));

    choose_best(root, actions);

    free(preallocated_nodes);
    amount_of_nodes = 0;

    return actions;
}