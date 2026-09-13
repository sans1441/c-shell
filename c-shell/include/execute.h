#ifndef EXECUTE_H
#define EXECUTE_H

#include "command.h"
#include "hop.h"

typedef enum { EXECUTION_OK, EXECUTION_LAUNCH_FAILED } ExecutionResult;

void executeLine(CommandLine *line, ShellState *state);

#endif
