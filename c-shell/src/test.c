#include <stdio.h>
#include "test.h"

const char *tokenTypeName(TokenType type)
{
    if(type == TOKEN_WORD) return "WORD";
    else if(type == TOKEN_PIPE) return "PIPE";
    else if(type == TOKEN_SEMI) return "SEMI";
    else if(type == TOKEN_AMP) return "AMP";
    else if(type == TOKEN_LT) return "LT";
    else if(type == TOKEN_GT) return "GT";
    else if(type == TOKEN_GTGT) return "GTGT";
    else return "UNKNOWN";
}

void printTokens(Token *tokens, int count)
{
    for(int i = 0; i < count; i++)
    {
        printf("[%s: \"%s\"]\n", tokenTypeName(tokens[i].type), tokens[i].value);
    }
}