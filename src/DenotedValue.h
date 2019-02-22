#ifndef PROMISEDYNTRACER_DENOTED_VALUE_H
#define PROMISEDYNTRACER_DENOTED_VALUE_H

#include "sexptypes.h"
#include "utilities.h"
class Argument;

/* forward declaration of Call to prevent cyclic dependency */

class DenotedValue {
  public:
    DenotedValue(denoted_value_id_t id, SEXP object, bool local)
        : DenotedValue(id, local) {
        type_ = type_of_sexp(object);
        if(type_ == PROMSXP) {
            SEXP expr = dyntrace_get_promise_expression(object);
            SEXP val = dyntrace_get_promise_value(object);
            SEXP rho = dyntrace_get_promise_environment(object);
            set_expression_type(type_of_sexp(expr));
            set_value_type(type_of_sexp(val));
            set_environment(rho);
            if(val != R_UnboundValue) {
                preforced_ = true;
            }
        }
    }

    denoted_value_id_t get_id() const { return id_; }

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

    bool is_argument() const {
        return argument_stack_.size() > 0;
    }

    bool was_argument() const {
        return was_argument_;
    }

    bool is_non_argument() const {
        return !(is_argument() || was_argument());
    }

    bool is_preforced() const { return preforced_; }

    bool is_forced() const {
        return get_force_count();
    }

    bool is_missing() const { return get_type() == MISSINGSXP; }

    Argument* get_last_argument() {
        return argument_stack_.back();
    }

    std::vector<Argument*> get_arguments() { return argument_stack_; }

    void add_argument(Argument * argument) {
        argument_stack_.push_back(argument);
    }

    void remove_argument(const call_id_t call_id,
                         const function_id_t& function_id,
                         const sexptype_t return_value_type,
                         const int formal_parameter_count,
                         const Argument *argument);

    const function_id_t &get_scope() const { return scope_; }

    void set_scope(const function_id_t &scope) { scope_ = scope; }

    const std::string &get_class_name() const { return class_name_; }

    void set_class_name(const std::string &class_name) {
        class_name_ = class_name;
    }

    int get_S3_dispatch_count() const { return S3_dispatch_count_; }

    void used_for_S3_dispatch();

    int get_S4_dispatch_count() const { return S4_dispatch_count_; }

    void used_for_S4_dispatch();

    void set_non_local_return();

    bool does_non_local_return() const { return non_local_return_; }

    void set_creation_timestamp(timestamp_t creation_timestamp) {
        creation_timestamp_ = creation_timestamp;
    }

    timestamp_t get_creation_timestamp() const { return creation_timestamp_; }

    double get_execution_time() const { return execution_time_; }

    void set_execution_time(double execution_time) {
        execution_time_ = execution_time;
    }

    void add_execution_time(double execution_time) {
        execution_time_ += execution_time;
    }

    void force();

    std::uint8_t get_force_count() const {
        return get_force_count_before_escape() + get_force_count_after_escape();
    }

    std::uint8_t get_force_count_before_escape() const {
        return before_escape_force_count_;
    }

    std::uint8_t get_force_count_after_escape() const { return force_count_; }

    void lookup_value();

    std::uint8_t get_value_lookup_count() const {
        return get_value_lookup_count_before_escape() +
               get_value_lookup_count_after_escape();
    }

    std::uint8_t get_value_lookup_count_before_escape() const {
        return before_escape_value_lookup_count_;
    }

    std::uint8_t get_value_lookup_count_after_escape() const {
        return value_lookup_count_;
    }

    void metaprogram();

    std::uint8_t get_metaprogram_count() const {
        return get_metaprogram_count_before_escape() +
               get_metaprogram_count_after_escape();
    }

    std::uint8_t get_metaprogram_count_before_escape() const {
        return before_escape_metaprogram_count_;
    }

    std::uint8_t get_metaprogram_count_after_escape() const {
        return metaprogram_count_;
    }

    void assign_value() {
        check_and_set_escape_();
        ++value_assign_count_;
    }

    std::uint8_t get_value_assign_count() const {
        return get_value_assign_count_before_escape() +
               get_value_assign_count_after_escape();
    }

    std::uint8_t get_value_assign_count_before_escape() const {
        return before_escape_value_assign_count_;
    }

