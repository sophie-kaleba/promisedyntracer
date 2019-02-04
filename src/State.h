#ifndef PROMISEDYNTRACER_STATE_H
#define PROMISEDYNTRACER_STATE_H

#include "sexptypes.h"
#include "stdlibs.h"
#include "Environment.h"
#include "Variable.h"
#include "PromiseMapper.h"


struct arg_t {
    arg_id_t id;
    std::string name;
    sexptype_t expression_type;
    sexptype_t name_type;
    prom_id_t promise_id; // only set if sexptype_t == PROM
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
        prom_id_t promise_id;
        call_id_t call_id;
        rid_t context_id;
    };
    SEXP environment;
    // Only initialized for type == CALL
    struct {
        fn_id_t function_id;
        function_type type;
    } function_info;
    std::uint64_t execution_time;
};

typedef std::map<std::string, std::string> metadata_t;

struct call_stack_elem_t {
    call_id_t call_id;
    fn_id_t function_id;
    function_type type;
    SEXP environment;
};

struct context_t {
    rid_t context;
    SEXP environment;
};

struct call_info_t {
    function_type fn_type;
    fn_id_t fn_id;
    SEXP fn_addr; // TODO unnecessary?
    std::string fn_definition;
    std::string definition_location;
    std::string callsite_location;
    bool fn_compiled;

    std::string name; // fully qualified function name, if available
    call_id_t call_id;
    SEXP call_ptr;
    call_id_t
        parent_call_id; // the id of the parent call that executed this call
    prom_id_t in_prom_id;

    stack_event_t parent_on_stack;
    sexptype_t return_value_type;
    std::string call_expression;
    int formal_parameter_count;
    int eval;
};

typedef std::vector<arg_t> arglist_t;

struct closure_info_t : call_info_t {
    arglist_t arguments;
};

struct builtin_info_t : call_info_t {};

// FIXME would it make sense to add type of action here?
struct prom_basic_info_t {
    prom_id_t prom_id;

    sexptype_t prom_type;
    full_sexp_type full_type;

    prom_id_t in_prom_id;
    stack_event_t parent_on_stack;
    int depth;
    std::string expression;
};

struct prom_info_t : prom_basic_info_t {
    call_id_t in_call_id;
    call_id_t from_call_id;
    sexptype_t return_type;
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

call_id_t make_funcall_id(dyntracer_t *dyntracer, SEXP);
std::string get_function_definition(dyntracer_t *dyntracer, const SEXP function);
void remove_function_definition(dyntracer_t *dyntracer, const SEXP function);
fn_id_t get_function_id(dyntracer_t *dyntracer, const std::string &def,
                        bool builtin = false);

// Returns false if function already existed, true if it was registered now
bool register_inserted_function(dyntracer_t *dyntracer, fn_id_t id);

bool function_already_inserted(fn_id_t id);
bool negative_promise_already_inserted(dyntracer_t *dyntracer, prom_id_t id);
template <typename T>
void get_stack_parent(T &info, std::vector<stack_event_t> &stack) {
    // put the body here
    static_assert(std::is_base_of<prom_basic_info_t, T>::value ||
                      std::is_base_of<prom_info_t, T>::value ||
                      std::is_base_of<call_info_t, T>::value,
                  "get_stack_parent is only applicable for arguments of types: "
                  "prom_basic_info_t,  prom_info_t, or call_info_t.");

    if (!stack.empty()) {
        stack_event_t stack_elem = stack.back();
        // parent type
        info.parent_on_stack.type = stack_elem.type;
        switch (info.parent_on_stack.type) {
            case stack_type::PROMISE:
                info.parent_on_stack.promise_id = stack_elem.call_id;
                break;
            case stack_type::CALL:
                info.parent_on_stack.call_id = stack_elem.promise_id;
                break;
            case stack_type::NONE:
                break;
        }
    } else {
        info.parent_on_stack.type = stack_type::NONE;
        info.parent_on_stack.call_id = 0;
    }
}

template <typename T>
void get_stack_parent2(T &info, std::vector<stack_event_t> &stack) {
    // put the body here
    static_assert(std::is_base_of<prom_basic_info_t, T>::value ||
                      std::is_base_of<prom_info_t, T>::value ||
                      std::is_base_of<call_info_t, T>::value,
                  "get_stack_parent is only applicable for arguments of types: "
                  "prom_basic_info_t,  prom_info_t, or call_info_t.");

    if (stack.size() > 1) {
        stack_event_t stack_elem = stack.rbegin()[1];
        info.parent_on_stack.type = stack_elem.type;
        switch (info.parent_on_stack.type) {
            case stack_type::PROMISE:
                info.parent_on_stack.promise_id = stack_elem.call_id;
                break;
            case stack_type::CALL:
                info.parent_on_stack.call_id = stack_elem.promise_id;
                break;
            case stack_type::NONE:
                break;
        }
    } else {
        info.parent_on_stack.type = stack_type::NONE;
        info.parent_on_stack.call_id = 0;
    }
}

stack_event_t get_last_on_stack_by_type(std::vector<stack_event_t> &stack,
                                        stack_type type);
stack_event_t get_from_back_of_stack_by_type(std::vector<stack_event_t> &stack,
                                             stack_type type, int rposition);

prom_id_t get_parent_promise(dyntracer_t *dyntracer);
arg_id_t get_argument_id(dyntracer_t *dyntracer, call_id_t call_id,
                         const std::string &argument);

void update_closure_arguments(closure_info_t &info, dyntracer_t *dyntracer,
                              const call_id_t call_id, const SEXP formals,
                              const SEXP args, const SEXP environment);

size_t get_no_of_ancestor_promises_on_stack(dyntracer_t *dyntracer);
size_t get_no_of_ancestors_on_stack();
size_t get_no_of_ancestor_calls_on_stack();


struct tracer_state_t {
    std::vector<stack_event_t> full_stack; // Should be reset on each tracer pass

