#ifndef __PROBES_H__
#define __PROBES_H__

#include "Context.h"
#include "recorder.h"
#include "utilities.h"

#define R_USE_SIGNALS 1
#include "Defn.h"

extern "C" {

void dyntrace_entry(dyntracer_t *dyntracer, SEXP expression, SEXP environment);
void dyntrace_exit(dyntracer_t *dyntracer, SEXP expression, SEXP environment,
                   SEXP result, int error);
void closure_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void closure_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void builtin_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void special_entry(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                   const SEXP args, const SEXP rho);
void builtin_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void special_exit(dyntracer_t *dyntracer, const SEXP call, const SEXP op,
                  const SEXP args, const SEXP rho, const SEXP retval);
void gc_allocate(dyntracer_t *dyntracer, const SEXP object);
void promise_force_entry(dyntracer_t *dyntracer, const SEXP promise);
void promise_force_exit(dyntracer_t *dyntracer, const SEXP promise);
void promise_value_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_expression_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_environment_lookup(dyntracer_t *dyntracer, const SEXP promise);
void promise_value_assign(dyntracer_t *dyntracer, const SEXP promise,
                          const SEXP value);
void promise_expression_assign(dyntracer_t *dyntracer, const SEXP promise,
                               const SEXP expression);
void promise_environment_assign(dyntracer_t *dyntracer, const SEXP promise,
                                const SEXP environment);
void gc_unmark(dyntracer_t *dyntracer, const SEXP expression);
void gc_promise_unmark(tracer_state_t & state, Analyzer& analyzer, const SEXP promise);
void gc_closure_unmark(tracer_state_t& state, const SEXP closure);
void gc_environment_unmark(tracer_state_t& state, const SEXP expression);
void gc_entry(dyntracer_t *dyntracer, R_size_t size_needed);
void gc_exit(dyntracer_t *dyntracer, int gc_count);
void context_entry(dyntracer_t *dyntracer, const RCNTXT *);
void context_jump(dyntracer_t *dyntracer, const RCNTXT *, SEXP return_value,
                  int restart);
void context_exit(dyntracer_t *dyntracer, const RCNTXT *);
void environment_variable_define(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
void environment_variable_assign(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
void environment_variable_remove(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP rho);
void environment_variable_lookup(dyntracer_t *dyntracer, const SEXP symbol,
                                 const SEXP value, const SEXP rho);
}
#endif /* __PROBES_H__ */
