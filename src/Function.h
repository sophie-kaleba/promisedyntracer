#ifndef PROMISEDYNTRACER_FUNCTION_H
#define PROMISEDYNTRACER_FUNCTION_H

#include "utilities.h"
#include "Call.h"
#include "sexptypes.h"
#include <fstream>
#include "Rinternals.h"


class Function {

public:
    Function(const SEXP op) : op_(op), formal_parameter_count_(0), method_name_(""), dispatcher_(false), wrapper_assigned_(false) {

        type_ = type_of_sexp(op);

        definition_ = get_expression(op);

        id_ = compute_hash(definition_.c_str());

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

    // TODO - make global variable
    bool is_internal() const {
        return (!is_closure() && dyntrace_get_primitive_offset(op_) == 26);
    }

    // TODO - make global variable
    bool is_primitive() const {
        return (!is_closure() && dyntrace_get_primitive_offset(op_) == 27);
    }

    const function_id_t& get_id() const { return id_; }

    const std::string &get_definition() const { return definition_; }

    const std::string& get_namespace() const {
        return namespace_;
    }

    std::size_t get_summary_count() const {
        return force_orders_.size();
    }

    const std::string& get_force_order(std::size_t summary_index) const {
        return force_orders_[summary_index];
    }

    sexptype_t get_return_value_type(std::size_t summary_index) const {
        return return_value_types_[summary_index];
    }

    int get_call_count(std::size_t summary_index) const {
        return call_counts_[summary_index];
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
        /* wrapper is never assigned for builtins, specials and closures that are never called.
           this means that this path will not be taked for them.*/
        if(wrapper_assigned_)
            return wrapper_;
        return false;
    }

    void update_wrapper(bool callee_is_internal_or_primitive) {
        if(wrapper_assigned_) {
            wrapper_ = wrapper_ && callee_is_internal_or_primitive;
        }
        else {
            wrapper_ = callee_is_internal_or_primitive;
            wrapper_assigned_ = true;
        }
    }

    void add_summary(Call* call) {

        int i;

        for(i = 0; i < names_.size(); ++i) {
            if(names_[i] == call -> get_function_name()) {
                break;
            }
        }

        if(i == names_.size()) {
            names_.push_back(call -> get_function_name());
        }

        std::string force_order = call -> get_force_order();
        sexptype_t return_value_type = call -> get_return_value_type();

        for(i = 0; i < force_orders_.size(); ++i) {
            if(force_orders_[i] == force_order &&
               return_value_types_[i] == return_value_type) {
                ++call_counts_[i];
                return;
            }
        }

        force_orders_.push_back(force_order);
        return_value_types_.push_back(return_value_type);
        call_counts_.push_back(1);
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
    std::vector<std::string> force_orders_;
    std::vector<sexptype_t> return_value_types_;
    std::vector<int> call_counts_;

    std::string find_namespace_() {
        SEXP env = CLOENV(op_);
        void (*probe)(dyntracer_t *, SEXP, SEXP, SEXP);
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
