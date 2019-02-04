#include "probes.h"
#include "State.h"

inline void resume_execution_timer(tracer_state_t& state) {
    state.resume_execution_timer();
}

inline void pause_execution_timer(tracer_state_t& state) {
    state.pause_execution_timer();
}

void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment) {

    write_environment_variables(tracer_output_dir(dyntracer) + "/ENVVAR");

    write_configuration(tracer_context(dyntracer),
                        tracer_output_dir(dyntracer) + "/CONFIGURATION");

    analysis_driver(dyntracer).begin(dyntracer);

    /* resume resets the timer to the current time.
       since the code has just begun executing, we don't pause the timer in
       this function. */
    resume_execution_timer(tracer_state(dyntracer));
}

void dyntrace_exit(dyntracer_t *dyntracer, SEXP expression, SEXP environment,
                   SEXP result, int error) {
    /* pause winds up the execution time of active functions and promises.
       since the code has finished executing, we don't need to resume the
       timer. */
    pause_execution_timer(tracer_state(dyntracer));

    tracer_state(dyntracer).finish_pass();

    if (!tracer_state(dyntracer).full_stack.empty()) {
        dyntrace_log_warning(
            "Function/promise stack is not balanced: %d remaining",
            tracer_state(dyntracer).full_stack.size());
        tracer_state(dyntracer).full_stack.clear();
    }

    analysis_driver(dyntracer).end(dyntracer);

    tracer_state(dyntracer).clear_promises();

    if (error) {
        std::ofstream error_file{tracer_output_dir(dyntracer) + "/ERROR"};
        error_file << "ERROR";
        error_file.close();
    } else {
        std::ofstream noerror_file{tracer_output_dir(dyntracer) + "/NOERROR"};
        noerror_file << "NOERROR";
        noerror_file.close();
    }
}

// Triggered when entering function evaluation.
void closure_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));

    closure_info_t info =
        function_entry_get_info(dyntracer, call, op, args, rho);

    tracer_state(dyntracer).push_execution_context(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, sexptype_to_string(CLOSXP),
        info.fn_id, info.call_id,
        tracer_state(dyntracer).lookup_environment(rho, true).get_id());

    auto &fresh_promises = tracer_state(dyntracer).fresh_promises;

    // Associate promises with call ID
    for (auto argument: info.arguments) {
        auto &promise = argument.promise_id;
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        auto it = fresh_promises.find(promise);
        if (it != fresh_promises.end()) {
            tracer_state(dyntracer).promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }

        if (argument.parameter_mode == parameter_mode_t::DEFAULT ||
            argument.parameter_mode == parameter_mode_t::CUSTOM) {

            tracer_serializer(dyntracer).serialize(
                TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE, info.fn_id,
                info.call_id, argument.formal_parameter_position,
                tracer_state(dyntracer).lookup_variable(rho, argument.name).get_id(),
                argument.name, argument.promise_id);


            // first we insert the promise as usual.
            env_id_t env_id = tracer_state(dyntracer)
                .lookup_environment(argument.promise_environment)
                .get_id();

            tracer_state(dyntracer).insert_promise(argument.promise_id, env_id);

            // now, we make this promise object, a function argument by providing the
            // relevant information.
            PromiseState &promise_state =
                tracer_state(dyntracer).lookup_promise(argument.promise_id);

            promise_state.make_function_argument(
                info.fn_id, info.call_id,
                argument.formal_parameter_position, argument.parameter_mode);
        }
    }

    analysis_driver(dyntracer).closure_entry(info);

    resume_execution_timer(tracer_state(dyntracer));
}

void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    pause_execution_timer(tracer_state(dyntracer));

    closure_info_t info =
        function_exit_get_info(dyntracer, call, op, args, rho, retval);

    tracer_state(dyntracer).pop_execution_context(info);

    analysis_driver(dyntracer).closure_exit(info);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, info.call_id, false);

    resume_execution_timer(tracer_state(dyntracer));
}

void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    print_entry_info(dyntracer, call, op, args, rho, fn_type);
    resume_execution_timer(tracer_state(dyntracer));
}

void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    pause_execution_timer(tracer_state(dyntracer));
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(dyntracer, call, op, args, rho, fn_type, retval);
    resume_execution_timer(tracer_state(dyntracer));
}

