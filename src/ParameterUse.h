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
          mode_(parameter_mode_t::UNASSIGNED), force_index_(-1),
          use_index_(-1) {}

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

    int get_force_index() const { return force_index_; }

    void set_force_index(const int force_index) { force_index_ = force_index; }

    int get_use_index() const { return use_index_; }

    void set_use_index(const int use_index) { use_index_ = use_index; }

  private:
    sexptype_t expression_type_;
    sexptype_t value_type_;
    std::uint8_t force_;
    std::uint8_t lookup_;
    std::uint8_t metaprogram_;
    parameter_mode_t mode_;
    int use_index_;
    int force_index_;
};

#endif /* PROMISE_DYNTRACER_PARAMETER_USE_H */
