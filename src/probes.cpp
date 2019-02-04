#include "probes.h"
#include "State.h"


void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    /* we do not do state.enter_probe() in this function because this is a
       pseudo probe that executes before the tracing actually starts. this is
       only for initialization purposes. */

    write_environment_variables(tracer_output_dir(dyntracer) + "/ENVVAR");

    write_configuration(tracer_context(dyntracer),
                        tracer_output_dir(dyntracer) + "/CONFIGURATION");

    analyzer.begin(dyntracer);

    /* probe_exit here ensures we start the timer for timing argument execution. */
    state.exit_probe();
}


void dyntrace_exit(dyntracer_t *dyntracer, SEXP expression, SEXP environment,
                   SEXP result, int error) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    analyzer.end(dyntracer);

    state.finish_pass();

    if (error) {
        std::ofstream error_file{tracer_output_dir(dyntracer) + "/ERROR"};
        error_file << "ERROR";
        error_file.close();
    } else {
        std::ofstream noerror_file{tracer_output_dir(dyntracer) + "/NOERROR"};
        noerror_file << "NOERROR";
        noerror_file.close();
    }

    /* we do not do start.exit_probe() because the tracer has finished
       executing and we don't need to resume the timer. */
}


void closure_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    closure_info_t info =
        function_entry_get_info(dyntracer, call, op, args, rho);

    state.push_execution_context(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, sexptype_to_string(CLOSXP),
        info.fn_id, info.call_id,
        state.lookup_environment(rho, true).get_id());

    auto &fresh_promises = state.fresh_promises;

    // Associate promises with call ID
    for (auto argument: info.arguments) {
        auto &promise = argument.promise_id;
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        auto it = fresh_promises.find(promise);
        if (it != fresh_promises.end()) {
            state.promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }

        if (argument.parameter_mode == parameter_mode_t::DEFAULT ||
            argument.parameter_mode == parameter_mode_t::CUSTOM) {

            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE, info.fn_id,
                info.call_id, argument.formal_parameter_position,
                state.lookup_variable(rho, argument.name).get_id(),
                argument.name, argument.promise_id);


            // first we insert the promise as usual.
            env_id_t env_id = state
                .lookup_environment(argument.promise_environment)
                .get_id();

            state.insert_promise(argument.promise_id, env_id);

            // now, we make this promise object, a function argument by providing the
            // relevant information.
            PromiseState &promise_state =
                state.lookup_promise(argument.promise_id);

            promise_state.make_function_argument(
                info.fn_id, info.call_id,
                argument.formal_parameter_position, argument.parameter_mode);
        }
    }

    analyzer.closure_entry(info);

    state.exit_probe();
}


void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    closure_info_t info =
        function_exit_get_info(dyntracer, call, op, args, rho, retval);

    state.pop_execution_context(info);

    analyzer.closure_exit(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, info.call_id, false);

    state.exit_probe();
}


void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    function_type fn_type;

    if (TYPEOF(op) == BUILTINSXP) {
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    } else {
        /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    }

    print_entry_info(dyntracer, call, op, args, rho, fn_type);

    state.exit_probe();
}


void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP) {
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    } else {
        fn_type = function_type::TRUE_BUILTIN;
    }

    print_exit_info(dyntracer, call, op, args, rho, fn_type, retval);

    state.exit_probe();
}


void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    print_entry_info(dyntracer, call, op, args, rho, function_type::SPECIAL);

    state.exit_probe();
}


void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    print_exit_info(dyntracer, call, op, args, rho, function_type::SPECIAL,
                    retval);

    state.exit_probe();
}


void print_entry_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                      const SEXP args, const SEXP rho, function_type fn_type) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    builtin_info_t info =
        builtin_entry_get_info(dyntracer, call, op, rho, fn_type);

    if (info.fn_type == function_type::SPECIAL) {
        analyzer.special_entry(info);
    }
    else {
        analyzer.builtin_entry(info);
    }

    // TODO - check that pushing happens after the analyzer is called.
    // This is inconsistent with the case of promises.
    state.push_execution_context(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN,
        sexptype_to_string(info.fn_type == function_type::SPECIAL ? SPECIALSXP
                                                                  : BUILTINSXP),
        info.fn_id, info.call_id,
        state.lookup_environment(rho).get_id());

}

void print_exit_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                     const SEXP args, const SEXP rho, function_type fn_type,
                     const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    builtin_info_t info =
        builtin_exit_get_info(dyntracer, call, op, rho, fn_type, retval);

    if (info.fn_type == function_type::SPECIAL) {
        analyzer.special_exit(info);
    }
    else {
        analyzer.builtin_exit(info);
    }

    // TODO - the object is being removed after the analyzer is done
    //        this should be made consistent.
    state.pop_execution_context(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, info.call_id, false);

}


