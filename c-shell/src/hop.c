#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "hop.h"

void initState(ShellState *state)
{
    getcwd(state->home, sizeof(state->home));

    state->previous_dir[0] = '\0';
    state->has_previous = 0;
}

HopType getHopType(char *arg)
{
    if(strcmp(arg, "~") == 0)
        return HOP_HOME;

    if(strcmp(arg, ".") == 0)
        return HOP_CURRENT;

    if(strcmp(arg, "..") == 0)
        return HOP_PARENT;

    if(strcmp(arg, "-") == 0)
        return HOP_PREVIOUS;

    return HOP_PATH;
}

void goToDirectory(char *path, ShellState *state)
{
    char current_dir[PATH_MAX];
    char *cdir = getcwd(current_dir, sizeof(current_dir));
    if(!cdir) return;
    int res = chdir(path);
    if(res != 0)
    {
        printf("hop: no such directory\n");
        return;
    }

    strcpy(state->previous_dir, current_dir);
    state->has_previous = 1;
}

void hop(Token *tokens, int token_count, ShellState *state)
{
    if(token_count == 1)
    {
        goToDirectory(state->home, state);
        return;
    }

    for(int i = 1; i < token_count; i++)
    {
        char *arg = tokens[i].value;

        switch(getHopType(arg))
        {
            case HOP_HOME:
                goToDirectory(state->home, state);
                break;

            case HOP_CURRENT:
                break;

            case HOP_PARENT:
                goToDirectory("..", state);
                break;

            case HOP_PREVIOUS:
                if(state->has_previous)
                    goToDirectory(state->previous_dir, state);
                break;

            case HOP_PATH:
                goToDirectory(arg, state);
                break;
        }
    }
}