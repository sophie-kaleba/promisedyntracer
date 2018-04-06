#include "probes.h"
#include "State.h"

const std::string OPCODE_CLOSURE_BEGIN = "clb";
const std::string OPCODE_CLOSURE_FINISH = "clf";
const std::string OPCODE_BUILTIN_BEGIN = "bub";
const std::string OPCODE_BUILTIN_FINISH = "buf";
const std::string OPCODE_SPECIAL_BEGIN = "spb";
const std::string OPCODE_SPECIAL_FINISH = "spf";
const std::string OPCODE_FUNCTION_CONTEXT_JUMP = "fnj";
const std::string OPCODE_PROMISE_CREATE = "prc";
const std::string OPCODE_PROMISE_BEGIN = "prb";
const std::string OPCODE_PROMISE_FINISH = "prf";
const std::string OPCODE_PROMISE_VALUE = "prv";
const std::string OPCODE_PROMISE_CONTEXT_JUMP = "prj";
const std::string OPCODE_PROMISE_EXPRESSION = "pre";
const std::string OPCODE_PROMISE_ENVIRONMENT = "pen";
const std::string OPCODE_ENVIRONMENT_CREATE = "enc";
const std::string OPCODE_ENVIRONMENT_ASSIGN = "ena";
const std::string OPCODE_ENVIRONMENT_REMOVE = "enr";
const std::string OPCODE_ENVIRONMENT_DEFINE = "end";
const std::string OPCODE_ENVIRONMENT_LOOKUP = "enl";

void begin(dyntrace_context_t *context, const SEXP prom) {
    tracer_state(context).start_pass(context, prom);
    debug_serializer(context).serialize_start_trace();
    tracer_serializer(context).serialize_start_trace();
    environment_variables_t environment_variables =
        context->dyntracing_context->environment_variables;
    tracer_serializer(context).serialize_metadatum("GIT_COMMIT_INFO",
                                                   GIT_COMMIT_INFO);
    tracer_serializer(context).serialize_metadatum(
        "DYNTRACE_BEGIN_DATETIME",
        remove_null(context->dyntracing_context->begin_datetime));
    tracer_serializer(context).serialize_metadatum(
        "R_COMPILE_PKGS", remove_null(environment_variables.r_compile_pkgs));
    tracer_serializer(context).serialize_metadatum(
        "R_DISABLE_BYTECODE",
        remove_null(environment_variables.r_disable_bytecode));
    tracer_serializer(context).serialize_metadatum(
        "R_ENABLE_JIT", remove_null(environment_variables.r_enable_jit));
    tracer_serializer(context).serialize_metadatum(
        "R_KEEP_PKG_SOURCE",
        remove_null(environment_variables.r_keep_pkg_source));
    tracer_serializer(context).serialize_metadatum(
        "RDT_COMPILE_VIGNETTE", remove_null(getenv("RDT_COMPILE_VIGNETTE")));
}