void gc_allocate(dyntracer_t *dyntracer, const SEXP object) {

    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    if(TYPEOF(object) == PROMSXP) {
        const SEXP rho = dyntrace_get_promise_environment(object);

        env_id_t env_id = state.lookup_environment(rho).get_id();

        prom_basic_info_t info = create_promise_get_info(dyntracer, object, rho);

        state.create_promise(info.prom_id, env_id);

        analyzer.promise_created(info, object);

        std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);

        tracer_serializer(dyntracer).serialize(
            TraceSerializer::OPCODE_PROMISE_CREATE, info.prom_id,
            env_id, info.expression);
    }
    else if(TYPEOF(object) == ENVSXP) {
        //TODO - check if this is needed at all, most likely, it is not.
        stack_event_t event =
            get_last_on_stack_by_type(state.full_stack, stack_type::CALL);

        fn_id_t fn_id = event.type == stack_type::NONE
                            ? compute_hash("")
                            : event.function_info.function_id;

        env_id_t env_id = state.create_environment(object).get_id();

        tracer_serializer(dyntracer).serialize(
            TraceSerializer::OPCODE_ENVIRONMENT_CREATE, env_id);
    }
    else if(isVector(object)) {

        type_gc_info_t info{state.get_gc_trigger_counter(),
                            TYPEOF(object),
                            (unsigned long)xlength(object),
                            (unsigned long)BYTE2VEC(xlength(object))};

        // analyzer.vector_alloc(info);
    }

    state.exit_probe();
}


void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = force_promise_entry_get_info(dyntracer, promise);

    SEXP promise_env = dyntrace_get_promise_environment(promise);
    env_id_t env_id =
            state.lookup_environment(promise_env).get_id();

    state.insert_promise(info.prom_id, env_id);

    analyzer.promise_force_entry(info, promise);

    state.push_execution_context(info, promise_env);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_BEGIN, info.prom_id);

    state.exit_probe();
}


void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = force_promise_exit_get_info(dyntracer, promise);

    // TODO - check that promise exists at this point in the mapper
    //        throw an error if it does not
    // state.insert_promise(info.prom_id, promise);

    analyzer.promise_force_exit(info, promise);

    state.pop_execution_context(info);

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_FINISH, info.prom_id, false);

    state.exit_probe();
}


void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_lookup_get_info(dyntracer, promise);

    env_id_t env_id =
        state
            .lookup_environment(dyntrace_get_promise_environment(promise))
            .get_id();

    state.insert_promise(info.prom_id, env_id);

    analyzer.promise_value_lookup(info, promise);

    std::string val_id = std::string("val ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP, info.prom_id,
        value_type_to_string(dyntrace_get_promise_value(promise)));

    state.exit_probe();
}


void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        state
        .lookup_environment(dyntrace_get_promise_environment(prom))
        .get_id();

    state.insert_promise(info.prom_id, env_id);

    analyzer.promise_expression_lookup(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP, info.prom_id,
        get_expression(dyntrace_get_promise_expression(prom)));

    state.exit_probe();
}


void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    const SEXP rho = dyntrace_get_promise_environment(prom);

    env_id_t env_id = state.lookup_environment(rho).get_id();

    state.insert_promise(info.prom_id, env_id);

    auto environment_id = state.lookup_environment(rho).get_id();

    analyzer.promise_environment_lookup(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP, info.prom_id,
        environment_id);

    state.exit_probe();
}


void promise_expression_assign(dyntracer_t *dyntracer, const SEXP prom,
                               const SEXP expression) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    const SEXP rho = dyntrace_get_promise_environment(prom);

    env_id_t env_id = state.lookup_environment(rho).get_id();

    state.insert_promise(info.prom_id, env_id);

    analyzer.promise_expression_assign(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN, info.prom_id,
        get_expression(expression));

    state.exit_probe();
}


void promise_value_assign(dyntracer_t *dyntracer, const SEXP prom,
                          const SEXP value) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        state
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    state.insert_promise(info.prom_id, env_id);

    analyzer.promise_value_assign(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN, info.prom_id,
        value_type_to_string(value));

    state.exit_probe();
}


void promise_environment_assign(dyntracer_t *dyntracer, const SEXP prom,
                                const SEXP environment) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        state
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    state.insert_promise(info.prom_id, env_id);

    auto environment_id =
        state.lookup_environment(environment).get_id();

    analyzer.promise_environment_assign(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN, info.prom_id,
        environment_id);
    state.exit_probe();
}


