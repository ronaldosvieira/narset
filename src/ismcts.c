#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "ismcts.h"

int LOGS_ARE_PRECOMPUTED = FALSE;
int NODES_ARE_PREALLOCATED = FALSE;
State *state_copy = NULL;

Card card_centroids[8] = {
        {.id = 1000, .instance_id = -1, .type = CREATURE, .cost = 5,
         .attack = 3, .defense = 4, .player_hp = 4, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b000001, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1001, .instance_id = -1, .type = CREATURE, .cost = 1,
         .attack = 1, .defense = 2, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b000000, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1002, .instance_id = -1, .type = CREATURE, .cost = 6,
         .attack = 5, .defense = 5, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b000000, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1003, .instance_id = -1, .type = RED_ITEM, .cost = 2,
         .attack = 0, .defense = -1, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b000000, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1004, .instance_id = -1, .type = CREATURE, .cost = 9,
         .attack = 8, .defense = 8, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b001001, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1005, .instance_id = -1, .type = RED_ITEM, .cost = 6,
         .attack = 0, .defense = -10, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b111111, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1006, .instance_id = -1, .type = CREATURE, .cost = 3,
         .attack = 2, .defense = 4, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b001000, .lane = -1, .location = OUTSIDE, .can_attack = FALSE},
        {.id = 1007, .instance_id = -1, .type = CREATURE, .cost = 4,
         .attack = 5, .defense = 2, .player_hp = 0, .enemy_hp = 0, .card_draw = 0,
         .keywords = 0b000000, .lane = -1, .location = OUTSIDE, .can_attack = FALSE}
};

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

void print_node(Node* node) {
    debug_print("[ %d", node->action);
    while (node->parent != NULL) {
            node = node->parent;
            debug_print(" <- %d", node->action);
        }
    debug_print(" %s", "]");
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

        if (exploration_weight == 0) {
            print_node(node);
            debug_print(" -> %d: %d / %d = %.3f (height %d)\n",
                        child->action, child->rewards, child->visits, score, child->height);
        }

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

int random_default_policy(State* state) {
    // chose a random action
    int valid_actions = calculate_valid_actions(state);
    int ith_action = (int) random() % valid_actions;

    // find the i-th action
    int action = 0; int counter = 0;
    while (counter != ith_action)
        if (state->valid_actions[++action] == TRUE)
            counter++;

    return action;
}

int max_atk_default_policy(State* state) {
    // for each lane, try to attack
    for (int lane = 0; lane < 2; lane++) {
        int lane_pos = lane == 0? LEFT_LANE : RIGHT_LANE;

        int first_guard_pos = -1;

        // check for guards
        for (int creature_pos = 0; creature_pos < 3; creature_pos++) {
            Card* enemy_creature = &state->opp_board[lane_pos + creature_pos];

            if (enemy_creature->id < 0) continue;

            if (has_keyword(*enemy_creature, GUARD)) {
                first_guard_pos = creature_pos;
                break;
            }
        }

        int best_atk_pos = -1;
        int best_atk = -1;

        // check for creatures able to attack
        for (int creature_pos = 0; creature_pos < 3; creature_pos++) {
            Card* friendly_creature = &state->player_board[lane_pos + creature_pos];

            if (friendly_creature->id < 0) continue;
            if (!friendly_creature->can_attack) continue;

            if (friendly_creature->attack > best_atk) {
                best_atk = friendly_creature->attack;
                best_atk_pos = creature_pos;
            }
        }

        if (best_atk_pos != -1) {
            return ATTACK_START_INDEX + (best_atk_pos + (3 * lane)) * 4 + 1 + first_guard_pos;
        }
    }

    int best_atk_pos = -1;
    int best_atk = -1;

    // check for creature cards able to be cast
    for (int card_pos = 0; card_pos < state->current_player->hand_size; card_pos++) {
        Card* card_in_hand = &state->player_hand[card_pos];

        if (card_in_hand->id < 0) continue;
        if (card_in_hand->type != CREATURE) continue;
        if (card_in_hand->cost > state->current_player->mana) continue;

        if (card_in_hand->attack > best_atk) {
            best_atk = card_in_hand->attack;
            best_atk_pos = card_pos;
        }
    }

    if (best_atk_pos != -1) {
        if (state->current_player->left_lane_size < 3) {
            return SUMMON_START_INDEX + (best_atk_pos * 2);
        } else if (state->current_player->right_lane_size < 3) {
            return SUMMON_START_INDEX + (best_atk_pos * 2) + 1;
        }
    }

    return 0;
}

int simulate(State* state) {
    while (state->winner == NONE) {
        // use a random default policy
        int action = max_atk_default_policy(state);

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
        int action = 0;

        if (node->unvisited_children-- > 1) {
            int counter = 0;
            while (counter != node->children - node->unvisited_children) {
                if (action >= 96) break;
                if (node->valid_actions[++action] == TRUE)
                    counter++;
            }
        }

        // expand the leaf, creating a new leaf
        leaf = expand(node, action);

        // apply the action
        act_on_state(state, action);

        int valid_actions = calculate_valid_actions(state);

        leaf->children = valid_actions;
        leaf->unvisited_children = valid_actions;
        memcpy(leaf->valid_actions, state->valid_actions, sizeof(Bool) * 97);
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
    memcpy(root->valid_actions, state->valid_actions, sizeof(Bool) * 97);

    if (state_copy == NULL)
        state_copy = malloc(sizeof(State));

    int fake_instance_id = 1000;

    // determinize the player's deck
    for (int j = 0; j < state->current_player->deck_size; j++) {
        Card card = draft_options[3 * j + player_choices[j]];
        card.instance_id = fake_instance_id++;
        memcpy(&state->decks[state->current_player->id][j],
               &card, sizeof(Card));
    }

    // determinize the opponents's deck
    for (int j = 0; j < state->opposing_player->deck_size; j++) {
        Card card = card_centroids[j % 8];
        card.instance_id = fake_instance_id++;
        memcpy(&state->decks[state->opposing_player->id][j],
               &card, sizeof(Card));
    }

    // determinize the opponent's hand
    for (int j = 0; j < state->opposing_player->hand_size; j++) {
        Card card = card_centroids[j];
        card.instance_id = fake_instance_id++;
        memcpy(&state->opp_hand[j], &card, sizeof(Card));
    }

    shuffle_draft_options(draft_options, player_choices);

    for (int i = 1; TRUE; i++) {
        state_copy = copy_state(state, state_copy);

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