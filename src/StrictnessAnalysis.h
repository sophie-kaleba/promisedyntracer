#ifndef __STRICTNESS_ANALYSIS_H__
#define __STRICTNESS_ANALYSIS_H__

#include "CallState.h"
#include "FunctionState.h"
#include "PromiseMapper.h"
#include "State.h"
#include "table.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class StrictnessAnalysis {
  public:
    StrictnessAnalysis(const tracer_state_t &tracer_state,
                       PromiseMapper *const promise_mapper,
                       const std::string &output_dir, bool truncate,
                       bool binary, int compression_level);
    void closure_entry(const closure_info_t &closure_info);
    void special_entry(const builtin_info_t &special_info);
    void builtin_entry(const builtin_info_t &builtin_info);
    void closure_exit(const closure_info_t &closure_info);
    void special_exit(const builtin_info_t &special_info);
    void builtin_exit(const builtin_info_t &builtin_info);
    void promise_force_entry(const prom_info_t &prom_info, const SEXP promise);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void promise_value_lookup(const prom_info_t &prom_info, const SEXP promise);
    void promise_value_assign(const prom_info_t &info, const SEXP promise);
    void promise_environment_lookup(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_environment_assign(const prom_info_t &prom_info,
                                    const SEXP promise);
    void promise_expression_lookup(const prom_info_t &prom_info,
                                   const SEXP promise);
    void promise_expression_assign(const prom_info_t &prom_info,
                                   const SEXP promise);
    void context_jump(const unwind_info_t &info);
    void end(dyntracer_t *dyntracer);
    ~StrictnessAnalysis();

  private:
    void serialize_call_(const CallState &call_state);
    void serialize_arguments_(const CallState &call_state);
    void add_call_graph_edge_(const call_id_t callee_id);
    void metaprogram_(const prom_info_t &prom_info, const SEXP promise);
    CallState *get_call_state(const call_id_t call_id);
    void write_function_body_(const fn_id_t &fn_id,
                              const std::string &definition);
    void function_entry_(call_id_t call_id, fn_id_t fn_id,
                         const std::string &fn_type, const std::string &name,
                         int formal_parameter_count, const std::string &order,
                         const std::string &definition);
    CallState function_exit_(call_id_t call_id, sexptype_t return_value_type);

    const tracer_state_t &tracer_state_;
    std::string output_dir_;
    PromiseMapper *const promise_mapper_;
    std::unordered_map<fn_id_t, FunctionState> functions_;
    const std::string closure_type_;
    const std::string builtin_type_;
    const std::string special_type_;
    DataTableStream *argument_data_table_;
    DataTableStream *call_data_table_;
    DataTableStream *call_graph_data_table_;
    std::vector<CallState *> call_stack_;
    std::unordered_map<call_id_t, CallState *> call_map_;
    std::unordered_set<fn_id_t> handled_functions_;
};

#endif /* __STRICTNESS_ANALYSIS_H__ */
