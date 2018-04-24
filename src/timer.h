#include <chrono>
#include <vector>
#include <string>

#ifndef PROMISEDYNTRACER_TIMER_H
#define PROMISEDYNTRACER_TIMER_H

namespace timing {

    enum segment {
        FUNCTION_ENTRY_RECORDER_OTHER = 0,
        FUNCTION_ENTRY_RECORDER_FUNCTION_ID,
        FUNCTION_ENTRY_RECORDER_CALL_ID,
        FUNCTION_ENTRY_RECORDER_PARENT_ID,
        FUNCTION_ENTRY_RECORDER_LOCATION,
        FUNCTION_ENTRY_RECORDER_EXPRESSION,
        FUNCTION_ENTRY_RECORDER_NAME,
        FUNCTION_ENTRY_RECORDER_ARGUMENTS,
        FUNCTION_ENTRY_RECORDER_DEFINITION,
        FUNCTION_ENTRY_RECORDER_RECURSIVE,
        FUNCTION_ENTRY_RECORDER_PARENT_PROMISE,

        FUNCTION_ENTRY_STACK,
        FUNCTION_ENTRY_WRITE_SQL,
        FUNCTION_ENTRY_WRITE_TRACE,

        FUNCTION_EXIT_RECORDER_OTHER,
        FUNCTION_EXIT_RECORDER_FUNCTION_ID,
        FUNCTION_EXIT_RECORDER_CALL_ID,
        FUNCTION_EXIT_RECORDER_PARENT_ID,
        FUNCTION_EXIT_RECORDER_LOCATION,
        FUNCTION_EXIT_RECORDER_EXPRESSION,
        FUNCTION_EXIT_RECORDER_NAME,
        FUNCTION_EXIT_RECORDER_ARGUMENTS,
        FUNCTION_EXIT_RECORDER_DEFINITION,
        FUNCTION_EXIT_RECORDER_RECURSIVE,
        FUNCTION_EXIT_RECORDER_PARENT_PROMISE,

        FUNCTION_EXIT_STACK,
        FUNCTION_EXIT_WRITE_SQL,
        FUNCTION_EXIT_WRITE_TRACE,

        BUILTIN_ENTRY_RECORDER,
        BUILTIN_ENTRY_STACK,
        BUILTIN_ENTRY_WRITE_SQL,
        BUILTIN_ENTRY_WRITE_TRACE,

        BUILTIN_EXIT_RECORDER,
        BUILTIN_EXIT_STACK,
        BUILTIN_EXIT_WRITE_SQL,
        BUILTIN_EXIT_WRITE_TRACE,

        CREATE_PROMISE_RECORDER,
        CREATE_PROMISE_WRITE_SQL,
        CREATE_PROMISE_WRITE_TRACE,

        FORCE_PROMISE_ENTRY_RECORDER,
        FORCE_PROMISE_ENTRY_STACK,
        FORCE_PROMISE_ENTRY_WRITE_SQL,
        FORCE_PROMISE_ENTRY_WRITE_TRACE,

        FORCE_PROMISE_EXIT_RECORDER,
        FORCE_PROMISE_EXIT_STACK,
        FORCE_PROMISE_EXIT_WRITE_SQL,
        FORCE_PROMISE_EXIT_WRITE_TRACE,

        LOOKUP_PROMISE_VALUE_RECORDER,
        LOOKUP_PROMISE_VALUE_WRITE_SQL,
        LOOKUP_PROMISE_VALUE_WRITE_TRACE,

        LOOKUP_PROMISE_EXPRESSION_RECORDER,
        LOOKUP_PROMISE_EXPRESSION_WRITE_SQL,
        LOOKUP_PROMISE_EXPRESSION_WRITE_TRACE,

        LOOKUP_PROMISE_ENVIRONMENT_RECORDER,
        LOOKUP_PROMISE_ENVIRONMENT_WRITE_SQL,
        LOOKUP_PROMISE_ENVIRONMENT_WRITE_TRACE,

        SET_PROMISE_VALUE_RECORDER,
        SET_PROMISE_VALUE_WRITE_SQL,
        SET_PROMISE_VALUE_WRITE_TRACE,

        SET_PROMISE_EXPRESSION_RECORDER,
        SET_PROMISE_EXPRESSION_WRITE_SQL,
        SET_PROMISE_EXPRESSION_WRITE_TRACE,

        SET_PROMISE_ENVIRONMENT_RECORDER,
        SET_PROMISE_ENVIRONMENT_WRITE_SQL,
        SET_PROMISE_ENVIRONMENT_WRITE_TRACE,

        GC_PROMISE_UNMARKED_RECORDER,
        GC_PROMISE_UNMARKED_WRITE_SQL,
        GC_PROMISE_UNMARKED_RECORD_KEEPING,

        GC_FUNCTION_UNMARKED_RECORD_KEEPING,

        GC_ENTRY_RECORDER,
        GC_ENTRY_WRITE_SQL,

        GC_EXIT_RECORDER,
        GC_EXIT_WRITE_SQL,

        VECTOR_ALLOC_RECORDER,
        VECTOR_ALLOC_WRITE_SQL,

        NEW_ENVIRONMENT_RECORDER,
        NEW_ENVIRONMENT_WRITE_SQL,
        NEW_ENVIRONMENT_WRITE_TRACE,

        CONTEXT_ENTRY_STACK,
        CONTEXT_JUMP_STACK,
        CONTEXT_EXIT_STACK,

        number_of_segments
    };

    class Timer {
    private:
        long timers[number_of_segments] = {};
        long occurances[number_of_segments] = {};
        std::chrono::time_point<std::chrono::high_resolution_clock> start_time;

    public:
        Timer() {};

        void start();
        void reset();
        void zero();
        void endSegment(segment s);
        std::vector<std::pair<std::string, std::string>> stats();

        Timer(Timer const&) = delete;
        void operator=(Timer const&) = delete;

        static Timer& getInstance() {
            static Timer instance;
            return instance;
        }
    };
}


#endif //PROMISEDYNTRACER_TIMER_H