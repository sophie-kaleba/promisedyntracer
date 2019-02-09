#ifndef PROMISEDYNTRACER_CALL_STATE_H
#define PROMISEDYNTRACER_CALL_STATE_H

#include "Rdyntrace.h"
#include "table.h"
#include "utilities.h"
#include "Rinternals.h"
#include "PromiseState.h"

class Function;

class CallState {
  public:
    explicit CallState(const call_id_t id,
                       const function_id_t& function_id,
                       const sexptype_t function_type,
                       const std::string &function_name,
                       const int formal_parameter_count,
                       const int actual_argument_count,
                       const SEXP environment,
                       Function * function)
        : id_(id), function_id_(function_id),
          function_type_(function_type), function_name_(function_name),
          formal_parameter_count_(formal_parameter_count),
          actual_argument_count_(actual_argument_count),
          environment_(environment), function_(function),
          return_value_type_(UNASSIGNEDSXP) {

        /* most calls have only one argument. Argument list of size 5
           covers almost every call */
        arguments_.reserve(std::max(get_actual_argument_count(), 5));
        /* INFO - Reserve size to 15 bytes to prevent repeated string
         * allocations when forced arguments are added. This increases
         * the memory requirement but should speed up the program. */
        force_order_.reserve(15);
    }

    ~CallState() {

        if(get_function_type() != CLOSXP) return;

        for(int i = 0; i < arguments_.size(); ++i) {
            /* delete a promise iff it is inactive. An active promise
               is currently not garbage collected by R and is present
               in the promise mapping managed by the tracer state. */
            if(!arguments_[i] -> is_active()) {
                delete arguments_[i];
            } else {
                arguments_[i] -> free_argument();
            }
            arguments_[i] = nullptr;
        }
    }

    call_id_t get_id() const { return id_; }

    const function_id_t &get_function_id() const { return function_id_; }

    sexptype_t get_function_type() const { return function_type_; }

    bool is_closure() const {
        return get_function_type() == CLOSXP;
    }

    bool is_special() const { return get_function_type() == SPECIALSXP; }

    bool is_builtin() const { return get_function_type() == BUILTINSXP; }

    const std::string &get_function_name() const { return function_name_; }

    Function* get_function() { return function_; }

    void set_formal_parameter_count(int formal_parameter_count) {
        formal_parameter_count_ = formal_parameter_count;
    }

    int get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

    void set_actual_argument_count(int actual_argument_count) {
        actual_argument_count_ = actual_argument_count;
    }

    int get_actual_argument_count() const {
        return actual_argument_count_;
    }

    SEXP get_environment() const { return environment_; }

    void set_return_value_type(sexptype_t return_value_type) {
        return_value_type_ = return_value_type;
    }

    sexptype_t get_return_value_type() const { return return_value_type_; }

    std::vector<PromiseState *> &get_arguments() {
        return arguments_;
    }

    PromiseState* get_argument(int actual_argument_position) {
        return arguments_[actual_argument_position];
    }

    void add_argument(PromiseState* argument) {
        arguments_.push_back(argument);
        ++actual_argument_count_;
    }

    const std::string &get_force_order() const { return force_order_; }

    void set_force_order(const std::string& force_order) { force_order_ = force_order; }

    void add_to_force_order(int formal_parameter_position) {
        force_order_.append(" | ").append(std::to_string(formal_parameter_position));
    }

private:
    const call_id_t id_;
    const function_id_t function_id_;
    const sexptype_t function_type_;
    const std::string function_name_;
    int formal_parameter_count_;
    int actual_argument_count_;
    const SEXP environment_;
    Function* function_;
    sexptype_t return_value_type_;
    // TODO - change this to wrapper type - bool leaf_;
    std::vector<PromiseState*> arguments_;
    std::string force_order_;
};

inline std::ostream &operator<<(std::ostream &os, const CallState &call_state) {
    os << "CallState(" << call_state.get_function_id() << ","
       << call_state.get_id() << ","
       << call_state.get_formal_parameter_count() << ")";
    return os;
}

#endif /* PROMISEDYTRACER_CALL_STATE_H */
