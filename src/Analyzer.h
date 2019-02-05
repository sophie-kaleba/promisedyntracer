#ifndef PROMISEDYNTRACER_ANALYZER_H
#define PROMISEDYNTRACER_ANALYZER_H

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


class Analyzer {
  public:
    Analyzer(tracer_state_t &tracer_state,
             const std::string &output_dir, bool truncate,
             bool binary, int compression_level);
    void begin(dyntracer_t* dyntracer) {}
    void closure_entry(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void closure_exit(const closure_info_t &closure_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_exit(const builtin_info_t &builtin_info);

    void promise_created(PromiseState* promise_state,
                         const SEXP promise) {
        // TODO check the source of creation of this promise?
    }

    void promise_force_entry(PromiseState *promise_state, const SEXP promise) {
        /* if promise is not an argument, then don't process it. */
        if (!promise_state->argument) {
            return;
        }

        auto *call_state = get_call_state(promise_state->call_id);
        eval_depth_t eval_depth = get_evaluation_depth_(promise_state->call_id);
        call_state->force_entry(
            promise, promise_state->formal_parameter_position, eval_depth);
    }

    void promise_force_exit(PromiseState *promise_state, const SEXP promise) {
        /* if promise is not an argument, then don't process it. */
        if (!promise_state->argument) {
            return;
        }

        auto *call_state = get_call_state(promise_state->call_id);

        call_state->force_exit(promise,
                               promise_state->formal_parameter_position,
                               tracer_state_.full_stack.back().execution_time);
    }

    void promise_value_lookup(PromiseState *promise_state, const SEXP promise) {

        /* if promise is not an argument, then don't process it. */
        if (!promise_state->argument) {
            return;
        }

        auto *call_state = get_call_state(promise_state->call_id);

        call_state->lookup(promise_state->formal_parameter_position);
    }

    void promise_value_assign(PromiseState *promise_state, const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void promise_environment_lookup(PromiseState *promise_state,
                                    const SEXP promise) {
        metaprogram_(promise_state, promise);
    }
    void promise_environment_assign(PromiseState *promise_state,
                                    const SEXP promise) {
        metaprogram_(promise_state, promise);
    }
    void promise_expression_lookup(PromiseState *promise_state,
                                   const SEXP promise) {
        metaprogram_(promise_state, promise);
    }
    void promise_expression_assign(PromiseState *promise_state,
                                   const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void context_jump(const unwind_info_t &info);

    void gc_promise_unmarked(PromiseState * promise_state,
                             const SEXP promise) {
        // side_effecting_promises_data_table_ -> write_row(
        //     promise_state -> fn_id, promise_state.formal_parameter_position,
        //     promise_state -> is_direct_side_effect_observer(),
        //     promise_state -> is_transitive_side_effect_observer());
    }

    void environment_variable_define(const Variable &var) {

        inspect_symbol_caller_(var);
    }

    void environment_variable_assign(const Variable &var) {

        inspect_symbol_caller_(var);

        identify_side_effect_creators_(var);
    }

    void environment_variable_remove(const Variable &var) {

        inspect_symbol_caller_(var);
    }

    void environment_variable_lookup(const Variable& var) {

        inspect_symbol_caller_(var);

        identify_side_effect_observers_(var);
    }

    void identify_side_effect_creators_(const Variable& var) {

        const timestamp_t var_timestamp = var.get_modification_timestamp();

        /* this implies we have not seen the variable before.
           TODO - see when this case is triggered and ensure
           there is no bug */
        if (timestamp_is_undefined(var_timestamp)) {
            return;
        }

        size_t stack_size = tracer_state_.full_stack.size();

        bool direct = true;

        for (int i = stack_size - 1; i >= 0; --i) {

            stack_event_t exec_context = tracer_state_.full_stack[i];

            if (exec_context.type == stack_type::PROMISE) {

                PromiseState *promise_state = exec_context.promise_state;

                /* if promise is created after the variable it is currently
                   assigning to, then its a side effect creator. */
                if (promise_state -> get_creation_timestamp() > var_timestamp) {

                    if (direct)
                        promise_state -> set_direct_side_effect_creator();
                    else
                        promise_state -> set_transitive_side_effect_creator();

                    /* after the first promise is encountered, all promises
                       are
                       only indirectly responsible for this side effect */

                    direct = false;
                }
            }
        }
    }

    void identify_side_effect_observers_(const Variable& var) {

        const timestamp_t var_timestamp = var.get_modification_timestamp();

        /* this implies we have not seen the variable before.
           TODO - see when this case is triggered and ensure
           there is no bug */
        if (timestamp_is_undefined(var_timestamp)) {
            return;
        }

        /* if the modification timestamp of the variable is
           greater than the creation timestamp of the promise,
           then, that promise has identified a side effect */

        size_t stack_size = tracer_state_.full_stack.size();
        bool direct = true;

        for (int i = stack_size - 1; i >= 0; --i) {
            stack_event_t exec_context = tracer_state_.full_stack[i];

            // /* If the most recent context that is responsible for this
            //    side effect is a closure, then return. Currently, we only
            //    care about promises directly responsible for side effects.
            //    We don't return in case of specials and builtins because
            //    they are more like operators in a programming language and
            //    everything ultimately happens in them and returning on
            //    encountering them will make it look like no promise has
            //    caused a side effect. */
            // if (exec_context.type == stack_type::CALL &&
            //     exec_context.function_info.type == function_type::CLOSURE) {
            //     return;
            // }

            if (exec_context.type == stack_type::PROMISE) {

                PromiseState *promise_state = exec_context.promise_state;

                /* if promise is created before the variable it is looking at
                   is modified, then it is a side effect observer. In other
                   words, if this promise is forced at its point of creation,
                   it will lookup a different value bound to this variable.*/
                if (var_timestamp > promise_state -> get_creation_timestamp()) {
                    if (direct)
                        promise_state -> set_direct_side_effect_observer();
                    else
                        promise_state -> set_transitive_side_effect_observer();

                    /* after the first promise is encountered, all promises are
                       only indirectly responsible for this side effect */

                    direct = false;
                }
            }
        }
    }


    void end(dyntracer_t *dyntracer);
    ~Analyzer();

  private:
    void serialize_call_(const CallState &call_state);
    void serialize_arguments_(const CallState &call_state);
    void add_call_graph_edge_(const call_id_t callee_id);

    void metaprogram_(const PromiseState* promise_state,
                                const SEXP promise) {
        /* if promise is not an argument, then don't process it. */
        if (!promise_state -> argument) {
            return;
        }

        auto *call_state = get_call_state(promise_state -> call_id);

        call_state->metaprogram(promise_state -> formal_parameter_position);
    }

    CallState *get_call_state(const call_id_t call_id);
    void write_function_body_(const fn_id_t &fn_id,
                              const std::string &definition);
    void function_entry_(call_id_t call_id, fn_id_t fn_id,
                         const std::string &fn_type, const std::string &name,
                         int formal_parameter_count, const std::string &order,
                         const std::string &definition);
    CallState function_exit_(call_id_t call_id, sexptype_t return_value_type);
    void inspect_function_caller_(const std::string &name);
    bool is_dot_dot_variable_(const Variable& var) {
        const std::string& name = var.get_name();
        if (name.size() < 2) {
            return false;
        }

        /* first we check that the variable starts with .. */
        if (name[0] != '.' || name[1] != '.')
            return false;
        /* now we check that the variable ends with
           a sequence of digits. */
        for (int i = 2; i < name.size(); ++i) {
            if (name[i] > '9' || name[i] < '0') {
                return false;
            }
        }

        /* if we reach here, its because the variable name has the form
           ..n where n is a number >= 0. Because indexing in R starts
           from 1, ..0 will never occur but the code above will still let
           ..0 pass to this point. This should not be a problem in practice.
        */
        return true;
    }

    fn_id_t get_nth_closure_(int nth) {
        int n = 0;
        for (auto it = call_stack_.rbegin(); it != call_stack_.rend(); ++it) {
            if ((*it)->get_function_type() == "Closure") {
                ++n;
                if (n == nth) {
                    return (*it)->get_function_id();
                }
            }
        }
        return "tracer_failed_to_infer_caller";
    }

    void inspect_symbol_caller_(const Variable& var) {
        if(is_dot_dot_variable_(var))
            symbol_user_count_mapping_.insert(var.get_name(),
                                              get_nth_closure_(0));
    }

    bool is_undefined(const timestamp_t timestamp) const {
        return timestamp == undefined_timestamp_;
    }

    eval_depth_t get_evaluation_depth_(call_id_t call_id) {
        eval_depth_t eval_depth = {0, 0, 0};
        bool nesting = true;
        size_t stack_size = tracer_state_.full_stack.size();
        int i;
        /* we start from last - 1 element because the promise context
           in question will itself be the last element. It does not make
           sense to count the promise itself in the computation of its own
           evaluation depth */
        for (i = stack_size - 2; i >= 0; --i) {

            stack_event_t exec_context = tracer_state_.full_stack[i];

            if (exec_context.type == stack_type::CALL &&
                exec_context.function_info.type == function_type::CLOSURE) {
                nesting = false;
                if(exec_context.call_id == call_id) break;
                ++eval_depth.call_depth;
            } else if (exec_context.type == stack_type::PROMISE) {
                ++eval_depth.promise_depth;
                if(nesting) ++eval_depth.nested_promise_depth;
            } else {
                // nesting should be made false for other cases as well.
                nesting = false;
            }
        }

        // if this happens, it means we could not locate the call from which
        // this promise originated. This means that this is an escaped promise.
        if(i < 0) {
            return ESCAPED_PROMISE_EVAL_DEPTH;
        }

        return eval_depth;
    }

    tracer_state_t &tracer_state_;
    std::string output_dir_;

    // bool truncate_;
    // bool binary_;
    // int compression_level_;
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

#endif /* PROMISEDYNTRACER_ANALYZER_H */
