#ifndef PROMISEDYNTRACER_ANALYZER_H
#define PROMISEDYNTRACER_ANALYZER_H

//#include "FunctionState.h"
#include "State.h"
#include "table.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>
#include "CallerCountMapping.h"


class Analyzer {
  public:
    Analyzer(tracer_state_t &tracer_state, const std::string &output_dir,
             bool truncate, bool binary, int compression_level)
        : tracer_state_(tracer_state), output_dir_(output_dir),
          // truncate_{truncate}, binary_{binary},
          // compression_level_{compression_level},
          //functions_(std::unordered_map<function_id_t, FunctionState>(
          //    FUNCTION_MAPPING_BUCKET_SIZE)),
          closure_type_(sexptype_to_string(CLOSXP)),
          builtin_type_(sexptype_to_string(BUILTINSXP)),
          special_type_(sexptype_to_string(SPECIALSXP)),
          undefined_timestamp_(std::numeric_limits<std::size_t>::max()) {

        argument_data_table_ = create_data_table(
            output_dir + "/" + "arguments",
            {"call_id", "function_id", "formal_parameter_position",
             "actual_argument_position", "argument_type", "expression_type",
             "value_type", "default", "escape", "call_depth",
             "promise_depth", "nested_promise_depth", "force_count",
             "lookup_count", "metaprogram_count", "execution_time"},
            truncate, binary, compression_level);

        call_graph_data_table_ = create_data_table(
            output_dir + "/" + "call-graph", {"caller_id", "callee_id"},
            truncate, binary, compression_level);

        function_caller_data_table_ = create_data_table(
            output_dir_ + "/" + "function-callers",
            {"callee_function_name", "caller_function_id", "call_count"},
            truncate, binary, compression_level);

        symbol_user_data_table_ = create_data_table(
            output_dir_ + "/" + "symbol-users",
            {"symbol_name", "caller_function_id", "use_count"}, truncate,
            binary, compression_level);

        // caller_count_mapping_.insert({"delayedAssign", {}});
        // caller_count_mapping_.insert({"::delayedAssign", {}});
        // caller_count_mapping_.insert({"base::delayedAssign", {}});

        // caller_count_mapping_.insert({"force", {}});
        // caller_count_mapping_.insert({"::force", {}});
        // caller_count_mapping_.insert({"base::force", {}});

        // caller_count_mapping_.insert({"forceAndCall", {}});
        // caller_count_mapping_.insert({"::forceAndCall", {}});
        // caller_count_mapping_.insert({"base::forceAndCall", {}});

        // caller_count_mapping_.insert({"substitute", {}});
        // caller_count_mapping_.insert({"::substitute", {}});
        // caller_count_mapping_.insert({"base::substitute", {}});

        // caller_count_mapping_.insert({"sQuote", {}});
        // caller_count_mapping_.insert({"::sQuote", {}});
        // caller_count_mapping_.insert({"base::sQuote", {}});

        // caller_count_mapping_.insert({"dQuote", {}});
        // caller_count_mapping_.insert({"::dQuote", {}});
        // caller_count_mapping_.insert({"base::dQuote", {}});

        // caller_count_mapping_.insert({"quote", {}});
        // caller_count_mapping_.insert({"::quote", {}});
        // caller_count_mapping_.insert({"base::quote", {}});

        // caller_count_mapping_.insert({"enquote", {}});
        // caller_count_mapping_.insert({"::enquote", {}});
        // caller_count_mapping_.insert({"base::enquote", {}});

        // if (verbose) {
        //     std::cout << analysis_switch;
        // }

        for (const std::string &function_name : std::vector(
                 {"force", "forceAndCall", "delayedAssign", "substitute",
                  "sQuote", "dQuote", "quote", "enquote"})) {
            interesting_function_names_.push_back(function_name);
            interesting_function_names_.push_back("::" + function_name);
            interesting_function_names_.push_back("base::" + function_name);
        }
    }

    void begin(dyntracer_t* dyntracer) {}

    void closure_entry(CallState * call_state) {
        // TODO - remove this
        // write the caller callee edge to file
        //add_call_graph_edge_(call_id);


        /* this checks the caller of functions we are interested in
           and adds them to a table */
        // TODO fix this
        //inspect_function_caller_(closure_);
    }

    void closure_exit(CallState *call_state) {
        serialize_arguments_(call_state);
    }

    void special_entry(CallState *call_state) {
        // TODO - remove this
        // write the caller callee edge to file
        //add_call_graph_edge_(call_id);
    }

    void special_exit(CallState *call_state) {
        serialize_call_(call_state);
    }

