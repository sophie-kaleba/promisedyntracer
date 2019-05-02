#ifndef TURBOTRACER_PROBES_H
#define TURBOTRACER_PROBES_H

#include "TracerState.h"
#include "utilities.h"

#define R_USE_SIGNALS 1
#include "Defn.h"

extern "C" {

  void dyntrace_entry(dyntracer_t* dyntracer, SEXP expression, SEXP environment);

  void dyntrace_exit(dyntracer_t* dyntracer,
                     SEXP expression,
                     SEXP environment,
                     SEXP result,
                     int error);

  void eval_entry(dyntracer_t* dyntracer, const SEXP expr, const SEXP rho);

  void closure_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void closure_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void builtin_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void special_entry(dyntracer_t* dyntracer,
                     const SEXP call,
                     const SEXP op,
                     const SEXP args,
                     const SEXP rho,
                     const dyntrace_dispatch_t dispatch);

  void builtin_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void special_exit(dyntracer_t* dyntracer,
                    const SEXP call,
                    const SEXP op,
                    const SEXP args,
                    const SEXP rho,
                    const dyntrace_dispatch_t dispatch,
                    const SEXP return_value);

  void context_entry(dyntracer_t* dyntracer, const RCNTXT*);

  void context_jump(dyntracer_t* dyntracer,
                    const RCNTXT*,
                    SEXP return_value,
                    int restart);

  void promise_value_lookup(dyntracer_t* dyntracer, const SEXP promise);

  void promise_expression_lookup(dyntracer_t* dyntracer, const SEXP promise);

  void promise_environment_lookup(dyntracer_t* dyntracer, const SEXP promise);

  void promise_value_assign(dyntracer_t* dyntracer,
                            const SEXP promise,
                            const SEXP value);

  void promise_expression_assign(dyntracer_t* dyntracer,
                                 const SEXP promise,
                                 const SEXP expression);

  void promise_environment_assign(dyntracer_t* dyntracer,
                                  const SEXP promise,
                                  const SEXP environment);

  void promise_substitute(dyntracer_t* dyntracer, const SEXP promise);

  void context_exit(dyntracer_t* dyntracer, const RCNTXT*);
  };
#endif /* TURBOTRACER_PROBES_H */
