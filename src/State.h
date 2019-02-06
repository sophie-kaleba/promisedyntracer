#ifndef PROMISEDYNTRACER_STATE_H
#define PROMISEDYNTRACER_STATE_H

#include "CallState.h"
#include "Environment.h"
#include "PromiseMapper.h"
#include "RExecutionContext.h"
#include "Variable.h"
#include "sexptypes.h"
#include "stdlibs.h"

struct arg_t {
    arg_id_t id;
    std::string name;
    sexptype_t expression_type;
    sexptype_t name_type;
    SEXP promise;
    SEXP promise_environment;
    parameter_mode_t parameter_mode;
    int formal_parameter_position;
};

enum class function_type {
    CLOSURE = 0,
    BUILTIN = 1,
    SPECIAL = 2,
    TRUE_BUILTIN = 3
};

enum class stack_type { PROMISE = 1, CALL = 2, CONTEXT = 3, NONE = 0 };

struct stack_event_t {

    stack_event_t() : execution_time(0) {}

    stack_type type;
    union {
        PromiseState *promise_state;
        CallState *call_state;
        RExecutionContext *execution_context;
    };
    // SEXP environment;
    // Only initialized for type == CALL
    struct {
        fn_id_t function_id;
        function_type type;
    } function_info;
    std::uint64_t execution_time;
};

// struct context_t {
//     rid_t context;
//     SEXP environment;
// };

// struct call_info_t {
//     function_type fn_type;
//     fn_id_t fn_id;
//     SEXP fn_addr; // TODO unnecessary?
//     std::string fn_definition;
//     std::string definition_location;
//     std::string callsite_location;
//     bool fn_compiled;

//     std::string name; // fully qualified function name, if available
//     call_id_t call_id;
//     SEXP call_ptr;

//     sexptype_t return_value_type;
//     std::string call_expression;
//     int formal_parameter_count;
//     int eval;
// };

// struct closure_info_t : call_info_t {
//     arglist_t arguments;
// };

// struct builtin_info_t : call_info_t {};

typedef std::vector<arg_t> arglist_t;

struct unwind_info_t {
    rid_t jump_context;
    int restart;
    std::vector<stack_event_t> unwound_frames;
};

struct gc_info_t {
    int counter;
};

struct type_gc_info_t {
    int gc_trigger_counter;
    unsigned int type;
    unsigned long int length;
    unsigned long int bytes;
};

call_id_t make_funcall_id(dyntracer_t *dyntracer, SEXP);
std::string get_function_definition(dyntracer_t *dyntracer,
                                    const SEXP function);
void remove_function_definition(dyntracer_t *dyntracer, const SEXP function);
fn_id_t get_function_id(dyntracer_t *dyntracer, const std::string &def,
                        bool builtin = false);

// Returns false if function already existed, true if it was registered now
bool register_inserted_function(dyntracer_t *dyntracer, fn_id_t id);

bool function_already_inserted(fn_id_t id);


void update_closure_arguments(closure_info_t &info, dyntracer_t *dyntracer,
                              const call_id_t call_id, const SEXP formals,
                              const SEXP args, const SEXP environment);

struct tracer_state_t {
    std::vector<stack_event_t>
        full_stack; // Should be reset on each tracer pass

    std::unordered_map<promise_id_t, int> promise_lookup_gc_trigger_counter;
    call_id_t call_id_counter;  // IDs assigned should be globally unique but we
                                // can reset it after each pass if overwrite is
                                // true)
    promise_id_t fn_id_counter; // IDs assigned should be globally unique but we
                                // can reset it after each pass if overwrite is
                                // true)

    std::unordered_map<SEXP, std::string> function_definitions;

    std::unordered_map<fn_key_t, fn_id_t> function_ids; // Should be kept across
                                                        // Rdt calls (unless
                                                        // overwrite is true)
    std::unordered_set<fn_id_t>
        already_inserted_functions; // Should be kept across
                                    // Rdt calls (unless
                                    // overwrite is true)

    int gc_trigger_counter; // Incremented each time there is a gc_entry

    tracer_state_t() {
        call_id_counter = 0;
        fn_id_counter = 0;
        gc_trigger_counter = 0;
        environment_id_ = 0;
        variable_id_ = 0;
        timestamp_ = 0;
    }

    void increment_gc_trigger_counter() { gc_trigger_counter++; }

    int get_gc_trigger_counter() const { return gc_trigger_counter; }

