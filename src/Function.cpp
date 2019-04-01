#include "Function.h"

std::pair<std::string, function_id_t>
Function::compute_definition_and_id(const SEXP op) {
    std::string definition;
    function_id_t id;

    if (type_of_sexp(op) == CLOSXP) {
        definition = serialize_r_expression(op);
        id = compute_hash(definition.c_str());
    } else {
        definition = "function body not extracted for non closures";
        id = dyntrace_get_c_function_name(op);
    }

    return make_pair(definition, id);
}
