#ifndef PROMISEDYNTRACER_STRICTNESS_ANALYSIS_H
#define PROMISEDYNTRACER_STRICTNESS_ANALYSIS_H

#include "CallState.h"
#include "FunctionState.h"
#include "PromiseMapper.h"
#include "State.h"
#include "table.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "CallerCountMapping.h"


class StrictnessAnalysis {
  public:
    StrictnessAnalysis(tracer_state_t &tracer_state,
                       PromiseMapper *const promise_mapper,
                       const std::string &output_dir, bool truncate,
                       bool binary, int compression_level);
    void closure_entry(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void closure_exit(const closure_info_t &closure_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_exit(const builtin_info_t &builtin_info);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void promise_value_lookup(const prom_info_t &prom_info, const SEXP promise);
    void promise_value_assign(const prom_info_t &info, const SEXP promise);
    void promise_environment_lookup(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_environment_assign(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_expression_lookup(const prom_info_t &prom_info,
                                   const SEXP promise);
    void promise_expression_assign(const prom_info_t &prom_info,
                                   const SEXP promise);
    void context_jump(const unwind_info_t &info);

    void gc_promise_unmarked(const prom_id_t prom_id,
                             const SEXP promise) {
        /* TODO - this case throws exception for foreign promises.
                  Figure out a way to fix that. */
        return;
        const PromiseState& promise_state = promise_mapper_ -> find(prom_id);
        side_effecting_promises_data_table_->write_row(
            promise_state.fn_id, promise_state.formal_parameter_position,
            promise_state.is_side_effect_observer(),
            promise_state.is_side_effect_creator());
    }

    void environment_define_var(const SEXP symbol, const SEXP value,
                                const SEXP rho) {
        inspect_symbol_caller_(symbol);

        tracer_state_.define_variable(rho, symbol);

        identify_side_effect_creators_(rho);
    }

    void environment_assign_var(const SEXP symbol, const SEXP value,
                                const SEXP rho) {
        inspect_symbol_caller_(symbol);

        tracer_state_.update_variable(rho, symbol);

        identify_side_effect_creators_(rho);
    }

    void environment_remove_var(const SEXP symbol, const SEXP value,
                                const SEXP rho) {
        inspect_symbol_caller_(symbol);

        tracer_state_.remove_variable(rho, symbol);

        identify_side_effect_creators_(rho);
    }

    void environment_lookup_var(const SEXP symbol, const SEXP value,
                                const SEXP rho) {
        inspect_symbol_caller_(symbol);

        timestamp_t variable_timestamp =
            tracer_state_.lookup_variable(rho, symbol).get_modification_timestamp();

        /* this implies we have not seen the variable before.
           TODO - see when this case is triggered and ensure
           there is no bug */
        if (is_undefined(variable_timestamp)) {
            return;
        }

        identify_side_effect_observers_(variable_timestamp);
    }

    void identify_side_effect_creators_(const SEXP rho) {
        size_t stack_size = tracer_state_.full_stack.size();

        for (int i = stack_size - 1; i >= 0; --i) {

            stack_event_t exec_context = tracer_state_.full_stack[i];

            // closure side effects matter iff they are done in external
            // environments.
            if (exec_context.type == stack_type::CALL &&
                exec_context.function_info.type == function_type::CLOSURE) {
                if(exec_context.environment != rho) {
                    /* TODO - handle this case */
                }
                return;
            } else if (exec_context.type == stack_type::PROMISE) {
                if(exec_context.environment != rho) {
                    promise_mapper_
                        -> find(exec_context.promise_id)
                        .set_side_effect_creator();
                }
                return;
            }
        }
    }

    void identify_side_effect_observers_(const timestamp_t variable_timestamp) {
        size_t stack_size = tracer_state_.full_stack.size();

        for (int i = stack_size - 1; i >= 0; --i) {
            stack_event_t exec_context = tracer_state_.full_stack[i];

            /* If the most recent context that is responsible for this
               side effect is a closure, then return. Currently, we only
               care about promises directly responsible for side effects.
               We don't return in case of specials and builtins because
               they are more like operators in a programming language and
               everything ultimately happens in them and returning on
               encountering them will make it look like no promise has
               caused a side effect. */
            if (exec_context.type == stack_type::CALL &&
                exec_context.function_info.type == function_type::CLOSURE) {
                return;
            }

            if (exec_context.type == stack_type::PROMISE) {
                timestamp_t promise_timestamp =
                    get_promise_timestamp_(exec_context.promise_id);

                /* This happens if the promise was created before the variable
                   binding was last modified. This means the promise is observing
                   a side effect. In other words, if this promise is
                   forced at its point of creation, it will lookup a different
                   value bound to this variable.*/
                if (!is_undefined(promise_timestamp) &&
                    promise_timestamp < variable_timestamp) {
                    /* Now we need to check if the promise is an argument promise
                       or a non argument promise. */
                    promise_mapper_ -> find(exec_context.promise_id).set_side_effect_observer();
                }
            }
        }
    }

    void environment_remove_var(const SEXP symbol, const SEXP rho) {
        inspect_symbol_caller_(symbol);
    }

    void end(dyntracer_t *dyntracer);
    ~StrictnessAnalysis();

  private:
    void serialize_call_(const CallState &call_state);
    void serialize_arguments_(const CallState &call_state);
    void add_call_graph_edge_(const call_id_t callee_id);
    void metaprogram_(const prom_info_t &prom_info, const SEXP promise);
    CallState *get_call_state(const call_id_t call_id);
    void write_function_body_(const fn_id_t &fn_id,
                              const std::string &definition);
    void function_entry_(call_id_t call_id, fn_id_t fn_id,
                         const std::string &fn_type, const std::string &name,
                         int formal_parameter_count, const std::string &order,
                         const std::string &definition);
    CallState function_exit_(call_id_t call_id, sexptype_t return_value_type);
    fn_id_t infer_caller_(int nth);
    void inspect_function_caller_(const std::string &name);
    timestamp_t get_promise_timestamp_(prom_id_t promise_id);
    void update_promise_timestamp_(prom_id_t promise_id);

    void inspect_symbol_caller_(const SEXP symbol) {
        std::string name = symbol_to_string(symbol);
        /* first we check that the variable starts with .. */
        if(name[0] != '.' || name[1] != '.') return;
        /* now we check that the variable ends with
           a sequence of digits. */
        for(int i = 2; i < name.size(); ++i) {
            if(name[i] > '9' || name[i] < '0') {
                return;
            }
        }

        /* if we reach here, its because the variable name has the form
           ..n where n is a number >= 0. Because indexing in R starts
           from 1, ..0 will never occur but the code above will still let
           ..0 pass to this point. This should not be a problem in practice.
        */
        symbol_user_count_mapping_.insert(name, infer_caller_(3));
    }

    bool is_undefined(const timestamp_t timestamp) const {
        return timestamp == undefined_timestamp_;
    }

    tracer_state_t &tracer_state_;
    std::string output_dir_;
    // bool truncate_;
    // bool binary_;
    // int compression_level_;
    PromiseMapper *const promise_mapper_;
    std::unordered_map<fn_id_t, FunctionState> functions_;
    std::vector<std::string> interesting_function_names_;
    const std::string closure_type_;
    const std::string builtin_type_;
    const std::string special_type_;
    const timestamp_t undefined_timestamp_;
    DataTableStream *argument_data_table_;
    DataTableStream *call_data_table_;
    DataTableStream *call_graph_data_table_;
    DataTableStream *function_caller_data_table_;
    DataTableStream *symbol_user_data_table_;
    DataTableStream *side_effecting_promises_data_table_;
    std::vector<CallState *> call_stack_;
    std::unordered_map<call_id_t, CallState *> call_map_;
    std::unordered_set<fn_id_t> handled_functions_;
    CallerCountMapping function_caller_count_mapping_;
    CallerCountMapping symbol_user_count_mapping_;
};

#endif /* PROMISEDYNTRACER_STRICTNESS_ANALYSIS_H */
