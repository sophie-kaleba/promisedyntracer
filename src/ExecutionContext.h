#ifndef PROMISEDYNTRACER_EXECUTION_CONTEXT_H
#define PROMISEDYNTRACER_EXECUTION_CONTEXT_H

#include "sexptypes.h"
#include <Rinternals.h>

/* forward declarations to prevent cyclic dependencies */
class PromiseState;
class CallState;

class ExecutionContext {
  public:
    explicit ExecutionContext(PromiseState* promise_state):
        type_(PROMSXP), promise_state_(promise_state) {}

    explicit ExecutionContext(const RCNTXT* r_context)
        : type_(CONTEXTSXP), r_context_(r_context) {}

    explicit ExecutionContext(CallState * call_state)
        : type_(call_state -> get_function_type()),
          call_state_(call_state) {}

    bool is_promise() const { return (type_ == PROMSXP); }

    bool is_builtin() const { return (type_ == BUILTINSXP); }

    bool is_special() const { return (type_ == SPECIALSXP); }

    bool is_closure() const { return (type_ == CLOSXP); }

    bool is_call() const { return is_builtin() || is_special() || is_closure(); }

    bool is_r_context() const { return (type_ == CONTEXTSXP); }

    PromiseState *get_promise() { return promise_state_; }

    CallState *get_builtin() { return call_state_; }

    CallState *get_special() { return call_state_; }

    CallState *get_closure() { return call_state_; }

    CallState *get_call() { return call_state_; }

    const RCNTXT *get_r_context() const { return r_context_; }

  private:
    sexptype_t type_;
    union {
        PromiseState *promise_state_;
        CallState *call_state_;
        const RCNTXT *r_context_;
    };
};

using execution_contexts_t = std::vector<ExecutionContext>;
using execution_context_iterator = execution_contexts_t::iterator;
using reverse_execution_context_iterator =
    execution_contexts_t::reverse_iterator;
using const_execution_context_iterator = execution_contexts_t::const_iterator;
using const_reverse_execution_context_iterator =
    execution_contexts_t::const_reverse_iterator;

#endif /* PROMISEDYNTRACER_EXECUTION_CONTEXT_H */
