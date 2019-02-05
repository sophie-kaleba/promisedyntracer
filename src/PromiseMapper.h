#ifndef __PROMISE_MAPPER_H__
#define __PROMISE_MAPPER_H__

#include "utilities.h"
#include "PromiseState.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class PromiseMapper {
    using promises_t = std::unordered_map<SEXP, PromiseState*>;

  public:
    using iterator = promises_t::iterator;
    using const_iterator = promises_t::const_iterator;

    PromiseMapper()
        : promises_(promises_t(PROMISE_MAPPING_BUCKET_COUNT)),
          promise_id_counter_(0) {
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

    void remove(const SEXP promise) {

        // TODO - think about deleting this promise pointer
        /* it is possible that the promise does not exist in mapping.
           this happens if the promise was created outside of tracing
           but is being garbage collected in the middle of tracing */
        auto iter = promises_.find(promise);
        if(iter == promises_.end()) {
            dyntrace_log_error("Unable to delete promise %p\n", promise);
        }
        promises_.erase(iter);
        delete iter -> second;
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

    iterator begin() { return promises_.begin(); }

    iterator end() { return promises_.end(); }

    const_iterator begin() const { return promises_.begin(); }

    const_iterator end() const { return promises_.end(); }

    const_iterator cbegin() const { return promises_.cbegin(); }

    const_iterator cend() const { return promises_.cend(); }

  private:

    promise_id_t get_next_promise_id_() {
        return promise_id_counter_++;
    }

    promises_t promises_;
    promise_id_t promise_id_counter_;
};

#endif /* __PROMISE_ACCESS_ANALYSIS_H__ */
