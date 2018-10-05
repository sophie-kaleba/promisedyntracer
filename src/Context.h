#ifndef PROMISEDYNTRACER_CONTEXT_H
#define PROMISEDYNTRACER_CONTEXT_H

#include "AnalysisDriver.h"
#include "AnalysisSwitch.h"
#include "DebugSerializer.h"
#include "State.h"
#include "TraceSerializer.h"
#include <string>

class Context {
  public:
    Context(std::string trace_filepath, bool truncate, bool enable_trace,
            bool verbose, std::string output_dir, bool binary,
            int compression_level, AnalysisSwitch analysis_switch)
        : state_(new tracer_state_t()), analysis_switch_{analysis_switch},
          serializer_(
              new TraceSerializer(trace_filepath, truncate, enable_trace)),
          driver_(new AnalysisDriver(*state_, verbose, output_dir, truncate,
                                     binary, compression_level,
                                     analysis_switch)),
          debugger_(new DebugSerializer(verbose)), output_dir_{output_dir},
          binary_{binary}, verbose_{verbose}, truncate_{truncate},
          compression_level_{compression_level} {}

    tracer_state_t &get_state() { return *state_; }

    TraceSerializer &get_serializer() { return *serializer_; }

    DebugSerializer &get_debug_serializer() {
        if (debugger_->needsState())
            debugger_->setState(&get_state());
        return *debugger_;
    }

    AnalysisDriver &get_analysis_driver() { return *driver_; }

    const std::string &get_output_dir() const { return output_dir_; }

    int get_compression_level() const { return compression_level_; }

    bool is_binary() const { return binary_; }

    bool is_verbose() const { return verbose_; }

    bool get_truncate() const { return truncate_; }

    const AnalysisSwitch &get_analysis_switch() const {
        return analysis_switch_;
    }

    ~Context() {

        delete debugger_;
        delete driver_;
        delete serializer_;
        /* delete state in the end as everything else
           can store reference to the state */
        delete state_;
    }

  private:
    tracer_state_t *state_;
    TraceSerializer *serializer_;
    DebugSerializer *debugger_;
    AnalysisSwitch analysis_switch_;
    AnalysisDriver *driver_;
    std::string output_dir_;
    bool binary_;
    bool verbose_;
    bool truncate_;
    int compression_level_;
};

inline Context &tracer_context(dyntracer_t *dyntracer) {
    return *static_cast<Context *>(dyntracer->state);
}

inline tracer_state_t &tracer_state(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_state();
}

inline TraceSerializer &tracer_serializer(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_serializer();
}

inline DebugSerializer &debug_serializer(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_debug_serializer();
}

inline AnalysisDriver &analysis_driver(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_analysis_driver();
}

inline const std::string &tracer_output_dir(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_output_dir();
}
#endif /* PROMISEDYNTRACER_CONTEXT_H */