void serialize_execution_time(dyntrace_context_t *context) {
    SqlSerializer &serializer = tracer_serializer(context);
    execution_time_t execution_time =
        context->dyntracing_context->execution_time;
    serializer.serialize_metadatum(
        "PROBE_FUNCTION_ENTRY",
        clock_ticks_to_string(execution_time.probe_function_entry));
    serializer.serialize_metadatum(
        "PROBE_FUNCTION_EXIT",
        clock_ticks_to_string(execution_time.probe_function_exit));
    serializer.serialize_metadatum(
        "PROBE_BUILTIN_ENTRY",
        clock_ticks_to_string(execution_time.probe_builtin_entry));
    serializer.serialize_metadatum(
        "PROBE_BUILTIN_EXIT",
        clock_ticks_to_string(execution_time.probe_builtin_exit));
    serializer.serialize_metadatum(
        "PROBE_SPECIALSXP_ENTRY",
        clock_ticks_to_string(execution_time.probe_specialsxp_entry));
    serializer.serialize_metadatum(
        "PROBE_SPECIALSXP_EXIT",
        clock_ticks_to_string(execution_time.probe_specialsxp_exit));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_CREATED",
        clock_ticks_to_string(execution_time.probe_promise_created));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_FORCE_ENTRY",
        clock_ticks_to_string(execution_time.probe_promise_force_entry));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_FORCE_EXIT",
        clock_ticks_to_string(execution_time.probe_promise_force_exit));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_VALUE_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_value_lookup));
    serializer.serialize_metadatum(
        "PROBE_PROMISE_EXPRESSION_LOOKUP",
        clock_ticks_to_string(execution_time.probe_promise_expression_lookup));
    serializer.serialize_metadatum(
        "PROBE_ERROR", clock_ticks_to_string(execution_time.probe_error));
    serializer.serialize_metadatum(
        "PROBE_VECTOR_ALLOC",
        clock_ticks_to_string(execution_time.probe_vector_alloc));
    serializer.serialize_metadatum(
        "PROBE_EVAL_ENTRY",
        clock_ticks_to_string(execution_time.probe_eval_entry));
    serializer.serialize_metadatum(
        "PROBE_EVAL_EXIT",
        clock_ticks_to_string(execution_time.probe_eval_exit));
    serializer.serialize_metadatum(
        "PROBE_GC_ENTRY", clock_ticks_to_string(execution_time.probe_gc_entry));
    serializer.serialize_metadatum(
        "PROBE_GC_EXIT", clock_ticks_to_string(execution_time.probe_gc_exit));
    serializer.serialize_metadatum(
        "PROBE_GC_PROMISE_UNMARKED",
        clock_ticks_to_string(execution_time.probe_gc_promise_unmarked));
    serializer.serialize_metadatum(
        "PROBE_JUMP_CTXT",
        clock_ticks_to_string(execution_time.probe_jump_ctxt));
    serializer.serialize_metadatum(
        "PROBE_NEW_ENVIRONMENT",
        clock_ticks_to_string(execution_time.probe_new_environment));
    serializer.serialize_metadatum(
        "PROBE_S3_GENERIC_ENTRY",
        clock_ticks_to_string(execution_time.probe_S3_generic_entry));
    serializer.serialize_metadatum(
        "PROBE_S3_GENERIC_EXIT",
        clock_ticks_to_string(execution_time.probe_S3_generic_exit));
    serializer.serialize_metadatum(
        "PROBE_S3_DISPATCH_ENTRY",
        clock_ticks_to_string(execution_time.probe_S3_dispatch_entry));
    serializer.serialize_metadatum(
        "PROBE_S3_DISPATCH_EXIT",
        clock_ticks_to_string(execution_time.probe_S3_dispatch_exit));
    serializer.serialize_metadatum(
        "EXPRESSION", clock_ticks_to_string(execution_time.expression));
}

void end(dyntrace_context_t *context) {
    tracer_state(context).finish_pass();
    serialize_execution_time(context);
    // serialize_execution_count(context);
    debug_serializer(context).serialize_finish_trace();
    tracer_serializer(context).serialize_finish_trace();
    tracer_serializer(context).serialize_metadatum(
        "DYNTRACE_END_DATETIME",
        remove_null(context->dyntracing_context->end_datetime));

    if (!tracer_state(context).full_stack.empty()) {
        dyntrace_log_warning(
            "Function/promise stack is not balanced: %d remaining",
            tracer_state(context).full_stack.size());
        tracer_state(context).full_stack.clear();
    }

    // create a file if the execution is normal.
    // end function is only called if the execution is normal,
    // so this file will only be created if everything goes fine.
    std::string database_filepath =
        tracer_serializer(context).get_database_filepath();
    size_t lastindex = database_filepath.find_last_of(".");
    std::string ok_filepath = database_filepath.substr(0, lastindex) + ".OK";
    std::ofstream ok_file(ok_filepath);
    ok_file << "NORMAL EXIT";
    ok_file.close();
}

// Triggered when entering function evaluation.
void function_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                    const SEXP rho) {
    tracer_state(context).increment_closure_counter();
    closure_info_t info = function_entry_get_info(context, call, op, rho);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);

    debug_serializer(context).serialize_function_entry(info);
    tracer_serializer(context).serialize_function_entry(context, info);

    auto &fresh_promises = tracer_state(context).fresh_promises;
    // Associate promises with call ID
    for (auto arg_ref : info.arguments.all()) {
        const arg_t &argument = arg_ref.get();
        auto &promise = get<2>(argument);
        auto it = fresh_promises.find(promise);
        // if promise environment is same as the caller's environment, then
        // serialize this promise as it is a default argument.

        debug_serializer(context).serialize_promise_argument_type(
            promise, get<3>(argument));
        tracer_serializer(context).serialize_promise_argument_type(
            promise, get<3>(argument));

        if (it != fresh_promises.end()) {
            tracer_state(context).promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }
    }

    tracer_serializer(context).serialize_trace(
        OPCODE_CLOSURE_BEGIN, info.fn_id, info.call_id,
        tracer_state(context).to_environment_id(rho));
}

