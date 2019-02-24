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
             const env_id_t env_id)
        : name_(name)
        , id_(id)
        , modification_timestamp_(modification_timestamp)
        , rho_(rho)
        , env_id_(env_id) {
    }

    var_id_t get_id() const {
        return id_;
    }

    const std::string& get_name() const {
        return name_;
    }

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

    bool is_dot_dot_dot() const {
        const std::string& name = get_name();
        if (name.size() < 2) {
            return false;
        }

        /* first we check that the variable starts with .. */
        if (name[0] != '.' || name[1] != '.')
            return false;
        /* now we check that the variable ends with
           a sequence of digits. */
        for (int i = 2; i < name.size(); ++i) {
            if (name[i] > '9' || name[i] < '0') {
                return false;
            }
        }

        /* if we reach here, its because the variable name has the form
           ..n where n is a number >= 0. Because indexing in R starts
           from 1, ..0 will never occur but the code above will still let
           ..0 pass to this point. This should not be a problem in practice.
        */
        return true;
    }

  private:
    const std::string name_;
    const var_id_t id_;
    timestamp_t modification_timestamp_;
    const SEXP rho_;
    const env_id_t env_id_;
};

#endif /* PROMISEDYNTRACER_VARIABLE_H */
