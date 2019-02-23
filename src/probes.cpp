#include "probes.h"

#include "TracerState.h"

inline TracerState& tracer_state(dyntracer_t* dyntracer) {
    return *(static_cast<TracerState*>(dyntracer->state));
}

void dyntrace_entry(dyntracer_t* dyntracer, SEXP expression, SEXP environment) {
    TracerState& state = tracer_state(dyntracer);

    /* we do not do state.enter_probe() in this function because this is a
       pseudo probe that executes before the tracing actually starts. this is
       only for initialization purposes. */

    state.initialize();

    /* probe_exit here ensures we start the timer for timing argument execution.
     */
    state.exit_probe();
}

void dyntrace_exit(dyntracer_t* dyntracer,
                   SEXP         expression,
                   SEXP         environment,
                   SEXP         result,
                   int          error) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    state.cleanup(error);

    /* we do not do start.exit_probe() because the tracer has finished
       executing and we don't need to resume the timer. */
}

void closure_entry(dyntracer_t* dyntracer,
                   const SEXP   call,
                   const SEXP   op,
                   const SEXP   args,
                   const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Call* function_call = state.create_call(call, op, args, rho);

    state.update_wrapper_state(function_call);

    state.get_stack().push(function_call);

    state.exit_probe();

    /* TRACE GENERATION
    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_FUNCTION_BEGIN, sexptype_to_string(CLOSXP),
        call -> get_function_id(),
        call -> get_id(),
        state.lookup_environment(rho, true).get_id());

    This function call has to be made only for the DEFAULT and
    CUSTOM arguments. The trick is to loop over all the promises
    in the call and only call this function for those that
    satisfy the criterion.

    tracer_serializer(dyntracer).serialize(
        TraceSerializer::OPCODE_ARGUMENT_PROMISE_ASSOCIATE, info.fn_id,
        info.call_id, argument.formal_parameter_position,
        state.lookup_variable(rho, argument.name).get_id(), argument.name,
        promise_state->get_id());
    */
}

void closure_exit(dyntracer_t* dyntracer,
                  const SEXP   call,
                  const SEXP   op,
                  const SEXP   args,
                  const SEXP   rho,
                  const SEXP   return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_closure()) {
        dyntrace_log_error("Not found matching closure on stack");
    }

    Call* function_call = exec_ctxt.get_closure();

    function_call->set_return_value_type(type_of_sexp(return_value));

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
    //                                        call -> get_id(), false);

    state.destroy_call(function_call);

    state.exit_probe();
}

void builtin_entry(dyntracer_t* dyntracer,
                   const SEXP   call,
                   const SEXP   op,
                   const SEXP   args,
                   const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Call* function_call = state.create_call(call, op, args, rho);

    state.update_wrapper_state(function_call);

    state.get_stack().push(function_call);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_BEGIN,
    //     sexptype_to_string(BUILTINSXP), call -> get_function_id(),
    //     call -> get_id(), state.lookup_environment(rho).get_id());

    state.exit_probe();
}

void builtin_exit(dyntracer_t* dyntracer,
                  const SEXP   call,
                  const SEXP   op,
                  const SEXP   args,
                  const SEXP   rho,
                  const SEXP   return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_builtin()) {
        dyntrace_log_error("Not found matching builtin on stack");
    }

    Call* function_call = exec_ctxt.get_builtin();

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.destroy_call(function_call);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_FINISH, call->get_id(),
    //     false);

    state.exit_probe();
}

void special_entry(dyntracer_t* dyntracer,
                   const SEXP   call,
                   const SEXP   op,
                   const SEXP   args,
                   const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Call* function_call = state.create_call(call, op, args, rho);

    state.update_wrapper_state(function_call);

    state.get_stack().push(function_call);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_FUNCTION_BEGIN,
    //     sexptype_to_string(SPECIALSXP),
    //     call -> get_function_id(),
    //     call -> get_id(),
    //     state.lookup_environment(rho).get_id());

    state.exit_probe();
}

