#include "PromiseState.h"
#include "CallState.h"

/* this function is defined here instead of .h file to break circular dependency
   issues that arise due to accessing get_environment method of call_state */
void PromiseState::make_argument(CallState *call_state, int formal_parameter_position,
                                 int actual_argument_position) {

    call_ = call_state;
    formal_parameter_position_ = formal_parameter_position;
    actual_argument_position_ = actual_argument_position;
    if (is_promise()) {
        default_ = call_state->get_environment() == get_environment();
    }
}
