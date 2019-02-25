#ifndef PROMISEDYNTRACER_FUNCTION_H
#define PROMISEDYNTRACER_FUNCTION_H

#include "Call.h"
#include "CallSummary.h"
#include "Rinternals.h"
#include "sexptypes.h"
#include "utilities.h"

#include <fstream>

class Function {
  public:
    explicit Function(const SEXP op)
        : op_(op)
        , formal_parameter_count_(0)
        , method_name_("")
        , dispatcher_(false)
        , wrapper_assigned_(false) {
        type_ = type_of_sexp(op);

        if (type_ == CLOSXP) {
            definition_ = get_expression(op);
            id_ = compute_hash(definition_.c_str());
        } else {
            definition_ = "function body not extracted for non closures";
            id_ = dyntrace_get_c_function_name(op);
        }

        if (type_ == CLOSXP) {
            for (SEXP formal = FORMALS(op); formal != R_NilValue;
                 formal = CDR(formal)) {
                ++formal_parameter_count_;
            }
            /* All closures are assumed to be wrappers to begin with. */
            wrapper_ = true;
        } else {
            formal_parameter_count_ = dyntrace_get_c_function_arity(op);
            /* non closures are never wrappers.
               being a wrapper is a property of closures. */
            wrapper_ = false;
        }

        namespace_ = find_namespace_();
    }

    bool is_byte_compiled() const {
        return TYPEOF(BODY(op_)) == BCODESXP;
    }

    int get_formal_parameter_count() const {
        return formal_parameter_count_;
    }

    sexptype_t get_type() const {
        return type_;
    }

    bool is_closure() const {
        return type_ == CLOSXP;
    }

    bool is_special() const {
        return type_ == SPECIALSXP;
    }

    bool is_internal() const {
        return (!is_closure() && dyntrace_get_primitive_offset(op_) ==
                                     PRIMITIVE_INTERNAL_OFFSET);
    }

    bool is_primitive() const {
        return (!is_closure() && dyntrace_get_primitive_offset(op_) ==
                                     PRIMITIVE_PRIMITIVE_OFFSET);
    }

    bool is_return() const {
        return (is_special() &&
                dyntrace_get_primitive_offset(op_) == PRIMITIVE_RETURN_OFFSET);
    }

    bool is_curly_bracket() const {
        return (is_special() && dyntrace_get_primitive_offset(op_) ==
                                    PRIMITIVE_CURLY_BRACKET_OFFSET);
    }

    const function_id_t& get_id() const {
        return id_;
    }

    const std::string& get_definition() const {
        return definition_;
    }

    const std::string& get_namespace() const {
        return namespace_;
    }

    std::size_t get_summary_count() const {
        return call_summaries_.size();
    }

    const CallSummary& get_call_summary(std::size_t summary_index) const {
        return call_summaries_[summary_index];
    }

    const std::vector<std::string>& get_names() const {
        return names_;
    }

    void set_generic_method_name(const std::string& method_name) {
        method_name_ = method_name;
    }

    const std::string& get_generic_method_name() const {
        return method_name_;
    }

    bool is_dispatcher() const {
        return dispatcher_;
    }

    void set_dispatcher() {
        dispatcher_ = true;
    }

    bool is_wrapper() const {
        /* wrapper is never assigned for builtins, specials and closures that
           are never called. this means that this path will not be taked for
           them.*/
        if (wrapper_assigned_)
            return wrapper_;
        return false;
    }

    void update_wrapper(bool callee_is_internal_or_primitive) {
        if (wrapper_assigned_) {
            wrapper_ = wrapper_ && callee_is_internal_or_primitive;
        } else {
            wrapper_ = callee_is_internal_or_primitive;
            wrapper_assigned_ = true;
        }
    }

    void add_summary(Call* call) {
        int i;

        for (i = 0; i < names_.size(); ++i) {
            if (names_[i] == call->get_function_name()) {
                break;
            }
        }

        if (i == names_.size()) {
            names_.push_back(call->get_function_name());
        }

        for (i = 0; i < call_summaries_.size(); ++i) {
            if (call_summaries_[i].try_to_merge(call)) {
                return;
            }
        }

        call_summaries_.push_back(CallSummary(call));
    }

  private:
    const SEXP op_;
    sexptype_t type_;
    std::size_t formal_parameter_count_;
    std::string method_name_;
    bool dispatcher_;
    bool wrapper_;
    bool wrapper_assigned_;
    std::string definition_;
    function_id_t id_;
    std::string namespace_;
    std::vector<std::string> names_;
    std::vector<CallSummary> call_summaries_;

    std::string find_namespace_() {
        SEXP env = CLOENV(op_);
        void (*probe)(dyntracer_t*, SEXP, SEXP, SEXP);
        probe = dyntrace_active_dyntracer->probe_environment_variable_lookup;
        dyntrace_active_dyntracer->probe_environment_variable_lookup = NULL;
        SEXP spec = R_NamespaceEnvSpec(env);
        dyntrace_active_dyntracer->probe_environment_variable_lookup = probe;
        if (spec != R_NilValue) {
            if (TYPEOF(spec) == STRSXP && LENGTH(spec) > 0) {
                return CHAR(STRING_ELT(spec, 0));
            } else if (TYPEOF(spec) == CHARSXP) {
                return CHAR(spec);
            }
        }
        return "";
    }
};

#endif /* PROMISEDYNTRACER_FUNCTION_H */
