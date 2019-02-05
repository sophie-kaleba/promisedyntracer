#include "Context.h"
#include "State.h"
#include "lookup.h"
#include "utilities.h"
#include <cassert>
#include <sstream>

std::string get_function_definition(dyntracer_t *dyntracer, const SEXP function) {
    auto &definitions = tracer_state(dyntracer).function_definitions;
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


fn_id_t get_function_id(dyntracer_t *dyntracer,
                        const std::string &function_definition, bool builtin) {
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

bool register_inserted_function(dyntracer_t *dyntracer, fn_id_t id) {
    auto &already_inserted_functions =
        tracer_state(dyntracer).already_inserted_functions;
    auto result = already_inserted_functions.insert(id);
    return result.second;
}

bool function_already_inserted(dyntracer_t *dyntracer, fn_id_t id) {
    auto &already_inserted_functions =
        tracer_state(dyntracer).already_inserted_functions;
    return already_inserted_functions.count(id) > 0;
}

call_id_t make_funcall_id(dyntracer_t *dyntracer, SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++tracer_state(dyntracer).call_id_counter;
}


arg_id_t get_argument_id(dyntracer_t *dyntracer, call_id_t call_id,
                         const std::string &argument) {
    arg_id_t argument_id = ++tracer_state(dyntracer).argument_id_sequence;
    return argument_id;
}
