#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <limits.h>
#include "lexer.h"

#define INITIAL_CAPACITY 4096
#define BUFFER_CAPACITY 4096

void freeTokens(Token *tokens, int count)
{
    for(int i = 0; i < count; i++) free(tokens[i].value);
    free(tokens);
    return;
}

int is_special(char c)
{
    return c == '|' || c == '&' || c == '>' || c == '<' || c == ';';
}

int is_space(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char *readWord(const char *input, int *i, int *error)
{
    int length = 0;
    char *word = malloc(BUFFER_CAPACITY);
    *error = 0;

    while(input[*i] != '\0' && !is_space(input[*i]) && !is_special(input[*i]))
    {
        char current = input[*i];

        if(current == '\\')
        {
            (*i)++;
            if(input[*i] == '\0')
            {
                *error = 1;
                free(word);
                return NULL;
            }
            word[length++] = input[*i];
            (*i)++;
        }
        else if(current == '"')
        {
            (*i)++;
            int terminated = 0;

            while(input[*i] != '\0')
            {
                if(input[*i] == '"')
                {
                    terminated = 1;
                    (*i)++;
                    break;
                }

                if(input[*i] == '\\')
                {
                    (*i)++;
                    if(input[*i] == '\0')
                    {
                        *error = 1;
                        free(word);
                        return NULL;
                    }

                    else if(input[*i] == '"')
                    {
                        word[length++] = '"';
                    }
                    else if(input[*i] == '\\')
                    {
                        word[length++] = '\\';
                    }
                    else
                    {
                        word[length++] = '\\';
                        word[length++] = input[*i];
                    }
                    (*i)++;
                }
                else
                {
                    word[length++] = input[*i];
                    (*i)++;
                }
            }

            if(!terminated)
            {
                *error = 1;
                free(word);
                return NULL;
            }
        }
        else if(current == '\'')
        {
            (*i)++;
            int terminated = 0;

            while(input[*i] != '\0')
            {
                if(input[*i] == '\'')
                {
                    terminated = 1;
                    (*i)++;
                    break;
                }
                word[length++] = input[*i];
                (*i)++;
            }

            if(!terminated)
            {
                *error = 1;
                free(word);
                return NULL;
            }
        }
        else
        {
            word[length++] = current;
            (*i)++;
        }

        if(length >= BUFFER_CAPACITY - 2)
            break;
    }

    word[length] = '\0';
    return word;
}

char *copyString(const char *s)
{
    char *copy = malloc(strlen(s) + 1);
    strcpy(copy, s);
    return copy;
}

Token *lexer(char *input, int *token_count)
{
    int cap = INITIAL_CAPACITY;
    Token *tokens = malloc(sizeof(Token) * cap);
    int count = 0;
    int i = 0;

    while(input[i] != '\0')
    {
        if(is_space(input[i]))
        {
            i++;
            continue;
        }

        if(input[i] == '>' && input[i + 1] == '>')
        {
            tokens[count].type = TOKEN_GTGT;
            tokens[count].value = copyString(">>");
            count++;
            i += 2;
        }
        else if(input[i] == '|')
        {
            tokens[count].type = TOKEN_PIPE;
            tokens[count].value = copyString("|");
            count++;
            i++;
        }
        else if(input[i] == '&')
        {
            tokens[count].type = TOKEN_AMP;
            tokens[count].value = copyString("&");
            count++;
            i++;
        }
        else if(input[i] == ';')
        {
            tokens[count].type = TOKEN_SEMI;
            tokens[count].value = copyString(";");
            count++;
            i++;
        }
        else if(input[i] == '<')
        {
            tokens[count].type = TOKEN_LT;
            tokens[count].value = copyString("<");
            count++;
            i++;
        }
        else if(input[i] == '>')
        {
            tokens[count].type = TOKEN_GT;
            tokens[count].value = copyString(">");
            count++;
            i++;
        }
        else
        {
            int error = 0;
            char *word = readWord(input, &i, &error);
            if(error)
            {
                freeTokens(tokens, count);
                printf("cshell: invalid syntax\n");
                *token_count = -1;
                return NULL;
            }
            tokens[count].type = TOKEN_WORD;
            tokens[count].value = word;
            count++;
        }
    }

    *token_count = count;
    return tokens;
}