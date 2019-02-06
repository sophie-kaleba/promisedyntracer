#ifndef PROMISEDYNTRACER_STATE_H
#define PROMISEDYNTRACER_STATE_H

#include "sexptypes.h"
#include "stdlibs.h"
#include "Environment.h"
#include "Variable.h"
#include "PromiseMapper.h"
#include "CallState.h"


enum class function_type {
    CLOSURE = 0,
    BUILTIN = 1,
    SPECIAL = 2,
    TRUE_BUILTIN = 3
};

// TODO - remove this and use sexptype_t instead
enum class stack_type { PROMISE = 1, CALL = 2, CONTEXT = 3, NONE = 0 };

struct stack_event_t {

    stack_event_t() : execution_time(0) {}

    stack_type type;
    std::uint64_t execution_time;
    union {
        PromiseState * promise_state;
        CallState* call_state;
        rid_t context_id;
    };

};

struct context_t {
    rid_t context;
    SEXP environment;
};


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



struct tracer_state_t {
    std::vector<stack_event_t> full_stack; // Should be reset on each tracer pass

    std::unordered_map<promise_id_t, int> promise_lookup_gc_trigger_counter;

    int gc_trigger_counter; // Incremented each time there is a gc_entry

    tracer_state_t() {
        gc_trigger_counter = 0;
        environment_id_ = 0;
        variable_id_ = 0;
        timestamp_ = 0;
        call_id_counter_ = 0;
    }

    void increment_gc_trigger_counter() {
        gc_trigger_counter++;
    }

    int get_gc_trigger_counter() const {
        return gc_trigger_counter;
    }

    void finish_pass() {

        clear_promises();

        if (!full_stack.empty()) {
            dyntrace_log_warning(
                "Function/promise stack is not balanced: %d remaining",
                full_stack.size());
            full_stack.clear();
        }
    }

    Environment& create_environment(const SEXP rho) {
        auto iter = environment_mapping_.find(rho);
        if(iter != environment_mapping_.end()) {
            return iter -> second;
        }
        return environment_mapping_.insert({rho, Environment(rho,
                                                             create_next_environment_id_())}).first -> second;
    }

    void remove_environment(const SEXP rho) {
        environment_mapping_.erase(rho);
    }

    Environment& lookup_environment(const SEXP rho, bool create = true) {
        auto iter = environment_mapping_.find(rho);
        if(iter == environment_mapping_.end()) {
            return create_environment(rho);
        }
        return iter->second;
    }

    Variable& lookup_variable(const SEXP rho,
                              const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        return lookup_variable(rho, symbol_to_string(symbol),
                               create_environment,
                               create_variable);
    }

    Variable& lookup_variable(const SEXP rho,
                              const std::string& symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Environment& env = lookup_environment(rho, create_environment);

        bool var_exists = env.exists(symbol);

        if (create_variable && !var_exists) {
            return env.define(symbol,
                              create_next_variable_id_(),
                              get_current_timestamp());
        }

        return env.lookup(symbol);
    }

    Variable& define_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true) {
        return lookup_environment(rho, true).define(symbol_to_string(symbol),
                                                    create_next_variable_id_(),
                                                    get_current_timestamp());
    }

    Variable& update_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Variable& var = lookup_variable(rho, symbol);
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
           std::chrono::duration_cast<std::chrono::nanoseconds>(execution_pause_time -
                                                                execution_resume_time_).count();
       for (auto &element : full_stack) {
           element.execution_time += execution_time;
       }
    }

    std::chrono::time_point<std::chrono::high_resolution_clock> execution_resume_time_;

    std::unordered_map<SEXP, Environment> environment_mapping_;

    timestamp_t get_current_timestamp() const {
        return timestamp_;
    }

    timestamp_t increment_timestamp() {
        return timestamp_++;
    }

    PromiseState* create_promise(const SEXP promise) {
        const SEXP rho = dyntrace_get_promise_environment(promise);
        env_id_t env_id = lookup_environment(rho).get_id();
        return promise_mapper_.create(promise, env_id, get_current_timestamp());
    }

    PromiseState* insert_promise(const SEXP promise) {
        const SEXP rho = dyntrace_get_promise_environment(promise);
        env_id_t env_id = lookup_environment(rho).get_id();
        return promise_mapper_.insert(promise, env_id);
    }

    void remove_promise(const SEXP promise) {
        promise_mapper_.remove(promise);
    }

    PromiseState* lookup_promise(const SEXP promise) {
        return promise_mapper_.find(promise);
    }

    void clear_promises() {
        promise_mapper_.clear();
    }

    void push_execution_context(PromiseState * promise_state) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::PROMISE;
        stack_elem.promise_state = promise_state;
        full_stack.push_back(stack_elem);
    }

    void pop_execution_context(PromiseState *promise_state) {
        auto exec_context = full_stack.back();
        if (exec_context.type != stack_type::PROMISE ||
            exec_context.promise_state != promise_state) {
            dyntrace_log_warning(
                "Object on stack was %s with id %d,"
                " but was expected to be promise with id %d",
                exec_context.type == stack_type::PROMISE ? "promise" : "call",
                exec_context.promise_state -> get_id(), promise_state -> get_id());
        }
        full_stack.pop_back();
    }

  private:
    env_id_t create_next_environment_id_() {
        return environment_id_++;
    }

    var_id_t create_next_variable_id_() {
        return variable_id_++;
    }

    env_id_t environment_id_;
    var_id_t variable_id_;
    timestamp_t timestamp_;

    PromiseMapper promise_mapper_;

