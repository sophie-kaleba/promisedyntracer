#include "constants.h"

/* https://stackoverflow.com/questions/8206387/using-non-printable-characters-as-a-delimiter-in-php
 */
const char RECORD_SEPARATOR = 0x1e;
const char UNIT_SEPARATOR = 0x1f;

const eval_depth_t ESCAPED_PROMISE_EVAL_DEPTH = {-2, -2, -2, -1};
const eval_depth_t UNASSIGNED_PROMISE_EVAL_DEPTH = {-1, -1, -1, -1};

const size_t PROMISE_MAPPING_BUCKET_COUNT = 1000000;
const size_t FUNCTION_MAPPING_BUCKET_SIZE = 20000;


const std::vector<std::string> ENVIRONMENT_VARIABLES{
    "R_COMPILE_PKGS", "R_DISABLE_BYTECODE", "R_ENABLE_JIT",
    "R_KEEP_PKG_SOURCE"};

const timestamp_t UNDEFINED_TIMESTAMP = -1;

const denoted_value_id_t UNASSIGNED_DENOTED_VALUE_ID = -1;

const function_id_t UNASSIGNED_FUNCTION_ID = "";

const std::string UNASSIGNED_CLASS_NAME = "";

const call_id_t UNASSIGNED_CALL_ID = -1;

const int UNASSIGNED_FORMAL_PARAMETER_POSITION = -1;

const int UNASSIGNED_ACTUAL_ARGUMENT_POSITION = -1;

const int UNASSIGNED_FORMAL_PARAMETER_COUNT = -1;

const int PRIMITIVE_RETURN_OFFSET = 6;
const int PRIMITIVE_INTERNAL_OFFSET = 26;
const int PRIMITIVE_PRIMITIVE_OFFSET = 27;

const unsigned int OBJECT_TYPE_TABLE_COUNT = 100;
