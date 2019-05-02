#include "tracer.h"

#include "probes.h"

extern "C" {

  SEXP create_dyntracer(SEXP output_dirpath,
                        SEXP verbose,
                        SEXP truncate,
                        SEXP binary,
                        SEXP compression_level) {
    void* state = new TracerState(sexp_to_string(output_dirpath),
                                  sexp_to_bool(verbose),
                                  sexp_to_bool(truncate),
                                  sexp_to_bool(binary),
                                  sexp_to_int(compression_level));

    /* calloc initializes the memory to zero. This ensures that probes not
     attached will be NULL. Replacing calloc with malloc will cause
     segfaults. */
    dyntracer_t* dyntracer = (dyntracer_t*) calloc(1, sizeof(dyntracer_t));
    dyntracer->probe_dyntrace_entry = dyntrace_entry;
    dyntracer->probe_dyntrace_exit = dyntrace_exit;
    dyntracer->probe_eval_entry = eval_entry;
    dyntracer->probe_closure_entry = closure_entry;
    dyntracer->probe_closure_exit = closure_exit;
    dyntracer->probe_builtin_entry = builtin_entry;
    dyntracer->probe_builtin_exit = builtin_exit;
    dyntracer->probe_special_entry = special_entry;
    dyntracer->probe_special_exit = special_exit;
    dyntracer->state = state;
    dyntracer->probe_promise_value_lookup = promise_value_lookup;
    dyntracer->probe_promise_expression_lookup = promise_expression_lookup;
    dyntracer->probe_promise_environment_lookup = promise_environment_lookup;
    dyntracer->probe_promise_value_assign = promise_value_assign;
    dyntracer->probe_promise_expression_assign = promise_expression_assign;
    dyntracer->probe_promise_environment_assign = promise_environment_assign;
    dyntracer->probe_promise_substitute = promise_substitute;
    dyntracer->probe_context_entry = context_entry;
    dyntracer->probe_context_jump = context_jump;
    dyntracer->probe_context_exit = context_exit;
    return dyntracer_to_sexp(dyntracer, "dyntracer.promise");
  }

  static void destroy_promise_dyntracer(dyntracer_t* dyntracer) {
    /* free dyntracer iff it has not already been freed.
     this check ensures that multiple calls to destroy_dyntracer on the same
     object do not crash the process. */
    if (dyntracer) {
      delete (static_cast<TracerState*>(dyntracer->state));
      free(dyntracer);
    }
  }

  SEXP destroy_dyntracer(SEXP dyntracer_sexp) {
    return dyntracer_destroy_sexp(dyntracer_sexp, destroy_promise_dyntracer);
  }

} // extern "C"
