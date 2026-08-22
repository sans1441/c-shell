#include <stdio.h>
#include "parser.h"

int parser(Token *tokens, int token_count)
{
    int i = 0;
    int state = LINE;

    while(1)
    {
        switch(state)
        {
            case LINE:
                if(i == token_count)
                    return 1;

                if(tokens[i].type == TOKEN_WORD)
                {
                    state = ARG;
                    i++;
                }
                else
                    return 0;

                break;

            case ARG:
                if(i == token_count)
                    return 1;

                switch(tokens[i].type)
                {
                    case TOKEN_WORD:
                        state = ARG;
                        i++;
                        break;

                    case TOKEN_LT:
                    case TOKEN_GT:
                    case TOKEN_GTGT:
                        state = TGT;
                        i++;
                        break;

                    case TOKEN_PIPE:
                    case TOKEN_SEMI:
                        state = CMD;
                        i++;
                        break;

                    case TOKEN_AMP:
                        state = BG;
                        i++;
                        break;

                    default:
                        return 0;
                }

                break;

            case CMD:
                if(i == token_count)
                    return 0;

                if(tokens[i].type == TOKEN_WORD)
                {
                    state = ARG;
                    i++;
                }
                else
                    return 0;

                break;

            case TGT:
                if(i == token_count)
                    return 0;

                if(tokens[i].type == TOKEN_WORD)
                {
                    state = ARG;
                    i++;
                }
                else
                    return 0;

                break;

            case BG:
                if(i == token_count)
                    return 1;

                if(tokens[i].type == TOKEN_WORD)
                {
                    state = ARG;
                    i++;
                }
                else
                    return 0;

                break;
        }
    }
}