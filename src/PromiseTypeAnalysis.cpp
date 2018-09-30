#include "PromiseTypeAnalysis.h"

PromiseTypeAnalysis::PromiseTypeAnalysis(const tracer_state_t &tracer_state,
                                         const std::string &output_dir,
                                         bool truncate, bool binary,
                                         int compression_level)
    : tracer_state_(tracer_state),
      output_dir_(output_dir), evaluated_data_table_{create_data_table(
                                   output_dir + "/" + "evaluated-promise-type",
                                   {"promise_type", "promise_expression_type",
                                    "promise_value_type", "count"},
                                   truncate, binary, compression_level)},
      unevaluated_data_table_{
          create_data_table(output_dir + "/" + "unevaluated-promise-type",
                            {"promise_type", "promise_expression_type",
                             "inferred_promise_value_type", "count"},
                            truncate, binary, compression_level)} {

    for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
        for (int j = 0; j < MAX_NUM_SEXPTYPE; ++j) {
            default_argument_promise_types_[i][j] = 0;
            custom_argument_promise_types_[i][j] = 0;
            non_argument_promise_types_[i][j] = 0;
        }
    }
}

inline bool
PromiseTypeAnalysis::add_default_argument_promise(prom_id_t prom_id) {
    return default_argument_promises_.insert(prom_id).second;
}

inline bool
PromiseTypeAnalysis::add_custom_argument_promise(prom_id_t prom_id) {
    return custom_argument_promises_.insert(prom_id).second;
}

inline bool PromiseTypeAnalysis::add_non_argument_promise(prom_id_t prom_id) {
    return non_argument_promises_.insert(prom_id).second;
}

inline size_t
PromiseTypeAnalysis::remove_default_argument_promise(prom_id_t prom_id) {
    return default_argument_promises_.erase(prom_id);
}

inline size_t
PromiseTypeAnalysis::remove_custom_argument_promise(prom_id_t prom_id) {
    return custom_argument_promises_.erase(prom_id);
}

inline size_t
PromiseTypeAnalysis::remove_non_argument_promise(prom_id_t prom_id) {
    return non_argument_promises_.erase(prom_id);
}

inline bool
PromiseTypeAnalysis::is_default_argument_promise(prom_id_t prom_id) {
    return (default_argument_promises_.find(prom_id) !=
            default_argument_promises_.end());
}

inline bool PromiseTypeAnalysis::is_custom_argument_promise(prom_id_t prom_id) {
    return (custom_argument_promises_.find(prom_id) !=
            custom_argument_promises_.end());
}

inline bool PromiseTypeAnalysis::is_non_argument_promise(prom_id_t prom_id) {
    return (non_argument_promises_.find(prom_id) !=
            non_argument_promises_.end());
}

void PromiseTypeAnalysis::promise_created(
    const prom_basic_info_t &prom_basic_info, const SEXP promise) {
    add_non_argument_promise(prom_basic_info.prom_id);
}

void PromiseTypeAnalysis::closure_entry(const closure_info_t &closure_info) {
    for (const auto &argument : closure_info.arguments) {
        prom_id_t promise_id = argument.promise_id;
        int formal_parameter_position = argument.formal_parameter_position;
        auto parameter_mode = argument.parameter_mode;
        if (parameter_mode == parameter_mode_t::DEFAULT)
            add_default_argument_promise(promise_id);
        else if (parameter_mode == parameter_mode_t::CUSTOM)
            add_custom_argument_promise(promise_id);
        remove_non_argument_promise(promise_id);
    }
}

void PromiseTypeAnalysis::promise_force_exit(const prom_info_t &prom_info,
                                             const SEXP promise) {

    if (is_default_argument_promise(prom_info.prom_id)) {
        ++default_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                         [TYPEOF(PRVALUE(promise))];
        remove_default_argument_promise(prom_info.prom_id);
    } else if (is_custom_argument_promise(prom_info.prom_id)) {
        ++custom_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                        [TYPEOF(PRVALUE(promise))];
        remove_custom_argument_promise(prom_info.prom_id);
    } else if (is_non_argument_promise(prom_info.prom_id)) {
        ++non_argument_promise_types_[TYPEOF(PRCODE(promise))]
                                     [TYPEOF(PRVALUE(promise))];
        remove_non_argument_promise(prom_info.prom_id);
    }
}

void PromiseTypeAnalysis::gc_promise_unmarked(prom_id_t promise_id,
                                              const SEXP promise) {
    if (is_default_argument_promise(promise_id)) {
        add_unevaluated_promise("da", promise);
        remove_default_argument_promise(promise_id);
    } else if (is_custom_argument_promise(promise_id)) {
        add_unevaluated_promise("ca", promise);
        remove_custom_argument_promise(promise_id);
    } else if (is_non_argument_promise(promise_id)) {
        add_unevaluated_promise("na", promise);
        remove_non_argument_promise(promise_id);
    }
}

void PromiseTypeAnalysis::end(dyntracer_t *dyntracer) { serialize(); }

void PromiseTypeAnalysis::add_unevaluated_promise(
    const std::string promise_type, SEXP promise) {
    auto result = unevaluated_promises_.insert(
        {{promise_type, sexptype_to_string(TYPEOF(PRCODE(promise))),
          infer_sexptype(promise)},
         1});
    if (!result.second)
        ++result.first->second;
}

void PromiseTypeAnalysis::serialize() {
    serialize_unevaluated_promises();
    serialize_evaluated_promises();
}

PromiseTypeAnalysis::~PromiseTypeAnalysis() {
    delete unevaluated_data_table_;
    delete evaluated_data_table_;
}

void PromiseTypeAnalysis::serialize_evaluated_promises() {
    auto serializer = [&](const auto &matrix, std::string type) {
        for (int i = 0; i < MAX_NUM_SEXPTYPE; ++i) {
            for (int j = 0; j < MAX_NUM_SEXPTYPE; ++j) {
                if (matrix[i][j] != 0) {
                    evaluated_data_table_->write_row(
                        type, sexptype_to_string(i), sexptype_to_string(j),
                        matrix[i][j]);
                }
            }
        }
    };

    serializer(default_argument_promise_types_, "da");
    serializer(custom_argument_promise_types_, "ca");
    serializer(non_argument_promise_types_, "na");
}

void PromiseTypeAnalysis::serialize_unevaluated_promises() {

    for (auto &key_value : unevaluated_promises_) {
        unevaluated_data_table_->write_row(
            std::get<0>(key_value.first), std::get<1>(key_value.first),
            std::get<2>(key_value.first), key_value.second);
    }
}
