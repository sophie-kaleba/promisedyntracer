#ifndef PROMISEDYNTRACER_TRACER_STATE_H
#define PROMISEDYNTRACER_TRACER_STATE_H

#include "Argument.h"
#include "Call.h"
#include "Environment.h"
#include "Event.h"
#include "ExecutionContextStack.h"
#include "Function.h"
#include "Variable.h"
#include "sexptypes.h"
#include "stdlibs.h"

#include <unordered_map>

class TracerState {
  private:
    const std::string output_dirpath_;
    const bool verbose_;
    const bool truncate_;
    const bool binary_;
    const int compression_level_;

  public:
    TracerState(const std::string& output_dirpath,
                bool verbose,
                bool truncate,
                bool binary,
                int compression_level)
        : output_dirpath_(output_dirpath)
        , verbose_(verbose)
        , truncate_(truncate)
        , binary_(binary)
        , compression_level_(compression_level)
        , denoted_value_id_counter_(0)
        , environment_id_(0)
        , variable_id_(0)
        , timestamp_(0)
        , call_id_counter_(0)
        , object_count_(OBJECT_TYPE_TABLE_COUNT, 0)
        , event_counter_(to_underlying(Event::COUNT), 0) {
        event_counts_data_table_ =
            create_data_table(output_dirpath_ + "/" + "event_counts",
                              {"event", "count"},
                              truncate_,
                              binary_,
                              compression_level_);

        object_counts_data_table_ =
            create_data_table(output_dirpath_ + "/" + "object_counts",
                              {"type", "count"},
                              truncate_,
                              binary_,
                              compression_level_);

        call_summaries_data_table_ =
            create_data_table(output_dirpath_ + "/" + "call_summaries",
                              {"function_id",
                               "package",
                               "function_name",
                               "function_type",
                               "formal_parameter_count",
                               "wrapper",
                               "S3_method",
                               "S4_method",
                               "force_order",
                               "missing_arguments",
                               "return_value_type",
                               "jumped",
                               "call_count"},
                              truncate_,
                              binary_,
                              compression_level_);

        function_definitions_data_table_ =
            create_data_table(output_dirpath_ + "/" + "function_definitions",
                              {"function_id",
                               "package",
                               "function_name",
                               "formal_parameter_count",
                               "byte_compiled",
                               "definition"},
                              truncate_,
                              binary_,
                              compression_level_);

        arguments_data_table_ =
            create_data_table(output_dirpath_ + "/" + "arguments",
                              {"call_id",
                               "function_id",
                               "value_id",
                               "formal_parameter_position",
                               "actual_argument_position",
                               "argument_type",
                               "expression_type",
                               "value_type",
                               "default",
                               "dot_dot_dot",
                               "preforce",
                               "direct_force",
                               "direct_lookup_count",
                               "direct_metaprogram_count",
                               "indirect_force",
                               "indirect_lookup_count",
                               "indirect_metaprogram_count",
                               "S3_dispatch",
                               "S4_dispatch",
                               "forcing_actual_argument_position",
                               "non_local_return",
                               "execution_time",
                               "expression"},
                              truncate_,
                              binary_,
                              compression_level_);

        side_effects_data_table_ =
            create_data_table(output_dirpath_ + "/" + "side_effects",
                              {"value_id",
                               "call_id",
                               "function_id",
                               "package",
                               "function_name",
                               "formal_parameter_position",
                               "actual_argument_position",
                               "dot_dot_dot",
                               "direct_self_scope_mutation_count",
                               "indirect_self_scope_mutation_count",
                               "direct_lexical_scope_mutation_count",
                               "indirect_lexical_scope_mutation_count",
                               "direct_non_lexical_scope_mutation_count",
                               "indirect_non_lexical_scope_mutation_count",
                               "direct_self_scope_observation_count",
                               "indirect_self_scope_observation_count",
                               "direct_lexical_scope_observation_count",
                               "indirect_lexical_scope_observation_count",
                               "direct_non_lexical_scope_observation_count",
                               "indirect_non_lexical_scope_observation_count",
                               "expression"},
                              truncate_,
                              binary_,
                              compression_level_);

        escaped_arguments_data_table_ = create_data_table(
            output_dirpath_ + "/" + "escaped_arguments",
            {"call_id",
             "function_id",
             "return_value_type",
             "formal_parameter_count",
             "formal_parameter_position",
             "actual_argument_position",
             "value_id",
             "class",
             "S3_dispatch",
             "S4_dispatch",
             "argument_type",
             "expression_type",
             "value_type",
             "default",
             "non_local_return",
             "escape",
             "call_depth",
             "promise_depth",
             "nested_promise_depth",
             "forcing_actual_argument_position",
             "preforce",
             "before_escape_force_count",
             "before_escape_metaprogram_count",
             "before_escape_value_lookup_count",
             "before_escape_value_assign_count",
             "before_escape_expression_lookup_count",
             "before_escape_expression_assign_count",
             "before_escape_environment_lookup_count",
             "before_escape_environment_assign_count",
             "after_escape_force_count",
             "after_escape_metaprogram_count",
             "after_escape_value_lookup_count",
             "after_escape_value_assign_count",
             "after_escape_expression_lookup_count",
             "after_escape_expression_assign_count",
             "after_escape_environment_lookup_count",
             "after_escape_environment_assign_count",
             "before_escape_direct_self_scope_mutation_count",
             "before_escape_indirect_self_scope_mutation_count",
             "before_escape_direct_lexical_scope_mutation_count",
             "before_escape_indirect_lexical_scope_mutation_count",
             "before_escape_direct_non_lexical_scope_mutation_count",
             "before_escape_indirect_non_lexical_scope_mutation_count",
             "before_escape_direct_self_scope_observation_count",
             "before_escape_indirect_self_scope_observation_count",
             "before_escape_direct_lexical_scope_observation_count",
             "before_escape_indirect_lexical_scope_observation_count",
             "before_escape_direct_non_lexical_scope_observation_count",
             "before_escape_indirect_non_lexical_scope_observation_count",
             "after_escape_direct_self_scope_mutation_count",
             "after_escape_indirect_self_scope_mutation_count",
             "after_escape_direct_lexical_scope_mutation_count",
             "after_escape_indirect_lexical_scope_mutation_count",
             "after_escape_direct_non_lexical_scope_mutation_count",
             "after_escape_indirect_non_lexical_scope_mutation_count",
             "after_escape_direct_self_scope_observation_count",
             "after_escape_indirect_self_scope_observation_count",
             "after_escape_direct_lexical_scope_observation_count",
             "after_escape_indirect_lexical_scope_observation_count",
             "after_escape_direct_non_lexical_scope_observation_count",
             "after_escape_indirect_non_lexical_scope_observation_count",
             "execution_time"},
            truncate_,
            binary_,
            compression_level_);

        promises_data_table_ =
            create_data_table(output_dirpath_ + "/" + "promises",
                              {"value_id",
                               "argument",
                               "expression_type",
                               "value_type",
                               "creation_scope",
                               "forcing_scope",
                               "S3_dispatch",
                               "S4_dispatch",
                               "preforce",
                               "force_count",
                               "call_depth",
                               "promise_depth",
                               "nested_promise_depth",
                               "metaprogram_count",
                               "value_lookup_count",
                               "value_assign_count",
                               "expression_lookup_count",
                               "expression_assign_count",
                               "environment_lookup_count",
                               "environment_assign_count",
                               "execution_time"},
                              truncate_,
                              binary_,
                              compression_level_);

        promise_lifecycles_data_table_ =
            create_data_table(output_dirpath_ + "/" + "promise_lifecycles",
                              {"action", "count", "promise_count"},
                              truncate_,
                              binary_,
                              compression_level_);
    }