    void finish_pass() {

        clear_promises();

        if (!full_stack.empty()) {
            dyntrace_log_warning(
                "Function/promise stack is not balanced: %d remaining",
                full_stack.size());
            full_stack.clear();
        }
    }

    Environment &create_environment(const SEXP rho) {
        auto iter = environment_mapping_.find(rho);
        if (iter != environment_mapping_.end()) {
            return iter->second;
        }
        return environment_mapping_
            .insert({rho, Environment(rho, create_next_environment_id_())})
            .first->second;
    }

    void remove_environment(const SEXP rho) { environment_mapping_.erase(rho); }

    Environment &lookup_environment(const SEXP rho, bool create = true) {
        auto iter = environment_mapping_.find(rho);
        if (iter == environment_mapping_.end()) {
            return create_environment(rho);
        }
        return iter->second;
    }

    Variable &lookup_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        return lookup_variable(rho, symbol_to_string(symbol),
                               create_environment, create_variable);
    }

    Variable &lookup_variable(const SEXP rho, const std::string &symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Environment &env = lookup_environment(rho, create_environment);

        bool var_exists = env.exists(symbol);

        if (create_variable && !var_exists) {
            return env.define(symbol, create_next_variable_id_(),
                              get_current_timestamp());
        }

        return env.lookup(symbol);
    }

    Variable &define_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true) {
        return lookup_environment(rho, true).define(symbol_to_string(symbol),
                                                    create_next_variable_id_(),
                                                    get_current_timestamp());
    }

    Variable &update_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Variable &var = lookup_variable(rho, symbol);
        var.set_modification_timestamp(get_current_timestamp());
        return var;
    }

    Variable remove_variable(const SEXP rho, const SEXP symbol,
                             bool create_environment = true) {
        return lookup_environment(rho, create_environment)
            .remove(symbol_to_string(symbol));
    }

    void resume_execution_timer() {
        execution_resume_time_ = std::chrono::high_resolution_clock::now();
    }

    void pause_execution_timer() {
        auto execution_pause_time = std::chrono::high_resolution_clock::now();
        std::uint64_t execution_time =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                execution_pause_time - execution_resume_time_)
                .count();
        for (auto &element : full_stack) {
            element.execution_time += execution_time;
        }
    }

    std::chrono::time_point<std::chrono::high_resolution_clock>
        execution_resume_time_;

    std::unordered_map<SEXP, Environment> environment_mapping_;

    timestamp_t get_current_timestamp() const { return timestamp_; }

    timestamp_t increment_timestamp() { return timestamp_++; }

    PromiseState *create_promise(const SEXP promise) {
        const SEXP rho = dyntrace_get_promise_environment(promise);
        env_id_t env_id = lookup_environment(rho).get_id();
        return promise_mapper_.create(promise, env_id, get_current_timestamp());
    }

    PromiseState *insert_promise(const SEXP promise) {
        const SEXP rho = dyntrace_get_promise_environment(promise);
        env_id_t env_id = lookup_environment(rho).get_id();
        return promise_mapper_.insert(promise, env_id);
    }

    void remove_promise(const SEXP promise) { promise_mapper_.remove(promise); }

    PromiseState *lookup_promise(const SEXP promise) {
        return promise_mapper_.find(promise);
    }

    void clear_promises() { promise_mapper_.clear(); }

    void push_execution_context(const closure_info_t &info) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        stack_elem.function_info.function_id = info.fn_id;
        stack_elem.function_info.type = function_type::CLOSURE;
        full_stack.push_back(stack_elem);
    }

    void push_execution_context(const builtin_info_t &info) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        stack_elem.function_info.function_id = info.fn_id;
        stack_elem.function_info.type = info.fn_type;
        full_stack.push_back(stack_elem);
    }

    void push_execution_context(PromiseState *promise_state) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::PROMISE;
        stack_elem.promise_state = promise_state;
        full_stack.push_back(stack_elem);
    }

    void pop_execution_context(const closure_info_t &info) {
        auto exec_context = full_stack.back();
        if (exec_context.type != stack_type::CALL ||
            exec_context.call_id != info.call_id) {
            dyntrace_log_warning(
                "Object on stack was %s with id %d,"
                " but was expected to be closure with id %d",
                exec_context.type == stack_type::PROMISE ? "promise" : "call",
                exec_context.call_id, info.call_id);
        }
        full_stack.pop_back();
    }

    void pop_execution_context(const builtin_info_t &info) {
        auto thing_on_stack = full_stack.back();
        if (thing_on_stack.type != stack_type::CALL ||
            thing_on_stack.call_id != info.call_id) {
            dyntrace_log_warning(
                "Object on stack was %s with id %d,"
                " but was expected to be built-in with id %d",
                thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
                thing_on_stack.call_id, info.call_id);
        }
        full_stack.pop_back();
    }

    void pop_execution_context(PromiseState *promise_state) {
        auto exec_context = full_stack.back();
        if (exec_context.type != stack_type::PROMISE ||
            exec_context.promise_state != promise_state) {
            dyntrace_log_warning(
                "Object on stack was %s with id %d,"
                " but was expected to be promise with id %d",
                exec_context.type == stack_type::PROMISE ? "promise" : "call",
                exec_context.promise_state->get_id(), promise_state->get_id());
        }
        full_stack.pop_back();
    }


    CallState* create_call(const SEXP op) {
        call_id_t call_id = get_next_call_id_();
        fn_id = get_function_id(op);
        
        CallState* call_state = new CallState(call_id,
                                              fn_id,
                                              fn_type,
                                              name,
                                              formal_parameter_count,
                                              order);
        call_map_.insert({call_id, call_state});
        return call_state;
    }

  private:
    env_id_t create_next_environment_id_() { return environment_id_++; }

    var_id_t create_next_variable_id_() { return variable_id_++; }

    env_id_t environment_id_;
    var_id_t variable_id_;
    timestamp_t timestamp_;

    PromiseMapper promise_mapper_;

    std::unordered_map<call_id_t, CallState *> call_map_;

  public:
    void exit_probe() { resume_execution_timer(); }

    void enter_probe() {
        pause_execution_timer();
        increment_timestamp();
    }
};

