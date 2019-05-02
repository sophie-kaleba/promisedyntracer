#ifndef TURBOTRACER_TRACER_H
#define TURBOTRACER_TRACER_H

#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

  SEXP create_dyntracer(SEXP output_dirpath,
                        SEXP verbose,
                        SEXP truncate,
                        SEXP binary,
                        SEXP compression_level);

  SEXP destroy_dyntracer(SEXP dyntracer_sexp);

#ifdef __cplusplus
}
#endif

#endif /* TURBOTRACER_TRACER_H */
