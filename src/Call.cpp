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
    , jumped_(false)
    , S3_method_(false)
    , S4_method_(false)
    , callee_counter_(0) {
    wrapper_ = function_->is_dot_internal() || function_->is_dot_primitive() ||
               function_->is_dot_c() || function_->is_dot_fortran() ||
               function_->is_dot_external() || function_->is_dot_external2() ||
               function_->is_dot_call() ||
               function_->is_dot_external_graphics() ||
               function_->is_dot_call_graphics();

    arguments_.reserve(std::max(function_->get_formal_parameter_count(), 0));
    force_order_.reserve(std::max(function_->get_formal_parameter_count(), 0));
}
