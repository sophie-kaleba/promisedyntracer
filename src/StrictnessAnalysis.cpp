#include "StrictnessAnalysis.h"

const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;

StrictnessAnalysis::StrictnessAnalysis(const tracer_state_t &tracer_state,
                                       PromiseMapper *const promise_mapper,
                                       const std::string &output_dir,
                                       bool truncate, bool binary,
                                       int compression_level)
    : tracer_state_(tracer_state), output_dir_(output_dir),
      promise_mapper_(promise_mapper),
      functions_(std::unordered_map<fn_id_t, FunctionState>(
          FUNCTION_MAPPING_BUCKET_SIZE)),
      closure_type_{sexptype_to_string(CLOSXP)},
      builtin_type_{sexptype_to_string(BUILTINSXP)},
      special_type_{sexptype_to_string(SPECIALSXP)} {

    argument_data_table_ =
        create_data_table(output_dir + "/" + "arguments",
                          {"call_id", "function_id", "parameter_position",
                           "argument_mode", "expression_type", "value_type",
                           "force_count", "lookup_count", "metaprogram_count"},
                          truncate, binary, compression_level);

    call_data_table_ = create_data_table(
        output_dir + "/" + "calls",
        {"call_id", "function_id", "function_type", "formal_parameter_count",
         "function_name", "return_value_type", "force_order"},
        truncate, binary, compression_level);

    call_graph_data_table_ = create_data_table(
        output_dir + "/" + "call-graph", {"caller_id", "callee_id"}, truncate,
        binary, compression_level);
}

void StrictnessAnalysis::function_entry_(call_id_t call_id, fn_id_t fn_id,
                                         const std::string &fn_type,
                                         const std::string &name,
                                         int formal_parameter_count,
                                         const std::string &order,
                                         const std::string &definition) {
    // write the caller callee edge to file
    add_call_graph_edge_(call_id);

    // write function body to file
    write_function_body_(fn_id, definition);

    // add entry to call stack
    call_stack_.push_back(CallState(call_id, fn_id, fn_type, name,
                                    formal_parameter_count, order));
}

CallState StrictnessAnalysis::function_exit_(call_id_t call_id,
                                             sexptype_t return_value_type) {
    CallState call_state{call_stack_.back()};
    call_stack_.pop_back();

    if (call_state.get_call_id() != call_id) {
        std::cerr << "call " << call_id << " does not match "
                  << call_state.get_call_id() << " on stack." << std::endl;
        exit(EXIT_FAILURE);
    }
    call_state.set_return_value_type(return_value_type);
    serialize_call_(call_state);
    return call_state;
}

/* When we enter a function, push information about it on a custom call stack.
   We also update the function table to create an entry for this function.
   This entry contains the evaluation information of the function's arguments.
 */
void StrictnessAnalysis::closure_entry(const closure_info_t &closure_info) {
    // push call_id to call_stack
    call_id_t call_id = closure_info.call_id;
    fn_id_t fn_id = closure_info.fn_id;

    function_entry_(closure_info.call_id, closure_info.fn_id, closure_type_,
                    closure_info.name, closure_info.formal_parameter_count, "",
                    closure_info.fn_definition);

    auto fn_iter = functions_.insert(std::make_pair(
        fn_id, FunctionState(closure_info.formal_parameter_count)));
    fn_iter.first->second.increment_call();

    for (const auto &argument : closure_info.arguments) {
        call_stack_.back().set_parameter_mode(
            argument.formal_parameter_position, argument.parameter_mode);

        /* set expression type for all arguments whether they are promises or
           not. */
        call_stack_.back().set_expression_type(
            argument.expression_type, argument.formal_parameter_position);

        /* if expression type is missing, set value type to missing */
        if (argument.expression_type == MISSINGSXP) {
            call_stack_.back().set_value_type(
                argument.expression_type, argument.formal_parameter_position);
        }
    }
}

void StrictnessAnalysis::closure_exit(const closure_info_t &closure_info) {
    serialize_arguments_(
        function_exit_(closure_info.call_id, closure_info.return_value_type));
}

void StrictnessAnalysis::special_entry(const builtin_info_t &special_info) {
    function_entry_(special_info.call_id, special_info.fn_id, special_type_,
                    special_info.name, special_info.formal_parameter_count,
                    std::to_string(special_info.eval),
                    special_info.fn_definition);
}

