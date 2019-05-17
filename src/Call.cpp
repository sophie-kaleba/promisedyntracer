#include "Call.h"

#include "Function.h"

Call::Call(const call_id_t id,
           const std::string& function_name,
           const SEXP environment,
           Function* function, 
           const SEXP args)
    : id_(id)
    , function_name_(function_name)
    , actual_argument_count_(0)
    , environment_(environment)
    , args_(args)
    , function_(function)
    , return_value_type_(UNASSIGNEDSXP)
    , jumped_(false)
    , S3_method_(false)
    , S4_method_(false)
    , call_as_arg_(0)
    , load_midway_(false)
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


void Call::process_calls_affecting_lookup(){
    //std::string fn_id = compute_hash((get_function() -> get_namespace() + get_function() -> get_definition()).c_str());
    // <<- is a special, we have to access its arguments in a specific way
    if (function_name_.compare("<<-") == 0) {
        if (value_type_to_string(CAR(CDR(args_))).compare("Function Call") == 0) {set_call_as_arg(1);}
        else {
            set_call_as_arg(0);
        }
        
    }
    // assign and with are closures
    else if ((function_name_.compare("assign") == 0)) {
        //Function* fn = get_function();
        //std::cout << fn->get_summary_count() << "\n";
        Argument * arg =  get_argument(1);  
        DenotedValue* value = arg->get_denoted_value();
        std::string expression_type = sexptype_to_string(value->get_expression_type());
        
        if (expression_type.compare("Function Call") == 0) {set_call_as_arg(1);}
        else {
            set_call_as_arg(0);
        }
    }
    else if ((function_name_.compare("with") == 0)) {
        Argument * arg =  get_argument(1);  
        DenotedValue* value = arg->get_denoted_value();
        std::string expression_type = sexptype_to_string(value->get_expression_type());
        
        if (expression_type.compare("Function Call") == 0) {set_call_as_arg(1);}
        else {
            set_call_as_arg(0);
        }
    }
    
    else if ((function_name_.compare("library") == 0)) {
        Function* fn = get_function();
        std::cout << "So far" << fn->get_summary_count();
    }
    else {
        set_call_as_arg(0);
    }
}

SEXP Call::get_args() {
    return args_;
}

const std::string& Call::get_function_name() const {
    return function_name_;
}

Function* Call::get_function() {
    return function_;
}

void Call::set_actual_argument_count(int actual_argument_count) {
    actual_argument_count_ = actual_argument_count;
}

int Call::get_actual_argument_count() const {
    return actual_argument_count_;
}

void Call::set_call_as_arg(int nr) {
    call_as_arg_ = nr;
}

int Call::get_call_as_arg() const {
    return call_as_arg_;
}

SEXP Call::get_environment() const {
    return environment_;
}

void Call::set_return_value_type(sexptype_t return_value_type) {
    return_value_type_ = return_value_type;
}

sexptype_t Call::get_return_value_type() const {
    return return_value_type_;
}

void Call::set_jumped() {
    jumped_ = true;
}

bool Call::is_jumped() const {
    return jumped_;
}

void Call::set_S3_method() {
    S3_method_ = true;
}

bool Call::is_S3_method() const {
    return S3_method_;
}

bool Call::is_loaded_midway() const {
    return load_midway_;
}

void Call::set_load_midway(bool state) {
    load_midway_ = state;
}

void Call::set_S4_method() {
    S4_method_ = true;
}

bool Call::is_S4_method() const {
    return S4_method_;
}

void Call::analyze_callee(Call* callee) {
    ++callee_counter_;
    /* if a caller has more than one callee it is not a wrapper. */
    if (callee_counter_ > 1) {
        wrapper_ = false;
    } else {
        wrapper_ = callee->is_wrapper();
    }
}

bool Call::is_wrapper() const {
    return wrapper_;
}

std::vector<Argument*>& Call::get_arguments() {
    return arguments_;
}

Argument* Call::get_argument(int actual_argument_position) const {
    return arguments_[actual_argument_position];
}

void Call::add_argument(Argument* argument) {
    arguments_.push_back(argument);
    ++actual_argument_count_;
}

const pos_seq_t& Call::get_force_order() const {
    return force_order_;
}

void Call::set_force_order(int force_order) {
    force_order_ = {force_order};
}

void Call::add_to_force_order(int formal_parameter_position) {
    if (std::find(force_order_.begin(),
                  force_order_.end(),
                  formal_parameter_position) == force_order_.end()) {
        force_order_.push_back(formal_parameter_position);
    }
}

pos_seq_t Call::get_missing_argument_positions() const {
    pos_seq_t missing_argument_positions;
    int position;
    
    for (auto argument: arguments_) {
        if (argument->get_denoted_value()->is_missing()) {
            position = argument->get_formal_parameter_position();
            
            /* this condition handles multiple missing values
             in dot dot arguments. */
            if (missing_argument_positions.size() == 0 ||
                missing_argument_positions.back() != position) {
                missing_argument_positions.push_back(position);
            }
        }
    }
    
    return missing_argument_positions;
}
