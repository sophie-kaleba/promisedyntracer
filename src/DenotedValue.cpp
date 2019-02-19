#include "DenotedValue.h"
#include "Call.h"

/* this function is defined here instead of .h file to break circular dependency
   issues that arise due to accessing get_environment method of call */
void DenotedValue::make_argument(Call *call, int formal_parameter_position,
                                 int actual_argument_position, bool dot_dot) {

    bool def = is_promise() ? (call->get_environment() == get_environment()) : false;

    argument_stack_.push_back(Argument(call,
                                       formal_parameter_position,
                                       actual_argument_position,
                                       def,
                                       dot_dot));
}

void DenotedValue::free_argument(call_id_t call_id,
                                 function_id_t function_id,
                                 int formal_parameter_count,
                                 sexptype_t return_value_type) {

    /* TODO - this information should be put in a separate function */
    previous_call_id_ = call_id;
    previous_function_id_ = function_id;
    previous_formal_parameter_count_ = formal_parameter_count;
    previous_call_return_value_type_ = return_value_type;
    previous_formal_parameter_position_ = get_formal_parameter_position();
    previous_actual_argument_position_ = get_actual_argument_position();

    argument_stack_.pop_back();
    was_argument_ = true;
    used_for_S3_dispatch_ = false;
    used_for_S4_dispatch_ = false;
}
