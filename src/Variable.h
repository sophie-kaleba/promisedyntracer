#ifndef PROMISEDYNTRACER_VARIABLE_H
#define PROMISEDYNTRACER_VARIABLE_H

#include "utilities.h"

typedef int var_id_t;

class Variable {

public:
    Variable(const std::string& name,
             const var_id_t id,
             const timestamp_t modification_timestamp,
             const SEXP rho,
             const env_id_t env_id):
        name_(name),
        id_(id),
        modification_timestamp_(modification_timestamp),
        rho_(rho),
        env_id_(env_id) { }

    var_id_t get_id() const { return id_; }

    const std::string& get_name() const { return name_; }

    void set_modification_timestamp(timestamp_t modification_timestamp) {
        modification_timestamp_ = modification_timestamp;
    }

    timestamp_t get_modification_timestamp() const {
        return modification_timestamp_;
    }

    const SEXP get_rho() const {
        return rho_;
    }

    const env_id_t get_environment_id() const {
        return env_id_;
    }

private:
    const std::string name_;
    const var_id_t id_;
    timestamp_t modification_timestamp_;
    const SEXP rho_;
    const env_id_t env_id_;
};

#endif /* PROMISEDYNTRACER_VARIABLE_H */
