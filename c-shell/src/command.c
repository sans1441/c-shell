#include "command.h"
#include <stdlib.h>

void freeCommandLine(CommandLine *line) {
    if (line == NULL) return;

    for (size_t i = 0; i < line->pipeline_count; i++) {
        Pipeline *pipeline = &line->pipelines[i];

        for (size_t j = 0; j < pipeline->command_count; j++) {
            Command *command = &pipeline->commands[j];

            for (size_t k = 0; k < command->argc; k++) free(command->argv[k]);
            free(command->argv);

            for (size_t k = 0; k < command->redirection_count; k++) free(command->redirections[k].filename);
            free(command->redirections);
        }
        free(pipeline->commands);
    }

    free(line->pipelines);
    free(line);
}
