#ifndef PROMISEDYNTRACER_STATE_H
#define PROMISEDYNTRACER_STATE_H

#include "sexptypes.h"
#include "stdlibs.h"
#include "Environment.h"
#include "Variable.h"
#include "CallState.h"
#include "Function.h"
#include "ExecutionContextStack.h"
#include <unordered_map>


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

private:
    ExecutionContextStack stack_;

public:

    ExecutionContextStack& get_stack() {
        return stack_;
    }

    std::unordered_map<promise_id_t, int> promise_lookup_gc_trigger_counter;

    int gc_trigger_counter; // Incremented each time there is a gc_entry

    tracer_state_t(const std::string &output_dirpath, bool truncate,
                   bool binary, int compression_level) :
        output_dirpath_(output_dirpath), truncate_(truncate), binary_(binary),
        compression_level_(compression_level) {

        gc_trigger_counter = 0;
        environment_id_ = 0;
        variable_id_ = 0;
        timestamp_ = 0;
        call_id_counter_ = 0;
        promise_id_counter_ = 0;

        call_summary_data_table_ =
            create_data_table(output_dirpath_ + "/" + "call-summary",
                              {"function_id",
                               "function_type",
                               "is_byte_compiled",
                               "formal_parameter_count",
                               "function_name",
                               "force_order",
                               "return_value_type",
                               "call_count"},
                              truncate_, binary_, compression_level_);
    }

    void increment_gc_trigger_counter() {
        gc_trigger_counter++;
    }

    int get_gc_trigger_counter() const {
        return gc_trigger_counter;
    }

    void finish_pass() {

        promises_.clear();

        for(auto iter = functions_.begin(); iter != functions_.end(); ++iter) {
            serialize_function_(iter -> second);
            delete iter -> second;
        }

        functions_.clear();

        delete call_summary_data_table_;
        // TODO - check if stack only has top level element
        // if (!full_stack.empty()) {
        //     dyntrace_log_warning(
        //         "Function/promise stack is not balanced: %d remaining",
        //         full_stack.size());
        //     full_stack.clear();
        // }
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
                              get_current_timestamp_());
        }

        return env.lookup(symbol);
    }

    Variable& define_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true) {
        return lookup_environment(rho, true).define(symbol_to_string(symbol),
                                                    create_next_variable_id_(),
                                                    get_current_timestamp_());
    }

    Variable& update_variable(const SEXP rho, const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Variable& var = lookup_variable(rho, symbol);
        var.set_modification_timestamp(get_current_timestamp_());
        return var;
    }

    Variable remove_variable(const SEXP rho, const SEXP symbol,
                             bool create_environment = true) {
        return lookup_environment(rho, create_environment)
            .remove(symbol_to_string(symbol));
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
    std::unordered_map<SEXP, Environment> environment_mapping_;

public:
    void resume_execution_timer() {
       execution_resume_time_ = std::chrono::high_resolution_clock::now();
    }

    void pause_execution_timer() {
       auto execution_pause_time = std::chrono::high_resolution_clock::now();
       std::uint64_t execution_time =
           std::chrono::duration_cast<std::chrono::nanoseconds>(execution_pause_time -
                                                                execution_resume_time_).count();
       for (auto &element : stack_) {
           if(element.is_promise()) {
               element.get_promise() -> add_execution_time(execution_time);
           }
       }
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> execution_resume_time_;


    /***************************************************************************
     * PROMISE
     **************************************************************************/

public:
    PromiseState* create_promise(const SEXP promise) {

        PromiseState* promise_state(create_raw_promise_(promise, true));
        auto result = promises_.insert_or_assign(promise, promise_state);
        promise_state -> set_creation_timestamp(get_current_timestamp_());
        return promise_state;
    }

    PromiseState* lookup_promise(const SEXP promise,
                                 bool create = false,
                                 bool local = false) {
        auto iter = promises_.find(promise);

        /* all promises encountered are added to the map. Its not possible for
           a promise id to be encountered which is not already mapped.
           If this happens, possibly, the mapper methods are not the first to
           be called in the analysis. Hence, they are not able to update the mapping. */
        if (iter == promises_.end()) {
            if(create) {
                PromiseState* promise_state(create_raw_promise_(promise, local));
                promises_.insert({promise, promise_state});
                return promise_state;
            } else {
                return nullptr;
            }
        }

        return iter->second;
    }

    void destroy_promise(const SEXP promise, PromiseState* promise_state) {

        /* here we delete a promise iff we are the only one holding a
           reference to it. A promise can be simultaneously held by
           a call and this promise map. While it is held by the promise
           map, the active flag is set and while it is held by the call
           the argument flag is set. So, to decide if we have to delete
           the promise at this point, we first unset the active flag
           (because we are interesting in removing the promise) and then,
           we check the argument flag. If argument flag is unset, it means
           the promise is not held by a call and can be deleted. If the
           argument flag is set, it means the promise is held by a call
           and when that call gets deleted, it will delete this promise */
        promise_state -> set_inactive();
        promises_.erase(promise);
        if (!promise_state -> is_argument()) {
            delete promise_state;
        }
    }

private:
    promise_id_t get_next_promise_id_() {
        return promise_id_counter_++;
    }

    PromiseState * create_raw_promise_(const SEXP promise, bool local) {
        const SEXP rho = dyntrace_get_promise_environment(promise);
        env_id_t env_id = lookup_environment(rho).get_id();
        PromiseState* promise_state = new PromiseState(promise, local);
        /* Setting this bit tells us that the promise is currently in the promises
           table. As long as this is set, the call holding a reference to it will not
           garbage collect it. */
        promise_state -> set_active();
        return promise_state;
    }

    std::unordered_map<SEXP, PromiseState*> promises_;
    promise_id_t promise_id_counter_;

private:
    timestamp_t get_current_timestamp_() const {
        return timestamp_;
    }

    timestamp_t increment_timestamp_() {
        return timestamp_++;
    }

    timestamp_t timestamp_;

public:
    void exit_probe() {
        resume_execution_timer();
    }

    void enter_probe() {
        pause_execution_timer();
        increment_timestamp_();
    }

public:
    CallState *create_call(const SEXP call, const SEXP op, const SEXP args,
                           const SEXP rho) {

        Function* function = lookup_function(op);
        CallState *call_state = nullptr;
        call_id_t call_id = get_next_call_id_();
        const function_id_t& function_id = function -> get_id();
        const std::string function_name = get_name(call);
        int eval = 0;

        call_state = call_state = new CallState(call_id, function_id, TYPEOF(op),
                                                function_name, 0, 0, rho, function);
        if(TYPEOF(op) == CLOSXP) {
            process_closure_arguments_(call_state, op);
        } else {
            int eval = dyntrace_get_c_function_argument_evaluation(op);
            call_state -> set_force_order(std::to_string(eval));
        }

        // TODO - use PRIMARITY(op) to compute arity of SPECIAL and BUILTIN;

        return call_state;
    }

    void destroy_call(CallState* call_state) {
        Function* function = call_state -> get_function();
        function -> add_summary(call_state);
        delete call_state;
    }

    void remove_closure(const SEXP op) {
        auto it = functions_.find(op);

        if (it != functions_.end()) {
            serialize_function_(it->second);
            delete it->second;
            functions_.erase(it);
        }
    }

  private:

    call_id_t get_next_call_id_() {
        return ++call_id_counter_;
    }

    void process_closure_argument_(CallState *call_state,
                                   int formal_parameter_position,
                                   int actual_argument_position,
                                   const SEXP name, const SEXP argument) {
        PromiseState *promise_state = nullptr;
        /* only add to promise map if the argument is a promise */
        if (type_of_sexp(argument) == PROMSXP) {
           promise_state = lookup_promise(argument, true);
        }
        else {
            promise_state = new PromiseState(argument, false);
        }

        promise_state -> make_argument(call_state, formal_parameter_position,
                                       actual_argument_position);

        call_state -> add_argument(promise_state);
    }

    void process_closure_arguments_(CallState* call_state,
                                    const SEXP op) {
        SEXP formal = nullptr;
        SEXP name = nullptr;
        SEXP argument = nullptr;
        SEXP dot_dot_argument = nullptr;
        SEXP rho = call_state -> get_environment();
        int formal_parameter_position = -1;
        int actual_argument_position = -1;

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
                    ++actual_argument_position;

                    name = TAG(dot_dot_arguments);

                    dot_dot_argument = CAR(dot_dot_arguments);

                    process_closure_argument_(call_state,
                                              formal_parameter_position,
                                              actual_argument_position,
                                              name,
                                              dot_dot_argument);
                }
                break;

            default:
                ++actual_argument_position;
                process_closure_argument_(call_state,
                                          formal_parameter_position,
                                          actual_argument_position,
                                          name,
                                          argument);
                break;
            }
        }

        call_state -> set_formal_parameter_count(formal_parameter_position + 1);
    }

    Function* lookup_function(const SEXP op) {
        auto iter = functions_.find(op);
        if(iter == functions_.end()) {
            Function* function = new Function(op);
            functions_.insert({op, function});
            return function;
        }
        return iter -> second;
    }

    void serialize_function_(Function *function) {
        function->serialize_call_summary(call_summary_data_table_);
        function->serialize_definition(output_dirpath_);
    }

private :
    std::unordered_map<SEXP, Function*> functions_;
    call_id_t call_id_counter_;
    DataTableStream *call_summary_data_table_;
    std::string output_dirpath_;
    bool truncate_;
    bool binary_;
    int compression_level_;
};


#endif /* PROMISEDYNTRACER_STATE_H */