void function_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho, const SEXP retval) {
    closure_info_t info =
        function_exit_get_info(context, call, op, rho, retval);

    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be closure with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(context).full_stack.pop_back();

    debug_serializer(context).serialize_function_exit(info);
    tracer_serializer(context).serialize_function_exit(info);
    tracer_serializer(context).serialize_trace(
        OPCODE_CLOSURE_FINISH, info.fn_id, info.call_id,
        tracer_state(context).to_environment_id(rho));
}

void builtin_entry(dyntrace_context_t *context, const SEXP call, const SEXP op,
                   const SEXP rho) {
    tracer_state(context).increment_builtin_counter();
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    print_entry_info(context, call, op, rho, fn_type);
}

void builtin_exit(dyntrace_context_t *context, const SEXP call, const SEXP op,
                  const SEXP rho, const SEXP retval) {
    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(context, call, op, rho, fn_type, retval);
}

void specialsxp_entry(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho) {
    tracer_state(context).increment_special_counter();
    print_entry_info(context, call, op, rho, function_type::SPECIAL);
}

void specialsxp_exit(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, const SEXP retval) {
    print_exit_info(context, call, op, rho, function_type::SPECIAL, retval);
}

void print_entry_info(dyntrace_context_t *context, const SEXP call,
                      const SEXP op, const SEXP rho, function_type fn_type) {
    builtin_info_t info =
        builtin_entry_get_info(context, call, op, rho, fn_type);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    stack_elem.enclosing_environment = info.call_ptr;
    tracer_state(context).full_stack.push_back(stack_elem);

    debug_serializer(context).serialize_builtin_entry(info);
    tracer_serializer(context).serialize_builtin_entry(context, info);

    tracer_serializer(context).serialize_trace(
        info.fn_type == function_type::SPECIAL ? OPCODE_SPECIAL_BEGIN
                                               : OPCODE_BUILTIN_BEGIN,
        info.fn_id, info.call_id, tracer_state(context).to_environment_id(rho));
}

void print_exit_info(dyntrace_context_t *context, const SEXP call,
                     const SEXP op, const SEXP rho, function_type fn_type,
                     const SEXP retval) {
    builtin_info_t info =
        builtin_exit_get_info(context, call, op, rho, fn_type, retval);

    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::CALL ||
        thing_on_stack.call_id != info.call_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be built-in with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.call_id, info.call_id);
    }
    tracer_state(context).full_stack.pop_back();

    debug_serializer(context).serialize_builtin_exit(info);
    tracer_serializer(context).serialize_builtin_exit(info);

    tracer_serializer(context).serialize_trace(
        info.fn_type == function_type::SPECIAL ? OPCODE_SPECIAL_FINISH
                                               : OPCODE_BUILTIN_FINISH,
        info.fn_id, info.call_id, tracer_state(context).to_environment_id(rho));
}

