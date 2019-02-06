#ifndef PROMISEDYNTRACER_R_EXECUTION_CONTEXT
#define PROMISEDYNTRACER_R_EXECUTION_CONTEXT

class RExecutionContext {
  public:
    explicit RExecutionContext(const SEXP environment)
        : environment_(environment) {}

    const SEXP get_environment() const { return environment_; }

  private:
    const SEXP environment_;
};

#endif /* PROMISEDYNTRACER_R_EXECUTION_CONTEXT */
