#ifndef PROMISEDYNTRACER_FUNCTION_MAPPER_H
#define PROMISEDYNTRACER_FUNCTION_MAPPER_H

#include "utilities.h"
#include "CallState.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

typedef std::string function_id_t;

class FunctionMapper {
    using functions_t = std::unordered_map<function_id_t, FunctionState*>;

  public:
    using iterator = functions_t::iterator;
    using const_iterator = functions__t::const_iterator;

    FunctionMapper()
        : functions_(functions_t(FUNCTION_MAPPING_BUCKET_COUNT)) {
    }

    CallState* create_call(const SEXP call, const SEXP op, const SEXP rho) {
        FunctionState* function_state = create_or_retrieve(compute_function_id_(op));
        function_state
        auto iter = functions_.find(function_id);
    }

    PromiseState* create(const SEXP promise,
                         const env_id_t env_id,
                         const timestamp_t timestamp) {

        PromiseState *promise_state = new PromiseState(get_next_promise_id_(),
                                                       env_id,
                                                       true);
        auto result = promises_.insert_or_assign(promise, promise_state);
        promise_state -> set_creation_timestamp(timestamp);
        return promise_state;
    }

    PromiseState* remove(const SEXP promise) {

        // TODO - think about deleting this promise pointer
        /* it is possible that the promise does not exist in mapping.
           this happens if the promise was created outside of tracing
           but is being garbage collected in the middle of tracing */
        auto iter = promises_.find(promise);
        if(iter == promises_.end()) {
            dyntrace_log_error("Unable to delete promise %p\n", promise);
        }
        return iter -> second;
    }

    PromiseState* insert(const SEXP promise, const env_id_t env_id) {

        /* the insertion only happens if the promise with this id does not
           already exist. If the promise does not already exist, it means that
           we have not seen its creation which means it is non local.
           the timestamp is automatically set to UNDEFINED_TIMESTAMP */

        auto iter = promises_.find(promise);
        if(iter == promises_.end()) {
            PromiseState *promise_state = new PromiseState(get_next_promise_id_(),
                                                           env_id,
                                                           false);
            auto iter = promises_.insert({promise, promise_state});
            return promise_state;
        }
        else {
            return iter -> second;
        }
    }

    void clear() {
        promises_.clear();
    }

    PromiseState* find(const SEXP promise) {
        auto iter = promises_.find(promise);

        /* all promises encountered are added to the map. Its not possible for
           a promise id to be encountered which is not already mapped.
           If this happens, possibly, the mapper methods are not the first to
           be called in the analysis. Hence, they are not able to update the mapping. */
        if (iter == promises_.end()) {
            dyntrace_log_error("Unable to find promise %p\n", promise);
        }

        return iter->second;
    }

    iterator begin() { return functions_.begin(); }

    iterator end() { return functions_.end(); }

    const_iterator begin() const { return functions_.begin(); }

    const_iterator end() const { return functions_.end(); }

    const_iterator cbegin() const { return functions_.cbegin(); }

    const_iterator cend() const { return functions_.cend(); }

  private:

    call_id_t get_next_call_id_() {
        return call_id_counter_++;
    }

    functions_t functions_;
    call_id_t call_id_counter_;

    std::string get_function_definition(const SEXP op) {
        auto &definitions = function_definition_mapping_;
        auto it = definitions.find(function);
        if (it != definitions.end()) {
#ifdef RDT_DEBUG
            string test = get_expression(function);
            if (it->second.compare(test) != 0) {
                cout << "Function definitions are wrong.";
            }
#endif
            return it->second;
        } else {
            std::string definition = get_expression(function);
            tracer_state(dyntracer).function_definitions[function] = definition;
            return definition;
        }
    }

    function_id_t compute_function_id_(op) {
        fn_key_t definition(function_definition);

        auto &function_ids = tracer_state(dyntracer).function_ids;
        auto it = function_ids.find(definition);

        if (it != function_ids.end()) {
            return it->second;
        } else {
            /*Use hash on the function body to compute a unique (hopefully) id
             for each function.*/

            fn_id_t fn_id = compute_hash(definition.c_str());
            tracer_state(dyntracer).function_ids[definition] = fn_id;
            return fn_id;
        }
    }
};

#endif /* PROMISEDYNTRACER_FUNCTION_MAPPER_H */
