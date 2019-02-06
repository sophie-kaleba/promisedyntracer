#include "recorder.h"
#include "lookup.h"

void update_closure_argument(closure_info_t &info, dyntracer_t *dyntracer,
                             call_id_t call_id, const SEXP arg_name,
                             const SEXP arg_value, const SEXP environment,
                             bool dot_argument, bool missing, int position) {

    arg_t argument;
    SEXPTYPE arg_value_type = TYPEOF(arg_value);
    SEXPTYPE arg_name_type = TYPEOF(arg_name);

    if (arg_name != R_NilValue) {
        argument.name = std::string(get_name(arg_name));
    } else {
        argument.name = "promise_dyntracer::missing_name";
    }

    if (arg_value_type == PROMSXP) {
        argument.promise = arg_value;
        argument.parameter_mode = PRENV(arg_value) == environment
                                      ? parameter_mode_t::DEFAULT
                                      : parameter_mode_t::CUSTOM;
        argument.promise_environment = PRENV(arg_value);
        argument.expression_type = static_cast<sexptype_t>(arg_value_type);
    } else {
        argument.promise = nullptr;
        argument.parameter_mode =
            missing ? parameter_mode_t::MISSING : parameter_mode_t ::NONPROMISE;

        argument.promise_environment = NULL;
        argument.expression_type = missing ? MISSINGSXP : TYPEOF(arg_value);
    }

    if (dot_argument) {
        argument.name_type = (sexptype_t)DOTSXP;
    } else {
        argument.name_type = static_cast<sexptype_t>(arg_name_type);
    }

    argument.id = get_argument_id(dyntracer, call_id, argument.name);

    argument.formal_parameter_position = position;
    info.arguments.push_back(argument);
}


closure_info_t function_entry_get_info(dyntracer_t *dyntracer, const SEXP call,
                                       const SEXP op, const SEXP args,
                                       const SEXP rho) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_type = function_type::CLOSURE;

    info.fn_definition = get_function_definition(dyntracer, op);

    info.fn_id = get_function_id(dyntracer, info.fn_definition);

    info.fn_addr = op;
    info.call_ptr = rho;

    info.call_id = make_funcall_id(dyntracer, op);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(1);

    void (*probe)(dyntracer_t *, SEXP);
    probe = dyntrace_active_dyntracer->probe_promise_expression_lookup;
    dyntrace_active_dyntracer->probe_promise_expression_lookup = NULL;
    info.call_expression = get_expression(call);

    dyntrace_active_dyntracer->probe_promise_expression_lookup = probe;
    if (ns) {
        info.name = std::string(ns) + "::" + check_string(name);
    } else {
        if (name != NULL)
            info.name = name;
    }

    update_closure_arguments(info, dyntracer, info.call_id, FORMALS(op),
                             FRAME(rho), rho);

    return info;
}

closure_info_t function_exit_get_info(dyntracer_t *dyntracer, const SEXP call,
                                      const SEXP op, const SEXP args,
                                      const SEXP rho, const SEXP retval) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);

    info.fn_definition = get_function_definition(dyntracer, op);

    info.fn_id = get_function_id(dyntracer, info.fn_definition);

    info.fn_addr = op;

    info.call_id = 0;
    auto& stack = tracer_state(dyntracer).full_stack;
    for(auto i = stack.rbegin(); i != stack.rend(); ++i) {
        if(i -> type == stack_type::CALL) {
            info.call_id = i -> call_id;
            break;
        }
    }

    info.fn_type = function_type::CLOSURE;

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    if (ns) {
        info.name = std::string(ns) + "::" + check_string(name);
    } else {
        if (name != NULL)
            info.name = name;
    }

    update_closure_arguments(info, dyntracer, info.call_id, FORMALS(op),
                             FRAME(rho), rho);

    info.fn_definition = get_expression(op);

    //get_stack_parent2(info, tracer_state(dyntracer).full_stack);

    info.return_value_type = static_cast<sexptype_t>(TYPEOF(retval));

    return info;
}

builtin_info_t builtin_entry_get_info(dyntracer_t *dyntracer, const SEXP call,
                                      const SEXP op, const SEXP rho,
                                      function_type fn_type) {
    builtin_info_t info;
    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_definition = get_function_definition(dyntracer, op);
    info.fn_id = get_function_id(dyntracer, info.fn_definition, true);
    info.fn_addr = op;
    info.name = info.name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);

    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);
    info.call_ptr = rho;
    info.call_id = make_funcall_id(dyntracer, op);

    //get_stack_parent(info, tracer_state(dyntracer).full_stack);

    info.formal_parameter_count = PRIMARITY(op);
    info.eval = (R_FunTab[(op)->u.primsxp.offset].eval) % 10;

    return info;
}

builtin_info_t builtin_exit_get_info(dyntracer_t *dyntracer, const SEXP call,
                                     const SEXP op, const SEXP rho,
                                     function_type fn_type, const SEXP retval) {
    builtin_info_t info;

    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_definition = get_function_definition(dyntracer, op);
    info.fn_id = get_function_id(dyntracer, info.fn_definition, true);
    info.fn_addr = op;

    info.call_id = 0;
    auto &stack = tracer_state(dyntracer).full_stack;
    for (auto i = stack.rbegin(); i != stack.rend(); ++i) {
        if (i->type == stack_type::CALL) {
            info.call_id = i->call_id;
            break;
        }
    }

    if (name != NULL)
        info.name = name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    info.definition_location = get_definition_location_cpp(op);
    info.callsite_location = get_callsite_cpp(0);

    //get_stack_parent2(info, tracer_state(dyntracer).full_stack);

    info.return_value_type = static_cast<sexptype_t>(TYPEOF(retval));
    info.formal_parameter_count = PRIMARITY(op);
    info.eval = (R_FunTab[(op)->u.primsxp.offset].eval) % 10;

    return info;
}

void write_environment_variables(const std::string &filepath) {
    std::ofstream fout(filepath, std::ios::trunc);

    auto serialize_row = [&fout](const std::string &key,
                                 const std::string &value) {
        fout << key << "=" << value << std::endl;
    };

    serialize_row("R_COMPILE_PKGS", to_string(getenv("R_COMPILE_PKGS")));
    serialize_row("R_DISABLE_BYTECODE",
                  to_string(getenv("R_DISABLE_BYTECODE")));
    serialize_row("R_ENABLE_JIT", to_string(getenv("R_ENABLE_JIT")));
    serialize_row("R_KEEP_PKG_SOURCE", to_string(getenv("R_KEEP_PKG_SOURCE")));
}

void write_configuration(const Context &context, const std::string &filepath) {
    std::ofstream fout(filepath, std::ios::trunc);
    const AnalysisSwitch &analysis_switch{context.get_analysis_switch()};
    auto serialize_row = [&fout](const std::string &key,
                                 const std::string &value) {
        fout << key << "=" << value << std::endl;
    };

    serialize_row("truncate", std::to_string(context.get_truncate()));
    serialize_row("verbose", std::to_string(context.is_verbose()));
    serialize_row("binary", std::to_string(context.is_binary()));
    serialize_row("compression_level",
                  std::to_string(context.get_compression_level()));
    serialize_row("GIT_COMMIT_INFO", GIT_COMMIT_INFO);
}
