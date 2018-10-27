#ifndef PROMISEDYNTRACER_TYPE_ANALYSIS_H
#define PROMISEDYNTRACER_TYPE_ANALYSIS_H

#include "State.h"
#include "table.h"
#include "utilities.h"

class PromiseTypeAnalysis {
  public:
    PromiseTypeAnalysis(const tracer_state_t &tracer_state,
                        const std::string &output_dir, bool truncate,
                        bool binary, int compression_level);
    void promise_created(const prom_basic_info_t &prom_basic_info,
                         const SEXP promise);
    void closure_entry(const closure_info_t &closure_info);
    void promise_force_exit(const prom_info_t &prom_info, const SEXP promise);
    void gc_promise_unmarked(prom_id_t prom_id, const SEXP promise);
    inline bool add_default_argument_promise(prom_id_t prom_id);
    inline bool add_custom_argument_promise(prom_id_t prom_id);
    inline bool add_non_argument_promise(prom_id_t prom_id);
    inline size_t remove_default_argument_promise(prom_id_t prom_id);
    inline size_t remove_custom_argument_promise(prom_id_t prom_id);
    inline size_t remove_non_argument_promise(prom_id_t prom_id);
    inline bool is_default_argument_promise(prom_id_t prom_id);
    inline bool is_custom_argument_promise(prom_id_t prom_id);
    inline bool is_non_argument_promise(prom_id_t prom_id);
    void end(dyntracer_t *dyntracer);
    ~PromiseTypeAnalysis();

  private:
    using unevaluated_promise_key_t =
        std::tuple<std::string, std::string, std::string>;

    struct UnevaluatedPromiseKeyHasher {
        std::size_t operator()(const unevaluated_promise_key_t &key) const {
            // Compute individual hash values for first, second and third
            // http://stackoverflow.com/a/1646913/126995
            std::size_t res = 17;
            res = res * 31 + std::hash<std::string>()(std::get<0>(key));
            res = res * 31 + std::hash<std::string>()(std::get<1>(key));
            res = res * 31 + std::hash<std::string>()(std::get<2>(key));
            return res;
        }
    };

    void serialize();
    void serialize_evaluated_promises();
    void serialize_unevaluated_promises();
    void add_unevaluated_promise(const std::string promise_type, SEXP promise);

    const tracer_state_t &tracer_state_;
    std::string output_dir_;
    DataTableStream *evaluated_data_table_;
    DataTableStream *unevaluated_data_table_;
    std::set<prom_id_t> default_argument_promises_;
    std::set<prom_id_t> custom_argument_promises_;
    std::set<prom_id_t> non_argument_promises_;
    int default_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    int custom_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    int non_argument_promise_types_[MAX_NUM_SEXPTYPE][MAX_NUM_SEXPTYPE];
    std::unordered_map<unevaluated_promise_key_t, int,
                       UnevaluatedPromiseKeyHasher>
        unevaluated_promises_;
};

#endif /* PROMISEDYNTRACER_TYPE_ANALYSIS_H */
