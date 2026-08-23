#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include "lexer.h"
#include "parser.h"
#include "prompt.h"
#include "test.h"
#include "hop.h"
#include "locate.h"
#include "reveal.h"

int main()
{
    getHomeShell();

    ShellState state;
    initState(&state);

    while(1)
    {
        printPath();

        char *line = readLine();

        if(line == NULL)
        {
            printf("\n");
            break;
        }

        int token_count = 0;
        Token *tokens = lexer(line, &token_count);

        if(token_count == -1)
        {
            free(line);
            continue;
        }

        int valid = parser(tokens, token_count);

        if(!valid)
            printf("cshell: invalid syntax\n");
        else if(token_count > 0 && strcmp(tokens[0].value, "hop") == 0)
            hop(tokens, token_count, &state);
        else if(token_count > 0 && strcmp(tokens[0].value, "locate") == 0)
            locate(tokens, token_count);
        else if(token_count > 0 && strcmp(tokens[0].value, "reveal") == 0)
            reveal(tokens, token_count, &state);

        freeTokens(tokens, token_count);
        free(line);
    }

    return 0;
}