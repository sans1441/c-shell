#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "peek.h"

void peekNormal(char *filename)
{
    FILE *file = fopen(filename, "r");

    if(file == NULL)
    {
        printf("peek: no such file or directory\n");
        return;
    }

    char line[1024];

    while(fgets(line, sizeof(line), file) != NULL)
        printf("%s", line);

    fclose(file);
}

void peekNormalNumbered(char *filename)
{
    FILE *file = fopen(filename, "r");

    if(file == NULL)
    {
        printf("peek: no such file or directory\n");
        return;
    }

    char line[1024];
    int line_number = 1;

    while(fgets(line, sizeof(line), file) != NULL)
    {
        if(line[0] != '\n')
        {
            printf("%d %s", line_number, line);
            line_number++;
        }
    }

    fclose(file);
}

void peekReverse(char *filename)
{
    FILE *file = fopen(filename, "r");

    if(file == NULL)
    {
        printf("peek: no such file or directory\n");
        return;
    }

    char line[1024];
    char **lines = NULL;
    int count = 0;

    while(fgets(line, sizeof(line), file) != NULL)
    {
        lines = realloc(lines, (count + 1) * sizeof(char *));

        if(lines == NULL)
        {
            fclose(file);
            return;
        }

        lines[count] = malloc(strlen(line) + 1);

        if(lines[count] == NULL)
        {
            fclose(file);
            return;
        }

        strcpy(lines[count], line);
        count++;
    }

    for(int i = count - 1; i >= 0; i--)
        printf("%s", lines[i]);

    for(int i = 0; i < count; i++)
        free(lines[i]);

    free(lines);
    fclose(file);
}

void peekReverseNumbered(char *filename)
{
    FILE *file = fopen(filename, "r");

    if(file == NULL)
    {
        printf("peek: no such file or directory\n");
        return;
    }

    char line[1024];
    char **lines = NULL;
    int count = 0;

    while(fgets(line, sizeof(line), file) != NULL)
    {
        if(line[0] != '\n')
        {
            lines = realloc(lines, (count + 1) * sizeof(char *));

            if(lines == NULL)
            {
                fclose(file);
                return;
            }

            lines[count] = malloc(strlen(line) + 1);

            if(lines[count] == NULL)
            {
                fclose(file);
                return;
            }

            strcpy(lines[count], line);
            count++;
        }
    }

    for(int i = count - 1; i >= 0; i--)
        printf("%d %s", i + 1, lines[i]);

    for(int i = 0; i < count; i++)
        free(lines[i]);

    free(lines);
    fclose(file);
}

void peek(Token *tokens, int token_count)
{
    int numbered = 0;
    int reversed = 0;
    int reachedFiles = 0;

    for(int i = 1; i < token_count; i++)
    {
        char *arg = tokens[i].value;

        if(arg[0] == '-' && strcmp(arg, "-") != 0 && reachedFiles == 0)
        {
            for(int j = 1; arg[j] != '\0'; j++)
            {
                if(arg[j] == 'n')
                    numbered = 1;
                else if(arg[j] == 'r')
                    reversed = 1;
                else
                {
                    printf("peek: invalid syntax\n");
                    return;
                }
            }
        }
        else
        {
            reachedFiles = 1;

            struct stat info;

            if(strcmp(arg, "-") != 0)
            {
                if(stat(arg, &info) != 0)
                {
                    printf("peek: no such file or directory\n");
                    continue;
                }

                if(S_ISDIR(info.st_mode))
                {
                    printf("peek: is a directory\n");
                    continue;
                }
            }

            if(numbered && reversed)
                peekReverseNumbered(arg);
            else if(numbered)
                peekNormalNumbered(arg);
            else if(reversed)
                peekReverse(arg);
            else
                peekNormal(arg);
        }
    }

    if(!reachedFiles)
    {
        if(numbered && reversed)
            peekReverseNumbered("-");
        else if(numbered)
            peekNormalNumbered("-");
        else if(reversed)
            peekReverse("-");
        else
            peekNormal("-");
    }
}