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

    CallState *call_state = state.create_call(call, op, args, rho);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, sexptype_to_string(CLOSXP),
        call_state -> get_function_id(),
        call_state -> get_call_id(),
        state.lookup_environment(rho, true).get_id());

    // TODO - now we are calling this before putting the closure on the stack.
    // This should be made consistent.
    analyzer.closure_entry(call_state);

    // TODO - check that pushing happens after the analyzer is called.
    // This is inconsistent with the case of promises.
    state.push_execution_context(call_state);


    // TODO - this function call has to be made only for the DEFAULT and
    //        CUSTOM arguments. The trick is to loop over all the promises
    //        in the call_state and only call this function for those that
    //        satisfy the criterion.
    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE, info.fn_id,
    //     info.call_id, argument.formal_parameter_position,
    //     state.lookup_variable(rho, argument.name).get_id(), argument.name,
    //     promise_state->get_id());

    state.exit_probe();
}


void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    CallState *call_state =  state.pop_execution_context(retval);

    analyzer.closure_exit(call_state);

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
                                           call_state -> get_call_id(), false);

    //TODO delete call_state

    state.exit_probe();
}


void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    CallState *call_state = state.create_call(call, op, args, rho);

    analyzer.builtin_entry(call_state);

    // TODO - check that pushing happens after the analyzer is called.
    // This is inconsistent with the case of promises.
    state.push_execution_context(call_state);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN,
        sexptype_to_string(BUILTINSXP), call_state -> get_function_id(),
        call_state -> get_call_id(), state.lookup_environment(rho).get_id());

    state.exit_probe();
}


void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    // TODO - the object is removed before the analyzer is done.
    //        This should be taken into account on the analyzer side.
    CallState *call_state = state.pop_execution_context(retval);

    analyzer.builtin_exit(call_state);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_FINISH, call_state->get_call_id(),
        false);

    // TODO - delete call_state
    state.exit_probe();
}


void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    CallState * call_state = state.create_call(call, op, args, rho);

    analyzer.special_entry(call_state);

    // TODO - check that pushing happens after the analyzer is called.
    // This is inconsistent with the case of promises.
    state.push_execution_context(call_state);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN,
        sexptype_to_string(SPECIALSXP),
        call_state -> get_function_id(),
        call_state -> get_call_id(),
        state.lookup_environment(rho).get_id());

    state.exit_probe();
}


void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval) {

    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    // TODO - the object is removed before the analyzer is done.
    //        This should be taken into account on the analyzer side.
    CallState * call_state = state.pop_execution_context(retval);

    analyzer.special_exit(call_state);

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
                                           call_state -> get_call_id(),
                                           false);
    // TODO - delete call_state

    state.exit_probe();
}


void gc_allocate(dyntracer_t *dyntracer, const SEXP object) {

    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    if(TYPEOF(object) == PROMSXP) {

        PromiseState *promise_state = state.create_promise(object);

        analyzer.promise_created(promise_state, object);

        std::string cre_id = std::string("cre ") + std::to_string(promise_state -> get_id());

        tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_CREATE,
                                               promise_state -> get_id(),
                                               promise_state -> get_environment_id(),
                                               promise_state -> get_expression());
    }
    else if(TYPEOF(object) == ENVSXP) {
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

    PromiseState* promise_state = state.lookup_promise(promise);

    analyzer.promise_force_entry(promise_state, promise);

    state.push_execution_context(promise_state);

    std::string ent_id = std::string("ent ") + std::to_string(promise_state -> get_id());

    tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_BEGIN,
                                           promise_state -> get_id());

    state.exit_probe();
}


void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    /* lookup_promise will internally throw error if promise does not exist */
    PromiseState* promise_state = state.lookup_promise(promise);

    // TODO - check that promise exists at this point in the mapper
    //        throw an error if it does not
    // state.insert_promise(info.prom_id, promise);

    analyzer.promise_force_exit(promise_state, promise);

    state.pop_execution_context(promise_state);

    std::string ext_id = std::string("ext ") + std::to_string(promise_state -> get_id());

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_FINISH,
        promise_state -> get_id(),
        false);

    state.exit_probe();
}


void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.insert_promise(promise);

    analyzer.promise_value_lookup(promise_state, promise);

    std::string val_id = std::string("val ") + std::to_string(promise_state -> get_id());

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP,
        promise_state -> get_id(),
        value_type_to_string(dyntrace_get_promise_value(promise)));

    state.exit_probe();
}


void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.insert_promise(promise);

    analyzer.promise_expression_lookup(promise_state, promise);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP,
        promise_state -> get_id(),
        get_expression(dyntrace_get_promise_expression(promise)));

    state.exit_probe();
}


void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.insert_promise(promise);

    analyzer.promise_environment_lookup(promise_state, promise);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP,
        promise_state->get_id(),
        promise_state->get_environment_id());

    state.exit_probe();
}


void promise_expression_assign(dyntracer_t *dyntracer, const SEXP promise,
                               const SEXP expression) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.insert_promise(promise);

    analyzer.promise_expression_assign(promise_state, promise);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN,
        promise_state -> get_id(),
        get_expression(expression));

    state.exit_probe();
}


void promise_value_assign(dyntracer_t *dyntracer, const SEXP promise,
                          const SEXP value) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState * promise_state = state.insert_promise(promise);

    analyzer.promise_value_assign(promise_state, promise);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN,
        promise_state -> get_id(),
        value_type_to_string(value));

    state.exit_probe();
}


void promise_environment_assign(dyntracer_t *dyntracer, const SEXP promise,
                                const SEXP environment) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState * promise_state = state.insert_promise(promise);

    analyzer.promise_environment_assign(promise_state, promise);

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN,
        promise_state->get_id(),
        promise_state->get_environment_id());

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

    PromiseState * promise_state = state.insert_promise(promise);

    analyzer.gc_promise_unmarked(promise_state, promise);

    state.remove_promise(promise);
}


void gc_closure_unmark(tracer_state_t& state, const SEXP function) {
    state.remove_closure(function);
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
            tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
                                                   // Change api to get_id instead
                                                   element.call_state -> get_call_id(),
                                                   true);
            info.unwound_frames.push_back(element);
        } else if (element.type == stack_type::PROMISE) {
            tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_FINISH,
                                                   element.promise_state -> get_id(),
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