    std::uint8_t get_value_assign_count_after_escape() const {
        return value_assign_count_;
    }

    void lookup_expression() {
        check_and_set_escape_();
        ++expression_lookup_count_;
    }

    std::uint8_t get_expression_lookup_count() const {
        return get_expression_lookup_count_before_escape() +
               get_expression_lookup_count_after_escape();
    }

    std::uint8_t get_expression_lookup_count_before_escape() const {
        return before_escape_expression_lookup_count_;
    }

    std::uint8_t get_expression_lookup_count_after_escape() const {
        return expression_lookup_count_;
    }

    void assign_expression() {
        check_and_set_escape_();
        ++expression_assign_count_;
    }

    std::uint8_t get_expression_assign_count() const {
        return get_expression_assign_count_before_escape() +
               get_expression_assign_count_after_escape();
    }

    std::uint8_t get_expression_assign_count_before_escape() const {
        return before_escape_expression_assign_count_;
    }

    std::uint8_t get_expression_assign_count_after_escape() const {
        return expression_assign_count_;
    }

    void lookup_environment() {
        check_and_set_escape_();
        ++environment_lookup_count_;
    }

    std::uint8_t get_environment_lookup_count() const {
        return get_environment_lookup_count_before_escape() +
               get_environment_lookup_count_after_escape();
    }

    std::uint8_t get_environment_lookup_count_before_escape() const {
        return before_escape_environment_lookup_count_;
    }

    std::uint8_t get_environment_lookup_count_after_escape() const {
        return environment_lookup_count_;
    }

    void assign_environment() {
        check_and_set_escape_();
        ++environment_assign_count_;
    }

    std::uint8_t get_environment_assign_count() const {
        return get_environment_assign_count_before_escape() +
               get_environment_assign_count_after_escape();
    }

    std::uint8_t get_environment_assign_count_before_escape() const {
        return before_escape_environment_assign_count_;
    }

    std::uint8_t get_environment_assign_count_after_escape() const {
        return environment_assign_count_;
    }

