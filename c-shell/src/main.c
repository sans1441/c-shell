#include "execute.h"
#include "hop.h"
#include "jobs.h"
#include "lexer.h"
#include "locate.h"
#include "parser.h"
#include "peek.h"
#include "prompt.h"
#include "reveal.h"
#include "test.h"
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    getHomeShell();

    ShellState state;
    initState(&state);
    jobsInit();
    jobsSetPromptCallback(printPath);

    while (1) {
        printPath();

        char *line = readLine();

        if (line == NULL) {
            printf("\n");
            break;
        }

        int token_count = 0;
        Token *tokens = lexer(line, &token_count);

        if (token_count == -1) {
            free(line);
            continue;
        }

        CommandLine *command_line = parser(tokens, token_count);

        if (command_line == NULL)
            printf("cshell: invalid syntax\n");
        else
            executeLine(command_line, &state);

        freeCommandLine(command_line);
        freeTokens(tokens, token_count);
        free(line);
    }

    return 0;
}