void special_exit(dyntracer_t* dyntracer,
                  const SEXP   call,
                  const SEXP   op,
                  const SEXP   args,
                  const SEXP   rho,
                  const SEXP   return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_special()) {
        dyntrace_log_error("Not found matching special object on stack");
    }

    Call* function_call = exec_ctxt.get_special();

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.destroy_call(function_call);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_FUNCTION_FINISH,
    //                                        call -> get_id(),
    //                                        false);

    state.exit_probe();
}

void S3_dispatch_entry(dyntracer_t* dyntracer,
                       const char*  generic,
                       const SEXP   cls,
                       SEXP         generic_method,
                       SEXP         specific_method,
                       SEXP         objects) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    std::string class_name(UNASSIGNED_CLASS_NAME);
    if (LENGTH(cls) != 0) {
        class_name = CHAR(STRING_ELT(cls, 0));
    }

    DenotedValue* value = state.lookup_promise(CAR(objects), true);
    value->set_class_name(class_name);
    value->used_for_S3_dispatch();

    state.lookup_function(specific_method)->set_generic_method_name(generic);
    state.lookup_function(generic_method)->set_dispatcher();

    state.exit_probe();
}

void S4_dispatch_argument(dyntracer_t* dyntracer, const SEXP argument) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    if (type_of_sexp(argument) == PROMSXP) {
        DenotedValue* value = state.lookup_promise(argument, true);

        value->used_for_S4_dispatch();
    }

    state.exit_probe();
}

void context_entry(dyntracer_t* dyntracer, const RCNTXT* cptr) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    state.get_stack().push(cptr);

    state.exit_probe();
}

void jump_single_context(TracerState&      state,
                         ExecutionContext& exec_ctxt,
                         bool              returned,
                         const sexptype_t  return_value_type,
                         const SEXP        rho) {
    if (exec_ctxt.is_call()) {
        Call* call = exec_ctxt.get_call();

        call->set_jumped();
        call->set_return_value_type(return_value_type);

        state.destroy_call(call);
    }

    else if (exec_ctxt.is_promise()) {
        DenotedValue* promise = exec_ctxt.get_promise();

        promise->set_value_type(JUMPSXP);

        if (returned && promise->is_argument() &&
            (promise->get_environment() == rho)) {
            promise->set_non_local_return();
        }
    }
}

void context_jump(dyntracer_t*  dyntracer,
                  const RCNTXT* context,
                  const SEXP    return_value,
                  int           restart) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    // When doing longjump (exception thrown, etc.) this function gets the
    // target environment and unwinds function call stack until that environment
    // is on top. It also fixes indentation.

    /* Add these serializer calls back when generating traces

       tracer_serializer(dyntracer).serialize(
       TraceSerializer::OPCODE_FUNCTION_FINISH,
       // Change api to get_id instead
       element.call->get_id(), true);

       tracer_serializer(dyntracer).serialize(
       TraceSerializer::OPCODE_PROMISE_FINISH, element.promise_state->get_id(),
       true);
    */

    /* Identify promises that do non local return. First, check if
   this special is a 'return', then check if the return happens
   right after a promise is forced, then walk back in the stack
   to the promise with the same environment as the return. This
   promise is the one that does non local return. Note that the
   loop breaks after the first such promise is found. This is
   because only one promise can be held responsible for non local
   return, the one that invokes the return function. */

    execution_contexts_t exec_ctxts =
        state.get_stack().unwind(ExecutionContext(context));

    const SEXP rho = context->cloenv;

    std::size_t context_count = exec_ctxts.size();

    if (context_count == 0) {
    } else if (context_count == 1) {
        jump_single_context(state, exec_ctxts.front(), false, JUMPSXP, rho);
    } else {
        auto begin_iter = exec_ctxts.begin();
        auto end_iter   = --exec_ctxts.end();

        bool returned =
            (begin_iter->is_special() &&
             begin_iter->get_special()->get_function()->is_return());

        for (auto iter = begin_iter; iter != end_iter; ++iter) {
            jump_single_context(state, *iter, returned, JUMPSXP, rho);
        }

        jump_single_context(
            state, *end_iter, returned, type_of_sexp(return_value), rho);
    }

    state.exit_probe();
}

