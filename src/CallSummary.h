#ifndef PROMISEDYNTRACER_CALL_SUMMARY_H
#define PROMISEDYNTRACER_CALL_SUMMARY_H

#include "Call.h"

class CallSummary {
  public:
    explicit CallSummary(const Call* const call) {
        force_order_ = call->get_force_order();
        missing_argument_positions_ = call->get_missing_argument_positions();
        return_value_type_ = call->get_return_value_type();
        call_count_ = 1;
    }

    const pos_seq_t& get_force_order() const {
        return force_order_;
    }

    const pos_seq_t& get_missing_argument_positions() const {
        return missing_argument_positions_;
    }

    sexptype_t get_return_value_type() const {
        return return_value_type_;
    }

    int get_call_count() const {
        return call_count_;
    }

    bool try_to_merge(const Call* const call) {
        if (is_mergeable_(call)) {
            call_count_++;
            return true;
        }
        return false;
    }

  private:
    pos_seq_t force_order_;
    pos_seq_t missing_argument_positions_;
    sexptype_t return_value_type_;
    int call_count_;

    bool is_mergeable_(const Call* const call) const {
        return (get_force_order() == call->get_force_order() &&
                get_missing_argument_positions() ==
                    call->get_missing_argument_positions() &&
                get_return_value_type() == call->get_return_value_type());
    }
};

#endif /* PROMISEDYNTRACER_CALL_SUMMARY_H */
