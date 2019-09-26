#include "engine.h"

#include <stdio.h>
#include <stdlib.h>

int instance_counter = 0;
Card _cards[160];

/* Card methods */
int next_instance_id() {
    return ++instance_counter;
}
