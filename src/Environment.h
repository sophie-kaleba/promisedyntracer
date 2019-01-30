#ifndef PROMISEDYNTRACER_ENVIRONMENT_H
#define PROMISEDYNTRACER_ENVIRONMENT_H

#include <unordered_map>

typedef int env_id_t;

#include "Variable.h"

class Environment {

public:
    Environment(const SEXP rho, env_id_t id):
        rho_(rho), id_(id) {}

    env_id_t get_id() const { return id_; }

    Variable& lookup(const std::string& symbol) {

        auto iter = variable_mapping_.find(symbol);
        if(iter == variable_mapping_.end()) {
            dyntrace_log_error("Unable to find variable %s in environment.",
                               symbol.c_str());
        }
        return iter -> second;
    }

    bool exists(const std::string& symbol) {
        auto iter = variable_mapping_.find(symbol);
        return(iter != variable_mapping_.end());
    }

    Variable& define(const std::string& symbol,
                const var_id_t var_id,
                const timestamp_t timestamp) {
        auto iter = variable_mapping_.insert({symbol, Variable(symbol,
                                                               var_id,
                                                               timestamp)});
        return iter.first -> second;
    }

    void remove(const std::string& symbol) {
        variable_mapping_.erase(symbol);
    }

private:
    const SEXP rho_;
    const env_id_t id_;

    std::unordered_map<std::string, Variable> variable_mapping_;
};

#endif /* PROMISEDYNTRACER_ENVIRONMENT_H */
