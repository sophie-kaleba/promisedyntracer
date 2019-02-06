#ifndef __RECORDER_H__
#define __RECORDER_H__

#include "Context.h"
#include "State.h"
#include "sexptypes.h"
#include "utilities.h"

void write_environment_variables(const std::string &filepath);

void write_configuration(const Context &context, const std::string &filepath);

#endif /* __RECORDER_H__ */