    ~TracerState() {
        delete event_counts_data_table_;
        delete object_counts_data_table_;
        delete call_summaries_data_table_;
        delete function_definitions_data_table_;
        delete arguments_data_table_;
        delete side_effects_data_table_;
        delete promises_data_table_;
        delete escaped_arguments_data_table_;
        delete promise_lifecycles_data_table_;
    }

    const std::string& get_output_dirpath() const {
        return output_dirpath_;
    }

    bool get_truncate() const {
        return truncate_;
    }

    bool is_verbose() const {
        return verbose_;
    }

    bool is_binary() const {
        return binary_;
    }

    int get_compression_level() const {
        return compression_level_;
    }

    void initialize() const {
        serialize_configuration_();
    }

    void cleanup(int error) {
        for (auto const& binding: promises_) {
            destroy_promise(binding.second);
        }

        promises_.clear();

        for (auto const& binding: function_cache_) {
            destroy_function_(binding.second);
        }

        functions_.clear();

        function_cache_.clear();

        serialize_event_counts_();

        serialize_object_count_();

        serialize_promise_lifecycle_summary_();

        if (!get_stack_().is_empty()) {
            dyntrace_log_error("stack not empty on tracer exit.")
        }

        if (error) {
            std::ofstream error_file(get_output_dirpath() + "/ERROR");
            error_file << "ERROR";
            error_file.close();
        } else {
            std::ofstream noerror_file(get_output_dirpath() + "/NOERROR");
            noerror_file << "NOERROR";
            noerror_file.close();
        }
    }

