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

void context_entry(dyntracer_t* dyntracer, const RCNTXT* cptr) {
  TracerState& state = tracer_state(dyntracer);

  state.enter_probe(Event::ContextEntry);

  state.push_stack(cptr);

  state.exit_probe(Event::ContextEntry);
}
