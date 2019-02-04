#include "Analyzer.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;

Analyzer::Analyzer(tracer_state_t &tracer_state,
                   const std::string &output_dir,
                   bool truncate, bool binary,
                   int compression_level)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      //truncate_{truncate}, binary_{binary}, compression_level_{compression_level},
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      closure_type_(sexptype_to_string(CLOSXP)),
      builtin_type_(sexptype_to_string(BUILTINSXP)),
      special_type_(sexptype_to_string(SPECIALSXP)),
      undefined_timestamp_(std::numeric_limits<std::size_t>::max()) {

    argument_data_table_ = create_data_table(
        output_dir + "/" + "arguments",
        {"call_id", "function_id", "parameter_position", "argument_mode",
         "expression_type", "value_type", "escape", "call_depth",
         "promise_depth", "nested_promise_depth", "force_count",
         "lookup_count", "metaprogram_count", "execution_time"},
        truncate, binary, compression_level);

    call_data_table_ = create_data_table(
        output_dir + "/" + "calls",
        {"call_id", "function_id", "function_type", "formal_parameter_count",
         "function_name", "return_value_type", "force_order",
         "intrinsic_force_order", "execution_time"},
        truncate, binary, compression_level);

    call_graph_data_table_ = create_data_table(
        output_dir + "/" + "call-graph", {"caller_id", "callee_id"}, truncate,
        binary, compression_level);

    function_caller_data_table_ = create_data_table(
        output_dir_ + "/" + "function-callers",
        {"callee_function_name", "caller_function_id", "call_count"}, truncate,
        binary, compression_level);

    symbol_user_data_table_ =
        create_data_table(output_dir_ + "/" + "symbol-users",
                          {"symbol_name", "caller_function_id", "use_count"},
                          truncate, binary, compression_level);

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

    for(const std::string& function_name : std::vector({"force",
                                                 "forceAndCall",
                                                 "delayedAssign",
                                                 "substitute",
                                                 "sQuote",
                                                 "dQuote",
                                                 "quote",
                                                 "enquote"})) {
        interesting_function_names_.push_back(function_name);
        interesting_function_names_.push_back("::" + function_name);
        interesting_function_names_.push_back("base::" + function_name);
    }
}


void Analyzer::inspect_function_caller_(const std::string &name) {
    for(auto& function_name : interesting_function_names_) {
        if(name != function_name) { continue; }
        function_caller_count_mapping_.insert(function_name, get_nth_closure_(1));
        return;
    }
    // for(auto& key_value : caller_count_mapping_) {
    //     /* if this is not the function we are looking for
    //        continue to check other functions in the table.
    //      */
    //     if(name != key_value.first) continue;
    //     const fn_id_t caller_fn_id = infer_caller_();
    //     /* insert the inferred caller of this function in the table.
    //        If insertion fails, it means the caller has called this
    //        function before. In that case, increment the function counter.
    //      */
    //     auto iter = key_value.second.insert({caller_fn_id, 1});
    //     if(!iter.second) {
    //         ++iter.first -> second;
    //     }
    //     return;
    // }
}

void Analyzer::function_entry_(call_id_t call_id, fn_id_t fn_id,
                                         const std::string &fn_type,
                                         const std::string &name,
                                         int formal_parameter_count,
                                         const std::string &order,
                                         const std::string &definition) {

    // write the caller callee edge to file
    add_call_graph_edge_(call_id);

    // write function body to file
    write_function_body_(fn_id, definition);

    // add entry to call stack and call map
    CallState *call_state{new CallState(call_id, fn_id, fn_type, name,
                                        formal_parameter_count, order)};
    call_stack_.push_back(call_state);
    call_map_.insert({call_id, call_state});
}

CallState Analyzer::function_exit_(call_id_t call_id,
                                             sexptype_t return_value_type) {
    CallState *call_state{call_stack_.back()};
    call_stack_.pop_back();

    if (call_state->get_call_id() != call_id) {
        std::cerr << "call " << call_id << " does not match "
                  << call_state->get_call_id() << " on stack." << std::endl;
        exit(EXIT_FAILURE);
    }
    call_state->set_return_value_type(return_value_type);
    call_state->make_inactive();
    call_state->set_execution_time(tracer_state_.full_stack.back().execution_time);
    serialize_call_(*call_state);
    return *call_state;
}

