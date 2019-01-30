#ifndef PROMISE_DYNTRACER_CALL_STATE_H
#define PROMISE_DYNTRACER_CALL_STATE_H

#include "ParameterUse.h"
#include "Rdyntrace.h"
#include "State.h"
#include "table.h"
#include "utilities.h"

class CallState {
  public:
    explicit CallState(call_id_t call_id, fn_id_t fn_id,
                       const std::string &function_type,
                       const std::string &function_name,
                       int formal_parameter_count, const std::string &order)
        : call_id_{call_id}, fn_id_{fn_id}, function_type_(function_type),
          function_name_(function_name),
          formal_parameter_count_{formal_parameter_count},
          parameter_uses_{
              static_cast<std::size_t>(std::max(formal_parameter_count, 0))},
          order_{order}, intrinsic_order_{order},
          return_value_type_{UNASSIGNEDSXP}, leaf_(true), active_(true),
          execution_time_(0.0) {

        /* INFO - Reserve size to 15 bytes to prevent repeated string
         * allocations when forced arguments are added. This increases
         * the memory requirement but should speed up the program. */
        order_.reserve(15);
        intrinsic_order_.reserve(15);
    }

    call_id_t get_call_id() const { return call_id_; }

    const fn_id_t &get_function_id() const { return fn_id_; }

    const std::string &get_function_type() const { return function_type_; }

    const std::string &get_function_name() const { return function_name_; }

    void set_leaf(bool leaf) { leaf_ = leaf; }

    bool is_leaf() const { return leaf_; }

    void make_active() { active_ = true; }

    bool is_active() const { return active_; }

    void make_inactive() { active_ = false; }

    void set_return_value_type(sexptype_t return_value_type) {
        return_value_type_ = return_value_type;
    }

    sexptype_t get_return_value_type() const { return return_value_type_; }

    const int get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

    double get_execution_time() const { return execution_time_; }

    void set_execution_time(double execution_time) {
        execution_time_ = execution_time;
    }

    void set_expression_type(sexptype_t expression_type, std::size_t position) {
        parameter_uses_[position].set_expression_type(expression_type);
    }

    void set_value_type(sexptype_t value_type, std::size_t position) {
        parameter_uses_[position].set_value_type(value_type);
    }

    void force_entry(const SEXP promise, std::size_t position) {

        bool previous_forced_state = parameter_uses_[position].get_force();
        parameter_uses_[position].force();

        /* reset the expression type when forcing in case metaprogramming
           happened after promise creation and before forcing
        */
        set_expression_type(TYPEOF(dyntrace_get_promise_expression(promise)),
                            position);
        /* iff the argument is forced and has not been previously forced
           do we add it to the order. This ensures that there is a single
           order entry for all elements of ...
        */

        if (!is_active()) {
            parameter_uses_[position].set_escape();
        } else if (!previous_forced_state) {
            order_.append(" | ").append(std::to_string(position));
            if (is_leaf()) {
                intrinsic_order_.append(" | ").append(std::to_string(position));
            }
        }
    }

    void force_exit(const SEXP promise, std::size_t position,
                    double execution_time) {
        set_value_type(TYPEOF(dyntrace_get_promise_value(promise)), position);
        set_argument_execution_time_(position, execution_time);
    }

    void lookup(std::size_t position) {
        if (!is_active()) {
            parameter_uses_[position].set_escape();
        }
        parameter_uses_[position].lookup();
    }

    void metaprogram(std::size_t position) {
        if (!is_active()) {
            parameter_uses_[position].set_escape();
        }
        parameter_uses_[position].metaprogram();
    }

    void set_parameter_mode(std::size_t position, parameter_mode_t mode) {
        parameter_uses_[position].set_parameter_mode(mode);
    }

    const std::vector<ParameterUse> &get_parameter_uses() const {
        return parameter_uses_;
    }

    const std::string &get_order() const { return order_; }

    const std::string &get_intrinsic_order() const { return intrinsic_order_; }

    void set_argument_execution_time_(std::size_t position, double execution_time) {
        parameter_uses_[position].set_execution_time(execution_time);
    }

private:
    call_id_t call_id_;
    fn_id_t fn_id_;
    std::string function_type_;
    std::string function_name_;
    int formal_parameter_count_;
    std::vector<ParameterUse> parameter_uses_;
    std::string order_;
    std::string intrinsic_order_;
    sexptype_t return_value_type_;
    bool leaf_;
    bool active_;
    double execution_time_;
};

inline std::ostream &operator<<(std::ostream &os, const CallState &call_state) {
    os << "CallState(" << call_state.get_function_id() << ","
       << call_state.get_call_id() << ","
       << call_state.get_formal_parameter_count() << ")";
    return os;
}

#endif /* PROMISE_DYTRACER_CALL_STATE_H */
