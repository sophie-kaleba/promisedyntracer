#ifndef PROMISEDYNTRACER_FUNCTION_ANALYSIS_H
#define PROMISEDYNTRACER_FUNCTION_ANALYSIS_H

#include "State.h"
#include "Timer.h"
#include "table.h"
#include "utilities.h"

class FunctionAnalysis {
  public:
    FunctionAnalysis(const tracer_state_t &tracer_state,
                     const std::string &output_dir, bool truncate, bool binary,
                     int compression_level)
        : tracer_state_{tracer_state}, output_dir_{output_dir},
          closure_type_{sexptype_to_string(CLOSXP)},
          builtin_type_{sexptype_to_string(BUILTINSXP)},
          special_type_{sexptype_to_string(SPECIALSXP)},
          function_data_table_{create_data_table(
              output_dir + "/" + "functions",
              {"function_id", "function_name", "function_type", "return_type",
               "parameter_count", "call_count"},
              truncate, binary, compression_level)} {}

    void closure_entry(const closure_info_t &closure_info) {
        push_function_(closure_info.fn_id, closure_info.name, closure_type_,
                       closure_info.formal_parameter_count);
        write_function_body_(closure_info.fn_id, closure_info.fn_definition);
    }

    void special_entry(const builtin_info_t &special_info) {
        push_function_(special_info.fn_id, special_info.name, special_type_,
                       special_info.formal_parameter_count);
        write_function_body_(special_info.fn_id, special_info.fn_definition);
    }

    void builtin_entry(const builtin_info_t &builtin_info) {
        push_function_(builtin_info.fn_id, builtin_info.name, builtin_type_,
                       builtin_info.formal_parameter_count);
        write_function_body_(builtin_info.fn_id, builtin_info.fn_definition);
    }

    void closure_exit(const closure_info_t &closure_info) {
        pop_function_(sexptype_to_string(closure_info.return_value_type));
    }

    void special_exit(const builtin_info_t &special_info) {
        pop_function_(sexptype_to_string(special_info.return_value_type));
    }

    void builtin_exit(const builtin_info_t &builtin_info) {
        pop_function_(sexptype_to_string(builtin_info.return_value_type));
    }

    void end(dyntracer_t *dyntracer) { serialize(); }

    void context_jump(const unwind_info_t &info) {
        for (auto &element : info.unwound_frames) {
            if (element.type == stack_type::CALL)
                pop_function_("Unknown (Jumped)");
        }
    }

    ~FunctionAnalysis() { delete function_data_table_; }

  private:
    struct function_key_t {
        std::string function_id;
        std::string function_name;
        std::string function_type;
        std::string return_type;
        int parameter_count;

        bool operator==(const function_key_t &right) const {
            return (function_id == right.function_id &&
                    function_name == right.function_name &&
                    function_type == right.function_type &&
                    return_type == right.return_type &&
                    parameter_count == right.parameter_count);
        }
    };

    void serialize() {
        for (const auto &key_value : functions_) {
            function_data_table_->write_row(
                key_value.first.function_id, key_value.first.function_name,
                key_value.first.function_type, key_value.first.return_type,
                key_value.first.parameter_count, key_value.second);
        }
    }

    void write_function_body_(const fn_id_t &fn_id,
                              const std::string &definition) {
        auto result = handled_functions_.insert(fn_id);
        if (!result.second)
            return;
        std::ofstream fout(output_dir_ + "/functions/" + fn_id,
                           std::ios::trunc);
        fout << definition;
        fout.close();
    }

    void pop_function_(const std::string &return_value_type) {
        auto function_key{function_stack_.back()};
        function_stack_.pop_back();
        function_key.return_type = return_value_type;
        auto result = functions_.insert({function_key, 1});
        if (!result.second) {
            ++result.first->second;
        }
    }

    void push_function_(fn_id_t fn_id, const std::string &fn_name,
                        const std::string &fn_type, int parameter_count) {
        function_key_t key{fn_id, fn_name, fn_type, "unknown", parameter_count};
        function_stack_.push_back(key);
    }

    struct FunctionKeyHasher {

        std::size_t operator()(const function_key_t &key) const {
            // Compute individual hash values for first, second and third
            // http://stackoverflow.com/a/1646913/126995
            std::size_t res = 17;
            res = res * 31 + std::hash<std::string>()(key.function_id);
            res = res * 31 + std::hash<std::string>()(key.function_name);
            res = res * 31 + std::hash<std::string>()(key.function_type);
            res = res * 31 + std::hash<std::string>()(key.return_type);
            res = res * 31 + std::hash<int>()(key.parameter_count);
            return res;
        }
    };

    const tracer_state_t &tracer_state_;
    std::string output_dir_;
    const std::string closure_type_;
    const std::string special_type_;
    const std::string builtin_type_;
    std::unordered_map<function_key_t, int, FunctionKeyHasher> functions_;
    std::unordered_set<fn_id_t> handled_functions_;
    std::vector<function_key_t> function_stack_;
    DataTableStream *function_data_table_;
};

#endif /* PROMISEDYNTRACER_FUNCTION_ANALYSIS_H */