void gc_unmark(dyntracer_t *dyntracer, const SEXP expression) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    switch (TYPEOF(expression)) {
        case PROMSXP:
            gc_promise_unmark(state, analyzer, expression);
            break;
        case CLOSXP:
            gc_closure_unmark(state, expression);
            break;
        case ENVSXP:
            gc_environment_unmark(state, expression);
            break;
        default:
            break;;
    }

    state.exit_probe();
}


void gc_promise_unmark(tracer_state_t & state,
                       Analyzer& analyzer,
                       const SEXP promise) {

    prom_id_t id = get_promise_id(state, promise);

    env_id_t env_id =
        state
            .lookup_environment(dyntrace_get_promise_environment(promise))
            .get_id();
    state.insert_promise(id, env_id);

    auto &promise_origin = state.promise_origin;

    analyzer.gc_promise_unmarked(id, promise);

    state.remove_promise(id);

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
    }

    state.promise_ids.erase(promise);
}


void gc_closure_unmark(tracer_state_t& state, const SEXP function) {

    auto &definitions = state.function_definitions;

    auto it = definitions.find(function);

    if (it != definitions.end())
        state.function_definitions.erase(it);
}


void gc_environment_unmark(tracer_state_t& state, const SEXP environment) {
    state.remove_environment(environment);
}


void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    state.increment_gc_trigger_counter();

    state.exit_probe();
}


void gc_exit(dyntracer_t *dyntracer, int gc_counts) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    gc_info_t info{state.get_gc_trigger_counter()};

    state.exit_probe();
}


void context_entry(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    stack_event_t event;
    event.context_id = (rid_t)cptr;
    event.type = stack_type::CONTEXT;
    state.full_stack.push_back(event);

    state.exit_probe();
}

void context_jump(dyntracer_t *dyntracer, const RCNTXT *cptr,
                  const SEXP return_value, int restart) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    unwind_info_t info;
    info.jump_context = ((rid_t)cptr);
    info.restart = restart;

    // When doing longjump (exception thrown, etc.) this function gets the
    // target environment
    // and unwinds function call stack until that environment is on top. It also
    // fixes indentation.
    while (!state.full_stack.empty()) {
        stack_event_t element = (state.full_stack.back());

        // if (info.jump_target == element.enclosing_environment)
        //    break;
        if (element.type == stack_type::CONTEXT)
            if (info.jump_context == element.context_id)
                break;
            else
                info.unwound_frames.push_back(element);
        else if (element.type == stack_type::CALL) {
            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_FUNCTION_FINISH, element.call_id, true);
            info.unwound_frames.push_back(element);
        } else if (element.type == stack_type::PROMISE) {
            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_PROMISE_FINISH, element.promise_id,
                true);
            info.unwound_frames.push_back(element);
        } else /* if (element.type == stack_type::NONE) */
            dyntrace_log_error("NONE object found on tracer's full stack.");

        state.full_stack.pop_back();
    }

    analyzer.context_jump(info);

    state.exit_probe();
}

void context_exit(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    stack_event_t event = state.full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t)cptr) == event.context_id)
        state.full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full "
                             "stack, but %d is on top of stack.",
                             ((rid_t)cptr), event.context_id);

    state.exit_probe();
}


void environment_variable_define(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);
    state.enter_probe();

    Variable &var = state.define_variable(rho, symbol);

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_DEFINE,
                                           var.get_environment_id(),
                                           var.get_id(),
                                           var.get_name(),
                                           value_type_to_string(value));

    analyzer.environment_variable_define(var);

    state.exit_probe();
}

void environment_variable_assign(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    Variable& var = state.update_variable(rho, symbol);

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN,
                                           var.get_environment_id(),
                                           var.get_id(),
                                           var.get_name(),
                                           value_type_to_string(value));

    analyzer.environment_variable_assign(var);

    state.exit_probe();
}

void environment_variable_remove(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    Variable var = state.lookup_variable(rho, symbol);

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_REMOVE,
                                           var.get_environment_id(),
                                           var.get_id(),
                                           var.get_name());

    analyzer.environment_variable_remove(var);

    state.exit_probe();
}

void environment_variable_lookup(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    Variable& var = state.lookup_variable(rho, symbol);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP, var.get_environment_id(), var.get_id(),
        var.get_name(), value_type_to_string(value));

    analyzer.environment_variable_lookup(var);

    state.exit_probe();
}
