#ifndef PROMISEDYNTRACER_CALLER_COUNT_MAPPING_H
#define PROMISEDYNTRACER_CALLER_COUNT_MAPPING_H

#include <string>
#include <unordered_map>
#include "DataTableStream.h"
#include "State.h"

class CallerCountMapping {
public:
    CallerCountMapping(const std::vector<std::string>& items = {}) {
        initialize(items);
    }

    void initialize(const std::vector<std::string>& items) {
        for(const std::string& item : items) {
            table_.insert({item, {}});
        }
    }

    void insert(const std::string& item, const fn_id_t& caller_id) {
        auto first_iter = table_.insert({item, {}});
        auto second_iter = first_iter.first -> second.insert({caller_id, 1});
        if(!second_iter.second) {
            ++second_iter.first->second;
        }
    }

    void serialize(DataTableStream* data_table) {
        for(const auto & key_value : table_) {
            for (const auto & caller_count : key_value.second) {
                data_table -> write_row(key_value.first,
                                        caller_count.first,
                                        caller_count.second);
            }
        }
    }

private:

    std::unordered_map<std::string, std::unordered_map<fn_id_t, int>> table_;
};

#endif /* PROMISEDYNTRACER_CALLER_COUNT_MAPPING_H */
