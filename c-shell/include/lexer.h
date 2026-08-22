#ifndef LEXER_H
#define LEXER_H

typedef enum
{
    TOKEN_WORD,
    TOKEN_PIPE,
    TOKEN_SEMI,
    TOKEN_AMP,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_GTGT
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
} Token;

Token *lexer(char *input, int *token_count);
void freeTokens(Token *tokens, int count);

#endif