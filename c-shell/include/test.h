#ifndef TEST_H
#define TEST_H

#include "lexer.h"

const char *tokenTypeName(TokenType type);

void printTokens(Token *tokens, int count);

#endif