void context_exit(dyntracer_t* dyntracer, const RCNTXT* cptr) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_r_context()) {
        dyntrace_log_error("Nonmatching r context on stack");
    }

    state.exit_probe();
}

void gc_allocate(dyntracer_t* dyntracer, const SEXP object) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    state.increment_object_count(type_of_sexp(object));

    if (TYPEOF(object) == PROMSXP) {
        DenotedValue* promise_state = state.create_promise(object);

        // std::string cre_id = std::string("cre ") +
        // std::to_string(promise_state -> get_id());

        // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_CREATE,
        //                                        promise_state -> get_id(),
        //                                        promise_state ->
        //                                        get_environment(),
        //                                        promise_state ->
        //                                        get_expression());
    } else if (TYPEOF(object) == ENVSXP) {
        env_id_t env_id = state.create_environment(object).get_id();

        // tracer_serializer(dyntracer).serialize(
        //     TraceSerializer::OPCODE_ENVIRONMENT_CREATE, env_id);
    } else if (isVector(object)) {
        // analyzer.vector_alloc(info);
    }

    state.exit_probe();
}

void promise_force_entry(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    /* if promise is not an argument, then don't process it. */
    if (promise_state->is_argument()) {
        /* we know that the call is valid because this is an argument promise
         */
        auto* call = promise_state->get_last_argument()->get_call();

        /* At this point we should store expression type because this is the
           expression that yields the value obtained on promise exit */
        if (promise_state->is_forced()) {
        } else {
            eval_depth_t eval_depth = state.get_evaluation_depth(call);
            promise_state->set_evaluation_depth(eval_depth);

            for (Argument* argument: promise_state->get_arguments()) {
                argument->get_call()->add_to_force_order(
                    argument->get_formal_parameter_position());
            }
        }
    }

    /* Force promise in the end. This is important to get correct force order
       from the call object. */
    promise_state->force();

    state.get_stack().push(promise_state);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_PROMISE_BEGIN,
    //                                        promise_state -> get_id());

    state.exit_probe();
}

void promise_force_exit(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    ExecutionContext exec_ctxt = state.get_stack().pop();

    if (!exec_ctxt.is_promise()) {
        dyntrace_log_error("unable to find matching promise on stack");
    }

    DenotedValue* promise_state = exec_ctxt.get_promise();

    const SEXP value = dyntrace_get_promise_value(promise);

    promise_state->set_value_type(type_of_sexp(value));

    // std::string ext_id =
    //     std::string("ext ") + std::to_string(promise_state->get_id());

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_FINISH,
    //     promise_state -> get_id(),
    //     false);

    state.exit_probe();
}

void promise_value_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->lookup_value();

    // std::string val_id = std::string("val ") + std::to_string(promise_state
    // -> get_id());

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_VALUE_LOOKUP,
    //     promise_state -> get_id(),
    //     value_type_to_string(dyntrace_get_promise_value(promise)));

    state.exit_probe();
}

void promise_expression_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->lookup_expression();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_EXPRESSION_LOOKUP,
    //     promise_state -> get_id(),
    //     get_expression(dyntrace_get_promise_expression(promise)));

    state.exit_probe();
}

void promise_environment_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->lookup_environment();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_LOOKUP,
    //     promise_state->get_id(),
    //     promise_state->get_environment_id());

    state.exit_probe();
}

void promise_expression_assign(dyntracer_t* dyntracer,
                               const SEXP   promise,
                               const SEXP   expression) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_expression();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_EXPRESSION_ASSIGN,
    //     promise_state -> get_id(),
    //     get_expression(expression));

    state.exit_probe();
}

