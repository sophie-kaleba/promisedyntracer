#include "DenotedValue.h"
#include "Call.h"

/* this function is defined here instead of .h file to break circular dependency
   issues that arise due to accessing get_environment method of call */
void DenotedValue::make_argument(Call *call, int formal_parameter_position,
                                 int actual_argument_position) {

    call_ = call;
    formal_parameter_position_ = formal_parameter_position;
    actual_argument_position_ = actual_argument_position;
    if (is_promise()) {
        default_ = call->get_environment() == get_environment();
    }
}
