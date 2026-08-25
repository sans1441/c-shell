#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "hop.h"

#define MAX_FRECENCY 1024

typedef struct
{
    char path[PATH_MAX];
    int score;
    time_t last_access;
} FrecencyEntry;

FrecencyEntry frecency[MAX_FRECENCY];
int frecency_count = 0;

void frecencyFilePath(ShellState *state, char *buf, size_t size)
{
    snprintf(buf, size, "%s/.hop_frecency", state->home);
}

void loadFrecency(ShellState *state)
{
    char file[PATH_MAX];
    frecencyFilePath(state, file, sizeof(file));

    FILE *fp = fopen(file, "r");
    if(!fp) return;

    frecency_count = 0;

    while(frecency_count < MAX_FRECENCY)
    {
        int score;
        long last_access;
        char path[PATH_MAX];

        int result = fscanf(fp, "%d %ld %[^\n]\n", &score, &last_access, path);

        if(result != 3) break;

        frecency[frecency_count].score = score;
        frecency[frecency_count].last_access = last_access;
        strcpy(frecency[frecency_count].path, path);

        frecency_count++;
    }

    fclose(fp);
}

void saveFrecency(ShellState *state)
{
    char file[PATH_MAX];
    frecencyFilePath(state, file, sizeof(file));

    FILE *fp = fopen(file, "w");
    if(!fp) return;

    for(int i = 0; i < frecency_count; i++)
    {
        int score = frecency[i].score;
        long last_access = (long)frecency[i].last_access;
        char *path = frecency[i].path;

        fprintf(fp, "%d %ld %s\n", score, last_access, path);
    }

    fclose(fp);
}

void updateFrecency(char *path, ShellState *state)
{
    loadFrecency(state);

    for(int i = 0; i < frecency_count; i++)
    {
        if(strcmp(frecency[i].path, path) == 0)
        {
            frecency[i].score++;
            frecency[i].last_access = time(NULL);
            saveFrecency(state);
            return;
        }
    }

    if(frecency_count < MAX_FRECENCY)
    {
        strcpy(frecency[frecency_count].path, path);
        frecency[frecency_count].score = 1;
        frecency[frecency_count].last_access = time(NULL);
        frecency_count++;
        saveFrecency(state);
    }
}

double frecencyScore(FrecencyEntry *e)
{
    double diff = difftime(time(NULL), e->last_access);

    if(diff < 3600) return e->score * 4;
    if(diff < 86400) return e->score * 2;
    if(diff < 604800) return e->score / 2.0;
    return e->score / 4.0;
}

int findFrecencyMatch(char *name, ShellState *state, char *result, size_t size)
{
    loadFrecency(state);

    double best = -1;
    int found = 0;

    for(int i = 0; i < frecency_count; i++)
    {
        if(strstr(frecency[i].path, name))
        {
            double sc = frecencyScore(&frecency[i]);
            if(sc > best)
            {
                best = sc;
                strncpy(result, frecency[i].path, size);
                found = 1;
            }
        }
    }

    return found;
}

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

    char newcwd[PATH_MAX];
    if(getcwd(newcwd, sizeof(newcwd)))
        updateFrecency(newcwd, state);
}

void hopToName(char *arg, ShellState *state)
{
    char current_dir[PATH_MAX];
    char *cdir = getcwd(current_dir, sizeof(current_dir));
    if(!cdir) return;

    if(chdir(arg) == 0)
    {
        strcpy(state->previous_dir, current_dir);
        state->has_previous = 1;

        char newcwd[PATH_MAX];
        if(getcwd(newcwd, sizeof(newcwd)))
            updateFrecency(newcwd, state);
        return;
    }

    char match[PATH_MAX];
    if(findFrecencyMatch(arg, state, match, sizeof(match)) && chdir(match) == 0)
    {
        strcpy(state->previous_dir, current_dir);
        state->has_previous = 1;
        updateFrecency(match, state);
        return;
    }

    printf("hop: no such directory\n");
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
                hopToName(arg, state);
                break;
        }
    }
}