    // Map from promise IDs to call IDs
    std::unordered_map<prom_id_t, call_id_t>
        promise_origin; // Should be reset on each tracer pass
    std::unordered_set<prom_id_t> fresh_promises;
    // Map from promise address to promise ID;
    std::unordered_map<SEXP, prom_id_t> promise_ids;
    std::unordered_map<prom_id_t, int> promise_lookup_gc_trigger_counter;
    call_id_t call_id_counter; // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t fn_id_counter;   // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t prom_id_counter; // IDs assigned should be globally unique but we
                               // can reset it after each pass if overwrite is
                               // true)
    prom_id_t prom_neg_id_counter;

    std::unordered_map<SEXP, std::string> function_definitions;

    std::unordered_map<fn_key_t, fn_id_t> function_ids; // Should be kept across Rdt
                                                   // calls (unless overwrite is
                                                   // true)
    std::unordered_set<fn_id_t> already_inserted_functions; // Should be kept across
                                                       // Rdt calls (unless
                                                       // overwrite is true)
    std::unordered_set<prom_id_t>
        already_inserted_negative_promises; // Should be kept
                                            // across Rdt
                                            // calls (unless
                                            // overwrite is
                                            // true)
    arg_id_t argument_id_sequence; // Should be globally unique (can reset
                                   // between tracer calls if overwrite is true)
    std::map<arg_key_t, arg_id_t> argument_ids; // Should be kept across Rdt calls
                                           // (unless overwrite is true)
    int gc_trigger_counter; // Incremented each time there is a gc_entry

    void finish_pass() {

        promise_origin.clear();

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

    // env_id_t to_environment_id(SEXP rho);
    // var_id_t to_variable_id(SEXP symbol, SEXP rho, bool &exists);
    // var_id_t to_variable_id(const std::string &symbol, SEXP rho, bool &exists);
    prom_id_t enclosing_promise_id();

    void increment_gc_trigger_counter();

    int get_gc_trigger_counter() const;

    tracer_state_t();

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

    void create_promise(const prom_id_t prom_id, const env_id_t env_id) {
        promise_mapper_.create(prom_id, env_id, get_current_timestamp());
    }

    void insert_promise(const prom_id_t& promise_id, const env_id_t env_id) {
        promise_mapper_.insert(promise_id, env_id);
    }

    void remove_promise(const prom_id_t prom_id) {
        promise_mapper_.remove(prom_id);
    }

    PromiseState& lookup_promise(const prom_id_t &promise_id) {
        return promise_mapper_.find(promise_id);
    }

    void clear_promises() {
        promise_mapper_.clear();
    }

    void push_execution_context(const closure_info_t &info) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        stack_elem.function_info.function_id = info.fn_id;
        stack_elem.function_info.type = function_type::CLOSURE;
        stack_elem.environment = info.call_ptr;
        full_stack.push_back(stack_elem);
    }

    void push_execution_context(const builtin_info_t &info) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        stack_elem.function_info.function_id = info.fn_id;
        stack_elem.function_info.type = info.fn_type;
        stack_elem.environment = info.call_ptr;
        full_stack.push_back(stack_elem);
    }

    void push_execution_context(const prom_info_t& info, const SEXP env) {
        stack_event_t stack_elem;
        stack_elem.type = stack_type::PROMISE;
        stack_elem.promise_id = info.prom_id;
        stack_elem.environment = env;
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

    void pop_execution_context(const prom_info_t& info) {
        auto thing_on_stack = full_stack.back();
        if (thing_on_stack.type != stack_type::PROMISE ||
            thing_on_stack.promise_id != info.prom_id) {
            dyntrace_log_warning(
                "Object on stack was %s with id %d,"
                " but was expected to be promise with id %d",
                thing_on_stack.type == stack_type::PROMISE ? "promise" : "call",
                thing_on_stack.promise_id, info.prom_id);
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
};


prom_id_t get_promise_id(tracer_state_t & state, SEXP promise);
prom_id_t make_promise_id(tracer_state_t & state, SEXP promise,
                          bool negative = false);

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