public:
    void exit_probe() {
        resume_execution_timer();
    }

    void enter_probe() {
        pause_execution_timer();
        increment_timestamp();
    }

public:
    CallState *create_call(const SEXP call, const SEXP op, const SEXP args,
                           const SEXP rho) {

        CallState *call_state = nullptr;
        call_id_t call_id = get_next_call_id_();
        function_id_t function_id = get_function_id_(op);
        const std::string function_name = get_function_name_(call, op);
        bool byte_compiled = is_function_byte_compiled_(op);
        int eval = 0;

        switch(TYPEOF(op)) {

        case CLOSXP:
            call_state = new CallState(call_id, function_id,
                                       "Closure", function_name,
                                       0, "", rho, byte_compiled);
            process_closure_arguments_(call_state, op);
            break;

        case SPECIALSXP:
            eval = (R_FunTab[(op)->u.primsxp.offset].eval) % 10;
            call_state = new CallState(call_id, function_id,
                                       "Special", function_name,
                                       PRIMARITY(op),
                                       std::to_string(eval),
                                       rho, byte_compiled);
            break;

        case BUILTINSXP:
            eval = (R_FunTab[(op)->u.primsxp.offset].eval) % 10;
            call_state = new CallState(call_id, function_id,
                                       "Builtin", function_name,
                                       PRIMARITY(op),
                                       std::to_string(eval),
                                       rho, byte_compiled);
            break;

        default:
            dyntrace_log_error("expected a function type but got a %s instead.",
                               sexptype_to_string(type_of_sexp(op)).c_str());
        }

        call_map_.insert({call_id, call_state});

        return call_state;
    }

    void remove_call(CallState* call_state) {
        // TODO this is the place to check reference counted Promise objects.
        delete call_state;
    }

    CallState* get_call_state(call_id_t call_id) {
        auto iter = call_map_.find(call_id);
        if(iter == call_map_.end()) {
            dyntrace_log_error("Call with id %d does not exist in the call mapping",
                               call_id);
        }
        return iter -> second;
    }

    void push_execution_context(CallState * call_state) {
        stack_event_t element;
        element.call_state = call_state;
        element.type = stack_type::CALL;
        element.execution_time = 0;
        full_stack.push_back(element);
    }

    CallState* pop_execution_context(SEXP return_value) {

        // TODO pass call, args, rho etc. here to make a check if we are
        // removing a call element of the right type.
        stack_event_t element = full_stack.back();
        full_stack.pop_back();
        if(element.type != stack_type::CALL) {
            dyntrace_log_error("Expected stack top to contain a call object.")
        }

        element.call_state->set_return_value_type(type_of_sexp(return_value));
        element.call_state->make_inactive();
        element.call_state->set_execution_time(element.execution_time);

        return element.call_state;
    }

    void remove_closure(const SEXP op) {
        auto it = function_definitions_.find(op);

        if (it != function_definitions_.end()) {
            function_definitions_.erase(it);
        }
    }

  private:
    bool is_function_byte_compiled_(SEXP op) {
        return TYPEOF(BODY(op)) == BCODESXP;
    }

    std::string get_function_name_(const SEXP call, const SEXP op) {
        const char *name = get_name(call);
        const char *ns = get_ns_name(op);
        if (name == NULL)
            name = "";
        if (ns) {
            return std::string(ns) + "::" + name;
        } else {
            return name;
        }
    }

    std::string get_function_definition_(const SEXP op) {
        auto it = function_definitions_.find(op);
        if (it != function_definitions_.end()) {
#ifdef RDT_DEBUG
            string test = get_expression(function);
            if (it->second.compare(test) != 0) {
                cout << "Function definitions are wrong.";
            }
#endif
            return it->second;
        } else {
            std::string definition = get_expression(op);
            function_definitions_.insert({op, definition});
            return definition;
        }
    }

    function_id_t get_function_id_(const SEXP op) {
        const std::string &definition = get_function_definition_(op);

        auto it = function_ids_.find(definition);

        if (it != function_ids_.end()) {
            return it->second;
        } else {
            /* Hash the function body to compute a unique id */
            function_id_t fn_id = compute_hash(definition.c_str());
            function_ids_.insert({definition, fn_id});
            return fn_id;
        }
    }

    call_id_t get_next_call_id_() {
        return ++call_id_counter_;
    }



    void process_closure_argument_(CallState* call_state,
                                   int formal_parameter_position,
                                   const SEXP name,
                                   const SEXP argument) {
        parameter_mode_t parameter_mode = parameter_mode_t ::NONPROMISE;
        sexptype_t argument_type = type_of_sexp(argument);

        // TODO - consider removing this enum.
        if(argument_type == MISSINGSXP) {
            parameter_mode = parameter_mode_t::MISSING;
            call_state -> set_value_type(MISSINGSXP,
                                         formal_parameter_position);
        }
        else if(argument_type == PROMSXP) {
            const SEXP promise_environment = dyntrace_get_promise_environment(argument);
            const SEXP call_environment = call_state -> get_environment();
            parameter_mode = promise_environment == call_environment
                ? parameter_mode_t::DEFAULT
                : parameter_mode_t::CUSTOM;

            // first we insert the promise
            PromiseState *promise_state = insert_promise(argument);

            // now, we make this promise object, a function argument by
            // providing the relevant information.

            // TODO - once we merge parameters and arguments, we may want to create
            // this object elsewhere
            promise_state->make_function_argument(call_state -> get_function_id(),
                                                  call_state -> get_call_id(),
                                                  formal_parameter_position,
                                                  parameter_mode);

        }

        call_state -> set_expression_type(argument_type,
                                          formal_parameter_position);

        // TODO make this api consistent, either formal parameter position should
        // be first argument, or it should be last argument
        call_state -> set_parameter_mode(formal_parameter_position,
                                         parameter_mode);
    }

    void process_closure_arguments_(CallState* call_state,
                                    const SEXP op) {
        SEXP formal = nullptr;
        SEXP name = nullptr;
        SEXP argument = nullptr;
        SEXP dot_dot_argument = nullptr;
        SEXP rho = call_state -> get_environment();
        int formal_parameter_position = 0;

        for (formal = FORMALS(op); formal != R_NilValue;
             formal = CDR(formal)) {

            ++formal_parameter_position;

            /* get argument name */
            name = TAG(formal);
            /* lookup argument in environment by name */
            argument = dyntrace_lookup_environment(rho, name);

            switch (TYPEOF(argument)) {
            case DOTSXP:
                for (SEXP dot_dot_arguments = argument;
                     dot_dot_arguments != R_NilValue;
                     dot_dot_arguments = CDR(dot_dot_arguments)) {

                    name = TAG(dot_dot_arguments);

                    dot_dot_argument = CAR(dot_dot_arguments);

                    process_closure_argument_(call_state,
                                              formal_parameter_position,
                                              name,
                                              dot_dot_argument);
                }
                break;

            default:
                process_closure_argument_(call_state,
                                          formal_parameter_position,
                                          name,
                                          argument);
                break;
            }
        }

        call_state -> set_formal_parameter_count(formal_parameter_position);
    }

private :
    std::unordered_map<SEXP, std::string> function_definitions_;
    std::unordered_map<std::string, function_id_t> function_ids_;
    call_id_t call_id_counter_;
    std::unordered_map<call_id_t, CallState *> call_map_;
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
