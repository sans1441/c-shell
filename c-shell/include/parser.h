#ifndef PARSER_H
#define PARSER_H

#include "command.h"
#include "lexer.h"

enum State { LINE, ARG, CMD, TGT, BG };

CommandLine *parser(Token *tokens, int token_count);

#endif
