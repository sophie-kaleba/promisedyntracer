#ifndef TURBOTRACER_CALL_H
#define TURBOTRACER_CALL_H

#include "Argument.h"
#include "Rdyntrace.h"
#include "Rinternals.h"
#include "table.h"
#include "utilities.h"

class Function;

typedef std::vector<int> force_order_t;

class Call {
  public:
    /* defined in cpp file to get around cyclic dependency issues. */
    explicit Call(const call_id_t id,
                  const std::string& function_name,
                  const SEXP environment,
                  Function* function);

    call_id_t get_id() const {
        return id_;
    }

    const std::string& get_function_name() const {
        return function_name_;
    }

    Function* get_function() {
        return function_;
    }

    void set_actual_argument_count(int actual_argument_count) {
        actual_argument_count_ = actual_argument_count;
    }

    int get_actual_argument_count() const {
        return actual_argument_count_;
    }

    void set_call_as_arg(int nr) {
      call_as_arg_ = nr;
    }

    int get_call_as_arg() const {
      return call_as_arg_;
    }

    SEXP get_environment() const {
        return environment_;
    }

    void set_return_value_type(sexptype_t return_value_type) {
        return_value_type_ = return_value_type;
    }

    sexptype_t get_return_value_type() const {
        return return_value_type_;
    }

    void set_jumped() {
        jumped_ = true;
    }

    bool is_jumped() const {
        return jumped_;
    }

    void set_S3_method() {
        S3_method_ = true;
    }

    bool is_S3_method() const {
        return S3_method_;
    }

    void set_S4_method() {
        S4_method_ = true;
    }

    bool is_S4_method() const {
        return S4_method_;
    }

    void analyze_callee(Call* callee) {
        ++callee_counter_;
        /* if a caller has more than one callee it is not a wrapper. */
        if (callee_counter_ > 1) {
            wrapper_ = false;
        } else {
            wrapper_ = callee->is_wrapper();
        }
    }

    bool is_wrapper() const {
        return wrapper_;
    }

    std::vector<Argument*>& get_arguments() {
        return arguments_;
    }

    Argument* get_argument(int actual_argument_position) const {
        return arguments_[actual_argument_position];
    }
//
//     bool arg_redefine_function(const int actual_argument_position) const{
//       Argument * arg =  this->get_argument(actual_argument_position);
//       DenotedValue* value = arg->get_denoted_value();
//       /* which one am I supposed to chose? I don't know, let's pick the 2d*/
//       std::string argument_type = sexptype_to_string(value->get_type());
//       std::string expression_type = sexptype_to_string(value->get_expression_type());
//       std::string value_type = sexptype_to_string(value->get_value_type());
//       return expression_type.compare("Function Call");
//     }

    void add_argument(Argument* argument) {
        arguments_.push_back(argument);
        ++actual_argument_count_;
    }


    /* check if a function is being introduced dynamically
     * Returns 1 if :
     * - for assign(x, value,...), if value is a call
     * - for with(data, expr, ...), if expr is a call
     * - for <<- if the right side is a call
     */
    int has_call_as_arg() const {
      if (function_name_.compare("base:<<-")) {
       return 33;
      }
      // if (function_name_.compare("base:<<-") && this->arg_redefine_function(1))
      // {return 1;}
      // else if (function_name_.compare("base::with") && this->arg_redefine_function(2))
      // {return 1;}
      // else if (function_name_.compare("base::assign") && this->arg_redefine_function(2))
      // {return 1;}
      // else
      return 0;
    }


    const pos_seq_t& get_force_order() const {
        return force_order_;
    }

    void set_force_order(int force_order) {
        force_order_ = {force_order};
    }

    void add_to_force_order(int formal_parameter_position) {
        if (std::find(force_order_.begin(),
                      force_order_.end(),
                      formal_parameter_position) == force_order_.end()) {
            force_order_.push_back(formal_parameter_position);
        }
    }

    pos_seq_t get_missing_argument_positions() const {
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

  private:
    const call_id_t id_;
    const std::string function_name_;
    int actual_argument_count_;
    const SEXP environment_;
    Function* function_;
    sexptype_t return_value_type_;
    bool jumped_;
    bool S3_method_;
    bool S4_method_;
    int callee_counter_;
    int call_as_arg_;
    bool wrapper_;
    std::vector<Argument*> arguments_;
    pos_seq_t force_order_;
};

#endif /* TURBOTRACER_CALL_H */
