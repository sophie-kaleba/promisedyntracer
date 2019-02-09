#ifndef PROMISEDYNTRACER_CONTEXT_H
#define PROMISEDYNTRACER_CONTEXT_H

#include "Analyzer.h"
#include "AnalysisSwitch.h"
#include "State.h"
#include "TraceSerializer.h"
#include <string>

class Context {
  public:
    Context(std::string trace_filepath, bool truncate, bool enable_trace,
            bool verbose, std::string output_dirpath, bool binary,
            int compression_level, AnalysisSwitch analysis_switch)
        : state_(new tracer_state_t(output_dirpath, truncate, binary, compression_level)),
          analysis_switch_(analysis_switch),
          serializer_(
              new TraceSerializer(trace_filepath, truncate, enable_trace)),
          analyzer_(new Analyzer(*state_, output_dirpath, truncate,
                                 binary, compression_level)),
          output_dirpath_(output_dirpath),
          binary_{binary}, verbose_{verbose}, truncate_{truncate},
          compression_level_{compression_level} {}

    tracer_state_t &get_state() { return *state_; }

    TraceSerializer &get_serializer() { return *serializer_; }

    Analyzer &get_analyzer() { return *analyzer_; }

    const std::string &get_output_dirpath() const { return output_dirpath_; }

    int get_compression_level() const { return compression_level_; }

    bool is_binary() const { return binary_; }

    bool is_verbose() const { return verbose_; }

    bool get_truncate() const { return truncate_; }

    const AnalysisSwitch &get_analysis_switch() const {
        return analysis_switch_;
    }

    ~Context() {

        delete analyzer_;
        delete serializer_;
        /* delete state in the end as everything else
           can store reference to the state */
        delete state_;
    }

  private:
    tracer_state_t *state_;
    AnalysisSwitch analysis_switch_;
    TraceSerializer *serializer_;
    Analyzer *analyzer_;
    std::string output_dirpath_;
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

inline Analyzer &tracer_analyzer(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_analyzer();
}

inline const std::string &tracer_output_dirpath(dyntracer_t *dyntracer) {
    return (static_cast<Context *>(dyntracer->state))->get_output_dirpath();
}
#endif /* PROMISEDYNTRACER_CONTEXT_H */
