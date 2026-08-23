#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include "locate.h"

int isExecutable(char *path)
{
    struct stat info;

    if(stat(path, &info) != 0 || !S_ISREG(info.st_mode) || access(path, X_OK) != 0)
        return 0;

    else
        return 1;
}

void makePath(char *path, char *directory, char *filename)
{
    int i = 0;
    int j = 0;

    while(directory[i] != '\0')
    {
        path[i] = directory[i];
        i++;
    }

    path[i++] = '/';

    while(filename[j] != '\0')
    {
        path[i] = filename[j];
        i++;
        j++;
    }

    path[i] = '\0';
}

int checkDirectory(char *directory, char *filename)
{
    char full_path[PATH_MAX];

    makePath(full_path, directory, filename);

    if(isExecutable(full_path))
    {
        printf("%s\n", full_path);
        return 1;
    }

    return 0;
}

int checkCurrentDirectory(char *filename)
{
    char cwd[PATH_MAX];

    if(getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;

    return checkDirectory(cwd, filename);
}

int checkPath(char *filename)
{
    char *path = getenv("PATH");

    if(path == NULL)
        return 0;

    int found = 0;
    int start = 0;
    int i = 0;

    while(1)
    {
        if(path[i] == ':' || path[i] == '\0')
        {
            char directory[PATH_MAX];
            int j = 0;
            int k = start;

            while(k < i)
            {
                directory[j] = path[k];
                j++;
                k++;
            }

            directory[j] = '\0';

            if(checkDirectory(directory, filename))
                found = 1;

            if(path[i] == '\0')
                break;

            start = i + 1;
        }

        i++;
    }

    return found;
}

void locate(Token *tokens, int token_count)
{
    if(token_count < 2)
    {
        printf("locate: invalid syntax\n");
        return;
    }

    for(int i = 1; i < token_count; i++)
    {
        char *filename = tokens[i].value;

        int found = 0;

        int res1 = checkCurrentDirectory(filename);
        int res2 = checkPath(filename);

        if(res1!=0 || res2!=0)
            found = 1;

        if(!found)
            printf("locate: command not found (%s)\n", filename);
    }
}