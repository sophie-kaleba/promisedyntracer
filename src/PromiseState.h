#ifndef __PROMISE_STATE_H__
#define __PROMISE_STATE_H__

#include "utilities.h"

typedef long int promise_id_t;

class PromiseState {
  public:
    enum class SlotMutation {
        ENVIRONMENT_LOOKUP = 0,
        ENVIRONMENT_ASSIGN,
        EXPRESSION_LOOKUP,
        EXPRESSION_ASSIGN,
        VALUE_LOOKUP,
        VALUE_ASSIGN,
        COUNT
    };

    bool local;
    bool argument;
    promise_id_t id;
    env_id_t env_id;
    function_id_t fn_id;
    call_id_t call_id;
    int formal_parameter_position;
    parameter_mode_t parameter_mode;
    bool evaluated;
    std::vector<int> mutations;

    PromiseState(promise_id_t id, env_id_t environment_id, bool local)
        : local(local), argument(false), id(id), env_id(environment_id), fn_id(""),
          call_id(0), formal_parameter_position(-1),
          parameter_mode(parameter_mode_t::UNASSIGNED), evaluated(false),
          mutations(std::vector<int>(to_underlying_type(SlotMutation::COUNT))),
          transitive_side_effect_observer_(false),
          direct_side_effect_observer_(false),
          transitive_side_effect_creator_(false),
          direct_side_effect_creator_(false),
          transitive_lexical_scope_mutator_(false),
          direct_lexical_scope_mutator_(false),
          transitive_non_lexical_scope_mutator_(false),
          direct_non_lexical_scope_mutator_(false),
          creation_timestamp_(UNDEFINED_TIMESTAMP) {
    }

    promise_id_t get_id() const {
        return id;
    }

    env_id_t get_environment_id() const {
        return env_id;
    }

    std::string get_expression() const {
        // TODO
        return "UNIMPLEMENTED";
    }

    void make_function_argument(function_id_t fn_id, call_id_t call_id,
                                    int formal_parameter_position,
                                    parameter_mode_t parameter_mode) {
        this->argument = true;
        this->local = true;
        this->fn_id = fn_id;
        this->call_id = call_id;
        this->formal_parameter_position = formal_parameter_position;
        this->parameter_mode = parameter_mode;
    }

    inline void increment_mutation_slot(SlotMutation slot_mutation) {
        ++mutations[to_underlying_type(slot_mutation)];
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

private:
    timestamp_t creation_timestamp_;
    bool transitive_side_effect_observer_;
    bool direct_side_effect_observer_;
    bool transitive_side_effect_creator_;
    bool direct_side_effect_creator_;
    bool transitive_lexical_scope_mutator_;
    bool direct_lexical_scope_mutator_;
    bool transitive_non_lexical_scope_mutator_;
    bool direct_non_lexical_scope_mutator_;
};

std::string to_string(PromiseState::SlotMutation slot_mutation);

#endif /* __PROMISE_STATE_H__ */
