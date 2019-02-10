#ifndef PROMISEDYNTRACER_CONSTANTS_H
#define PROMISEDYNTRACER_CONSTANTS_H

#include "definitions.h"
#include <string>
#include <vector>

extern const char UNIT_SEPARATOR;
extern const char RECORD_SEPARATOR;

extern const eval_depth_t ESCAPED_PROMISE_EVAL_DEPTH;
extern const eval_depth_t UNASSIGNED_PROMISE_EVAL_DEPTH;

extern const std::size_t PROMISE_MAPPING_BUCKET_COUNT;
extern const std::size_t FUNCTION_MAPPING_BUCKET_SIZE;

extern const std::vector<std::string> ENVIRONMENT_VARIABLES;

extern const timestamp_t UNDEFINED_TIMESTAMP;

extern const promise_id_t UNASSIGNED_DENOTED_VALUE_ID;


#endif /* PROMISEDYNTRACER_CONSTANTS_H */

