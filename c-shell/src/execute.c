#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "execute.h"

void runCommand(char **args, int argument_count, char *original, int path_only)
{
    args[argument_count] = NULL;

    int has_slash = 0;

    for(int i = 0; args[0][i] != '\0'; i++)
    {
        if(args[0][i] == '/')
        {
            has_slash = 1;
            break;
        }
    }

    if(has_slash)
    {
        execv(args[0], args);
    }
    else if(path_only)
    {
        execvp(args[0], args);
    }
    else
    {
        char path[PATH_MAX];
        int i = 0;
        int j = 0;

        path[i++] = '.';
        path[i++] = '/';

        while(args[0][j] != '\0')
        {
            path[i] = args[0][j];
            i++;
            j++;
        }

        path[i] = '\0';

        if(access(path, X_OK) == 0)
            execv(path, args);

        execvp(args[0], args);
    }

    printf("cshell: command not found (%s)\n", original);
    exit(1);
}

void executeCommand(Token *tokens, int token_count)
{
    char *args[token_count + 1];
    int argument_count = 0;

    for(int i = 0; i < token_count; i++)
    {
        if(tokens[i].type == TOKEN_LT ||
           tokens[i].type == TOKEN_GT ||
           tokens[i].type == TOKEN_GTGT ||
           tokens[i].type == TOKEN_PIPE)
            break;

        args[argument_count] = tokens[i].value;
        argument_count++;
    }

    if(argument_count == 0)
        return;

    char *original = args[0];
    int path_only = 0;

    if(args[0][0] == '%')
    {
        path_only = 1;
        args[0]++;
    }

    int pid = fork();

    if(pid == 0)
        runCommand(args, argument_count, original, path_only);
    else if(pid > 0)
        wait(NULL);
}