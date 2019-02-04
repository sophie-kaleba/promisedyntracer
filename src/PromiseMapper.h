#ifndef __PROMISE_MAPPER_H__
#define __PROMISE_MAPPER_H__

#include "utilities.h"
#include "PromiseState.h"
#include <algorithm>
#include <tuple>
#include <unordered_map>
#include <vector>

class PromiseMapper {
    using promises_t = std::unordered_map<prom_id_t, PromiseState>;

  public:
    using iterator = promises_t::iterator;
    using const_iterator = promises_t::const_iterator;

    PromiseMapper()
        : promises_(promises_t(PROMISE_MAPPING_BUCKET_COUNT)) {}

    void create(const prom_id_t prom_id, const env_id_t env_id, const timestamp_t timestamp) {

        auto result = promises_.insert({prom_id, PromiseState(prom_id, env_id, true)});
        // if result.second is false, this means that a promise with this id
        // already exists in map. This means that the promise with the same id
        // has either not been removed in the gc_promise_unmarked stage or the
        // the promise id assignment scheme assigns same id to two promises.
        if (!result.second) {
            dyntrace_log_error("PromiseMapper: New promise created already "
                               "exists in the promise map.");
        }

        result.first -> second.set_creation_timestamp(timestamp);
    }

    void remove(const prom_id_t prom_id) {

        // it is possible that the promise does not exist in mapping.
        // this happens if the promise was created outside of tracing
        // but is being garbage collected in the middle of tracing
        promises_.erase(prom_id);
    }

    void insert(const prom_id_t prom_id, const env_id_t env_id) {

        // the insertion only happens if the promise with this id does not
        // already exist. If the promise does not already exist, it means that
        // we have not seen its creation which means it is non local.
        // the timestamp is automatically set to BEFORE_TIME_BEGAN
        promises_.insert({prom_id, PromiseState(prom_id, env_id, false)});
    }

    void clear() {
        promises_.clear();
    }

    PromiseState &find(const prom_id_t prom_id) {
        auto iter = promises_.find(prom_id);
        // all promises encountered are added to the map. Its not possible for
        // a promise id to be encountered which is not already mapped.
        // If this happens, possibly, the mapper methods are not the first to
        // be called in the analysis driver methods. Hence, they are not able
        // to update the mapping.
        if (iter == promises_.end()) {
            dyntrace_log_error("Unable to find promise with id %d\n", prom_id);
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

    promises_t promises_;
};

#endif /* __PROMISE_ACCESS_ANALYSIS_H__ */
