#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <limits.h>
#include "execute.h"

void runCommand(char **args, int argument_count, char *original, int path_only)
{
    args[argument_count] = NULL;

    int has_slash = 0;

    for(int i = 0; args[0][i] != '\0'; i++)
    {
        if(args[0][i] == '/')
        {
            has_slash = 1;
            break;
        }
    }

    if(has_slash)
    {
        execv(args[0], args);
    }
    else if(path_only)
    {
        execvp(args[0], args);
    }
    else
    {
        char path[PATH_MAX];
        int i = 0;
        int j = 0;

        path[i++] = '.';
        path[i++] = '/';

        while(args[0][j] != '\0')
        {
            path[i] = args[0][j];
            i++;
            j++;
        }

        path[i] = '\0';

        if(access(path, X_OK) == 0)
            execv(path, args);

        execvp(args[0], args);
    }

    printf("cshell: command not found (%s)\n", original);
    exit(1);
}

void copyFileToPipe(int fd, int pipe_write)
{
    char buffer[4096];
    int bytes_read;
    int bytes_written;
    int result;

    while(1)
    {
        bytes_read = read(fd, buffer, sizeof(buffer));

        if(bytes_read <= 0)
            break;

        bytes_written = 0;

        while(bytes_written < bytes_read)
        {
            result = write(pipe_write, buffer + bytes_written, bytes_read - bytes_written);

            if(result <= 0)
                return;

            bytes_written += result;
        }
    }
}

void copyPipeToFiles(int pipe_read, int *output_fds, int output_count)
{
    char buffer[4096];
    int bytes_read;

    while(1)
    {
        bytes_read = read(pipe_read, buffer, sizeof(buffer));

        if(bytes_read <= 0)
            break;

        for(int i = 0; i < output_count; i++)
        {
            int bytes_written = 0;

            while(bytes_written < bytes_read)
            {
                int result = write(output_fds[i], buffer + bytes_written, bytes_read - bytes_written);

                if(result <= 0)
                    return;

                bytes_written += result;
            }
        }
    }
}

void closeInputFiles(int *input_fds, int input_count)
{
    for(int i = 0; i < input_count; i++)
        close(input_fds[i]);
}

void closeOutputFiles(int *output_fds, int output_count)
{
    for(int i = 0; i < output_count; i++)
        close(output_fds[i]);
}

