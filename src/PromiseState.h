#ifndef __PROMISE_STATE_H__
#define __PROMISE_STATE_H__

#include "State.h"
#include "utilities.h"

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
    prom_id_t id;
    env_id_t env_id;
    fn_id_t fn_id;
    call_id_t call_id;
    int formal_parameter_position;
    parameter_mode_t parameter_mode;
    bool evaluated;
    std::vector<int> mutations;
    bool side_effect_observer_;
    bool side_effect_creator_;
    timestamp_t timestamp_;

    PromiseState(prom_id_t id, env_id_t env_, bool local)
        : local(local), argument(false), id(id), env_id(env_id), fn_id(""),
          call_id(0), formal_parameter_position(-1),
          parameter_mode(parameter_mode_t::UNASSIGNED), evaluated(false),
          mutations(std::vector<int>(to_underlying_type(SlotMutation::COUNT))),
          side_effect_observer_(false), side_effect_creator_(false) {
    }

    void make_function_argument(fn_id_t fn_id, call_id_t call_id,
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

    void set_side_effect_observer() {
        side_effect_observer_ = true;
    }

    bool is_side_effect_observer() const {
        return side_effect_observer_;
    }

    void set_side_effect_creator() {
        side_effect_creator_ = true;
    }

    bool is_side_effect_creator() const {
        return side_effect_creator_;
    }

    void set_timestamp(timestamp_t timestamp) {
        timestamp_ = timestamp;
    }

    timestamp_t get_timestamp() const {
        return timestamp_;
    }
};

std::string to_string(PromiseState::SlotMutation slot_mutation);

#endif /* __PROMISE_STATE_H__ */
