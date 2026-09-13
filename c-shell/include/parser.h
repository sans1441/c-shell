#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "command.h"

enum State
{
    LINE,
    ARG,
    CMD,
    TGT,
    BG
};

CommandLine *parser(Token *tokens, int token_count);

#endif
