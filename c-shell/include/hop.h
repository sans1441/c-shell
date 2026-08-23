#ifndef HOP_H
#define HOP_H

#include <limits.h>
#include "lexer.h"

typedef enum
{
    HOP_HOME,
    HOP_CURRENT,
    HOP_PARENT,
    HOP_PREVIOUS,
    HOP_PATH
} HopType;

typedef struct
{
    char home[PATH_MAX];
    char previous_dir[PATH_MAX];
    int has_previous;
} ShellState;

void initState(ShellState *state);
HopType getHopType(char *arg);
void goToDirectory(char *path, ShellState *state);
void hop(Token *tokens, int token_count, ShellState *state);

#endif