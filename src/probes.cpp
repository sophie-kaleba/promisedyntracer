#include "probes.h"
#include "State.h"


void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    /* we do not do state.enter_probe() in this function because this is a
       pseudo probe that executes before the tracing actually starts. this is
       only for initialization purposes. */

    write_environment_variables(tracer_output_dirpath(dyntracer) + "/ENVVAR");

    write_configuration(tracer_context(dyntracer),
                        tracer_output_dirpath(dyntracer) + "/CONFIGURATION");

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
        std::ofstream error_file{tracer_output_dirpath(dyntracer) + "/ERROR"};
        error_file << "ERROR";
        error_file.close();
    } else {
        std::ofstream noerror_file{tracer_output_dirpath(dyntracer) + "/NOERROR"};
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

    analyzer.closure_entry(call_state);

    state.get_stack().push(call_state);

    state.exit_probe();

    /* TRACE GENERATION
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, sexptype_to_string(CLOSXP),
        call_state -> get_function_id(),
        call_state -> get_id(),
        state.lookup_environment(rho, true).get_id());

    This function call has to be made only for the DEFAULT and
    CUSTOM arguments. The trick is to loop over all the promises
    in the call_state and only call this function for those that
    satisfy the criterion.

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE, info.fn_id,
        info.call_id, argument.formal_parameter_position,
        state.lookup_variable(rho, argument.name).get_id(), argument.name,
        promise_state->get_id());
    */

}


void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP return_value) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if(!exec_ctxt.is_closure()) {
        dyntrace_log_error("Not found matching closure on stack");
    }

    CallState *call_state = exec_ctxt.get_closure();

    call_state->set_return_value_type(type_of_sexp(return_value));

    analyzer.closure_exit(call_state);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
    //                                        call_state -> get_id(), false);

    state.destroy_call(call_state);

    state.exit_probe();
}


void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    CallState *call_state = state.create_call(call, op, args, rho);

    analyzer.builtin_entry(call_state);

    state.get_stack().push(call_state);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_BEGIN,
    //     sexptype_to_string(BUILTINSXP), call_state -> get_function_id(),
    //     call_state -> get_id(), state.lookup_environment(rho).get_id());

    state.exit_probe();
}


void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP return_value) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_builtin()) {
        dyntrace_log_error("Not found matching builtin on stack");
    }

    CallState *call_state = exec_ctxt.get_builtin();

    call_state->set_return_value_type(type_of_sexp(return_value));

    analyzer.builtin_exit(call_state);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_FINISH, call_state->get_id(),
    //     false);

    state.destroy_call(call_state);

    state.exit_probe();
}


void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    CallState * call_state = state.create_call(call, op, args, rho);

    analyzer.special_entry(call_state);

    state.get_stack().push(call_state);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_BEGIN,
    //     sexptype_to_string(SPECIALSXP),
    //     call_state -> get_function_id(),
    //     call_state -> get_id(),
    //     state.lookup_environment(rho).get_id());

    state.exit_probe();
}


void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP return_value) {

    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer &analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_special()) {
        dyntrace_log_error("Not found matching special object on stack");
    }

    CallState *call_state = exec_ctxt.get_special();

    call_state->set_return_value_type(type_of_sexp(return_value));

    analyzer.special_exit(call_state);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
    //                                        call_state -> get_id(),
    //                                        false);

    state.destroy_call(call_state);

    state.exit_probe();
}


void context_entry(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    tracer_state_t& state = tracer_state(dyntracer);

    state.enter_probe();

    state.get_stack().push(cptr);

    state.exit_probe();
}


void context_jump(dyntracer_t *dyntracer, const RCNTXT *context,
                  const SEXP return_value, int restart) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    // When doing longjump (exception thrown, etc.) this function gets the
    // target environment and unwinds function call stack until that environment
    // is on top. It also fixes indentation.

    /* Add these serializer calls back when generating traces

       tracer_serializer(dyntracer).serialize(
       TraceSerializer::OPCODE_FUNCTION_FINISH,
       // Change api to get_id instead
       element.call_state->get_id(), true);

       tracer_serializer(dyntracer).serialize(
       TraceSerializer::OPCODE_PROMISE_FINISH, element.promise_state->get_id(),
       true);
    */

    execution_contexts_t exec_ctxts = state.get_stack().unwind(ExecutionContext(context));

    analyzer.context_jump(exec_ctxts);

    for (auto &exec_ctxt : exec_ctxts) {

        if (exec_ctxt.is_call()) {

            CallState *call_state = exec_ctxt.get_call();

            call_state->set_return_value_type(JUMPSXP);

            if (call_state->is_closure()) {

                analyzer.closure_exit(call_state);

            } else if (call_state->is_special()) {

                analyzer.special_exit(call_state);

            } else if (call_state->is_builtin()) {

                analyzer.builtin_exit(call_state);
            }

            delete exec_ctxt.get_call();

        } else if (exec_ctxt.is_promise()) {

            exec_ctxt.get_promise()->set_value_type(JUMPSXP);
        }
    }

    state.exit_probe();
}


