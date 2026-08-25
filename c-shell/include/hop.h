#ifndef HOP_H
#define HOP_H

#include <limits.h>

#define MAX_FRECENCY_ENTRIES 512

typedef enum {
    HOP_HOME,
    HOP_CURRENT,
    HOP_PARENT,
    HOP_PREVIOUS,
    HOP_PATH
} HopType;

typedef struct {
    char *value;
} Token;

typedef struct {
    char path[PATH_MAX];
    double score;
    long last_access;
} FrecencyEntry;

typedef struct {
    char home[PATH_MAX];
    char previous_dir[PATH_MAX];
    int has_previous;

    FrecencyEntry frecency[MAX_FRECENCY_ENTRIES];
    int frecency_count;
} ShellState;

void initState(ShellState *state);
HopType getHopType(char *arg);
void goToDirectory(char *path, ShellState *state);
void hop(Token *tokens, int token_count, ShellState *state);

void frecencyTouch(ShellState *state, char *path);
char *frecencyBestMatch(ShellState *state, char *substr);

#endif