void promise_created(dyntrace_context_t *context, const SEXP prom,
                     const SEXP rho) {
    prom_basic_info_t info = create_promise_get_info(context, prom, rho);
    debug_serializer(context).serialize_promise_created(info);
    tracer_serializer(context).serialize_promise_created(info);
    if (info.prom_id >= 0) { // maybe we don't need this check
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id,
            0,
            tracer_state(context).get_gc_trigger_counter(),
            tracer_state(context).get_builtin_counter(),
            tracer_state(context).get_special_counter(),
            tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }

    std::string cre_id = std::string("cre ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(cre_id);
    tracer_serializer(context).serialize_trace(
        OPCODE_PROMISE_CREATE, info.prom_id,
        tracer_state(context).to_environment_id(PRENV(prom)));
}

// Promise is being used inside a function body for the first time.
void promise_force_entry(dyntrace_context_t *context, const SEXP promise) {
    prom_info_t info = force_promise_entry_get_info(context, promise);

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    stack_elem.enclosing_environment =
        tracer_state(context)
            .full_stack.back()
            .enclosing_environment; // FIXME necessary?
    tracer_state(context).full_stack.push_back(stack_elem);

    std::string ent_id = std::string("ent ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ent_id);
    tracer_serializer(context).serialize_trace(
        OPCODE_PROMISE_BEGIN, info.prom_id, info.expression_id);
    debug_serializer(context).serialize_force_promise_entry(info);
    tracer_serializer(context).serialize_force_promise_entry(
        context, info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
    /* reset number of environment bindings. We want to know how many bindings
       happened while forcing this promise */
    if (info.prom_id >= 0) {
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id,
            1,
            tracer_state(context).get_gc_trigger_counter(),
            tracer_state(context).get_builtin_counter(),
            tracer_state(context).get_special_counter(),
            tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }
}

void promise_force_exit(dyntrace_context_t *context, const SEXP promise) {
    prom_info_t info = force_promise_exit_get_info(context, promise);

    auto thing_on_stack = tracer_state(context).full_stack.back();
    if (thing_on_stack.type != stack_type::PROMISE ||
        thing_on_stack.promise_id != info.prom_id) {
        dyntrace_log_warning(
            "Object on stack was %s with id %d,"
            " but was expected to be promise with id %d",
            thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
            thing_on_stack.promise_id, info.prom_id);
    }
    tracer_state(context).full_stack.pop_back();

    std::string ext_id = std::string("ext ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(ext_id);
    tracer_serializer(context).serialize_trace(OPCODE_PROMISE_FINISH,
                                               info.prom_id);
    debug_serializer(context).serialize_force_promise_exit(info);
    tracer_serializer(context).serialize_force_promise_exit(
        info, tracer_state(context).clock_id);
    tracer_state(context).clock_id++;
}

void promise_value_lookup(dyntrace_context_t *context, const SEXP promise, int in_force) {
    prom_info_t info = promise_lookup_get_info(context, promise);
    std::string val_id = std::string("val ") + std::to_string(info.prom_id);
    debug_serializer(context).serialize_interference_information(val_id);
    tracer_serializer(context).serialize_trace(OPCODE_PROMISE_VALUE,
                                               info.prom_id);
    if (info.prom_id >= 0) {
        debug_serializer(context).serialize_promise_lookup(info);
        tracer_serializer(context).serialize_promise_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info{
            info.prom_id, 5, tracer_state(context).gc_trigger_counter};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

void promise_expression_lookup(dyntrace_context_t *context, const SEXP prom, int in_force) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_EXPRESSION,
                                                   info.prom_id);
        tracer_serializer(context).serialize_promise_expression_lookup(
            info, tracer_state(context).clock_id);
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
            info.prom_id,
            3,
            tracer_state(context).get_gc_trigger_counter(),
            tracer_state(context).get_builtin_counter(),
            tracer_state(context).get_special_counter(),
            tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

void promise_environment_lookup(dyntrace_context_t *context, const SEXP prom, int in_force) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_ENVIRONMENT,
                                                   info.prom_id);
        //tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
                info.prom_id,
                4,
                tracer_state(context).get_gc_trigger_counter(),
                tracer_state(context).get_builtin_counter(),
                tracer_state(context).get_special_counter(),
                tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

//void promise_value_lookup(dyntrace_context_t *context, const SEXP prom, int in_force) {
//    prom_info_t info = promise_expression_lookup_get_info(context, prom);
//    if (info.prom_id >= 0) {
//        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_ENVIRONMENT,
//                                                   info.prom_id);
//        //tracer_serializer(context).serialize_promise_environment_lookup(
//        //         info, tracer_state(context).clock_id); // FIXME
//        tracer_state(context).clock_id++;
//        prom_lifecycle_info_t prom_gc_info = {
//                info.prom_id,
//                5,
//                tracer_state(context).get_gc_trigger_counter(),
//                tracer_state(context).get_builtin_counter(),
//                tracer_state(context).get_special_counter(),
//                tracer_state(context).get_closure_counter()};
//        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
//        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
//    }
//}


void promise_expression_set(dyntrace_context_t *context, const SEXP prom, int in_force) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_ENVIRONMENT,
                                                   info.prom_id);
        //tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
                info.prom_id,
                6,
                tracer_state(context).get_gc_trigger_counter(),
                tracer_state(context).get_builtin_counter(),
                tracer_state(context).get_special_counter(),
                tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

void promise_value_set(dyntrace_context_t *context, const SEXP prom, int in_force) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_ENVIRONMENT,
                                                   info.prom_id);
        //tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
                info.prom_id,
                8,
                tracer_state(context).get_gc_trigger_counter(),
                tracer_state(context).get_builtin_counter(),
                tracer_state(context).get_special_counter(),
                tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

void promise_environment_set(dyntrace_context_t *context, const SEXP prom, int in_force) {
    prom_info_t info = promise_expression_lookup_get_info(context, prom);
    if (info.prom_id >= 0) {
        tracer_serializer(context).serialize_trace(OPCODE_PROMISE_ENVIRONMENT,
                                                   info.prom_id);
        //tracer_serializer(context).serialize_promise_environment_lookup(
        //         info, tracer_state(context).clock_id); // FIXME
        tracer_state(context).clock_id++;
        prom_lifecycle_info_t prom_gc_info = {
                info.prom_id,
                7,
                tracer_state(context).get_gc_trigger_counter(),
                tracer_state(context).get_builtin_counter(),
                tracer_state(context).get_special_counter(),
                tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, in_force);
    }
}

void gc_promise_unmarked(dyntrace_context_t *context, const SEXP promise) {
    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(context, promise);
    auto &promise_origin = tracer_state(context).promise_origin;

    if (id >= 0) {
        prom_lifecycle_info_t prom_gc_info = {
            id,
            2,
            tracer_state(context).get_gc_trigger_counter(),
            tracer_state(context).get_builtin_counter(),
            tracer_state(context).get_special_counter(),
            tracer_state(context).get_closure_counter()};
        debug_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
        tracer_serializer(context).serialize_promise_lifecycle(prom_gc_info, -1);
    }

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
    }

    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(addr, prom_type, orig_type);
    tracer_state(context).promise_ids.erase(key);
}

void gc_entry(dyntrace_context_t *context, R_size_t size_needed) {
    tracer_state(context).increment_gc_trigger_counter();
}

void gc_exit(dyntrace_context_t *context, int gc_count, double vcells,
             double ncells) {
    gc_info_t info;
    info.vcells = vcells;
    info.ncells = ncells;
    info.counter = tracer_state(context).get_gc_trigger_counter();
    info.builtin_calls = tracer_state(context).get_builtin_calls();
    info.special_calls = tracer_state(context).get_special_calls();
    info.closure_calls = tracer_state(context).get_closure_calls();
    debug_serializer(context).serialize_gc_exit(info);
    tracer_serializer(context).serialize_gc_exit(info);
}

void vector_alloc(dyntrace_context_t *context, int sexptype, long length,
                  long bytes, const char *srcref) {
    type_gc_info_t info{tracer_state(context).get_gc_trigger_counter(),
                        sexptype, length, bytes};
    debug_serializer(context).serialize_vector_alloc(info);
    tracer_serializer(context).serialize_vector_alloc(info);
}

void new_environment(dyntrace_context_t *context, const SEXP rho) {
    // fn_id_t fn_id = (tracer_state(context).fun_stack.back().function_id);
    stack_event_t event = get_last_on_stack_by_type(
        tracer_state(context).full_stack, stack_type::CALL);
    fn_id_t fn_id = event.type == stack_type::NONE
                        ? compute_hash("")
                        : event.function_info.function_id;

    env_id_t env_id = tracer_state(context).environment_id_counter++;
    debug_serializer(context).serialize_new_environment(env_id, fn_id);
    tracer_serializer(context).serialize_new_environment(env_id, fn_id);
    tracer_state(context).environments[rho] =
        std::pair<env_id_t, unordered_map<string, var_id_t>>(env_id, {});
    tracer_serializer(context).serialize_trace(OPCODE_ENVIRONMENT_CREATE,
                                               env_id);
}

void begin_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
    stack_event_t event;
    event.context_id = (rid_t)cptr;
    event.type = stack_type::CONTEXT;
    tracer_state(context).full_stack.push_back(event);
    debug_serializer(context).serialize_begin_ctxt(cptr);
}