    void builtin_entry(CallState *call_state) {
        // TODO - remove this
        // write the caller callee edge to file
        //add_call_graph_edge_(call_id);
    }

    void builtin_exit(CallState *call_state) {
        serialize_call_(call_state);
    }

    void promise_created(PromiseState* promise_state,
                         const SEXP promise) {
        // TODO check the source of creation of this promise?
    }

    void promise_force_entry(ExecutionContextStack& stack,
                             PromiseState* promise_state,
                             const SEXP promise) {
        /* TODO - this is the place to deal with escaped promises */
        /* if promise is not an argument, then don't process it. */
        if (!promise_state->is_argument()) {
            return;
        }

        /* we know that the call state is valid because this is an argument promise */
        auto *call_state = promise_state -> get_call();

        /* At this point we should store expression type because this is the
           expression that yields the value obtained on promise exit */
        if(promise_state -> is_forced()) {

        }
        else {
            eval_depth_t eval_depth = get_evaluation_depth_(stack, call_state->get_id());
            promise_state -> set_evaluation_depth(eval_depth);
            promise_state -> force();
            /* TODO - do we care about actual argument position? */
            int formal_parameter_position = promise_state->get_formal_parameter_position();
            call_state->add_to_force_order(formal_parameter_position);
        }
    }

    // TODO remove promise argument from all of these cases
    void promise_value_lookup(PromiseState* promise_state,
                              const SEXP promise) {

        /* if promise is not an argument, then don't process it. */
        if (!promise_state -> is_argument()) {
            return;
        }

        promise_state -> lookup();
    }

