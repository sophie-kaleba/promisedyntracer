#include "MetadataAnalysis.h"

MetadataAnalysis::MetadataAnalysis(const tracer_state_t &tracer_state,
                                   const std::string &output_dir)
    : tracer_state_(tracer_state), output_dir_(output_dir) {}

void MetadataAnalysis::end(dyntracer_t *dyntracer) {
    std::ofstream fout(output_dir_ + "/metadata.csv", std::ios::trunc);

    serialize_row(fout, "GIT_COMMIT_INFO", GIT_COMMIT_INFO);
    // serialize_row(fout, "DYNTRACE_BEGIN_DATETIME",
    //               context->dyntracing_context->begin_datetime);

    serialize_row(fout, "R_COMPILE_PKGS",
    to_string(getenv("R_COMPILE_PKGS")));
    serialize_row(fout, "R_DISABLE_BYTECODE",
                  to_string(getenv("R_DISABLE_BYTECODE")));
    serialize_row(fout, "R_ENABLE_JIT", to_string(getenv("R_ENABLE_JIT")));
    serialize_row(fout, "R_KEEP_PKG_SOURCE",
                  to_string(getenv("R_KEEP_PKG_SOURCE")));
    serialize_row(fout, "RDT_COMPILE_VIGNETTE",
                  to_string(getenv("RDT_COMPILE_VIGNETTE")));

    // serialize_row(fout, "DYNTRACE_END_DATETIME",
    //               context->dyntracing_context->end_datetime);
    // serialize_row(fout, "PROBE_FUNCTION_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_function_entry));
    // serialize_row(fout, "PROBE_FUNCTION_EXIT",
    //               clock_ticks_to_string(execution_time.probe_function_exit));
    // serialize_row(fout, "PROBE_BUILTIN_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_builtin_entry));
    // serialize_row(fout, "PROBE_BUILTIN_EXIT",
    //               clock_ticks_to_string(execution_time.probe_builtin_exit));
    // serialize_row(fout, "PROBE_SPECIALSXP_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_specialsxp_entry));
    // serialize_row(fout, "PROBE_SPECIALSXP_EXIT",
    //               clock_ticks_to_string(execution_time.probe_specialsxp_exit));
    // serialize_row(fout, "PROBE_PROMISE_CREATED",
    //               clock_ticks_to_string(execution_time.probe_promise_created));
    // serialize_row(
    //     fout, "PROBE_PROMISE_FORCE_ENTRY",
    //     clock_ticks_to_string(execution_time.probe_promise_force_entry));
    // serialize_row(
    //     fout, "PROBE_PROMISE_FORCE_EXIT",
    //     clock_ticks_to_string(execution_time.probe_promise_force_exit));
    // serialize_row(
    //     fout, "PROBE_PROMISE_VALUE_LOOKUP",
    //     clock_ticks_to_string(execution_time.probe_promise_value_lookup));
    // serialize_row(
    //     fout, "PROBE_PROMISE_EXPRESSION_LOOKUP",
    //     clock_ticks_to_string(execution_time.probe_promise_expression_lookup));
    // serialize_row(fout, "PROBE_ERROR",
    //               clock_ticks_to_string(execution_time.probe_error));
    // serialize_row(fout, "PROBE_VECTOR_ALLOC",
    //               clock_ticks_to_string(execution_time.probe_vector_alloc));
    // serialize_row(fout, "PROBE_EVAL_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_eval_entry));
    // serialize_row(fout, "PROBE_EVAL_EXIT",
    //               clock_ticks_to_string(execution_time.probe_eval_exit));
    // serialize_row(fout, "PROBE_GC_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_gc_entry));
    // serialize_row(fout, "PROBE_GC_EXIT",
    //               clock_ticks_to_string(execution_time.probe_gc_exit));
    // serialize_row(
    //     fout, "PROBE_GC_PROMISE_UNMARKED",
    //     clock_ticks_to_string(execution_time.probe_gc_promise_unmarked));
    // serialize_row(fout, "PROBE_JUMP_CTXT",
    //               clock_ticks_to_string(execution_time.probe_jump_ctxt));
    // serialize_row(fout, "PROBE_NEW_ENVIRONMENT",
    //               clock_ticks_to_string(execution_time.probe_new_environment));
    // serialize_row(fout, "PROBE_S3_GENERIC_ENTRY",
    //               clock_ticks_to_string(execution_time.probe_S3_generic_entry));
    // serialize_row(fout, "PROBE_S3_GENERIC_EXIT",
    //               clock_ticks_to_string(execution_time.probe_S3_generic_exit));
    // serialize_row(
    //     fout, "PROBE_S3_DISPATCH_ENTRY",
    //     clock_ticks_to_string(execution_time.probe_S3_dispatch_entry));
    // serialize_row(fout, "PROBE_S3_DISPATCH_EXIT",
    //               clock_ticks_to_string(execution_time.probe_S3_dispatch_exit));
    // serialize_row(fout, "EXPRESSION",
    //               clock_ticks_to_string(execution_time.expression));

#ifdef DYNTRACE_ENABLE_TIMING

    auto timer_serializer = [&](Timer &timer) {
        for (const auto &time : timer.stats()) {
            serialize_row(fout, time.first, time.second);
        }
    };

    timer_serializer(Timer::main_timer());
    timer_serializer(Timer::recorder_timer());
    timer_serializer(Timer::analysis_timer());

#endif
}

void MetadataAnalysis::serialize_row(std::ofstream &fout, std::string key,
                                     std::string value) {
    fout << key << " , "
         << "\"" << value << "\"" << std::endl;
}
