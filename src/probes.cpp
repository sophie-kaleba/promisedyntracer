#include "probes.h"

#include "TracerState.h"

inline TracerState& tracer_state(dyntracer_t* dyntracer) {
    return *(static_cast<TracerState*>(dyntracer->state));
}

static void search_promises_in_frame(dyntracer_t* dyntracer, SEXP frame) {
    while (frame != R_NilValue) {
        SEXP value = CAR(frame);
        if (TYPEOF(value) == PROMSXP) {
            gc_allocate(dyntracer, value);
        }
        frame = CDR(frame);
    }
}

static void search_promises_in_hash_table(dyntracer_t* dyntracer,
                                          SEXP hashtab) {
    int n = Rf_length(hashtab);
    for (int i = 0; i < n; i++) {
        search_promises_in_frame(dyntracer, VECTOR_ELT(hashtab, i));
    }
}

static void search_promises_in_baseenv(dyntracer_t* dyntracer) {
    SEXP* symtab = dyntrace_get_symbol_table();
    for (int j = 0; j < HSIZE; j++) {
        for (SEXP s = symtab[j]; s != R_NilValue; s = CDR(s)) {
            SEXP vl = SYMVALUE(CAR(s));
            if (TYPEOF(vl) == PROMSXP) {
                gc_allocate(dyntracer, vl);
            }
        }
    }
}

static void search_promises(dyntracer_t* dyntracer, SEXP env) {
    if (env == R_BaseEnv || env == R_BaseNamespace) {
        search_promises_in_baseenv(dyntracer);
    } else if (HASHTAB(env) != R_NilValue) {
        search_promises_in_hash_table(dyntracer, HASHTAB(env));
    } else {
        search_promises_in_frame(dyntracer, FRAME(env));
    }
}

void dyntrace_entry(dyntracer_t* dyntracer, SEXP expression, SEXP environment) {
    TracerState& state = tracer_state(dyntracer);

    /* we do not do state.enter_probe() in this function because this is a
       pseudo probe that executes before the tracing actually starts. this is
       only for initialization purposes. */

    state.initialize();

    search_promises(dyntracer, R_BaseEnv);

    /* probe_exit here ensures we start the timer for timing argument execution.
     */
    state.exit_probe(Event::DyntraceEntry);
}

void dyntrace_exit(dyntracer_t* dyntracer,
                   SEXP expression,
                   SEXP environment,
                   SEXP result,
                   int error) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::DyntraceExit);

    state.cleanup(error);

    /* we do not do start.exit_probe() because the tracer has finished
       executing and we don't need to resume the timer. */
}

static inline void set_dispatch(Call* call,
                                const dyntrace_dispatch_t dispatch) {
    if (dispatch == DYNTRACE_DISPATCH_S3) {
        call->set_S3_method();
    } else if (dispatch == DYNTRACE_DISPATCH_S4) {
        call->set_S4_method();
    }
}

void eval_entry(dyntracer_t* dyntracer, const SEXP expr, const SEXP rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::EvalEntry);

    state.exit_probe(Event::EvalEntry);
}

void closure_entry(dyntracer_t* dyntracer,
                   const SEXP call,
                   const SEXP op,
                   const SEXP args,
                   const SEXP rho,
                   const dyntrace_dispatch_t dispatch) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ClosureEntry);

    Call* function_call = state.create_call(call, op, args, rho);

    // static int loopy = 1;
    // if(function_call -> get_function() -> get_id() ==
    // "jhmb9cUOgugzW1R+979kzg==") {
    //     while(loopy);
    // }

    set_dispatch(function_call, dispatch);

    state.push_stack(function_call);

    state.exit_probe(Event::ClosureEntry);
}

void closure_exit(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch,
                  const SEXP return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ClosureExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_closure()) {
        dyntrace_log_error("Not found matching closure on stack");
    }

    Call* function_call = exec_ctxt.get_closure();

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.notify_caller(function_call);

    state.destroy_call(function_call);

    state.exit_probe(Event::ClosureExit);
}

void builtin_entry(dyntracer_t* dyntracer,
                   const SEXP call,
                   const SEXP op,
                   const SEXP args,
                   const SEXP rho,
                   const dyntrace_dispatch_t dispatch) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::BuiltinEntry);

    Call* function_call = state.create_call(call, op, args, rho);

    set_dispatch(function_call, dispatch);

    state.push_stack(function_call);

    state.exit_probe(Event::BuiltinEntry);
}

