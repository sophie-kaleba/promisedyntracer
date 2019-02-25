#include "Call.h"

#include "Function.h"

Call::Call(const call_id_t id,
           const std::string& function_name,
           const SEXP environment,
           Function* function)
    : id_(id)
    , function_name_(function_name)
    , actual_argument_count_(0)
    , environment_(environment)
    , function_(function)
    , return_value_type_(UNASSIGNEDSXP)
    , jumped_(false) {
    /* most calls have only one argument. Argument list of size 5
       covers almost every call */
    arguments_.reserve(std::max(function_->get_formal_parameter_count(), 0));
    /* INFO - Reserve size to 15 bytes to prevent repeated string
     * allocations when forced arguments are added. This increases
     * the memory requirement but should speed up the program. */
    force_order_.reserve(std::max(function_->get_formal_parameter_count(), 0));
}