void jump_ctxt(dyntrace_context_t *context, const RCNTXT *cptr, const SEXP return_value, int restart) {
    unwind_info_t info;
    info.jump_context = ((rid_t) cptr);
    info.restart = restart;
    adjust_stacks(context, info);
    debug_serializer(context).serialize_unwind(info);
    tracer_serializer(context).serialize_unwind(info);
}

void end_ctxt(dyntrace_context_t *context, const RCNTXT *cptr) {
    stack_event_t event = tracer_state(context).full_stack.back();
    if (event.type == stack_type::CONTEXT && ((rid_t)cptr) == event.context_id)
        tracer_state(context).full_stack.pop_back();
    else
        dyntrace_log_warning("Context trying to remove context %d from full "
                             "stack, but %d is on top of stack.",
                             ((rid_t)cptr), event.context_id);
    debug_serializer(context).serialize_end_ctxt(cptr);
}

void adjust_stacks(dyntrace_context_t *context, unwind_info_t &info) {
    while (!tracer_state(context).full_stack.empty()) {
        stack_event_t event_from_fullstack =
            (tracer_state(context).full_stack.back());

        // if (info.jump_target == event_from_fullstack.enclosing_environment)
        //    break;
        if (event_from_fullstack.type == stack_type::CONTEXT)
            if (info.jump_context == event_from_fullstack.context_id)
                break;
            else
                info.unwound_contexts.push_back(
                    event_from_fullstack.context_id);
        else if (event_from_fullstack.type == stack_type::CALL) {
            tracer_serializer(context).serialize_trace(
                OPCODE_FUNCTION_CONTEXT_JUMP, event_from_fullstack.call_id);
            info.unwound_calls.push_back(event_from_fullstack.call_id);
        } else if (event_from_fullstack.type == stack_type::PROMISE) {
            tracer_serializer(context).serialize_trace(
                OPCODE_PROMISE_CONTEXT_JUMP, event_from_fullstack.promise_id);
            info.unwound_promises.push_back(event_from_fullstack.promise_id);
        } else /* if (event_from_fullstack.type == stack_type::NONE) */
            dyntrace_log_error("NONE object found on tracer's full stack.");

        tracer_state(context).full_stack.pop_back();
    }
}

