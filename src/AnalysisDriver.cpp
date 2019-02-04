#include "AnalysisDriver.h"

AnalysisDriver::AnalysisDriver(tracer_state_t &tracer_state, bool verbose,
                               const std::string &output_dir, bool truncate,
                               bool binary, int compression_level,
                               const AnalysisSwitch analysis_switch)
    : analysis_switch_{analysis_switch},
      strictness_analysis_{tracer_state, output_dir,
                           truncate,     binary,           compression_level} {
    if (verbose) {
        std::cout << analysis_switch;
    }
}

void AnalysisDriver::begin(dyntracer_t *dyntracer) {}

void AnalysisDriver::promise_created(const prom_basic_info_t &prom_basic_info,
                                     const SEXP promise) {
    strictness_analysis_.promise_created(prom_basic_info, promise);
}

void AnalysisDriver::closure_entry(const closure_info_t &closure_info) {

    if (analyze_strictness())
        strictness_analysis_.closure_entry(closure_info);

}

void AnalysisDriver::special_entry(const builtin_info_t &special_info) {

    if (analyze_strictness())
        strictness_analysis_.special_entry(special_info);

}

void AnalysisDriver::builtin_entry(const builtin_info_t &builtin_info) {

    if (analyze_strictness())
        strictness_analysis_.builtin_entry(builtin_info);

}

void AnalysisDriver::closure_exit(const closure_info_t &closure_info) {

    if (analyze_strictness())
        strictness_analysis_.closure_exit(closure_info);

}

void AnalysisDriver::builtin_exit(const builtin_info_t &builtin_info) {

    if (analyze_strictness())
        strictness_analysis_.builtin_exit(builtin_info);

}

void AnalysisDriver::special_exit(const builtin_info_t &special_info) {

    if (analyze_strictness())
        strictness_analysis_.special_exit(special_info);

}

void AnalysisDriver::promise_force_entry(const prom_info_t &prom_info,
                                         const SEXP promise) {
    if (analyze_strictness())
        strictness_analysis_.promise_force_entry(prom_info, promise);

}

void AnalysisDriver::promise_force_exit(const prom_info_t &prom_info,
                                        const SEXP promise) {

    if (analyze_strictness())
        strictness_analysis_.promise_force_exit(prom_info, promise);

}

void AnalysisDriver::gc_promise_unmarked(const prom_id_t prom_id,
                                         const SEXP promise) {

    if (analyze_strictness())
        strictness_analysis_.gc_promise_unmarked(prom_id, promise);

}

void AnalysisDriver::promise_environment_lookup(const prom_info_t &info,
                                                const SEXP promise) {
    if (analyze_strictness())
        strictness_analysis_.promise_environment_lookup(info, promise);

}

void AnalysisDriver::promise_expression_lookup(const prom_info_t &info,
                                               const SEXP promise) {


    if (analyze_strictness())
        strictness_analysis_.promise_expression_lookup(info, promise);

}

void AnalysisDriver::promise_value_lookup(const prom_info_t &info,
                                          const SEXP promise) {
    strictness_analysis_.promise_value_lookup(info, promise);

}

void AnalysisDriver::promise_environment_set(const prom_info_t &info,
                                             const SEXP promise) {

    if (analyze_strictness())
        strictness_analysis_.promise_environment_assign(info, promise);

}

void AnalysisDriver::promise_expression_set(const prom_info_t &info,
                                            const SEXP promise) {

    if (analyze_strictness())
        strictness_analysis_.promise_expression_assign(info, promise);
}

void AnalysisDriver::promise_value_set(const prom_info_t &info,
                                       const SEXP promise) {

    if (analyze_strictness())
        strictness_analysis_.promise_value_assign(info, promise);
}

void AnalysisDriver::vector_alloc(const type_gc_info_t &type_gc_info) {

}

void AnalysisDriver::environment_define_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {

    strictness_analysis_.environment_define_var(symbol, value, rho);

}

void AnalysisDriver::environment_assign_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {

    strictness_analysis_.environment_assign_var(symbol, value, rho);

}

void AnalysisDriver::environment_lookup_var(const SEXP symbol, const SEXP value,
                                            const SEXP rho) {

    strictness_analysis_.environment_lookup_var(symbol, value, rho);

}

void AnalysisDriver::environment_remove_var(const SEXP symbol, const SEXP rho) {

    strictness_analysis_.environment_remove_var(symbol, rho);

}

void AnalysisDriver::context_jump(const unwind_info_t &info) {

    if (analyze_strictness())
        strictness_analysis_.context_jump(info);

}

void AnalysisDriver::end(dyntracer_t *dyntracer) {

    if (analyze_strictness())
        strictness_analysis_.end(dyntracer);

}

inline bool AnalysisDriver::analyze_metadata() const {
    return analysis_switch_.metadata;
}

inline bool AnalysisDriver::analyze_functions() const {
    return analysis_switch_.function;
}

inline bool AnalysisDriver::analyze_strictness() const {
    return analysis_switch_.strictness;
}

inline bool AnalysisDriver::analyze_side_effects() const {
    return analysis_switch_.side_effect;
}

inline bool AnalysisDriver::map_promises() const {
    return analysis_switch_.strictness;
}