    void set_self_scope_mutation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_self_scope_mutation_count_;
        } else {
            ++indirect_self_scope_mutation_count_;
        }
    }

    std::uint8_t get_self_scope_mutation_count(bool direct) const {
        return get_self_scope_mutation_count_before_escape(direct) +
               get_self_scope_mutation_count_after_escape(direct);
    }

    std::uint8_t
    get_self_scope_mutation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_self_scope_mutation_count_;
        } else {
            return before_escape_indirect_self_scope_mutation_count_;
        }
    }

    std::uint8_t get_self_scope_mutation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_self_scope_mutation_count_;
        } else {
            return indirect_self_scope_mutation_count_;
        }
    }

    void set_lexical_scope_mutation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_lexical_scope_mutation_count_;
        } else {
            ++indirect_lexical_scope_mutation_count_;
        }
    }

    std::uint8_t get_lexical_scope_mutation_count(bool direct) const {
        return get_lexical_scope_mutation_count_before_escape(direct) +
               get_lexical_scope_mutation_count_after_escape(direct);
    }

    std::uint8_t
    get_lexical_scope_mutation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_lexical_scope_mutation_count_;
        } else {
            return before_escape_indirect_lexical_scope_mutation_count_;
        }
    }

    std::uint8_t
    get_lexical_scope_mutation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_lexical_scope_mutation_count_;
        } else {
            return indirect_lexical_scope_mutation_count_;
        }
    }

    void set_non_lexical_scope_mutation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_non_lexical_scope_mutation_count_;
        } else {
            ++indirect_non_lexical_scope_mutation_count_;
        }
    }

    std::uint8_t get_non_lexical_scope_mutation_count(bool direct) const {
        return get_non_lexical_scope_mutation_count_before_escape(direct) +
               get_non_lexical_scope_mutation_count_after_escape(direct);
    }

    std::uint8_t
    get_non_lexical_scope_mutation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_non_lexical_scope_mutation_count_;
        } else {
            return before_escape_indirect_non_lexical_scope_mutation_count_;
        }
    }

    std::uint8_t
    get_non_lexical_scope_mutation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_non_lexical_scope_mutation_count_;
        } else {
            return indirect_non_lexical_scope_mutation_count_;
        }
    }

    void set_self_scope_observation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_self_scope_observation_count_;
        } else {
            ++indirect_self_scope_observation_count_;
        }
    }

    std::uint8_t get_self_scope_observation_count(bool direct) const {
        return get_self_scope_observation_count_before_escape(direct) +
               get_self_scope_observation_count_after_escape(direct);
    }

    std::uint8_t
    get_self_scope_observation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_self_scope_observation_count_;
        } else {
            return before_escape_indirect_self_scope_observation_count_;
        }
    }

    std::uint8_t
    get_self_scope_observation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_self_scope_observation_count_;
        } else {
            return indirect_self_scope_observation_count_;
        }
    }

    void set_lexical_scope_observation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_lexical_scope_observation_count_;
        } else {
            ++indirect_lexical_scope_observation_count_;
        }
    }

    std::uint8_t get_lexical_scope_observation_count(bool direct) const {
        return get_lexical_scope_observation_count_before_escape(direct) +
               get_lexical_scope_observation_count_after_escape(direct);
    }

    std::uint8_t
    get_lexical_scope_observation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_lexical_scope_observation_count_;
        } else {
            return before_escape_indirect_lexical_scope_observation_count_;
        }
    }

    std::uint8_t
    get_lexical_scope_observation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_lexical_scope_observation_count_;
        } else {
            return indirect_lexical_scope_observation_count_;
        }
    }

    void set_non_lexical_scope_observation(bool direct) {
        check_and_set_escape_();
        if (direct) {
            ++direct_non_lexical_scope_observation_count_;
        } else {
            ++indirect_non_lexical_scope_observation_count_;
        }
    }

    std::uint8_t get_non_lexical_scope_observation_count(bool direct) const {
        return get_non_lexical_scope_observation_count_before_escape(direct) +
               get_non_lexical_scope_observation_count_after_escape(direct);
    }

    std::uint8_t
    get_non_lexical_scope_observation_count_before_escape(bool direct) const {
        if (direct) {
            return before_escape_direct_non_lexical_scope_observation_count_;
        } else {
            return before_escape_indirect_non_lexical_scope_observation_count_;
        }
    }

    std::uint8_t
    get_non_lexical_scope_observation_count_after_escape(bool direct) const {
        if (direct) {
            return direct_non_lexical_scope_observation_count_;
        } else {
            return indirect_non_lexical_scope_observation_count_;
        }
    }

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

    bool has_escaped() const { return escape_; }

    void set_evaluation_depth(const eval_depth_t &eval_depth);

    eval_depth_t get_evaluation_depth() const { return eval_depth_; }

    call_id_t get_previous_call_id() const { return previous_call_id_; }

    function_id_t get_previous_function_id() const {
        return previous_function_id_;
    }

    int get_previous_formal_parameter_position() const {
        return previous_formal_parameter_position_;
    }

    int get_previous_actual_argument_position() const {
        return previous_actual_argument_position_;
    }

    int get_previous_formal_parameter_count() const {
        return previous_formal_parameter_count_;
    }

    sexptype_t get_previous_call_return_value_type() const {
        return previous_call_return_value_type_;
    }

    bool get_previous_default_argument() const {
        return previous_default_argument_;
    }

  private:
    DenotedValue(denoted_value_id_t id, bool local)
        : id_(id), type_(UNASSIGNEDSXP), expression_type_(UNASSIGNEDSXP),
          value_type_(UNASSIGNEDSXP), preforced_(false), environment_(nullptr),
          local_(false), active_(false),
          argument_stack_({}), default_(false), evaluated_(false),
          was_argument_(false), scope_(UNASSIGNED_FUNCTION_ID),
          class_name_(UNASSIGNED_CLASS_NAME), S3_dispatch_count_(0),
          S4_dispatch_count_(0), non_local_return_(false),
          creation_timestamp_(UNDEFINED_TIMESTAMP), execution_time_(0.0),
          escape_(false), eval_depth_{UNASSIGNED_PROMISE_EVAL_DEPTH},
          previous_call_id_(UNASSIGNED_CALL_ID),
          previous_function_id_(UNASSIGNED_FUNCTION_ID),
          previous_formal_parameter_position_(
              UNASSIGNED_FORMAL_PARAMETER_POSITION),
          previous_formal_parameter_count_(UNASSIGNED_FORMAL_PARAMETER_COUNT),
          previous_actual_argument_position_(
              UNASSIGNED_ACTUAL_ARGUMENT_POSITION),
          previous_default_argument_(false),
          previous_call_return_value_type_(UNASSIGNEDSXP),
          before_escape_force_count_(0), force_count_(0),
          before_escape_value_lookup_count_(0), value_lookup_count_(0),
          before_escape_metaprogram_count_(0), metaprogram_count_(0),
          before_escape_value_assign_count_(0), value_assign_count_(0),
          before_escape_expression_lookup_count_(0),
          expression_lookup_count_(0),
          before_escape_expression_assign_count_(0),
          expression_assign_count_(0),
          before_escape_environment_lookup_count_(0),
          environment_lookup_count_(0),
          before_escape_environment_assign_count_(0),
          environment_assign_count_(0),
          before_escape_direct_self_scope_mutation_count_(0),
          direct_self_scope_mutation_count_(0),
          before_escape_indirect_self_scope_mutation_count_(0),
          indirect_self_scope_mutation_count_(0),
          before_escape_direct_lexical_scope_mutation_count_(0),
          direct_lexical_scope_mutation_count_(0),
          before_escape_indirect_lexical_scope_mutation_count_(0),
          indirect_lexical_scope_mutation_count_(0),
          before_escape_direct_non_lexical_scope_mutation_count_(0),
          direct_non_lexical_scope_mutation_count_(0),
          before_escape_indirect_non_lexical_scope_mutation_count_(0),
          indirect_non_lexical_scope_mutation_count_(0),
          before_escape_direct_self_scope_observation_count_(0),
          direct_self_scope_observation_count_(0),
          before_escape_indirect_self_scope_observation_count_(0),
          indirect_self_scope_observation_count_(0),
          before_escape_direct_lexical_scope_observation_count_(0),
          direct_lexical_scope_observation_count_(0),
          before_escape_indirect_lexical_scope_observation_count_(0),
          indirect_lexical_scope_observation_count_(0),
          before_escape_direct_non_lexical_scope_observation_count_(0),
          direct_non_lexical_scope_observation_count_(0),
          before_escape_indirect_non_lexical_scope_observation_count_(0),
          indirect_non_lexical_scope_observation_count_(0) {}

    void copy_and_reset_(std::uint8_t &left, std::uint8_t &right) {
        left = right;
        right = 0;
    }
    /* For a promise to escape:
       - It should not be an argument
       - It should have been an argument
       - */
    void check_and_set_escape_() {
        /* if we already ascertained that the promise has escaped,
           then we don't have any need of checking again */
        if (has_escaped()) {
            return;
        }
        if (!is_argument() && was_argument()) {

            escape_ = true;

            copy_and_reset_(before_escape_force_count_, force_count_);

            copy_and_reset_(before_escape_metaprogram_count_,
                            metaprogram_count_);

            copy_and_reset_(before_escape_value_lookup_count_,
                            value_lookup_count_);

            copy_and_reset_(before_escape_value_assign_count_,
                            value_assign_count_);

            copy_and_reset_(before_escape_expression_lookup_count_,
                            expression_lookup_count_);

            copy_and_reset_(before_escape_expression_assign_count_,
                            expression_assign_count_);

            copy_and_reset_(before_escape_environment_lookup_count_,
                            environment_lookup_count_);

            copy_and_reset_(before_escape_environment_assign_count_,
                            environment_assign_count_);

            copy_and_reset_(before_escape_direct_self_scope_mutation_count_,
                            direct_self_scope_mutation_count_);

            copy_and_reset_(before_escape_indirect_self_scope_mutation_count_,
                            indirect_self_scope_mutation_count_);

            copy_and_reset_(before_escape_direct_lexical_scope_mutation_count_,
                            direct_lexical_scope_mutation_count_);

            copy_and_reset_(
                before_escape_indirect_lexical_scope_mutation_count_,
                indirect_lexical_scope_mutation_count_);

            copy_and_reset_(
                before_escape_direct_non_lexical_scope_mutation_count_,
                direct_non_lexical_scope_mutation_count_);

            copy_and_reset_(
                before_escape_indirect_non_lexical_scope_mutation_count_,
                indirect_non_lexical_scope_mutation_count_);

            copy_and_reset_(before_escape_direct_self_scope_observation_count_,
                            direct_self_scope_observation_count_);

            copy_and_reset_(
                before_escape_indirect_self_scope_observation_count_,
                indirect_self_scope_observation_count_);

            copy_and_reset_(
                before_escape_direct_lexical_scope_observation_count_,
                direct_lexical_scope_observation_count_);

            copy_and_reset_(
                before_escape_indirect_lexical_scope_observation_count_,
                indirect_lexical_scope_observation_count_);

            copy_and_reset_(
                before_escape_direct_non_lexical_scope_observation_count_,
                direct_non_lexical_scope_observation_count_);

            copy_and_reset_(
                before_escape_indirect_non_lexical_scope_observation_count_,
                indirect_non_lexical_scope_observation_count_);
        }
    }

    denoted_value_id_t id_;
    sexptype_t type_;
    sexptype_t expression_type_;
    sexptype_t value_type_;
    bool preforced_;
    SEXP environment_;
    bool local_;
    bool active_;
    std::vector<Argument *> argument_stack_;
    bool default_;
    bool evaluated_;
    bool was_argument_;
    function_id_t scope_;
    std::string class_name_;
    int S3_dispatch_count_;
    int S4_dispatch_count_;
    bool non_local_return_;
    timestamp_t creation_timestamp_;
    double execution_time_;
    bool escape_;
    eval_depth_t eval_depth_;
    call_id_t previous_call_id_;
    function_id_t previous_function_id_;
    int previous_formal_parameter_position_;
    int previous_formal_parameter_count_;
    int previous_actual_argument_position_;
    sexptype_t previous_call_return_value_type_;
    bool previous_default_argument_;
    std::uint8_t before_escape_force_count_;
    std::uint8_t force_count_;
    std::uint8_t before_escape_value_lookup_count_;
    std::uint8_t value_lookup_count_;
    std::uint8_t before_escape_metaprogram_count_;
    std::uint8_t metaprogram_count_;
    std::uint8_t before_escape_value_assign_count_;
    std::uint8_t value_assign_count_;
    std::uint8_t before_escape_expression_lookup_count_;
    std::uint8_t expression_lookup_count_;
    std::uint8_t before_escape_expression_assign_count_;
    std::uint8_t expression_assign_count_;
    std::uint8_t before_escape_environment_lookup_count_;
    std::uint8_t environment_lookup_count_;
    std::uint8_t before_escape_environment_assign_count_;
    std::uint8_t environment_assign_count_;
    std::uint8_t before_escape_direct_self_scope_mutation_count_;
    std::uint8_t direct_self_scope_mutation_count_;
    std::uint8_t before_escape_indirect_self_scope_mutation_count_;
    std::uint8_t indirect_self_scope_mutation_count_;
    std::uint8_t before_escape_direct_lexical_scope_mutation_count_;
    std::uint8_t direct_lexical_scope_mutation_count_;
    std::uint8_t before_escape_indirect_lexical_scope_mutation_count_;
    std::uint8_t indirect_lexical_scope_mutation_count_;
    std::uint8_t before_escape_direct_non_lexical_scope_mutation_count_;
    std::uint8_t direct_non_lexical_scope_mutation_count_;
    std::uint8_t before_escape_indirect_non_lexical_scope_mutation_count_;
    std::uint8_t indirect_non_lexical_scope_mutation_count_;
    std::uint8_t before_escape_direct_self_scope_observation_count_;
    std::uint8_t direct_self_scope_observation_count_;
    std::uint8_t before_escape_indirect_self_scope_observation_count_;
    std::uint8_t indirect_self_scope_observation_count_;
    std::uint8_t before_escape_direct_lexical_scope_observation_count_;
    std::uint8_t direct_lexical_scope_observation_count_;
    std::uint8_t before_escape_indirect_lexical_scope_observation_count_;
    std::uint8_t indirect_lexical_scope_observation_count_;
    std::uint8_t before_escape_direct_non_lexical_scope_observation_count_;
    std::uint8_t direct_non_lexical_scope_observation_count_;
    std::uint8_t before_escape_indirect_non_lexical_scope_observation_count_;
    std::uint8_t indirect_non_lexical_scope_observation_count_;
};

#endif /* PROMISEDYNTRACER_DENOTED_VALUE_H */