void context_exit(dyntracer_t *dyntracer, const RCNTXT *cptr) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_r_context()) {
        dyntrace_log_error("Nonmatching r context on stack");
    }

    state.exit_probe();
}


void gc_allocate(dyntracer_t *dyntracer, const SEXP object) {

    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    if(TYPEOF(object) == PROMSXP) {

        PromiseState* promise_state = state.create_promise(object);

        analyzer.promise_created(promise_state, object);

        std::string cre_id = std::string("cre ") + std::to_string(promise_state -> get_id());

        // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_CREATE,
        //                                        promise_state -> get_id(),
        //                                        promise_state -> get_environment(),
        //                                        promise_state -> get_expression());
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

    PromiseState*  promise_state = state.lookup_promise(promise);

    analyzer.promise_force_entry(state.get_stack(), promise_state, promise);

    state.get_stack().push(promise_state);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_BEGIN,
    //                                        promise_state -> get_id());

    state.exit_probe();
}


void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    /* lookup_promise will internally throw error if promise does not exist */
    PromiseState*  promise_state = state.lookup_promise(promise);

    // TODO - check that promise exists at this point in the mapper
    //        throw an error if it does not
    // state.insert_promise(info.prom_id, promise);

    // this should happen after popping out the promise

    const SEXP value = dyntrace_get_promise_value(promise);

    promise_state->set_value_type(type_of_sexp(value));

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if(!exec_ctxt.is_promise()) {
        dyntrace_log_error("unable to find matching promise on stack");
    }

    // std::string ext_id =
    //     std::string("ext ") + std::to_string(promise_state->get_id());

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_FINISH,
    //     promise_state -> get_id(),
    //     false);

    state.exit_probe();
}


void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.lookup_promise(promise, true);

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

    PromiseState* promise_state = state.lookup_promise(promise, true);

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

    PromiseState* promise_state = state.lookup_promise(promise, true);

    analyzer.promise_environment_lookup(promise_state, promise);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP,
    //     promise_state->get_id(),
    //     promise_state->get_environment_id());

    state.exit_probe();
}


void promise_expression_assign(dyntracer_t *dyntracer, const SEXP promise,
                               const SEXP expression) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState* promise_state = state.lookup_promise(promise, true);

    analyzer.promise_expression_assign(promise_state, promise);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN,
    //     promise_state -> get_id(),
    //     get_expression(expression));

    state.exit_probe();
}


void promise_value_assign(dyntracer_t *dyntracer, const SEXP promise,
                          const SEXP value) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState*  promise_state = state.lookup_promise(promise, true);

    analyzer.promise_value_assign(promise_state, promise);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN,
    //     promise_state -> get_id(),
    //     value_type_to_string(value));

    state.exit_probe();
}


void promise_environment_assign(dyntracer_t *dyntracer, const SEXP promise,
                                const SEXP environment) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();

    PromiseState*  promise_state = state.lookup_promise(promise, true);

    analyzer.promise_environment_assign(promise_state, promise);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN,
    //     promise_state->get_id(),
    //     promise_state->get_environment_id());

    state.exit_probe();
}


void gc_unmark(dyntracer_t *dyntracer, const SEXP object) {
    tracer_state_t& state = tracer_state(dyntracer);
    Analyzer& analyzer = tracer_analyzer(dyntracer);

    state.enter_probe();
    switch (TYPEOF(object)) {
        case PROMSXP:
            gc_promise_unmark(state, analyzer, object);
            break;
        case CLOSXP:
            gc_closure_unmark(state, object);
            break;
        case ENVSXP:
            gc_environment_unmark(state, object);
            break;
        default:
            break;
    }

    state.exit_probe();
}


void gc_promise_unmark(tracer_state_t & state,
                       Analyzer& analyzer,
                       const SEXP promise) {

    PromiseState* promise_state = state.lookup_promise(promise, true);

    analyzer.gc_promise_unmarked(promise_state, promise);

    state.destroy_promise(promise, promise_state);
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

    analyzer.environment_variable_assign(state.get_stack(), var);

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

    analyzer.environment_variable_lookup(state.get_stack(), var);

    state.exit_probe();
}
