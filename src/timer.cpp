#include "timer.h"
using namespace std;
using namespace std::chrono;

namespace timing {

    string segment_names[] = {
        "FUNCTION_ENTRY/RECORDER",
        "FUNCTION_ENTRY/RECORDER/OTHER",
        "FUNCTION_ENTRY/RECORDER/FUNCTION_ID",
        "FUNCTION_ENTRY/RECORDER/CALL_ID",
        "FUNCTION_ENTRY/RECORDER/PARENT_ID",
        "FUNCTION_ENTRY/RECORDER/LOCATION",
        "FUNCTION_ENTRY/RECORDER/EXPRESSION",
        "FUNCTION_ENTRY/RECORDER/NAME",
        "FUNCTION_ENTRY/RECORDER/ARGUMENTS",
        "FUNCTION_ENTRY/RECORDER/DEFINITION",
        "FUNCTION_ENTRY/RECORDER/RECURSIVE",
        "FUNCTION_ENTRY/RECORDER/PARENT_PROMISE",

        "FUNCTION_ENTRY/STACK",
        "FUNCTION_ENTRY/WRITE_TRACE",

        "FUNCTION_ENTRY/WRITE_SQL",
        "FUNCTION_ENTRY/WRITE_SQL/RECORDKEEPING",
        "FUNCTION_ENTRY/WRITE_SQL/BIND",
        "FUNCTION_ENTRY/WRITE_SQL/EXECUTE",

        "FUNCTION_EXIT/RECORDER",
        "FUNCTION_EXIT/RECORDER/OTHER",
        "FUNCTION_EXIT/RECORDER/FUNCTION_ID",
        "FUNCTION_EXIT/RECORDER/CALL_ID",
        "FUNCTION_EXIT/RECORDER/PARENT_ID",
        "FUNCTION_EXIT/RECORDER/LOCATION",
        "FUNCTION_EXIT/RECORDER/EXPRESSION",
        "FUNCTION_EXIT/RECORDER/NAME",
        "FUNCTION_EXIT/RECORDER/ARGUMENTS",
        "FUNCTION_EXIT/RECORDER/DEFINITION",
        "FUNCTION_EXIT/RECORDER/RECURSIVE",
        "FUNCTION_EXIT/RECORDER/PARENT_PROMISE",

        "FUNCTION_EXIT/STACK",
        "FUNCTION_EXIT/WRITE_TRACE",

        "FUNCTION_EXIT/WRITE_SQL",
        "FUNCTION_EXIT/WRITE_SQL/RECORDKEEPING",
        "FUNCTION_EXIT/WRITE_SQL/BIND",
        "FUNCTION_EXIT/WRITE_SQL/EXECUTE",

        "BUILTIN_ENTRY/RECORDER",
        "BUILTIN_ENTRY/STACK",
        "BUILTIN_ENTRY/WRITE_SQL",
        "BUILTIN_ENTRY/WRITE_SQL/RECORDKEEPING",
        "BUILTIN_ENTRY/WRITE_SQL/BIND",
        "BUILTIN_ENTRY/WRITE_SQL/EXECUTE",
        "BUILTIN_ENTRY/WRITE_TRACE",

        "BUILTIN_EXIT/RECORDER",
        "BUILTIN_EXIT/STACK",
        "BUILTIN_EXIT/WRITE_SQL",
        "BUILTIN_EXIT/WRITE_SQL/RECORDKEEPING",
        "BUILTIN_EXIT/WRITE_SQL/BIND",
        "BUILTIN_EXIT/WRITE_SQL/EXECUTE",
        "BUILTIN_EXIT/WRITE_TRACE",

        "PROMISE_CREATED/RECORDER",
        "PROMISE_CREATED/WRITE_SQL",
        "PROMISE_CREATED/WRITE_SQL/BIND",
        "PROMISE_CREATED/WRITE_SQL/EXECUTE",
        "PROMISE_CREATED/WRITE_TRACE",

        "PROMISE_FORCE_ENTRY/RECORDER",
        "PROMISE_FORCE_ENTRY/STACK",
        "PROMISE_FORCE_ENTRY/WRITE_SQL",
        "PROMISE_FORCE_ENTRY/WRITE_SQL/RECORDKEEPING",
        "PROMISE_FORCE_ENTRY/WRITE_SQL/BIND",
        "PROMISE_FORCE_ENTRY/WRITE_SQL/EXECUTE",
        "PROMISE_FORCE_ENTRY/WRITE_TRACE",

        "FORCE_PROMISE_EXIT/RECORDER",
        "FORCE_PROMISE_EXIT/STACK",
        "FORCE_PROMISE_EXIT/WRITE_SQL",
        "FORCE_PROMISE_EXIT/WRITE_SQL/BIND",
        "FORCE_PROMISE_EXIT/WRITE_SQL/EXECUTE",
        "FORCE_PROMISE_EXIT/WRITE_SQL/RECORDKEEPING",
        "FORCE_PROMISE_EXIT/WRITE_TRACE",

        "PROMISE_VALUE_LOOKUP/RECORDER",
        "PROMISE_VALUE_LOOKUP/WRITE_SQL",
        "PROMISE_VALUE_LOOKUP/WRITE_SQL/BIND",
        "PROMISE_VALUE_LOOKUP/WRITE_SQL/EXECUTE",
        "PROMISE_VALUE_LOOKUP/WRITE_TRACE",

        "PROMISE_EXPRESSION_LOOKUP/RECORDER",
        "PROMISE_EXPRESSION_LOOKUP/WRITE_SQL",
        "PROMISE_EXPRESSION_LOOKUP/WRITE_SQL/BIND",
        "PROMISE_EXPRESSION_LOOKUP/WRITE_SQL/EXECUTE",
        "PROMISE_EXPRESSION_LOOKUP/WRITE_TRACE",

        "PROMISE_ENVIRONMENT_LOOKUP/RECORDER",
        "PROMISE_ENVIRONMENT_LOOKUP/WRITE_SQL",
        "PROMISE_ENVIRONMENT_LOOKUP/WRITE_TRACE",

        "PROMISE_VALUE_SET/RECORDER",
        "PROMISE_VALUE_SET/WRITE_SQL",
        "PROMISE_VALUE_SET/WRITE_TRACE",

        "PROMISE_EXPRESSION/RECORDER",
        "PROMISE_EXPRESSION/WRITE_SQL",
        "PROMISE_EXPRESSION/WRITE_TRACE",

        "PROMISE_ENVIRONMENT/RECORDER",
        "PROMISE_ENVIRONMENT/WRITE_SQL",
        "PROMISE_ENVIRONMENT/WRITE_TRACE",

        "GC_PROMISE_UNMARKED/RECORDER",
        "GC_PROMISE_UNMARKED/WRITE_SQL",
        "GC_PROMISE_UNMARKED/RECORD_KEEPING",

        "GC_FUNCTION_UNMARKED/RECORD_KEEPING",

        "GC_ENTRY/RECORDER",
        "GC_ENTRY/WRITE_SQL",

        "GC_EXIT/RECORDER",
        "GC_EXIT/WRITE_SQL",
        "GC_EXIT/WRITE_SQL/BIND",
        "GC_EXIT/WRITE_SQL/EXECUTE",

        "VECTOR_ALLOC/RECORDER",
        "VECTOR_ALLOC/WRITE_SQL",
        "VECTOR_ALLOC/WRITE_SQL/BIND",
        "VECTOR_ALLOC/WRITE_SQL/EXECUTE",

        "NEW_ENVIRONMENT/RECORDER",
        "NEW_ENVIRONMENT/WRITE_SQL",
        "NEW_ENVIRONMENT_WRITE_SQL/BIND",
        "NEW_ENVIRONMENT_WRITE_SQL/EXECUTE",
        "NEW_ENVIRONMENT/WRITE_TRACE",

        "CONTEXT_ENTRY/STACK",
        "CONTEXT_JUMP/STACK",
        "CONTEXT_JUMP/WRITE_SQL",
        "CONTEXT_JUMP/WRITE_SQL/BIND",
        "CONTEXT_JUMP/WRITE_SQL/EXECUTE",
        "CONTEXT_JUMP/WRITE_SQL/RECORDKEEPING",
        "CONTEXT_EXIT/STACK",

        "PROMISE_LIFECYCLE/WRITE_SQL/BIND",
        "PROMISE_LIFECYCLE/WRITE_SQL/EXECUTE",

        "ENVIRONMENT_ACTION/RECORDER",
        "ENVIRONMENT_ACTION_WRITE_SQL",
        "ENVIRONMENT_ACTION/WRITE_SQL/BIND",
        "ENVIRONMENT_ACTION/WRITE_SQL/EXECUTE",
        "ENVIRONMENT_ACTION/WRITE_TRACE"
    };

    string timer_names[] = {
            "MAIN",
            "RECORDER",
            "SQL"
    };

    void Timer::start() {
        start_time = chrono::high_resolution_clock::now();
    }

    void Timer::reset() {
        start_time = chrono::high_resolution_clock::now();
    }

    void Timer::zero() {
        for (int i = 0; i < number_of_segments; i++)
                timers[i] = 0;
    }

    void Timer::endSegment(segment s) {
        auto end_time = chrono::high_resolution_clock::now();
        const long duration = chrono::duration_cast<chrono::nanoseconds>(end_time-start_time).count();
        timers[s] += duration;
        occurances[s]++;
        start_time = chrono::high_resolution_clock::now();
    }

    std::vector<std::pair<std::string, std::string>> Timer::stats() {
        vector<pair<string, string>> r;

        for (int i = 0; i < number_of_segments; i++) {
            r.push_back(make_pair("TIMER_" + timer_names[name] + "_TIMING_" + segment_names[i],
                                  to_string(timers[i])));
            r.push_back(make_pair("TIMER_" + timer_names[name] + "_OCCURANCE_" + segment_names[i],
                                  to_string(occurances[i])));
        }

        return r;
    }
}