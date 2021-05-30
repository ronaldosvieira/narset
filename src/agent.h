#ifndef NARSET_AGENT_H
#define NARSET_AGENT_H

#include "engine.h"

void state_from_native_input(State* state);
void state_to_native_input(State* state);
char* action_index_to_native_action(State* state, int action_index);

#endif //NARSET_AGENT_H
