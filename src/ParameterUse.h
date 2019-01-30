#ifndef PROMISE_DYNTRACER_PARAMETER_USE_H
#define PROMISE_DYNTRACER_PARAMETER_USE_H

#include "State.h"
#include "sexptypes.h"
#include <cstdint>
#include <string>

class ParameterUse {
  public:
    explicit ParameterUse()
        : expression_type_(UNASSIGNEDSXP), value_type_(UNASSIGNEDSXP),
          force_(0), lookup_(0), metaprogram_(0),
          mode_(parameter_mode_t::UNASSIGNED), escape_(false), execution_time_(0.0) {}

    std::uint8_t get_metaprogram() const { return metaprogram_; }

    void metaprogram() { ++metaprogram_; }

    std::uint8_t get_lookup() const { return lookup_; }

    void lookup() { ++lookup_; }

    std::uint8_t get_force() const { return force_; }

    void force() { ++force_; }

    sexptype_t get_expression_type() const { return expression_type_; }

    void set_expression_type(const sexptype_t expression_type) {
        expression_type_ = expression_type;
    }

    sexptype_t get_value_type() const { return value_type_; }

    void set_value_type(const sexptype_t value_type) {
        value_type_ = value_type;
    }

    parameter_mode_t get_parameter_mode() const { return mode_; }

    void set_parameter_mode(parameter_mode_t mode) { mode_ = mode; }

    bool get_escape() const { return escape_; }

    void set_escape() { escape_ = true; }

    double get_execution_time() const { return execution_time_; }

    void set_execution_time(double execution_time) {
        execution_time_ = execution_time;
    }

  private:
    sexptype_t expression_type_;
    sexptype_t value_type_;
    std::uint8_t force_;
    std::uint8_t lookup_;
    std::uint8_t metaprogram_;
    parameter_mode_t mode_;
    bool escape_;
    double execution_time_;
};

#endif /* PROMISE_DYNTRACER_PARAMETER_USE_H */