void executeCommand(Token *tokens, int token_count)
{
    int command_count = 1;

    for(int i = 0; i < token_count; i++)
    {
        if(tokens[i].type == TOKEN_PIPE)
            command_count++;
        else if(tokens[i].type == TOKEN_SEMI || tokens[i].type == TOKEN_AMP)
            break;
    }

    char *args[command_count][token_count + 1];

    int input_fds[command_count][token_count];
    int output_fds[command_count][token_count];

    int input_count[command_count];
    int output_count[command_count];
    int argument_count[command_count];

    int start_positions[command_count];
    int end_positions[command_count];

    int pipes[command_count][2];

    int pid_count = 0;

    int command_index = 0;
    int start = 0;

    for(int i = 0; i <= token_count; i++)
    {
        if(i == token_count || tokens[i].type == TOKEN_PIPE || tokens[i].type == TOKEN_SEMI || tokens[i].type == TOKEN_AMP)
        {
            start_positions[command_index] = start;
            end_positions[command_index] = i;

            if(i == token_count || tokens[i].type == TOKEN_SEMI || tokens[i].type == TOKEN_AMP)
                break;

            command_index++;
            start = i + 1;
        }
    }

    for(int command = 0; command < command_count; command++)
    {
        input_count[command] = 0;
        output_count[command] = 0;
        argument_count[command] = 0;

        int i = start_positions[command];
        int end = end_positions[command];

        while(i < end)
        {
            if(tokens[i].type == TOKEN_LT)
            {
                i++;

                if(i >= end || tokens[i].type != TOKEN_WORD)
                {
                    printf("cshell: invalid syntax\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                int fd = open(tokens[i].value, O_RDONLY);

                if(fd == -1)
                {
                    printf("cshell: no such file or directory\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                input_fds[command][input_count[command]] = fd;
                input_count[command]++;

                i++;
            }
            else if(tokens[i].type == TOKEN_GT)
            {
                i++;

                if(i >= end || tokens[i].type != TOKEN_WORD)
                {
                    printf("cshell: invalid syntax\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                int fd = open(tokens[i].value, O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if(fd == -1)
                {
                    printf("cshell: unable to create file for writing\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                output_fds[command][output_count[command]] = fd;
                output_count[command]++;

                i++;
            }
            else if(tokens[i].type == TOKEN_GTGT)
            {
                i++;

                if(i >= end || tokens[i].type != TOKEN_WORD)
                {
                    printf("cshell: invalid syntax\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                int fd = open(tokens[i].value, O_WRONLY | O_CREAT | O_APPEND, 0644);

                if(fd == -1)
                {
                    printf("cshell: unable to create file for writing\n");

                    for(int j = 0; j < command_count; j++)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }

                    return;
                }

                output_fds[command][output_count[command]] = fd;
                output_count[command]++;

                i++;
            }
            else
            {
                args[command][argument_count[command]] = tokens[i].value;
                argument_count[command]++;
                i++;
            }
        }

        if(argument_count[command] == 0)
        {
            printf("cshell: invalid syntax\n");

            for(int j = 0; j < command_count; j++)
            {
                closeInputFiles(input_fds[j], input_count[j]);
                closeOutputFiles(output_fds[j], output_count[j]);
            }

            return;
        }

        args[command][argument_count[command]] = NULL;
    }

    for(int i = 0; i < command_count - 1; i++)
    {
        if(pipe(pipes[i]) != 0)
        {
            for(int j = 0; j < command_count; j++)
            {
                closeInputFiles(input_fds[j], input_count[j]);
                closeOutputFiles(output_fds[j], output_count[j]);
            }

            return;
        }
    }

    for(int command = 0; command < command_count; command++)
    {
        int input_pipe[2] = {-1, -1};
        int output_pipe[2] = {-1, -1};

        if(input_count[command] > 0)
        {
            if(pipe(input_pipe) != 0)
                return;

            int writer_pid = fork();

            if(writer_pid == 0)
            {
                close(input_pipe[0]);

                for(int j = 0; j < command_count; j++)
                {
                    if(j != command)
                    {
                        closeInputFiles(input_fds[j], input_count[j]);
                        closeOutputFiles(output_fds[j], output_count[j]);
                    }
                }

                for(int j = 0; j < command_count - 1; j++)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                for(int j = 0; j < input_count[command]; j++)
                {
                    copyFileToPipe(input_fds[command][j], input_pipe[1]);
                    close(input_fds[command][j]);
                }

                close(input_pipe[1]);
                exit(0);
            }

            pid_count++;
        }

        if(output_count[command] > 0)
        {
            if(pipe(output_pipe) != 0)
                return;

            int reader_pid = fork();

            if(reader_pid == 0)
            {
                close(output_pipe[1]);

                for(int j = 0; j < command_count; j++)
                {
                    closeInputFiles(input_fds[j], input_count[j]);

                    if(j != command)
                        closeOutputFiles(output_fds[j], output_count[j]);
                }

                for(int j = 0; j < command_count - 1; j++)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                copyPipeToFiles(output_pipe[0], output_fds[command], output_count[command]);

                close(output_pipe[0]);
                closeOutputFiles(output_fds[command], output_count[command]);

                exit(0);
            }

            pid_count++;
        }

        int command_pid = fork();

        if(command_pid == 0)
        {
            if(input_count[command] > 0)
            {
                close(input_pipe[1]);

                dup2(input_pipe[0], STDIN_FILENO);

                close(input_pipe[0]);
            }
            else if(command > 0)
            {
                dup2(pipes[command - 1][0], STDIN_FILENO);
            }

            if(output_count[command] > 0)
            {
                close(output_pipe[0]);

                dup2(output_pipe[1], STDOUT_FILENO);

                close(output_pipe[1]);
            }
            else if(command < command_count - 1)
            {
                dup2(pipes[command][1], STDOUT_FILENO);
            }

            for(int j = 0; j < command_count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if(input_pipe[0] != -1)
                close(input_pipe[0]);

            if(input_pipe[1] != -1)
                close(input_pipe[1]);

            if(output_pipe[0] != -1)
                close(output_pipe[0]);

            if(output_pipe[1] != -1)
                close(output_pipe[1]);

            for(int j = 0; j < command_count; j++)
            {
                closeInputFiles(input_fds[j], input_count[j]);
                closeOutputFiles(output_fds[j], output_count[j]);
            }

            char *original = args[command][0];
            int path_only = 0;

            if(args[command][0][0] == '%')
            {
                path_only = 1;
                args[command][0]++;
            }

            runCommand(args[command], argument_count[command], original, path_only);
        }

        pid_count++;

        if(input_pipe[0] != -1)
            close(input_pipe[0]);

        if(input_pipe[1] != -1)
            close(input_pipe[1]);

        if(output_pipe[0] != -1)
            close(output_pipe[0]);

        if(output_pipe[1] != -1)
            close(output_pipe[1]);
    }

    for(int i = 0; i < command_count - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for(int i = 0; i < command_count; i++)
    {
        closeInputFiles(input_fds[i], input_count[i]);
        closeOutputFiles(output_fds[i], output_count[i]);
    }

    for(int i = 0; i < pid_count; i++)
        wait(NULL);
}