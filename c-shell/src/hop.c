#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "hop.h"

void initState(ShellState *state)
{
    getcwd(state->home, sizeof(state->home));

    state->previous_dir[0] = '\0';
    state->has_previous = 0;

    state->frecency_count = 0;
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

void frecencyTouch(ShellState *state, char *path)
{
    time_t now = time(NULL);

    for(int i = 0; i < state->frecency_count; i++)
    {
        if(strcmp(state->frecency[i].path, path) == 0)
        {
            state->frecency[i].score += 1;
            state->frecency[i].last_access = (long)now;
            return;
        }
    }

    if(state->frecency_count >= MAX_FRECENCY_ENTRIES)
        return;

    FrecencyEntry *e = &state->frecency[state->frecency_count++];
    strcpy(e->path, path);
    e->score = 1;
    e->last_access = (long)now;
}

double frecencyWeight(FrecencyEntry *e, time_t now)
{
    double age = difftime(now, (time_t)e->last_access);

    if(age < 3600)
        return e->score * 4;
    if(age < 86400)
        return e->score * 2;
    if(age < 604800)
        return e->score / 2;

    return e->score / 4;
}

int pathExists(char *path)
{
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

char *frecencyBestMatch(ShellState *state, char *substr)
{
    time_t now = time(NULL);
    int excluded[MAX_FRECENCY_ENTRIES] = {0};

    for(int pass = 0; pass < state->frecency_count; pass++)
    {
        int best_idx = -1;
        double best_weight = -1;

        for(int i = 0; i < state->frecency_count; i++)
        {
            if(excluded[i]) continue;
            if(!strstr(state->frecency[i].path, substr)) continue;

            double w = frecencyWeight(&state->frecency[i], now);
            if(w > best_weight)
            {
                best_weight = w;
                best_idx = i;
            }
        }

        if(best_idx == -1)
            break;

        if(pathExists(state->frecency[best_idx].path))
            return state->frecency[best_idx].path;

        excluded[best_idx] = 1;
    }

    return NULL;
}

void goToDirectory(char *path, ShellState *state)
{
    char current_dir[PATH_MAX];
    char *cdir = getcwd(current_dir, sizeof(current_dir));
    if(!cdir) return;

    int res = chdir(path);
    if(res != 0)
    {
        char *match = frecencyBestMatch(state, path);
        if(match)
            res = chdir(match);

        if(res != 0)
        {
            printf("hop: no such directory\n");
            return;
        }
    }

    strcpy(state->previous_dir, current_dir);
    state->has_previous = 1;

    char new_dir[PATH_MAX];
    if(getcwd(new_dir, sizeof(new_dir)))
        frecencyTouch(state, new_dir);
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