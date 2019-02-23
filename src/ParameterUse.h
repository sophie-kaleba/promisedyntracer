#ifndef PROMISE_DYNTRACER_PARAMETER_USE_H
#define PROMISE_DYNTRACER_PARAMETER_USE_H

#include "sexptypes.h"
#include "utilities.h"

#include <cstdint>
#include <string>

class ParameterUse {
  public:
    explicit ParameterUse()
        : expression_type_(UNASSIGNEDSXP)
        , value_type_(UNASSIGNEDSXP)
        , force_(0)
        , lookup_(0)
        , metaprogram_(0)
        , mode_(parameter_mode_t::UNASSIGNED)
        , escape_mode_("X")
        , execution_time_(0.0)
        , eval_depth_{UNASSIGNED_PROMISE_EVAL_DEPTH} {
    }

    std::uint8_t get_metaprogram() const {
        return metaprogram_;
    }

    void metaprogram() {
        ++metaprogram_;
    }

    std::uint8_t get_lookup() const {
        return lookup_;
    }

    void lookup() {
        ++lookup_;
    }

    std::uint8_t get_force() const {
        return force_;
    }

    void force() {
        ++force_;
    }

    sexptype_t get_expression_type() const {
        return expression_type_;
    }

    void set_expression_type(const sexptype_t expression_type) {
        expression_type_ = expression_type;
    }

    sexptype_t get_value_type() const {
        return value_type_;
    }

    void set_value_type(const sexptype_t value_type) {
        value_type_ = value_type;
    }

    parameter_mode_t get_parameter_mode() const {
        return mode_;
    }

    void set_parameter_mode(parameter_mode_t mode) {
        mode_ = mode;
    }

    const std::string& get_escape() const {
        return escape_mode_;
    }

    void set_escape(const std::string& escape_mode) {
        escape_mode_ = escape_mode;
    }

    double get_execution_time() const {
        return execution_time_;
    }

    void set_execution_time(double execution_time) {
        execution_time_ = execution_time;
    }

    void set_evaluation_depth(const eval_depth_t& eval_depth) {
        eval_depth_ = eval_depth;
    }

    eval_depth_t get_evaluation_depth() const {
        return eval_depth_;
    }

  private:
    sexptype_t       expression_type_;
    sexptype_t       value_type_;
    std::uint8_t     force_;
    std::uint8_t     lookup_;
    std::uint8_t     metaprogram_;
    parameter_mode_t mode_;
    std::string      escape_mode_;
    double           execution_time_;
    eval_depth_t     eval_depth_;
};

#endif /* PROMISE_DYNTRACER_PARAMETER_USE_H */