/* When we enter a function, push information about it on a custom call stack.
   We also update the function table to create an entry for this function.
   This entry contains the evaluation information of the function's arguments.
 */
void Analyzer::closure_entry(const closure_info_t &closure_info) {
    // push call_id to call_stack
    call_id_t call_id = closure_info.call_id;
    fn_id_t fn_id = closure_info.fn_id;

    /* this checks the caller of functions we are interested in
       and adds them to a table */
    inspect_function_caller_(closure_info.name);

    function_entry_(closure_info.call_id, closure_info.fn_id, closure_type_,
                    closure_info.name, closure_info.formal_parameter_count, "",
                    closure_info.fn_definition);

    auto fn_iter = functions_.insert(std::make_pair(
        fn_id, FunctionState(closure_info.formal_parameter_count)));
    fn_iter.first->second.increment_call();

    for (const auto &argument : closure_info.arguments) {
        call_stack_.back()->set_parameter_mode(
            argument.formal_parameter_position, argument.parameter_mode);

        /* set expression type for all arguments whether they are promises or
           not. */
        call_stack_.back()->set_expression_type(
            argument.expression_type, argument.formal_parameter_position);

        /* if expression type is missing, set value type to missing */
        if (argument.expression_type == MISSINGSXP) {
            call_stack_.back()->set_value_type(
                argument.expression_type, argument.formal_parameter_position);
        } else if(argument.expression_type == PROMSXP) {
            // call_stack_.back()->set_value_type(
            //     type_of_sexp(dyntrace_get_promise_value(argument.pr)),
            //     argument.formal_parameter_position);
        }
    }
}

void Analyzer::closure_exit(const closure_info_t &closure_info) {
    function_exit_(closure_info.call_id, closure_info.return_value_type);
}

void Analyzer::special_entry(const builtin_info_t &special_info) {
    function_entry_(special_info.call_id, special_info.fn_id, special_type_,
                    special_info.name, special_info.formal_parameter_count,
                    std::to_string(special_info.eval),
                    special_info.fn_definition);
}

void Analyzer::special_exit(const builtin_info_t &special_info) {
    function_exit_(special_info.call_id, special_info.return_value_type);
}

void Analyzer::builtin_entry(const builtin_info_t &builtin_info) {
    function_entry_(builtin_info.call_id, builtin_info.fn_id, builtin_type_,
                    builtin_info.name, builtin_info.formal_parameter_count,
                    std::to_string(builtin_info.eval),
                    builtin_info.fn_definition);
}

void Analyzer::builtin_exit(const builtin_info_t &builtin_info) {
    function_exit_(builtin_info.call_id, builtin_info.return_value_type);
}

void Analyzer::context_jump(const unwind_info_t &info) {
    for (auto &element : info.unwound_frames) {
        if (element.type == stack_type::CALL) {
            CallState call_state{function_exit_(element.call_id, JUMPSXP)};
            if (call_state.get_function_type() == "Closure") {
                // serialize_arguments_(call_state);
            }
        }
    }
}

void Analyzer::promise_created(const prom_basic_info_t &prom_basic_info,
                                         const SEXP promise) {
}

timestamp_t Analyzer::get_promise_timestamp_(prom_id_t promise_id) {
    return tracer_state_.lookup_promise(promise_id).get_creation_timestamp();
}

void Analyzer::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    PromiseState &promise_state{tracer_state_.lookup_promise(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);
    eval_depth_t eval_depth = get_evaluation_depth_(promise_state.call_id);
    call_state->force_entry(promise,
                            promise_state.formal_parameter_position,
                            eval_depth);
}