void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));
    print_entry_info(dyntracer, call, op, args, rho, function_type::SPECIAL);
    resume_execution_timer(tracer_state(dyntracer));
}

void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    pause_execution_timer(tracer_state(dyntracer));
    print_exit_info(dyntracer, call, op, args, rho, function_type::SPECIAL,
                    retval);
    resume_execution_timer(tracer_state(dyntracer));
}

void print_entry_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                      const SEXP args, const SEXP rho, function_type fn_type) {

    builtin_info_t info =
        builtin_entry_get_info(dyntracer, call, op, rho, fn_type);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(dyntracer).special_entry(info);
    else
        analysis_driver(dyntracer).builtin_entry(info);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.function_info.function_id = info.fn_id;
    stack_elem.function_info.type = info.fn_type;
    stack_elem.environment = info.call_ptr;
    tracer_state(dyntracer).full_stack.push_back(stack_elem);


    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN,
        sexptype_to_string(info.fn_type == function_type::SPECIAL ? SPECIALSXP
                                                                  : BUILTINSXP),
        info.fn_id, info.call_id,
        tracer_state(dyntracer).lookup_environment(rho).get_id());

}

void print_exit_info(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                     const SEXP args, const SEXP rho, function_type fn_type,
                     const SEXP retval) {

    builtin_info_t info =
        builtin_exit_get_info(dyntracer, call, op, rho, fn_type, retval);

    if (info.fn_type == function_type::SPECIAL)
        analysis_driver(dyntracer).special_exit(info);
    else
        analysis_driver(dyntracer).builtin_exit(info);

    auto thing_on_stack = tracer_state(dyntracer).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be built-in with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(dyntracer).full_stack.pop_back();


    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, info.call_id, false);

}

void gc_allocate(dyntracer_t *dyntracer, const SEXP object) {
    pause_execution_timer(tracer_state(dyntracer));
    switch (TYPEOF(object)) {
        case PROMSXP:
            promise_created(dyntracer, object);
            resume_execution_timer(tracer_state(dyntracer));
            return;
        case ENVSXP:
            new_environment(dyntracer, object);
            resume_execution_timer(tracer_state(dyntracer));
            return;
    }
    if (isVector(object))
        vector_alloc(dyntracer, TYPEOF(object), xlength(object),
                     BYTE2VEC(xlength(object)), NULL);
    resume_execution_timer(tracer_state(dyntracer));
}

void promise_created(dyntracer_t *dyntracer, const SEXP prom) {
    pause_execution_timer(tracer_state(dyntracer));

    const SEXP rho = dyntrace_get_promise_environment(prom);

    env_id_t env_id = tracer_state(dyntracer) .lookup_environment(rho) .get_id();

    prom_basic_info_t info = create_promise_get_info(dyntracer, prom, rho);

    tracer_state(dyntracer).create_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_created(info, prom);

    std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_CREATE, info.prom_id,
        tracer_state(dyntracer).lookup_environment(PRENV(prom)).get_id(),
        info.expression);

    resume_execution_timer(tracer_state(dyntracer));
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = force_promise_entry_get_info(dyntracer, promise);

    env_id_t env_id = tracer_state(dyntracer)
        .lookup_environment(dyntrace_get_promise_environment(promise))
        .get_id();

    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_force_entry(info, promise);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    stack_elem.environment = dyntrace_get_promise_environment(promise);
    tracer_state(dyntracer).full_stack.push_back(stack_elem);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_BEGIN, info.prom_id);

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = force_promise_exit_get_info(dyntracer, promise);

    // TODO - check that promise exists at this point in the mapper
    //        throw an error if it does not
    // tracer_state(dyntracer).insert_promise(info.prom_id, promise);

    analysis_driver(dyntracer).promise_force_exit(info, promise);

    auto thing_on_stack = tracer_state(dyntracer).full_stack.back();
    if (thing_on_stack.type != stack_type::PROMISE ||
        thing_on_stack.promise_id != info.prom_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be promise with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.promise_id, info.prom_id);
    }
    tracer_state(dyntracer).full_stack.pop_back();

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_FINISH, info.prom_id, false);

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_lookup_get_info(dyntracer, promise);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(promise))
            .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_value_lookup(info, promise);

    std::string val_id = std::string("val ") + std::to_string(info.prom_id);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP, info.prom_id,
        value_type_to_string(dyntrace_get_promise_value(promise)));

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        tracer_state(dyntracer)
        .lookup_environment(dyntrace_get_promise_environment(prom))
        .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_expression_lookup(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP, info.prom_id,
        get_expression(dyntrace_get_promise_expression(prom)));

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP prom) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    auto environment_id{tracer_state(dyntracer).lookup_environment(
                                                                   dyntrace_get_promise_environment(prom)).get_id()};

    analysis_driver(dyntracer).promise_environment_lookup(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP, info.prom_id,
        environment_id);

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_expression_assign(dyntracer_t *dyntracer, const SEXP prom,
                               const SEXP expression) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_expression_set(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN, info.prom_id,
        get_expression(expression));

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_value_assign(dyntracer_t *dyntracer, const SEXP prom,
                          const SEXP value) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    analysis_driver(dyntracer).promise_value_set(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN, info.prom_id,
        value_type_to_string(value));

    resume_execution_timer(tracer_state(dyntracer));
}