    void increment_object_count(sexptype_t type) {
        ++object_count_[type];
    }

  private:
    DataTableStream* event_counts_data_table_;
    DataTableStream* object_counts_data_table_;
    DataTableStream* promises_data_table_;
    DataTableStream* promise_lifecycles_data_table_;

    void serialize_configuration_() const {
        std::ofstream fout(get_output_dirpath() + "/CONFIGURATION",
                           std::ios::trunc);

        auto serialize_row = [&fout](const std::string& key,
                                     const std::string& value) {
            fout << key << "=" << value << std::endl;
        };

        for (const std::string& envvar: ENVIRONMENT_VARIABLES) {
            serialize_row(envvar, to_string(getenv(envvar.c_str())));
        }

        serialize_row("GIT_COMMIT_INFO", GIT_COMMIT_INFO);
        serialize_row("truncate", std::to_string(get_truncate()));
        serialize_row("verbose", std::to_string(is_verbose()));
        serialize_row("binary", std::to_string(is_binary()));
        serialize_row("compression_level",
                      std::to_string(get_compression_level()));
    }

    void serialize_event_counts_() {
        for (int i = 0; i < to_underlying(Event::COUNT); ++i) {
            event_counts_data_table_->write_row(
                to_string(static_cast<Event>(i)),
                static_cast<double>(event_counter_[i]));
        }
    }

    void serialize_object_count_() {
        for (int i = 0; i < object_count_.size(); ++i) {
            if (object_count_[i] != 0) {
                object_counts_data_table_->write_row(
                    sexptype_to_string(i),
                    static_cast<double>(object_count_[i]));
            }
        }
    }

    ExecutionContextStack stack_;

  public:
    ExecutionContextStack& get_stack_() {
        return stack_;
    }

    Environment& create_environment(const SEXP rho) {
        auto iter = environment_mapping_.find(rho);
        if (iter != environment_mapping_.end()) {
            return iter->second;
        }
        return environment_mapping_
            .insert({rho, Environment(rho, create_next_environment_id_())})
            .first->second;
    }

    void remove_environment(const SEXP rho) {
        environment_mapping_.erase(rho);
    }

    Environment& lookup_environment(const SEXP rho, bool create = true) {
        auto iter = environment_mapping_.find(rho);
        if (iter == environment_mapping_.end()) {
            return create_environment(rho);
        }
        return iter->second;
    }

    Variable& lookup_variable(const SEXP rho,
                              const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        return lookup_variable(
            rho, symbol_to_string(symbol), create_environment, create_variable);
    }

    Variable& lookup_variable(const SEXP rho,
                              const std::string& symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Environment& env = lookup_environment(rho, create_environment);

        bool var_exists = env.exists(symbol);

        if (create_variable && !var_exists) {
            return env.define(
                symbol, create_next_variable_id_(), UNDEFINED_TIMESTAMP);
        }

        return env.lookup(symbol);
    }

    Variable& define_variable(const SEXP rho,
                              const SEXP symbol,
                              bool create_environment = true) {
        return lookup_environment(rho, true).define(symbol_to_string(symbol),
                                                    create_next_variable_id_(),
                                                    get_current_timestamp_());
    }

    Variable& update_variable(const SEXP rho,
                              const SEXP symbol,
                              bool create_environment = true,
                              bool create_variable = true) {
        Variable& var = lookup_variable(rho, symbol);
        var.set_modification_timestamp(get_current_timestamp_());
        return var;
    }