void builtin_exit(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch,
                  const SEXP return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::BuiltinExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_builtin()) {
        dyntrace_log_error("Not found matching builtin on stack");
    }

    Call* function_call = exec_ctxt.get_builtin();

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.notify_caller(function_call);

    state.destroy_call(function_call);

    state.exit_probe(Event::BuiltinExit);
}

void special_entry(dyntracer_t* dyntracer,
                   const SEXP call,
                   const SEXP op,
                   const SEXP args,
                   const SEXP rho,
                   const dyntrace_dispatch_t dispatch) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::SpecialEntry);

    Call* function_call = state.create_call(call, op, args, rho);

    set_dispatch(function_call, dispatch);

    state.push_stack(function_call);

    state.exit_probe(Event::SpecialEntry);
}

void special_exit(dyntracer_t* dyntracer,
                  const SEXP call,
                  const SEXP op,
                  const SEXP args,
                  const SEXP rho,
                  const dyntrace_dispatch_t dispatch,
                  const SEXP return_value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::SpecialExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_special()) {
        dyntrace_log_error("Not found matching special object on stack");
    }

    Call* function_call = exec_ctxt.get_special();

    function_call->set_return_value_type(type_of_sexp(return_value));

    state.notify_caller(function_call);

    state.destroy_call(function_call);

    state.exit_probe(Event::SpecialExit);
}

void S3_dispatch_entry(dyntracer_t* dyntracer,
                       const char* generic,
                       const SEXP cls,
                       SEXP generic_method,
                       SEXP specific_method,
                       SEXP objects) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::S3DispatchEntry);

    std::string class_name(UNASSIGNED_CLASS_NAME);
    if (LENGTH(cls) != 0) {
        class_name = CHAR(STRING_ELT(cls, 0));
    }

    DenotedValue* value = state.lookup_promise(CAR(objects), true);

    value->set_class_name(class_name);

    value->used_for_S3_dispatch();

    if (!value->is_forced()) {
        value->set_forcing_scope_if_unset("S3");
    }

    // state.lookup_function(specific_method)->set_generic_method_name(generic);
    // state.lookup_function(generic_method)->set_dispatcher();

    state.exit_probe(Event::S3DispatchEntry);
}

void S4_dispatch_argument(dyntracer_t* dyntracer, const SEXP argument) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::S4DispatchArgument);

    if (type_of_sexp(argument) == PROMSXP) {
        DenotedValue* value = state.lookup_promise(argument, true);

        value->used_for_S4_dispatch();

        if (!value->is_forced()) {
            value->set_forcing_scope_if_unset("S4");
        }
    }

    state.exit_probe(Event::S4DispatchArgument);
}

void context_entry(dyntracer_t* dyntracer, const RCNTXT* cptr) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ContextEntry);

    state.push_stack(cptr);

    state.exit_probe(Event::ContextEntry);
}

void jump_single_context(TracerState& state,
                         ExecutionContext& exec_ctxt,
                         bool returned,
                         const sexptype_t return_value_type,
                         const SEXP rho) {
    if (exec_ctxt.is_call()) {
        Call* call = exec_ctxt.get_call();

        call->set_jumped();
        call->set_return_value_type(return_value_type);

        state.notify_caller(call);

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

void context_jump(dyntracer_t* dyntracer,
                  const RCNTXT* context,
                  const SEXP return_value,
                  int restart) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ContextJump);

    /* Identify promises that do non local return. First, check if
   this special is a 'return', then check if the return happens
   right after a promise is forced, then walk back in the stack
   to the promise with the same environment as the return. This
   promise is the one that does non local return. Note that the
   loop breaks after the first such promise is found. This is
   because only one promise can be held responsible for non local
   return, the one that invokes the return function. */

    execution_contexts_t exec_ctxts(state.unwind_stack(context));

    const SEXP rho = context->cloenv;

    std::size_t context_count = exec_ctxts.size();

    if (context_count == 0) {
    } else if (context_count == 1) {
        jump_single_context(state, exec_ctxts.front(), false, JUMPSXP, rho);
    } else {
        auto begin_iter = exec_ctxts.begin();
        auto end_iter = --exec_ctxts.end();

        bool returned =
            (begin_iter->is_special() &&
             begin_iter->get_special()->get_function()->is_return());

        for (auto iter = begin_iter; iter != end_iter; ++iter) {
            jump_single_context(state, *iter, returned, JUMPSXP, rho);
        }

        jump_single_context(
            state, *end_iter, returned, type_of_sexp(return_value), rho);
    }

    state.exit_probe(Event::ContextJump);
}