void promise_environment_assign(dyntracer_t *dyntracer, const SEXP prom,
                                const SEXP environment) {
    pause_execution_timer(tracer_state(dyntracer));

    prom_info_t info = promise_expression_lookup_get_info(dyntracer, prom);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(prom))
            .get_id();
    tracer_state(dyntracer).insert_promise(info.prom_id, env_id);

    auto environment_id =
        tracer_state(dyntracer).lookup_environment(environment).get_id();

    analysis_driver(dyntracer).promise_environment_set(info, prom);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN, info.prom_id,
        environment_id);
    resume_execution_timer(tracer_state(dyntracer));
}

void gc_unmark(dyntracer_t *dyntracer, const SEXP expression) {
    pause_execution_timer(tracer_state(dyntracer));
    switch (TYPEOF(expression)) {
        case PROMSXP:
            gc_promise_unmark(dyntracer, expression);
            break;
        case CLOSXP:
            gc_closure_unmark(dyntracer, expression);
            break;
        case ENVSXP:
            gc_environment_unmark(dyntracer, expression);
            break;
        default:
            break;;
    }
    resume_execution_timer(tracer_state(dyntracer));
}

void gc_promise_unmark(dyntracer_t *dyntracer, const SEXP promise) {
    prom_id_t id = get_promise_id(dyntracer, promise);

    env_id_t env_id =
        tracer_state(dyntracer)
            .lookup_environment(dyntrace_get_promise_environment(promise))
            .get_id();
    tracer_state(dyntracer).insert_promise(id, env_id);

    auto &promise_origin = tracer_state(dyntracer).promise_origin;

    analysis_driver(dyntracer).gc_promise_unmarked(id, promise);

    tracer_state(dyntracer).remove_promise(id);

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
    }

    tracer_state(dyntracer).promise_ids.erase(promise);
}

void gc_closure_unmark(dyntracer_t *dyntracer, const SEXP function) {
    remove_function_definition(dyntracer, function);

}

void gc_environment_unmark(dyntracer_t *dyntracer, const SEXP environment) {

    tracer_state(dyntracer).remove_environment(environment);
}

void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed) {
    pause_execution_timer(tracer_state(dyntracer));

    tracer_state(dyntracer).increment_gc_trigger_counter();

    resume_execution_timer(tracer_state(dyntracer));
}

void gc_exit(dyntracer_t *dyntracer, int gc_counts) {
    pause_execution_timer(tracer_state(dyntracer));

    gc_info_t info{tracer_state(dyntracer).get_gc_trigger_counter()};

    resume_execution_timer(tracer_state(dyntracer));
}

void vector_alloc(dyntracer_t *dyntracer, int sexptype, long length, long bytes,
                  const char *srcref) {

    type_gc_info_t info{tracer_state(dyntracer).get_gc_trigger_counter(),
                        sexptype, length, bytes};

    analysis_driver(dyntracer).vector_alloc(info);

}

