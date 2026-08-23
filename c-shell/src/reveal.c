#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include "reveal.h"

int compareEntries(const struct dirent **a, const struct dirent **b)
{
    return strcmp((*a)->d_name, (*b)->d_name);
}

int isDirectory(char *path)
{
    struct stat info;

    if(stat(path, &info) == 0 && S_ISDIR(info.st_mode))
        return 1;
    else
        return 0;
}

void makeRevealPath(char *result, char *directory, char *name)
{
    int i = 0;
    int j = 0;

    while(directory[i] != '\0')
    {
        result[i] = directory[i];
        i++;
    }

    if(i > 0 && result[i - 1] != '/' && name[0] != '/')
        result[i++] = '/';

    while(name[j] != '\0')
    {
        result[i] = name[j];
        i++;
        j++;
    }

    result[i] = '\0';
}

int revealFilter(const struct dirent *entry)
{
    if(entry->d_name[0] == '.')
        return 0;
    else
        return 1;
}

int revealFilterHidden(const struct dirent *entry)
{
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        return 0;
    else
        return 1;
}

void revealDirectory(char *path, char *prefix, int show_hidden, int recursive)
{
    struct dirent **entries;

    int count;

    if(show_hidden)
        count = scandir(path, &entries, revealFilterHidden, compareEntries);
    else
        count = scandir(path, &entries, revealFilter, compareEntries);

    if(count == -1)
    {
        printf("reveal: no such directory\n");
        return;
    }

    for(int i = 0; i < count; i++)
    {
        char *name = entries[i]->d_name;
        char full_path[PATH_MAX];
        char display_name[PATH_MAX];

        makeRevealPath(full_path, path, name);
        makeRevealPath(display_name, prefix, name);

        int directory = isDirectory(full_path);

        if(directory)
            printf("%s/\n", display_name);
        else
            printf("%s\n", display_name);

        if(recursive && directory)
        {
            char new_prefix[PATH_MAX];

            makeRevealPath(new_prefix, prefix, name);

            int length = strlen(new_prefix);

            new_prefix[length] = '/';
            new_prefix[length + 1] = '\0';

            revealDirectory(full_path, new_prefix, show_hidden, recursive);
        }
    }

    for(int i = 0; i < count; i++)
        free(entries[i]);

    free(entries);
}

void reveal(Token *tokens, int token_count, ShellState *state)
{
    int show_hidden = 0;
    int recursive = 0;
    char *directory = NULL;

    for(int i = 1; i < token_count; i++)
    {
        char *arg = tokens[i].value;

        if(arg[0] == '-')
        {
            if(strcmp(arg, "-") == 0)
            {
                if(directory != NULL)
                {
                    printf("reveal: invalid syntax\n");
                    return;
                }

                directory = arg;
                continue;
            }

            for(int j = 1; arg[j] != '\0'; j++)
            {
                if(arg[j] == 'a')
                    show_hidden = 1;
                else if(arg[j] == 't')
                    recursive = 1;
                else
                {
                    printf("reveal: invalid syntax\n");
                    return;
                }
            }
        }
        else
        {
            if(directory != NULL)
            {
                printf("reveal: invalid syntax\n");
                return;
            }

            directory = arg;
        }
    }

    char current_path[PATH_MAX];

    if(directory == NULL)
    {
        if(getcwd(current_path, sizeof(current_path)) == NULL)
        {
            printf("reveal: no such directory\n");
            return;
        }
    }
    else if(strcmp(directory, "~") == 0)
    {
        strcpy(current_path, state->home);
    }
    else if(strcmp(directory, ".") == 0)
    {
        if(getcwd(current_path, sizeof(current_path)) == NULL)
        {
            printf("reveal: no such directory\n");
            return;
        }
    }
    else if(strcmp(directory, "-") == 0)
    {
        if(!state->has_previous)
        {
            printf("reveal: no such directory\n");
            return;
        }

        strcpy(current_path, state->previous_dir);
    }
    else
    {
        strcpy(current_path, directory);
    }

    revealDirectory(current_path, "", show_hidden, recursive);
}