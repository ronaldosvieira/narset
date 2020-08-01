#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ismcts.h"

int LOGS_ARE_PRECOMPUTED = FALSE;
int NODES_ARE_PREALLOCATED = FALSE;
State *state_copy = NULL;

float get_log(int n) {
    if (LOGS_ARE_PRECOMPUTED == FALSE) {
        logs[0] = 0;

        for (int i = 1; i < LOGS_TO_PRECOMPUTE; i++)
            logs[i] = logf((float) i);

        LOGS_ARE_PRECOMPUTED = TRUE;
    }

    return n < LOGS_TO_PRECOMPUTE? logs[n] : logf((float) n);
}

Node* get_next_node() {
    if (NODES_ARE_PREALLOCATED == FALSE) {
        preallocated_nodes = malloc(NODES_TO_PREALLOCATE * sizeof(struct Node));
        amount_of_nodes = NODES_TO_PREALLOCATE;
        next_node = 0;
        NODES_ARE_PREALLOCATED = TRUE;
    } else if (next_node >= amount_of_nodes) {
        next_node++;
        return malloc(sizeof(struct Node));
    }

    return &preallocated_nodes[next_node++];
}

double uct_score(Node* node, double exploration_weight) {
    float exploitation = (float) node->rewards / (float) node->visits;

    int parent_visits = node->parent != NULL? node->parent->visits : 0;

    float exploration = sqrtf(get_log(parent_visits) / (float) node->visits);

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

Node* expand(Node* node, int action) {
    Node* new_leaf = get_next_node();

    new_leaf->parent = node;
    new_leaf->height = node->height + 1;
    new_leaf->right_sibling = node->left_child;
    node->left_child = new_leaf;
    new_leaf->left_child = NULL;

    if (action != 0)
        new_leaf->current_player = node->current_player;
    else
        new_leaf->current_player = (node->current_player + 1) % 2;

    new_leaf->action = action;

    new_leaf->rewards = 0;
    new_leaf->visits = 0;

    return new_leaf;
}

int simulate(State* state) {
    while (state->winner == NONE) {
        // chose a random action
        int valid_actions = calculate_valid_actions(state);
        int ith_action = (int) random() % valid_actions;

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
    Node* node = select_node(root, state);
    Node* leaf = NULL;

    // if the leaf is not terminal, expand
    if (node->children > 0) {
        // find the i-th action
        calculate_valid_actions(state);
        int action = 0;

        if (node->unvisited_children-- > 1) {
            int counter = 0;
            while (counter != node->children - node->unvisited_children)
                if (state->valid_actions[++action] == TRUE)
                    counter++;
        }

        // expand the leaf, creating a new leaf
        leaf = expand(node, action);

        // apply the action
        act_on_state(state, action);

        int valid_actions = calculate_valid_actions(state);

        leaf->children = valid_actions;
        leaf->unvisited_children = valid_actions;
    } else {
        leaf = node;
    }

    // simulate the remainder of the match and get the result
    int winner = simulate(state);
    int reward = winner == root->current_player? 1 : 0;

    // backpropagate the simulation result upwards in the tree
    backpropagate(leaf, reward, root->current_player);
}

void choose_best(Node* root, int* actions) {
    Node* node = root;
    int i = 0;

    node = select_with_uct(node, 0);

    // while action is not to pass
    while (node != NULL && node->action != 0) {
        actions[i++] = node->action;
        node = select_with_uct(node, 0);
    }

    if (node != NULL)
        debug_print("chosen node: %.3f (height %d)\n",
                    (float) node->rewards / node->visits, node->height);

    actions[i] = 0;
}

void shuffle_draft_options(Card* draft_options, int* player_choices) {
    size_t i;
    for (i = 0; i < 30 - 1; i++)
    {
        size_t j = i + random() / (RAND_MAX / (30 - i) + 1);

        Card temp_do[3] = {draft_options[3 * j + 0],
                           draft_options[3 * j + 1],
                           draft_options[3 * j + 2]};
        draft_options[3 * j + 0] = draft_options[3 * i + 0];
        draft_options[3 * j + 1] = draft_options[3 * i + 1];
        draft_options[3 * j + 2] = draft_options[3 * i + 2];
        draft_options[3 * i + 0] = temp_do[0];
        draft_options[3 * i + 1] = temp_do[1];
        draft_options[3 * i + 2] = temp_do[2];

        int temp_pc = player_choices[j];
        player_choices[j] = player_choices[i];
        player_choices[i] = temp_pc;
    }
}

int* act(State* state, Card* draft_options, int* player_choices) {
    clock_t start_time = clock();

    int valid_actions = calculate_valid_actions(state);

    Node *root = get_next_node();
    root->action = -1;
    root->current_player = state->current_player->id;
    root->rewards = 0;
    root->visits = 0;
    root->height = 0;
    root->children = valid_actions;
    root->unvisited_children = valid_actions;
    root->parent = NULL;
    root->left_child = NULL;
    root->right_sibling = NULL;

    if (state_copy == NULL)
        state_copy = malloc(sizeof(State));

    for (int i = 1; TRUE; i++) {
        state_copy = copy_state(state, state_copy);

        shuffle_draft_options(draft_options, player_choices);

        int fake_instance_id = 1000;

        // determinize the player's deck
        for (int j = 0; j < state_copy->current_player->deck_size; j++) {
            Card card = draft_options[3 * j + player_choices[j]];
            card.instance_id = fake_instance_id++;
            state_copy->decks[state_copy->current_player->id][j] = card;
        }

        // determinize the opponents's deck
        for (int j = 0; j < state_copy->opposing_player->deck_size; j++) {
            Card card = draft_options[3 * j + random() % 3];
            card.instance_id = fake_instance_id++;
            state_copy->decks[state_copy->opposing_player->id][j] = card;
        }

        // determinize the opponent's hand
        for (int j = 0; j < state_copy->opposing_player->hand_size; j++) {
            Card card = draft_options[3 * (30 - j - 1) + random() % 3];
            card.instance_id = fake_instance_id++;
            state_copy->opp_hand[j] = card;
        }

        // perform a rollout
        do_rollout(root, state_copy);

        if (i % 100 == 0) {
            double time_elapsed = (double) (clock() - start_time) / CLOCKS_PER_SEC;

            if (time_elapsed > 0.190) {
                debug_print("rollouts: %d\n", i);
                debug_print("time spent searching: %.3f\n", time_elapsed);
                break;
            }
        }
    }

    int *actions = calloc(MAX_ACTIONS, sizeof(int));

    choose_best(root, actions);
    next_node = 0;

    return actions;
}