    Variable remove_variable(const SEXP rho,
                             const SEXP symbol,
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
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                execution_pause_time - execution_resume_time_)
                .count();
        ExecutionContextStack& stack(get_stack_());
        if (!stack.is_empty()) {
            stack.peek(1).increment_execution_time(execution_time);
        }
    }

    ExecutionContext pop_stack() {
        ExecutionContextStack& stack(get_stack_());
        ExecutionContext exec_ctxt(stack.pop());
        if (!stack.is_empty()) {
            stack.peek(1).increment_execution_time(
                exec_ctxt.get_execution_time());
        }
        return exec_ctxt;
    }

    template <typename T>
    void push_stack(T* context) {
        get_stack_().push(context);
    }

    execution_contexts_t unwind_stack(const RCNTXT* context) {
        return get_stack_().unwind(ExecutionContext(context));
    }

  private:
    std::chrono::time_point<std::chrono::high_resolution_clock>
        execution_resume_time_;

    /***************************************************************************
     * PROMISE
     **************************************************************************/

  public:
    DenotedValue* create_promise(const SEXP promise) {
        DenotedValue* promise_state(create_raw_promise_(promise, true));
        auto result = promises_.insert_or_assign(promise, promise_state);
        promise_state->set_creation_timestamp(get_current_timestamp_());
        return promise_state;
    }

    DenotedValue* lookup_promise(const SEXP promise,
                                 bool create = false,
                                 bool local = false) {
        // static int printed = 0;
        auto iter = promises_.find(promise);

        /* all promises encountered are added to the map. Its not possible for
           a promise id to be encountered which is not already mapped.
           If this happens, possibly, the mapper methods are not the first to
           be called in the analysis. Hence, they are not able to update the
           mapping. */
        if (iter == promises_.end()) {
            /* Some debugging code to be removed later
            if (symbol_to_string(CAR(dyntrace_get_promise_expression(
                   promise))) != "lazyLoadDBfetch") {
            ++printed;
            if (printed == 5) {
                printf("Address is %p\n", (void*) promise);
                printed = 0;
            }
            std::cerr << static int loopy = 1;
            while (loopy)
                ;
            }
            */
            if (create) {
                DenotedValue* promise_state(
                    create_raw_promise_(promise, local));
                promises_.insert({promise, promise_state});
                return promise_state;
            } else {
                return nullptr;
            }
        }

        return iter->second;
    }

    void remove_promise(const SEXP promise, DenotedValue* promise_state) {
        promises_.erase(promise);
        destroy_promise(promise_state);
    }

    void destroy_promise(DenotedValue* promise_state) {
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
        promise_state->set_inactive();

        serialize_promise_(promise_state);

        summarize_promise_lifecycle_(promise_state->get_lifecycle());

        if (promise_state->has_escaped()) {
            serialize_escaped_promise_(promise_state);
        }

        if (!promise_state->is_argument()) {
            delete promise_state;
        }
    }

  private:
    denoted_value_id_t get_next_denoted_value_id_() {
        return denoted_value_id_counter_++;
    }

    void serialize_promise_(DenotedValue* promise) {
        promises_data_table_->write_row(
            promise->get_id(),
            promise->was_argument(),
            sexptype_to_string(promise->get_expression_type()),
            sexptype_to_string(promise->get_value_type()),
            promise->get_creation_scope(),
            promise->get_forcing_scope(),
            promise->get_S3_dispatch_count(),
            promise->get_S4_dispatch_count(),
            promise->is_preforced(),
            promise->get_force_count(),
            promise->get_evaluation_depth().call_depth,
            promise->get_evaluation_depth().promise_depth,
            promise->get_evaluation_depth().nested_promise_depth,
            promise->get_metaprogram_count(),
            promise->get_value_lookup_count(),
            promise->get_value_assign_count(),
            promise->get_expression_lookup_count(),
            promise->get_expression_assign_count(),
            promise->get_environment_lookup_count(),
            promise->get_environment_assign_count(),
            promise->get_execution_time());
    }

    void serialize_escaped_promise_(DenotedValue* promise) {
        escaped_arguments_data_table_->write_row(
            promise->get_previous_call_id(),
            promise->get_previous_function_id(),
            sexptype_to_string(promise->get_previous_call_return_value_type()),
            promise->get_previous_formal_parameter_count(),
            promise->get_previous_formal_parameter_position(),
            promise->get_previous_actual_argument_position(),
            promise->get_id(),
            promise->get_class_name(),
            promise->get_S3_dispatch_count(),
            promise->get_S4_dispatch_count(),
            sexptype_to_string(promise->get_type()),
            sexptype_to_string(promise->get_expression_type()),
            sexptype_to_string(promise->get_value_type()),
            promise->get_previous_default_argument(),
            promise->does_non_local_return(),
            promise->has_escaped(),
            promise->get_evaluation_depth().call_depth,
            promise->get_evaluation_depth().promise_depth,
            promise->get_evaluation_depth().nested_promise_depth,
            promise->get_evaluation_depth().forcing_actual_argument_position,
            promise->is_preforced(),
            promise->get_force_count_before_escape(),
            promise->get_metaprogram_count_before_escape(),
            promise->get_value_lookup_count_before_escape(),
            promise->get_value_assign_count_before_escape(),
            promise->get_expression_lookup_count_before_escape(),
            promise->get_expression_assign_count_before_escape(),
            promise->get_environment_lookup_count_before_escape(),
            promise->get_environment_assign_count_before_escape(),
            promise->get_force_count_after_escape(),
            promise->get_metaprogram_count_after_escape(),
            promise->get_value_lookup_count_after_escape(),
            promise->get_value_assign_count_after_escape(),
            promise->get_expression_lookup_count_after_escape(),
            promise->get_expression_assign_count_after_escape(),
            promise->get_environment_lookup_count_after_escape(),
            promise->get_environment_assign_count_after_escape(),
            promise->get_self_scope_mutation_count_before_escape(true),
            promise->get_self_scope_mutation_count_before_escape(false),
            promise->get_lexical_scope_mutation_count_before_escape(true),
            promise->get_lexical_scope_mutation_count_before_escape(false),
            promise->get_non_lexical_scope_mutation_count_before_escape(true),
            promise->get_non_lexical_scope_mutation_count_before_escape(false),
            promise->get_self_scope_observation_count_before_escape(true),
            promise->get_self_scope_observation_count_before_escape(false),
            promise->get_lexical_scope_observation_count_before_escape(true),
            promise->get_lexical_scope_observation_count_before_escape(false),
            promise->get_non_lexical_scope_observation_count_before_escape(
                true),
            promise->get_non_lexical_scope_observation_count_before_escape(
                false),
            promise->get_self_scope_mutation_count_after_escape(true),
            promise->get_self_scope_mutation_count_after_escape(false),
            promise->get_lexical_scope_mutation_count_after_escape(true),
            promise->get_lexical_scope_mutation_count_after_escape(false),
            promise->get_non_lexical_scope_mutation_count_after_escape(true),
            promise->get_non_lexical_scope_mutation_count_after_escape(false),
            promise->get_self_scope_observation_count_after_escape(true),
            promise->get_self_scope_observation_count_after_escape(false),
            promise->get_lexical_scope_observation_count_after_escape(true),
            promise->get_lexical_scope_observation_count_after_escape(false),
            promise->get_non_lexical_scope_observation_count_after_escape(true),
            promise->get_non_lexical_scope_observation_count_after_escape(
                false),
            promise->get_execution_time());
    }

    DenotedValue* create_raw_promise_(const SEXP promise, bool local) {
        const SEXP rho = dyntrace_get_promise_environment(promise);

        DenotedValue* promise_state =
            new DenotedValue(get_next_denoted_value_id_(), promise, local);

        promise_state->set_creation_scope(infer_creation_scope());

        /* Setting this bit tells us that the promise is currently in the
           promises table. As long as this is set, the call holding a reference
           to it will not delete it. */
        promise_state->set_active();
        return promise_state;
    }

    std::unordered_map<SEXP, DenotedValue*> promises_;
    denoted_value_id_t denoted_value_id_counter_;

  private:
    timestamp_t get_current_timestamp_() const {
        return timestamp_;
    }

    timestamp_t increment_timestamp_() {
        return timestamp_++;
    }

    timestamp_t timestamp_;

  public:
    scope_t infer_creation_scope() {
        ExecutionContextStack& stack = get_stack_();

        for (auto iter = stack.crbegin(); iter != stack.crend(); ++iter) {
            if (iter->is_call()) {
                const Function* const function =
                    iter->get_call()->get_function();
                /* '{' function as promise creation source is not very
                   insightful. We want to keep going back until we find
                   something meaningful. */
                if (!function->is_curly_bracket()) {
                    return function->get_id();
                }
            }
        }
        return TOP_LEVEL_SCOPE;
    }

    scope_t infer_forcing_scope() {
        const ExecutionContextStack& stack = get_stack_();

        for (auto iter = stack.crbegin(); iter != stack.crend(); ++iter) {
            const ExecutionContext& exec_ctxt = *iter;

            if (exec_ctxt.is_r_context()) {
                continue;
            } else if (exec_ctxt.is_promise()) {
                return "Promise";
            } else {
                return exec_ctxt.get_call()->get_function_name();
            }
        }

        return TOP_LEVEL_SCOPE;
    }

    void exit_probe(const Event event) {
        resume_execution_timer();
    }

    void enter_probe(const Event event) {
        pause_execution_timer();
        increment_timestamp_();
        ++event_counter_[to_underlying(event)];
    }

  public:
    Call* create_call(const SEXP call,
                      const SEXP op,
                      const SEXP args,
                      const SEXP rho) {
        Function* function = lookup_function(op);
        Call* function_call = nullptr;
        call_id_t call_id = get_next_call_id_();
        const std::string function_name = get_name(call);

        function_call = new Call(call_id, function_name, rho, function);

        if (TYPEOF(op) == CLOSXP) {
            process_closure_arguments_(function_call, op);
        } else {
            int eval = dyntrace_get_c_function_argument_evaluation(op);
            function_call->set_force_order(eval);
        }

        return function_call;
    }

    void destroy_call(Call* call) {
        Function* function = call->get_function();

        function->add_summary(call);

        for (Argument* argument: call->get_arguments()) {
            serialize_argument_(argument);

            DenotedValue* value = argument->get_denoted_value();

            if (!value->is_active()) {
                delete value;
            } else {
                value->remove_argument(
                    call->get_id(),
                    call->get_function()->get_id(),
                    call->get_return_value_type(),
                    call->get_function()->get_formal_parameter_count(),
                    argument);
            }

            argument->set_denoted_value(nullptr);

            delete argument;
        }

        delete call;
    }

  private:
    call_id_t get_next_call_id_() {
        return ++call_id_counter_;
    }

    void process_closure_argument_(Call* call,
                                   int formal_parameter_position,
                                   int actual_argument_position,
                                   const SEXP name,
                                   const SEXP argument,
                                   bool dot_dot_dot) {
        DenotedValue* value = nullptr;
        /* only add to promise map if the argument is a promise */
        if (type_of_sexp(argument) == PROMSXP) {
            value = lookup_promise(argument, true);
        } else {
            value =
                new DenotedValue(get_next_denoted_value_id_(), argument, false);
            value->set_creation_scope(infer_creation_scope());
        }

        bool default_argument = true;

        if (value->is_promise()) {
            default_argument =
                call->get_environment() == value->get_environment();
        }

        Argument* arg = new Argument(call,
                                     formal_parameter_position,
                                     actual_argument_position,
                                     default_argument,
                                     dot_dot_dot);
        arg->set_denoted_value(value);

        value->add_argument(arg);

        call->add_argument(arg);
    }

    void process_closure_arguments_(Call* call, const SEXP op) {
        SEXP formal = nullptr;
        SEXP name = nullptr;
        SEXP argument = nullptr;
        SEXP rho = call->get_environment();
        int formal_parameter_position = -1;
        int actual_argument_position = -1;

        for (formal = FORMALS(op); formal != R_NilValue; formal = CDR(formal)) {
            ++formal_parameter_position;

            /* get argument name */
            name = TAG(formal);
            /* lookup argument in environment by name */
            argument = dyntrace_lookup_environment(rho, name);

            switch (type_of_sexp(argument)) {
            case DOTSXP:
                for (SEXP dot_dot_dot_arguments = argument;
                     dot_dot_dot_arguments != R_NilValue;
                     dot_dot_dot_arguments = CDR(dot_dot_dot_arguments)) {
                    ++actual_argument_position;

                    name = TAG(dot_dot_dot_arguments);

                    SEXP dot_dot_dot_argument = CAR(dot_dot_dot_arguments);

                    process_closure_argument_(call,
                                              formal_parameter_position,
                                              actual_argument_position,
                                              name,
                                              dot_dot_dot_argument,
                                              true);
                }
                break;

            default:
                ++actual_argument_position;
                process_closure_argument_(call,
                                          formal_parameter_position,
                                          actual_argument_position,
                                          name,
                                          argument,
                                          is_dots_symbol(name));
                break;
            }
        }
    }

    void serialize_argument_(Argument* argument) {
        Call* call = argument->get_call();
        Function* function = call->get_function();
        DenotedValue* value = argument->get_denoted_value();

        arguments_data_table_->write_row(
            call->get_id(),
            function->get_id(),
            value->get_id(),
            argument->get_formal_parameter_position(),
            argument->get_actual_argument_position(),
            sexptype_to_string(value->get_type()),
            sexptype_to_string(value->get_expression_type()),
            sexptype_to_string(value->get_value_type()),
            argument->is_default_argument(),
            argument->is_dot_dot_dot(),
            value->is_preforced(),
            argument->is_directly_forced(),
            argument->get_direct_lookup_count(),
            argument->get_direct_metaprogram_count(),
            argument->is_indirectly_forced(),
            argument->get_indirect_lookup_count(),
            argument->get_indirect_metaprogram_count(),
            argument->used_for_S3_dispatch(),
            argument->used_for_S4_dispatch(),
            argument->get_forcing_actual_argument_position(),
            argument->does_non_local_return(),
            value->get_execution_time(),
            value->get_serialized_expression());

        if (value->is_promise() && value->get_serialized_expression() != "") {
            side_effects_data_table_->write_row(
                value->get_id(),
                call->get_id(),
                function->get_id(),
                function->get_namespace(),
                function->get_name_string(),
                argument->get_formal_parameter_position(),
                argument->get_actual_argument_position(),
                argument->is_dot_dot_dot(),
                value->get_self_scope_mutation_count(true),
                value->get_self_scope_mutation_count(false),
                value->get_lexical_scope_mutation_count(true),
                value->get_lexical_scope_mutation_count(false),
                value->get_non_lexical_scope_mutation_count(true),
                value->get_non_lexical_scope_mutation_count(false),
                value->get_self_scope_observation_count(true),
                value->get_self_scope_observation_count(false),
                value->get_lexical_scope_observation_count(true),
                value->get_lexical_scope_observation_count(false),
                value->get_non_lexical_scope_observation_count(true),
                value->get_non_lexical_scope_observation_count(false),
                value->get_serialized_expression());
        }
    }

    DataTableStream* arguments_data_table_;
    DataTableStream* side_effects_data_table_;
    DataTableStream* escaped_arguments_data_table_;

    /***************************************************************************
     * Function API
     ***************************************************************************/
  public:
    Function* lookup_function(const SEXP op) {
        Function* function = nullptr;

        auto iter = functions_.find(op);

        if (iter != functions_.end()) {
            return iter->second;
        }

        const auto [package_name, function_definition, function_id] =
            Function::compute_definition_and_id(op);

        auto iter2 = function_cache_.find(function_id);

        if (iter2 == function_cache_.end()) {
            function = new Function(
                op, package_name, function_definition, function_id);
            function_cache_.insert({function_id, function});
        } else {
            function = iter2->second;
        }

        functions_.insert({op, function});
        return function;
    }

    void remove_function(const SEXP op) {
        auto it = functions_.find(op);

        if (it != functions_.end()) {
            functions_.erase(it);
        }
    }

  private:
    void destroy_function_(Function* function) {
        serialize_function_(function);
        delete function;
    }

    DataTableStream* call_summaries_data_table_;
    DataTableStream* function_definitions_data_table_;
    std::unordered_map<SEXP, Function*> functions_;
    std::unordered_map<function_id_t, Function*> function_cache_;

    void serialize_function_(Function* function) {
        const std::string all_names = function->get_name_string();
        serialize_function_call_summary_(function, all_names);
        serialize_function_definition_(function, all_names);
    }

    void serialize_function_call_summary_(const Function* function,
                                          const std::string& names) {
        for (std::size_t i = 0; i < function->get_summary_count(); ++i) {
            const CallSummary& call_summary = function->get_call_summary(i);

            call_summaries_data_table_->write_row(
                function->get_id(),
                function->get_namespace(),
                names,
                sexptype_to_string(function->get_type()),
                function->get_formal_parameter_count(),
                function->is_wrapper(),
                call_summary.is_S3_method(),
                call_summary.is_S4_method(),
                pos_seq_to_string(call_summary.get_force_order()),
                pos_seq_to_string(
                    call_summary.get_missing_argument_positions()),
                sexptype_to_string(call_summary.get_return_value_type()),
                call_summary.is_jumped(),
                call_summary.get_call_count());
        }
    }

    void serialize_function_definition_(const Function* function,
                                        const std::string& names) {
        function_definitions_data_table_->write_row(
            function->get_id(),
            function->get_namespace(),
            names,
            function->get_formal_parameter_count(),
            function->is_byte_compiled(),
            function->get_definition());
    }

  public:
    void identify_side_effect_creators(const Variable& var, const SEXP env) {
        bool direct = true;
        ExecutionContextStack& stack = get_stack_();

        for (auto iter = stack.rbegin(); iter != stack.rend(); ++iter) {
            ExecutionContext& exec_ctxt = *iter;

            if (exec_ctxt.is_closure()) {
                if (exec_ctxt.get_closure()->get_environment() == env) {
                    /* its normal for a function to mutate variables in its
                       own environment. this case is not interesting. */
                    return;
                }
            }

            if (exec_ctxt.is_promise()) {
                DenotedValue* promise = exec_ctxt.get_promise();

                const SEXP prom_env = promise->get_environment();

                const timestamp_t var_timestamp =
                    var.get_modification_timestamp();

                if (prom_env == env) {
                    if (promise->get_creation_timestamp() > var_timestamp) {
                        promise->set_self_scope_mutation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        return;
                    }
                } else if (is_parent_environment(env, prom_env)) {
                    if (promise->get_creation_timestamp() > var_timestamp) {
                        /* if this happens, promise is causing side effect
                           in its lexically scoped environment. */
                        promise->set_lexical_scope_mutation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        return;
                    }
                } else {
                    if (promise->get_creation_timestamp() > var_timestamp) {
                        /* if this happens, promise is causing side effect
                           in non lexically scoped environment */
                        promise->set_non_lexical_scope_mutation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        return;
                    }
                }
            }
        }
    }

    void identify_side_effect_observers(const Variable& var, const SEXP env) {
        const timestamp_t var_timestamp = var.get_modification_timestamp();

        /* this implies we have not seen the variable before.
           TODO - see when this case is triggered and ensure
           there is no bug */
        if (timestamp_is_undefined(var_timestamp)) {
            return;
        }

        /* if the modification timestamp of the variable is
           greater than the creation timestamp of the promise,
           then, that promise has identified a side effect */

        bool direct = true;

        ExecutionContextStack& stack = get_stack_();

        for (auto iter = stack.rbegin(); iter != stack.rend(); ++iter) {
            ExecutionContext& exec_ctxt = *iter;

            /* If the most recent context that is responsible for this
               side effect is a closure, then return. Currently, we only
               care about promises directly responsible for side effects.
               We don't return in case of specials and builtins because
               they are more like operators in a programming language and
               everything ultimately happens in them and returning on
               encountering them will make it look like no promise has
               caused a side effect. */

            if (exec_ctxt.is_closure()) {
                if (exec_ctxt.get_closure()->get_environment() == env) {
                    /* its normal for a function to mutate variables in its
                       own environment. this case is not interesting. */
                    return;
                }
            }

            if (exec_ctxt.is_promise()) {
                DenotedValue* promise = exec_ctxt.get_promise();

                const SEXP prom_env = promise->get_environment();

                if (prom_env == env) {
                    if (promise->get_creation_timestamp() < var_timestamp) {
                        promise->set_self_scope_observation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        /* return if the promise observes a variable in its
                         * environment created before it. */
                        return;
                    }
                } else if (is_parent_environment(env, prom_env)) {
                    /* if this happens, promise is observing side effect
                       in its lexically scoped environment. */
                    if (promise->get_creation_timestamp() < var_timestamp) {
                        promise->set_lexical_scope_observation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        return;
                    }
                } else {
                    /* if this happens, promise is observing side effect
                       in non lexically scoped environment */
                    if (promise->get_creation_timestamp() < var_timestamp) {
                        promise->set_non_lexical_scope_observation(direct);
                        direct = false;
                        return; // to remove
                    } else {
                        return;
                    }
                }
            }
        }
    }

    void notify_caller(Call* callee) {
        ExecutionContextStack& stack = get_stack_();

        if (!stack.is_empty()) {
            ExecutionContext exec_ctxt = stack.peek(1);

            if (!exec_ctxt.is_call()) {
                return;
            }

            Call* caller = exec_ctxt.get_call();
            Function* function = caller->get_function();
            if (function->is_closure() || function->is_curly_bracket()) {
                caller->analyze_callee(callee);
            }
        }
    }

    eval_depth_t get_evaluation_depth(Call* call) {
        ExecutionContextStack& stack = get_stack_();
        ExecutionContextStack::reverse_iterator iter;
        eval_depth_t eval_depth = {0, 0, 0, -1};
        bool nesting = true;

        for (iter = stack.rbegin(); iter != stack.rend(); ++iter) {
            ExecutionContext& exec_ctxt = *iter;

            if (exec_ctxt.is_closure()) {
                nesting = false;
                if (exec_ctxt.get_call() == call)
                    break;
                ++eval_depth.call_depth;
            } else if (exec_ctxt.is_promise()) {
                ++eval_depth.promise_depth;
                if (nesting)
                    ++eval_depth.nested_promise_depth;
                DenotedValue* promise = exec_ctxt.get_promise();
                if (eval_depth.forcing_actual_argument_position == -1 &&
                    promise->is_argument() &&
                    promise->get_last_argument()->get_call() == call) {
                    eval_depth.forcing_actual_argument_position =
                        promise->get_last_argument()
                            ->get_actual_argument_position();
                }
            }
        }

        // if this happens, it means we could not locate the call from which
        // this promise originated. This means that this is an escaped
        // promise.
        if (iter == stack.rend()) {
            return ESCAPED_PROMISE_EVAL_DEPTH;
        }

        return eval_depth;
    }

    void summarize_promise_lifecycle_(const lifecycle_t& lifecycle) {
        for (std::size_t i = 0; i < lifecycle_summary_.size(); ++i) {
            if (lifecycle_summary_[i].first.action == lifecycle.action &&
                lifecycle_summary_[i].first.count == lifecycle.count) {
                ++lifecycle_summary_[i].second;
                return;
            }
        }
        lifecycle_summary_.push_back({lifecycle, 1});
    }

    void serialize_promise_lifecycle_summary_() {
        for (const auto& summary: lifecycle_summary_) {
            promise_lifecycles_data_table_->write_row(
                summary.first.action,
                pos_seq_to_string(summary.first.count),
                summary.second);
        }
    }

  private:
    call_id_t call_id_counter_;
    std::vector<unsigned int> object_count_;
    std::vector<std::pair<lifecycle_t, int>> lifecycle_summary_;
    std::vector<unsigned long int> event_counter_;
};

#endif /* PROMISEDYNTRACER_TRACER_STATE_H */