    void promise_value_assign(PromiseState* promise_state,
                              const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void promise_environment_lookup(PromiseState* promise_state,
                                    const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void promise_environment_assign(PromiseState* promise_state,
                                    const SEXP promise) {

        metaprogram_(promise_state, promise);
    }

    void promise_expression_lookup(PromiseState* promise_state,
                                   const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void promise_expression_assign(PromiseState* promise_state,
                                   const SEXP promise) {
        metaprogram_(promise_state, promise);
    }

    void context_jump(execution_contexts_t &exec_ctxts) {
        // TODO - serialize calls which have been unwound and write their type
        // for (auto &element : info.unwound_frames) {
        //     if (element.type == stack_type::CALL) {
        //
        //         //        as JUMPSXP
        //     }
        // }
    }

    void gc_promise_unmarked(PromiseState*  promise_state,
                             const SEXP promise) {
        // side_effecting_promises_data_table_ -> write_row(
        //     promise_state -> fn_id, promise_state.formal_parameter_position,
        //     promise_state -> is_direct_side_effect_observer(),
        //     promise_state -> is_transitive_side_effect_observer());
    }

    void environment_variable_define(const Variable &var) {

        inspect_symbol_caller_(var);
    }

    void environment_variable_assign(ExecutionContextStack& stack,
                                     const Variable &var) {

        inspect_symbol_caller_(var);

        identify_side_effect_creators_(stack, var);
    }

    void environment_variable_remove(const Variable &var) {

        inspect_symbol_caller_(var);
    }

    void environment_variable_lookup(ExecutionContextStack& stack,
                                     const Variable& var) {

        inspect_symbol_caller_(var);

        identify_side_effect_observers_(stack, var);
    }

    void identify_side_effect_creators_(ExecutionContextStack& stack,
                                        const Variable& var) {

        const timestamp_t var_timestamp = var.get_modification_timestamp();

        /* this implies we have not seen the variable before.
           TODO - see when this case is triggered and ensure
           there is no bug */
        if (timestamp_is_undefined(var_timestamp)) {
            return;
        }

        bool direct = true;

        for (auto iter = stack.rbegin(); iter != stack.rend(); ++iter) {

            ExecutionContext& exec_ctxt = *iter;

            if (exec_ctxt.is_promise()) {

                PromiseState* promise_state = exec_ctxt.get_promise();

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

    void identify_side_effect_observers_(ExecutionContextStack& stack,
                                         const Variable& var) {

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

        bool direct = true;

        for (auto iter = stack.rbegin(); iter != stack.rend(); ++iter) {

            ExecutionContext& exec_ctxt = *iter;

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

            if (exec_ctxt.is_promise()) {

                PromiseState* promise_state = exec_ctxt.get_promise();

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


    void end(dyntracer_t *dyntracer) {
        // TODO - serialize functions on exit. This should be done later on
        // when the function state mappings are fully there.
        // the function state will summarize results from call state.
        // serializing it will result in a few entries in the functions table
        // and its definition file.

        // TODO check if this is non empty on exit.
        // it should not be.
        // for (const auto &key_value : call_map_) {
        //     const CallState &call_state = *key_value.second;
        //     if(call_state.get_function_type() == "Closure") {
        //         serialize_arguments_(call_state);
        //     }
        // }

        function_caller_count_mapping_.serialize(function_caller_data_table_);
        symbol_user_count_mapping_.serialize(symbol_user_data_table_);
    }

    ~Analyzer() {
        delete argument_data_table_;
        delete call_graph_data_table_;
        delete function_caller_data_table_;
        delete symbol_user_data_table_;
    }

  private:
    void serialize_call_(const CallState *call_state) {
        // call_data_table_->write_row(static_cast<double>(call_state -> get_id()),
        //                             call_state -> get_function_id(),
        //                             sexptype_to_string(call_state -> get_function_type()),
        //                             call_state -> get_formal_parameter_count(),
        //                             call_state -> get_function_name(),
        //                             sexptype_to_string(call_state -> get_return_value_type()),
        //                             call_state -> get_force_order());
    }

    void serialize_arguments_(CallState *call_state) {

            for (PromiseState* argument : call_state -> get_arguments()) {
                argument_data_table_->write_row(
                    static_cast<double>(call_state->get_id()),
                    call_state->get_function_id(),
                    argument->get_formal_parameter_position(),
                    argument->get_actual_argument_position(),
                    sexptype_to_string(argument->get_type()),
                    sexptype_to_string(argument->get_expression_type()),
                    sexptype_to_string(argument->get_value_type()),
                    argument -> is_default(),
                    argument->get_escape(),
                    argument->get_evaluation_depth().call_depth,
                    argument->get_evaluation_depth().promise_depth,
                    argument->get_evaluation_depth().nested_promise_depth,
                    argument->get_force(), argument->get_lookup(),
                    argument->get_metaprogram(),
                    argument->get_execution_time());
            }
    }

    void metaprogram_(PromiseState* promise_state,
                      const SEXP promise) {
        /* this is the point to check for escaped promises */
        /* if promise is not an argument, then don't process it. */
        promise_state -> metaprogram();
    }


    void function_entry_(call_id_t call_id, function_id_t fn_id,
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

    function_id_t get_nth_closure_(int nth) {
        int n = 0;
        // TODO - fix this stack walking.
        // for (auto it = call_stack_.rbegin(); it != call_stack_.rend(); ++it) {
        //     if ((*it)->get_function_type() == "Closure") {
        //         ++n;
        //         if (n == nth) {
        //             return (*it)->get_function_id();
        //         }
        //     }
        // }
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

    eval_depth_t get_evaluation_depth_(ExecutionContextStack& stack, call_id_t call_id) {
        eval_depth_t eval_depth = {0, 0, 0};
        bool nesting = true;
        ExecutionContextStack::reverse_iterator iter;

        // TODO - should we ignore builtins and specials?
        for (iter = stack.rbegin(); iter != stack.rend(); ++iter) {
            ExecutionContext& exec_ctxt = *iter;

            if(exec_ctxt.is_closure()) {
                const CallState * call_state = exec_ctxt.get_closure();
                nesting = false;
                if(call_state -> get_id() == call_id) break;
                ++eval_depth.call_depth;
            }
            else if(exec_ctxt.is_promise()) {
                ++eval_depth.promise_depth;
                if(nesting) ++eval_depth.nested_promise_depth;
            }
            else {
                // nesting should be made false for other cases as well.
                nesting = false;
            }
        }

        // if this happens, it means we could not locate the call from which
        // this promise originated. This means that this is an escaped promise.
        if(iter == stack.rend()) {
            return ESCAPED_PROMISE_EVAL_DEPTH;
        }

        return eval_depth;
    }

    tracer_state_t &tracer_state_;
    std::string output_dir_;

    // bool truncate_;
    // bool binary_;
    // int compression_level_;
    //std::unordered_map<function_id_t, FunctionState> functions_;
    std::vector<std::string> interesting_function_names_;
    const std::string closure_type_;
    const std::string builtin_type_;
    const std::string special_type_;
    const timestamp_t undefined_timestamp_;
    DataTableStream *argument_data_table_;
    DataTableStream *call_graph_data_table_;
    DataTableStream *function_caller_data_table_;
    DataTableStream *symbol_user_data_table_;
    DataTableStream *side_effecting_promises_data_table_;
    CallerCountMapping function_caller_count_mapping_;
    CallerCountMapping symbol_user_count_mapping_;
    };

#endif /* PROMISEDYNTRACER_ANALYZER_H */