void Analyzer::promise_force_exit(const prom_info_t &prom_info,
                                            const SEXP promise) {
    PromiseState &promise_state{
        tracer_state_.lookup_promise(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    call_state->force_exit(promise, promise_state.formal_parameter_position,
                           tracer_state_.full_stack.back().execution_time);
}

void Analyzer::promise_value_lookup(const prom_info_t &prom_info,
                                              const SEXP promise) {
    PromiseState &promise_state{
        tracer_state_.lookup_promise(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    call_state->lookup(promise_state.formal_parameter_position);
}

void Analyzer::promise_value_assign(const prom_info_t &prom_info,
                                              const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void Analyzer::promise_environment_lookup(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void Analyzer::promise_environment_assign(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void Analyzer::promise_expression_lookup(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void Analyzer::promise_expression_assign(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void Analyzer::end(dyntracer_t *dyntracer) {

    for (const auto &key_value : call_map_) {
        const CallState &call_state = *key_value.second;
        if(call_state.get_function_type() == "Closure") {
            serialize_arguments_(call_state);
        }
    }

    function_caller_count_mapping_.serialize(function_caller_data_table_);
    symbol_user_count_mapping_.serialize(symbol_user_data_table_);

}

// void Analyzer::serialize_function_callers_() {
//     for(const auto & key_value : function_caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             function_caller_data_table_ -> write_row(key_value.first,
//                                                     caller_count.first,
//                                                     caller_count.second);
//         }
//     }
// }

// void Analyzer::serialize_function_callers_() {
//     for(const auto & key_value : function_caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             function_caller_data_table_ -> write_row(key_value.first,
//                                                      caller_count.first,
//                                                      caller_count.second);
//         }
//     }
// }

// void Analyzer::serialize_caller_count_mapping_(const caller_table_t& caller_count_mapping,
//                                                          DataTableStream* data_table) {
//     for(const auto & key_value : caller_count_mapping_) {
//         for (const auto & caller_count : key_value.second) {
//             data_table_ -> write_row(key_value.first,
//                                      caller_count.first,
//                                      caller_count.second);
//         }
//     }
// }

Analyzer::~Analyzer() {
    delete argument_data_table_;
    delete call_data_table_;
    delete call_graph_data_table_;
    delete function_caller_data_table_;
    delete symbol_user_data_table_;
}

void Analyzer::serialize_arguments_(const CallState &call_state) {

    const auto &parameter_uses{call_state.get_parameter_uses()};

    for (int position = 0; position < call_state.get_formal_parameter_count();
         ++position) {

        const auto &parameter{parameter_uses[position]};

        argument_data_table_->write_row(
            static_cast<double>(call_state.get_call_id()),
            call_state.get_function_id(), position,
            parameter_mode_to_string(parameter.get_parameter_mode()),
            sexptype_to_string(parameter.get_expression_type()),
            sexptype_to_string(parameter.get_value_type()),
            parameter.get_escape(),
            parameter.get_evaluation_depth().call_depth,
            parameter.get_evaluation_depth().promise_depth,
            parameter.get_evaluation_depth().nested_promise_depth,
            parameter.get_force(),
            parameter.get_lookup(), parameter.get_metaprogram(),
            parameter.get_execution_time());
    }
}

void Analyzer::serialize_call_(const CallState &call_state) {
    call_data_table_->write_row(
        static_cast<double>(call_state.get_call_id()),
        call_state.get_function_id(), call_state.get_function_type(),
        call_state.get_formal_parameter_count(), call_state.get_function_name(),
        sexptype_to_string(call_state.get_return_value_type()),
        call_state.get_order(), call_state.get_intrinsic_order(),
        call_state.get_execution_time());
}

void Analyzer::metaprogram_(const prom_info_t &prom_info,
                                      const SEXP promise) {
    PromiseState &promise_state{
        tracer_state_.lookup_promise(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    call_state->metaprogram(promise_state.formal_parameter_position);
}

CallState *Analyzer::get_call_state(const call_id_t call_id) {

    auto it{call_stack_.rbegin()};
    bool leaf = true;

    for (; it != call_stack_.rend(); ++it) {
        if ((*it)->get_call_id() == call_id) {
            break;
        }
        leaf &= ((*it)->get_function_type() != "Closure");
    }

    if (it == call_stack_.rend()) {
        return call_map_.find(call_id)->second;
    }

    (*it)->set_leaf(leaf);

    return (*it);
}

void Analyzer::add_call_graph_edge_(const call_id_t callee_id) {
    if (call_stack_.empty())
        return;
    call_graph_data_table_->write_row(
        static_cast<double>(call_stack_.back()->get_call_id()),
        static_cast<double>(callee_id));
}

void Analyzer::write_function_body_(const fn_id_t &fn_id,
                                              const std::string &definition) {
    auto result = handled_functions_.insert(fn_id);
    if (!result.second)
        return;
    std::ofstream fout(output_dir_ + "/functions/" + fn_id, std::ios::trunc);
    fout << definition;
    fout.close();
}