void promise_value_assign(dyntracer_t* dyntracer,
                          const SEXP   promise,
                          const SEXP   value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_value();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_VALUE_ASSIGN,
    //     promise_state -> get_id(),
    //     value_type_to_string(value));

    state.exit_probe();
}

void promise_environment_assign(dyntracer_t* dyntracer,
                                const SEXP   promise,
                                const SEXP   environment) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_environment();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN,
    //     promise_state->get_id(),
    //     promise_state->get_environment_id());

    state.exit_probe();
}

void promise_substitute(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->metaprogram();

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_PROMISE_ENVIRONMENT_ASSIGN,
    //     promise_state->get_id(),
    //     promise_state->get_environment_id());

    state.exit_probe();
}

static void gc_promise_unmark(TracerState& state, const SEXP promise) {
    DenotedValue* promise_state = state.lookup_promise(promise, true);

    state.remove_promise(promise, promise_state);
}

static void gc_closure_unmark(TracerState& state, const SEXP function) {
    state.remove_function(function);
}

static void gc_environment_unmark(TracerState& state, const SEXP environment) {
    state.remove_environment(environment);
}

void gc_unmark(dyntracer_t* dyntracer, const SEXP object) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();
    switch (TYPEOF(object)) {
    case PROMSXP:
        gc_promise_unmark(state, object);
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

void environment_variable_define(dyntracer_t* dyntracer,
                                 const SEXP   symbol,
                                 const SEXP   value,
                                 const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);
    state.enter_probe();

    Variable& var = state.define_variable(rho, symbol);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_DEFINE,
    //                                        var.get_environment_id(),
    //                                        var.get_id(),
    //                                        var.get_name(),
    //                                        value_type_to_string(value));

    state.exit_probe();
}

void environment_variable_assign(dyntracer_t* dyntracer,
                                 const SEXP   symbol,
                                 const SEXP   value,
                                 const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Variable& var = state.update_variable(rho, symbol);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_ASSIGN,
    //                                        var.get_environment_id(),
    //                                        var.get_id(),
    //                                        var.get_name(),
    //                                        value_type_to_string(value));

    /* When a variable is assigned, then a promise might be doing a side-effect.
       A promise writing to its own environment is not considered a side effect
       unless it writes to a variable that is created after it.
       There are effectively three kinds of effects:
       - A promise modifying a variable which was created after the promise was
       created.. Whether this happens in lexical or non lexical scope, its
       - A promise writing to a variable in its parent function's lexical scope.
       - A promise writing to a variable outside of its lexical scope.

       There are three cases for side-effects:
       - Writing to *any* variable in non lexical scope
       - Writing to *any* variable in lexical scope
       - Writing to a variable in current scope that is created before the
       promise is created
    */
    state.identify_side_effect_creators(var, rho);

    state.exit_probe();
}

void environment_variable_remove(dyntracer_t* dyntracer,
                                 const SEXP   symbol,
                                 const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Variable var = state.lookup_variable(rho, symbol);

    // tracer_serializer(dyntracer).serialize(TraceSerializer::OPCODE_ENVIRONMENT_REMOVE,
    //                                        var.get_environment_id(),
    //                                        var.get_id(),
    //                                        var.get_name());

    state.exit_probe();
}

void environment_variable_lookup(dyntracer_t* dyntracer,
                                 const SEXP   symbol,
                                 const SEXP   value,
                                 const SEXP   rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe();

    Variable& var = state.lookup_variable(rho, symbol);

    // tracer_serializer(dyntracer).serialize(
    //     TraceSerializer::OPCODE_ENVIRONMENT_LOOKUP, var.get_environment_id(),
    //     var.get_id(), var.get_name(), value_type_to_string(value));

    state.identify_side_effect_observers(var, rho);

    state.exit_probe();
}