void new_environment(dyntracer_t *dyntracer, const SEXP rho) {

    // fn_id_t fn_id = (tracer_state(dyntracer).fun_stack.back().function_id);
    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(dyntracer).full_stack, stack_type::CALL);
    fn_id_t fn_id = event.type == stack_type::NONE
                        ? compute_hash("")
                        : event.function_info.function_id;

    env_id_t env_id = tracer_state(dyntracer).create_environment(rho).get_id();

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_ENVIRONMENT_CREATE, env_id);

}

void context_entry(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    pause_execution_timer(tracer_state(dyntracer));

    stack_event_t event;
    event.context_id = (rid_t)cptr;
    event.type = stack_type::CONTEXT;
    tracer_state(dyntracer).full_stack.push_back(event);

    resume_execution_timer(tracer_state(dyntracer));
}

void context_jump(dyntracer_t *dyntracer, const RCNTXT *cptr,
                  const SEXP return_value, int restart) {
    pause_execution_timer(tracer_state(dyntracer));

    unwind_info_t info;
    info.jump_context = ((rid_t)cptr);
    info.restart = restart;

    adjust_stacks(dyntracer, info);

    analysis_driver(dyntracer).context_jump(info);

    resume_execution_timer(tracer_state(dyntracer));
}

void context_exit(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    pause_execution_timer(tracer_state(dyntracer));

    stack_event_t event = tracer_state(dyntracer).full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t)cptr) == event.context_id)
        tracer_state(dyntracer).full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full "
                             "stack, but %d is on top of stack.",
                             ((rid_t)cptr), event.context_id);

    resume_execution_timer(tracer_state(dyntracer));
}

void adjust_stacks(dyntracer_t *dyntracer, unwind_info_t &info) {

    while (!tracer_state(dyntracer).full_stack.empty()) {
        stack_event_t element = (tracer_state(dyntracer).full_stack.back());

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

        tracer_state(dyntracer).full_stack.pop_back();
    }
}

void environment_action(dyntracer_t *dyntracer, const SEXP symbol, SEXP value,
                        const SEXP rho, const std::string &action) {
    bool exists = true;
    prom_id_t promise_id = tracer_state(dyntracer).enclosing_promise_id();
    env_id_t environment_id = tracer_state(dyntracer).lookup_environment(rho).get_id();
    // TODO - check if we need to force variable creation at this point
    var_id_t variable_id = tracer_state(dyntracer).lookup_variable(rho, symbol_to_string(symbol)).get_id();

    // std::string value = serialize_sexp(expression);
    // std::string exphash = compute_hash(value.c_str());
    // serialize variable iff it has not been seen before.
    // if it has been seen before, then it has already been serialized.

    if (!exists) {
        // TODO
    }

    std::string action_id = action + " " + std::to_string(variable_id);

    if (action == TraceSerializer::OPCODE_ENVIRONMENT_REMOVE) {
        tracer_serializer(dyntracer).serialize(action,
                                               tracer_state(dyntracer).lookup_environment(rho).get_id(),
                                               variable_id,
                                               CHAR(PRINTNAME(symbol)));
    } else {
        tracer_serializer(dyntracer).serialize(action,
                                               tracer_state(dyntracer).lookup_environment(rho).get_id(),
                                               variable_id,
                                               CHAR(PRINTNAME(symbol)),
                                               value_type_to_string(value));
    }

}

void environment_variable_define(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));

    analysis_driver(dyntracer).environment_define_var(symbol, value, rho);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_DEFINE);
    resume_execution_timer(tracer_state(dyntracer));
}

void environment_variable_assign(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));

    analysis_driver(dyntracer).environment_assign_var(symbol, value, rho);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN);
    resume_execution_timer(tracer_state(dyntracer));
}

void environment_variable_remove(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));

    analysis_driver(dyntracer).environment_remove_var(symbol, rho);

    environment_action(dyntracer, symbol, R_UnboundValue, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_REMOVE);
    resume_execution_timer(tracer_state(dyntracer));
}

void environment_variable_lookup(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho) {
    pause_execution_timer(tracer_state(dyntracer));

    analysis_driver(dyntracer).environment_lookup_var(symbol, value, rho);

    environment_action(dyntracer, symbol, value, rho,
                       TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP);
    resume_execution_timer(tracer_state(dyntracer));
}
