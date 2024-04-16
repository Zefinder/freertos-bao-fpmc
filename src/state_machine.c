#include <state_machine.h>

enum states current_state = WAITING;

void change_state(enum states new_state) {
    current_state = new_state;
}

enum states get_current_state() {
    return current_state;
}