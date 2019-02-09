#ifndef PROMISEDYNTRACER_PROMISE_STATE_H
#define PROMISEDYNTRACER_PROMISE_STATE_H

#include "sexptypes.h"
#include "utilities.h"

/* forward declaration of CallState to prevent cyclic dependency */
class CallState;

class PromiseState {
  public:
    PromiseState(SEXP object, bool local): PromiseState(local) {
        type_ = type_of_sexp(object);
        if(type_ == PROMSXP) {
            SEXP expr = dyntrace_get_promise_expression(object);
            SEXP val = dyntrace_get_promise_value(object);
            SEXP rho = dyntrace_get_promise_environment(object);
            set_expression_type(type_of_sexp(expr));
            set_value_type(type_of_sexp(val));
            set_environment(rho);
        }
    }

    promise_id_t get_id() const {
        return id_;
    }

    bool is_promise() const { return get_type() == PROMSXP; }

    sexptype_t get_type() const { return type_; }

    bool is_local() const {
        return local_;
    }

    bool is_active() const {
        return active_;
    }

    void set_active() {
        active_ = true;
    }

    void set_inactive() {
        active_ = false;
    }

    CallState* get_call() {
        return call_;
    }

    std::string get_expression() const {
        // TODO
        return "UNIMPLEMENTED";
    }

    bool is_argument() const {
        return (call_ != nullptr);
    }

    // TODO check if parameter_mode can be computed here instead
    void make_argument(CallState* call_state,
                       int formal_parameter_position,
                       int actual_argument_position);


    bool is_default() const {
        return default_;
    }

    void free_argument() {
        call_ = nullptr;
        formal_parameter_position_ = -1;
        actual_argument_position_ = -1;
    }

    int get_formal_parameter_position() const {
        return formal_parameter_position_;
    }

    int get_actual_argument_position() const {
        return actual_argument_position_;
    }

    bool is_transitive_side_effect_observer() const {
        return transitive_side_effect_observer_;
    }

    bool is_direct_side_effect_observer() const {
        return direct_side_effect_observer_;
    }

    bool is_transitive_side_effect_creator() const {
        return transitive_side_effect_creator_;
    }

    bool is_direct_side_effect_creator() const {
        return direct_side_effect_creator_;
    }

    bool is_transitive_lexical_scope_mutator() const {
        return transitive_lexical_scope_mutator_;
    }

    bool is_direct_lexical_scope_mutator() const {
        return direct_lexical_scope_mutator_;
    }

    bool is_transitive_non_lexical_scope_mutator() const {
        return transitive_non_lexical_scope_mutator_;
    }

    bool is_direct_non_lexical_scope_mutator() const {
        return direct_non_lexical_scope_mutator_;
    }

    void set_transitive_side_effect_observer() {
        transitive_side_effect_observer_ = true;
    }

    void set_direct_side_effect_observer() {
        direct_side_effect_observer_ = true;
    }

    void set_transitive_side_effect_creator() {
        transitive_side_effect_creator_ = true;
    }

    void set_direct_side_effect_creator() {
        direct_side_effect_creator_ = true;
    }

    void set_transitive_lexical_scope_mutator() {
        transitive_lexical_scope_mutator_ = true;
    }

    void set_direct_lexical_scope_mutator() {
        direct_lexical_scope_mutator_ = true;
    }

    void set_transitive_non_lexical_scope_mutator() {
        transitive_non_lexical_scope_mutator_ = true;
    }

    void set_direct_non_lexical_scope_mutator() {
        direct_non_lexical_scope_mutator_ = true;
    }

    void set_creation_timestamp(timestamp_t creation_timestamp) {
        creation_timestamp_ = creation_timestamp;
    }

    timestamp_t get_creation_timestamp() const {
        return creation_timestamp_;
    }

    double get_execution_time() const { return execution_time_; }

    void set_execution_time(double execution_time) {
        execution_time_ = execution_time;
    }

    void add_execution_time(double execution_time) {
        execution_time_ += execution_time;
    }

    std::uint8_t get_metaprogram() const { return metaprogram_; }

    /*TODO - check here and in other functions for escaped promise
      and increment the appropriate counter */
    void metaprogram() { ++metaprogram_; }

    std::uint8_t get_lookup() const { return lookup_; }

    void lookup() { ++lookup_; }

    std::uint8_t get_force() const { return force_; }


    bool is_forced() const { return force_; }

    void force() { ++force_; }

    void set_environment(SEXP environment) { environment_ = environment; }

    SEXP get_environment() { return environment_; }

    sexptype_t get_expression_type() const { return expression_type_; }

    void set_expression_type(const sexptype_t expression_type) {
        expression_type_ = expression_type;
    }

    sexptype_t get_value_type() const { return value_type_; }

    void set_value_type(const sexptype_t value_type) {
        value_type_ = value_type;
    }

    const std::string& get_escape() const { return escape_mode_; }

    void set_escape(const std::string& escape_mode) { escape_mode_ = escape_mode; }

    void set_evaluation_depth(const eval_depth_t& eval_depth) {
        eval_depth_ = eval_depth;
    }

    eval_depth_t get_evaluation_depth() const {
        return eval_depth_;
    }

private:
  PromiseState(bool local)
      : id_(UNASSIGNED_DENOTED_VALUE_ID), type_(UNASSIGNEDSXP),
        expression_type_(UNASSIGNEDSXP), value_type_(UNASSIGNEDSXP),
        environment_(nullptr), local_(false), active_(false), call_(nullptr),
        formal_parameter_position_(-1), actual_argument_position_(-1),
        default_(false), evaluated_(false),
        transitive_side_effect_observer_(false),
        direct_side_effect_observer_(false),
        transitive_side_effect_creator_(false),
        direct_side_effect_creator_(false),
        transitive_lexical_scope_mutator_(false),
        direct_lexical_scope_mutator_(false),
        transitive_non_lexical_scope_mutator_(false),
        direct_non_lexical_scope_mutator_(false),
        creation_timestamp_(UNDEFINED_TIMESTAMP), execution_time_(0.0),
        force_(0), lookup_(0), metaprogram_(0),
        escape_mode_("X"), eval_depth_{UNASSIGNED_PROMISE_EVAL_DEPTH} {}

    promise_id_t id_;
    sexptype_t type_;
    sexptype_t expression_type_;
    sexptype_t value_type_;
    SEXP environment_;
    bool local_;
    bool active_;
    CallState *call_;
    int formal_parameter_position_;
    int actual_argument_position_;
    bool default_;
    bool evaluated_;
    timestamp_t creation_timestamp_;
    bool transitive_side_effect_observer_;
    bool direct_side_effect_observer_;
    bool transitive_side_effect_creator_;
    bool direct_side_effect_creator_;
    bool transitive_lexical_scope_mutator_;
    bool direct_lexical_scope_mutator_;
    bool transitive_non_lexical_scope_mutator_;
    bool direct_non_lexical_scope_mutator_;
    double execution_time_;
    std::uint8_t force_;
    std::uint8_t lookup_;
    std::uint8_t metaprogram_;
    std::string escape_mode_;
    eval_depth_t eval_depth_;
};


#endif /* PROMISEDYNTRACER_PROMISE_STATE_H */