void context_exit(dyntracer_t* dyntracer, const RCNTXT* cptr) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::ContextExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_r_context()) {
        dyntrace_log_error("Nonmatching r context on stack");
    }

    state.exit_probe(Event::ContextExit);
}

void gc_allocate(dyntracer_t* dyntracer, const SEXP object) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::GcAllocate);

    state.increment_object_count(type_of_sexp(object));

    if (TYPEOF(object) == PROMSXP) {
        state.create_promise(object);

    } else if (TYPEOF(object) == ENVSXP) {
        state.create_environment(object).get_id();

    } else if (isVector(object)) {
        // analyzer.vector_alloc(info);
    }

    state.exit_probe(Event::GcAllocate);
}

void promise_force_entry(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseForceEntry);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->set_forcing_scope_if_unset(state.infer_forcing_scope());

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

    state.push_stack(promise_state);

    state.exit_probe(Event::PromiseForceEntry);
}

void promise_force_exit(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseForceExit);

    ExecutionContext exec_ctxt = state.pop_stack();

    if (!exec_ctxt.is_promise()) {
        dyntrace_log_error("unable to find matching promise on stack");
    }

    DenotedValue* promise_state = exec_ctxt.get_promise();

    const SEXP value = dyntrace_get_promise_value(promise);

    promise_state->set_value_type(type_of_sexp(value));

    promise_state->set_execution_time(exec_ctxt.get_execution_time());

    state.exit_probe(Event::PromiseForceExit);
}

void promise_value_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseValueLookup);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    /* If a promise is looked up then add it to force order.
       This ensures that S3 arguments are handled correctly.
       The argument is never repeated in the force order as
       the Call::add_to_force_order function does not duplicate
       entries. */
    for (Argument* argument: promise_state->get_arguments()) {
        argument->get_call()->add_to_force_order(
            argument->get_formal_parameter_position());
    }

    promise_state->lookup_value();

    state.exit_probe(Event::PromiseValueLookup);
}

void promise_expression_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseExpressionLookup);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->lookup_expression();

    state.exit_probe(Event::PromiseExpressionLookup);
}

void promise_environment_lookup(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseEnvironmentLookup);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->lookup_environment();

    state.exit_probe(Event::PromiseEnvironmentLookup);
}

void promise_expression_assign(dyntracer_t* dyntracer,
                               const SEXP promise,
                               const SEXP expression) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseExpressionAssign);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_expression();

    state.exit_probe(Event::PromiseExpressionAssign);
}

void promise_value_assign(dyntracer_t* dyntracer,
                          const SEXP promise,
                          const SEXP value) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseValueAssign);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_value();

    state.exit_probe(Event::PromiseValueAssign);
}

void promise_environment_assign(dyntracer_t* dyntracer,
                                const SEXP promise,
                                const SEXP environment) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseEnvironmentAssign);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->assign_environment();

    state.exit_probe(Event::PromiseEnvironmentAssign);
}

void promise_substitute(dyntracer_t* dyntracer, const SEXP promise) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::PromiseSubstitute);

    DenotedValue* promise_state = state.lookup_promise(promise, true);

    promise_state->metaprogram();

    state.exit_probe(Event::PromiseSubstitute);
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

    state.enter_probe(Event::GcUnmark);

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

    state.exit_probe(Event::GcUnmark);
}

void environment_variable_define(dyntracer_t* dyntracer,
                                 const SEXP symbol,
                                 const SEXP value,
                                 const SEXP rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::EnvironmentVariableDefine);

    Variable& var = state.define_variable(rho, symbol);

    state.exit_probe(Event::EnvironmentVariableDefine);
}

void environment_variable_assign(dyntracer_t* dyntracer,
                                 const SEXP symbol,
                                 const SEXP value,
                                 const SEXP rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::EnvironmentVariableAssign);

    Variable& var = state.update_variable(rho, symbol);

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

    state.exit_probe(Event::EnvironmentVariableAssign);
}

void environment_variable_remove(dyntracer_t* dyntracer,
                                 const SEXP symbol,
                                 const SEXP rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::EnvironmentVariableRemove);

    Variable var = state.lookup_variable(rho, symbol);

    state.exit_probe(Event::EnvironmentVariableRemove);
}

void environment_variable_lookup(dyntracer_t* dyntracer,
                                 const SEXP symbol,
                                 const SEXP value,
                                 const SEXP rho) {
    TracerState& state = tracer_state(dyntracer);

    state.enter_probe(Event::EnvironmentVariableLookup);

    Variable& var = state.lookup_variable(rho, symbol);

    state.identify_side_effect_observers(var, rho);

    state.exit_probe(Event::EnvironmentVariableLookup);
}