void StrictnessAnalysis::special_exit(const builtin_info_t &special_info) {
    function_exit_(special_info.call_id, special_info.return_value_type);
}

void StrictnessAnalysis::builtin_entry(const builtin_info_t &builtin_info) {
    function_entry_(builtin_info.call_id, builtin_info.fn_id, builtin_type_,
                    builtin_info.name, builtin_info.formal_parameter_count,
                    std::to_string(builtin_info.eval),
                    builtin_info.fn_definition);
}

void StrictnessAnalysis::builtin_exit(const builtin_info_t &builtin_info) {
    function_exit_(builtin_info.call_id, builtin_info.return_value_type);
}

void StrictnessAnalysis::context_jump(const unwind_info_t &info) {
    for (auto &element : info.unwound_frames) {
        if (element.type == stack_type::CALL) {
            function_exit_(element.call_id, JUMPSXP);
        }
    }
}

void StrictnessAnalysis::promise_force_entry(const prom_info_t &prom_info,
                                             const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->force_entry(promise, promise_state.formal_parameter_position);
}

void StrictnessAnalysis::promise_force_exit(const prom_info_t &prom_info,
                                            const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->force_exit(promise, promise_state.formal_parameter_position);
}

void StrictnessAnalysis::promise_value_lookup(const prom_info_t &prom_info,
                                              const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->lookup(promise_state.formal_parameter_position);
}

void StrictnessAnalysis::promise_value_assign(const prom_info_t &prom_info,
                                              const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void StrictnessAnalysis::promise_environment_lookup(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_environment_assign(
    const prom_info_t &prom_info, const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_expression_lookup(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}
void StrictnessAnalysis::promise_expression_assign(const prom_info_t &prom_info,
                                                   const SEXP promise) {
    metaprogram_(prom_info, promise);
}

void StrictnessAnalysis::end(dyntracer_t *dyntracer) {}

StrictnessAnalysis::~StrictnessAnalysis() {
    delete argument_data_table_;
    delete call_data_table_;
    delete call_graph_data_table_;
}

void StrictnessAnalysis::serialize_arguments_(const CallState &call_state) {

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
            parameter.get_force(), parameter.get_lookup(),
            parameter.get_metaprogram());
    }
}

void StrictnessAnalysis::serialize_call_(const CallState &call_state) {
    call_data_table_->write_row(
        static_cast<double>(call_state.get_call_id()),
        call_state.get_function_id(), call_state.get_function_type(),
        call_state.get_formal_parameter_count(), call_state.get_function_name(),
        sexptype_to_string(call_state.get_return_value_type()),
        call_state.get_order());
}

void StrictnessAnalysis::metaprogram_(const prom_info_t &prom_info,
                                      const SEXP promise) {
    PromiseState &promise_state{promise_mapper_->find(prom_info.prom_id)};

    /* if promise is not an argument, then don't process it. */
    if (!promise_state.argument) {
        return;
    }

    auto *call_state = get_call_state(promise_state.call_id);

    /* this implies an escaped promise, the promise is alive but not
       the function invocation that got this promise as its argument */
    if (call_state == nullptr) {
        return;
    }

    call_state->metaprogram(promise_state.formal_parameter_position);
}

CallState *StrictnessAnalysis::get_call_state(const call_id_t call_id) {

    auto it{call_stack_.rbegin()};

    for (; it != call_stack_.rend(); ++it) {
        if (it->get_call_id() == call_id) {
            break;
        }
    }

    if (it == call_stack_.rend()) {
        return nullptr;
    }

    return &(*it);
}

void StrictnessAnalysis::add_call_graph_edge_(const call_id_t callee_id) {
    if (call_stack_.empty())
        return;
    call_graph_data_table_->write_row(
        static_cast<double>(call_stack_.back().get_call_id()),
        static_cast<double>(callee_id));
}

void StrictnessAnalysis::write_function_body_(const fn_id_t &fn_id,
                                              const std::string &definition) {
    auto result = handled_functions_.insert(fn_id);
    if (!result.second)
        return;
    std::ofstream fout(output_dir_ + "/functions/" + fn_id, std::ios::trunc);
    fout << definition;
    fout.close();
}
