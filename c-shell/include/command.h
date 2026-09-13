#ifndef COMMAND_H
#define COMMAND_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { REDIR_INPUT, REDIR_OUTPUT, REDIR_APPEND } RedirectionType;

typedef struct {
    RedirectionType type;
    char *filename;
} Redirection;

typedef struct {
    char **argv;
    size_t argc;
    Redirection *redirections;
    size_t redirection_count;
} Command;

typedef struct {
    Command *commands;
    size_t command_count;
    bool background;
} Pipeline;

typedef struct {
    Pipeline *pipelines;
    size_t pipeline_count;
} CommandLine;

void freeCommandLine(CommandLine *line);

#endif
