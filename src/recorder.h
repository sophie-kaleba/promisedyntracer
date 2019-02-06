#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "Context.h"
#include "State.h"
#include "sexptypes.h"
#include "utilities.h"

// closure_info_t function_entry_get_info(dyntracer_t *dyntracer, const SEXP call,
//                                        const SEXP op, const SEXP args,
//                                        const SEXP rho);
// closure_info_t function_exit_get_info(dyntracer_t *dyntracer, const SEXP call,
//                                       const SEXP op, const SEXP args,
//                                       const SEXP rho, const SEXP retval);
// builtin_info_t builtin_entry_get_info(dyntracer_t *dyntracer, const SEXP call,
//                                       const SEXP op, const SEXP rho,
//                                       function_type fn_type);
// builtin_info_t builtin_exit_get_info(dyntracer_t *dyntracer, const SEXP call,
//                                      const SEXP op, const SEXP rho,
//                                      function_type fn_type, const SEXP retval);

void write_environment_variables(const std::string &filepath);

void write_configuration(const Context &context, const std::string &filepath);
#endif /* __RECORDER_H__ */
