#include <stdlib.h>
#include <string.h>
#include "parser.h"

static void freePartial(CommandLine *line)
{
    freeCommandLine(line);
}

static int addArgument(Command *command, const char *value)
{
    char **new_argv = realloc(command->argv,
                               sizeof(char *) * (command->argc + 2));
    if(new_argv == NULL)
        return 0;

    command->argv = new_argv;
    command->argv[command->argc] = strdup(value);
    if(command->argv[command->argc] == NULL)
        return 0;

    command->argc++;
    command->argv[command->argc] = NULL;
    return 1;
}

static int addRedirection(Command *command, RedirectionType type,
                          const char *filename)
{
    Redirection *new_redirections = realloc(
        command->redirections,
        sizeof(Redirection) * (command->redirection_count + 1));
    if(new_redirections == NULL)
        return 0;

    command->redirections = new_redirections;
    Redirection *redirection =
        &command->redirections[command->redirection_count];
    redirection->type = type;
    redirection->filename = strdup(filename);
    if(redirection->filename == NULL)
        return 0;

    command->redirection_count++;
    return 1;
}

static int addCommand(Pipeline *pipeline)
{
    Command *new_commands = realloc(
        pipeline->commands, sizeof(Command) * (pipeline->command_count + 1));
    if(new_commands == NULL)
        return 0;

    pipeline->commands = new_commands;
    Command *command = &pipeline->commands[pipeline->command_count];
    command->argv = NULL;
    command->argc = 0;
    command->redirections = NULL;
    command->redirection_count = 0;
    pipeline->command_count++;
    return 1;
}

static int addPipeline(CommandLine *line)
{
    Pipeline *new_pipelines = realloc(
        line->pipelines, sizeof(Pipeline) * (line->pipeline_count + 1));
    if(new_pipelines == NULL)
        return 0;

    line->pipelines = new_pipelines;
    Pipeline *pipeline = &line->pipelines[line->pipeline_count];
    pipeline->commands = NULL;
    pipeline->command_count = 0;
    pipeline->background = false;
    line->pipeline_count++;
    return 1;
}

CommandLine *parser(Token *tokens, int token_count)
{
    CommandLine *line = calloc(1, sizeof(CommandLine));
    if(line == NULL)
        return NULL;

    if(token_count == 0)
        return line;

    if(!addPipeline(line) || !addCommand(&line->pipelines[0]))
    {
        freePartial(line);
        return NULL;
    }

    size_t pipeline_index = 0;
    size_t command_index = 0;

    for(int i = 0; i < token_count; i++)
    {
        Token *token = &tokens[i];
        Pipeline *pipeline = &line->pipelines[pipeline_index];
        Command *command = &pipeline->commands[command_index];

        if(token->type == TOKEN_WORD)
        {
            if(!addArgument(command, token->value))
                goto invalid;
        }
        else if(token->type == TOKEN_LT || token->type == TOKEN_GT ||
                token->type == TOKEN_GTGT)
        {
            if(command->argc == 0 || i + 1 >= token_count ||
               tokens[i + 1].type != TOKEN_WORD)
                goto invalid;

            RedirectionType type = REDIR_INPUT;
            if(token->type == TOKEN_GT)
                type = REDIR_OUTPUT;
            else if(token->type == TOKEN_GTGT)
                type = REDIR_APPEND;

            if(!addRedirection(command, type, tokens[++i].value))
                goto invalid;
        }
        else if(token->type == TOKEN_PIPE)
        {
            if(command->argc == 0 || i + 1 >= token_count ||
               !addCommand(pipeline))
                goto invalid;
            command_index++;
        }
        else if(token->type == TOKEN_SEMI || token->type == TOKEN_AMP)
        {
            if(command->argc == 0)
                goto invalid;
            pipeline->background = token->type == TOKEN_AMP;

            if(i + 1 < token_count)
            {
                if(!addPipeline(line) ||
                   !addCommand(&line->pipelines[pipeline_index + 1]))
                    goto invalid;
                pipeline_index++;
                command_index = 0;
            }
            else if(token->type == TOKEN_SEMI)
                goto invalid;
        }
    }

    if(line->pipelines[pipeline_index].commands[command_index].argc == 0)
        goto invalid;
    return line;

invalid:
    freePartial(line);
    return NULL;
}