inline std::string parameter_mode_to_string(parameter_mode_t parameter_mode) {
    switch (parameter_mode) {
        case parameter_mode_t::UNASSIGNED:
            return "Unassigned";
        case parameter_mode_t::MISSING:
            return "Missing";
        case parameter_mode_t::DEFAULT:
            return "Default";
        case parameter_mode_t::CUSTOM:
            return "Custom";
        case parameter_mode_t::NONPROMISE:
            return "Nonpromise";
    }
    return "Error";
}

#endif /* PROMISEDYNTRACER_STATE_H */


void update_closure_arguments(closure_info_t &info, dyntracer_t *dyntracer,
                              const call_id_t call_id, const SEXP formals,
                              const SEXP args, const SEXP environment) {

    int formal_parameter_position = 0;
    for (SEXP current_formal = formals;
         current_formal != R_NilValue;
         current_formal = CDR(current_formal),
             formal_parameter_position++) {
        // Retrieve the argument name.
        arg_name = TAG(current_formal);
        arg_value = dyntrace_lookup_environment(environment, arg_name);

        // Encountered a triple-dot argument, break it up further.
        if (TYPEOF(arg_value) == DOTSXP) {
            int dot_index = 0;
            for (SEXP dot_args = arg_value; dot_args != R_NilValue;
                 dot_args = CDR(dot_args)) {
                update_closure_argument(info, dyntracer, call_id, TAG(dot_args),
                                        CAR(dot_args), environment, true,
                                        MISSING(current_formal),
                                        formal_parameter_position);
            }
        } else {

            // We want the promise associated with the symbol.
            // Generally, the argument value should be the promise.
            // But if JIT is enabled, its possible for the argument_expression
            // to be unpromised. In this case, we dereference the argument.
            if (TYPEOF(arg_value) == SYMSXP) {
                lookup_result r =
                    find_binding_in_environment(arg_value, environment);
                if (r.status == lookup_status::SUCCESS) {
                    arg_value = r.value;
                } else {
                    // So... since this is a function, then I assume we
                    // shouldn't
                    // get any arguments that are active bindings or anything
                    // like
                    // that. If we do, then we should fail here and re-think our
                    // life choices.
                    std::string msg = lookup_status_to_string(r.status);
                    dyntrace_log_error("%s", msg.c_str());
                }
            }
            // The general case: single argument.
            update_closure_argument(
                info, dyntracer, call_id, arg_name, arg_value, environment,
                false, arg_value == R_MissingArg, formal_parameter_position);
        }
    }

    info.formal_parameter_count = formal_parameter_position;
}
