#ifndef TURBOTRACER_CALL_H
#define TURBOTRACER_CALL_H

#include "Argument.h"
#include "Rdyntrace.h"
#include "Rinternals.h"
#include "table.h"
#include "utilities.h"

class Function;

typedef std::vector<int> force_order_t;

class Call {
  public:
    /* defined in cpp file to get around cyclic dependency issues. */
    explicit Call(const call_id_t id,
                  const std::string& function_name,
                  const SEXP environment,
                  Function* function,
                  const SEXP args);

    call_id_t get_id() const {
        return id_;
    }
    
    void process_calls_affecting_lookup();
    
    SEXP get_args();

    const std::string& get_function_name() const;

    Function* get_function();

    void set_actual_argument_count(int actual_argument_count);

    int get_actual_argument_count() const;

    void set_call_as_arg(int nr);

    int get_call_as_arg() const;

    SEXP get_environment() const;

    void set_return_value_type(sexptype_t return_value_type);

    sexptype_t get_return_value_type() const;

    void set_jumped();

    bool is_jumped() const;

    void set_S3_method();

    bool is_S3_method() const;

    void set_S4_method();

    bool is_S4_method() const;
    
    bool is_loaded_midway() const;
    
    void set_load_midway(bool state); 

    void analyze_callee(Call* callee);

    bool is_wrapper() const;

    std::vector<Argument*>& get_arguments();

    Argument* get_argument(int actual_argument_position) const;

    void add_argument(Argument* argument);

    const pos_seq_t& get_force_order() const;

    void set_force_order(int force_order);

    void add_to_force_order(int formal_parameter_position);

    pos_seq_t get_missing_argument_positions() const;

  private:
    const call_id_t id_;
    const std::string function_name_;
    int actual_argument_count_;
    const SEXP environment_;
    const SEXP args_;
    Function* function_;
    sexptype_t return_value_type_;
    bool jumped_;
    bool S3_method_;
    bool S4_method_;
    bool load_midway_;
    int callee_counter_;
    int call_as_arg_;
    bool wrapper_;
    std::vector<Argument*> arguments_;
    pos_seq_t force_order_;
};

#endif /* TURBOTRACER_CALL_H */