void environment_action(dyntrace_context_t *context, const SEXP symbol,
                        SEXP value, const SEXP rho, const std::string &action) {
    bool exists = true;
    prom_id_t promise_id = tracer_state(context).enclosing_promise_id();
    env_id_t environment_id = tracer_state(context).to_environment_id(rho);
    var_id_t variable_id =
        tracer_state(context).to_variable_id(symbol, rho, exists);
    // std::string value = serialize_sexp(expression);
    // std::string exphash = compute_hash(value.c_str());
    // serialize variable iff it has not been seen before.
    // if it has been seen before, then it has already been serialized.
    if (!exists) {
        debug_serializer(context).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
        tracer_serializer(context).serialize_variable(
            variable_id, CHAR(PRINTNAME(symbol)), environment_id);
    }
    debug_serializer(context).serialize_variable_action(promise_id, variable_id,
                                                        action);
    tracer_serializer(context).serialize_variable_action(promise_id,
                                                         variable_id, action);
    std::string action_id = action + " " + std::to_string(variable_id);
    debug_serializer(context).serialize_interference_information(action_id);
    tracer_serializer(context).serialize_trace(
        action, tracer_state(context).to_environment_id(rho), variable_id,
        CHAR(PRINTNAME(symbol)), sexp_type_to_string((sexp_type)TYPEOF(value)));
}

void environment_define_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, value, rho, OPCODE_ENVIRONMENT_DEFINE);
}

void environment_assign_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, value, rho, OPCODE_ENVIRONMENT_ASSIGN);
}

void environment_remove_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP rho) {
    environment_action(context, symbol, R_UnboundValue, rho,
                       OPCODE_ENVIRONMENT_REMOVE);
}

void environment_lookup_var(dyntrace_context_t *context, const SEXP symbol,
                            const SEXP value, const SEXP rho) {
    environment_action(context, symbol, value, rho, OPCODE_ENVIRONMENT_LOOKUP);
}
