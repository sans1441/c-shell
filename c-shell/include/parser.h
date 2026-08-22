#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

enum State
{
    LINE,
    ARG,
    CMD,
    TGT,
    BG
};

int parser(Token *tokens, int token_count);

#endif