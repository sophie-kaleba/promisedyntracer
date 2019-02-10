#ifndef PROMISEDYNTRACER_DEFINITIONS_H
#define PROMISEDYNTRACER_DEFINITIONS_H

#include <string>

typedef long int call_id_t;
typedef std::string function_id_t;

typedef int env_id_t;
typedef int var_id_t;

typedef long long int timestamp_t;
typedef long int promise_id_t;

struct eval_depth_t {
    int call_depth;
    int promise_depth;
    int nested_promise_depth;
};

#endif /* PROMISEDYNTRACER_DEFINITIONS_H */
