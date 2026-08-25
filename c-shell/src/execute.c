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
            result = write(pipe_write,
                           buffer + bytes_written,
                           bytes_read - bytes_written);

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
                int result = write(output_fds[i],
                                   buffer + bytes_written,
                                   bytes_read - bytes_written);

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
    char *args[token_count + 1];

    int input_fds[token_count];
    int output_fds[token_count];

    int input_count = 0;
    int output_count = 0;
    int argument_count = 0;

    for(int i = 0; i < token_count;)
    {
        if(tokens[i].type == TOKEN_SEMI ||
           tokens[i].type == TOKEN_AMP ||
           tokens[i].type == TOKEN_PIPE)
            break;

        if(tokens[i].type == TOKEN_LT)
        {
            i++;

            if(i >= token_count || tokens[i].type != TOKEN_WORD)
            {
                printf("cshell: invalid syntax\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            int fd = open(tokens[i].value, O_RDONLY);

            if(fd == -1)
            {
                printf("cshell: no such file or directory\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            input_fds[input_count] = fd;
            input_count++;

            i++;
        }
        else if(tokens[i].type == TOKEN_GT)
        {
            i++;

            if(i >= token_count || tokens[i].type != TOKEN_WORD)
            {
                printf("cshell: invalid syntax\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            int fd = open(tokens[i].value,
                          O_WRONLY | O_CREAT | O_TRUNC,
                          0644);

            if(fd == -1)
            {
                printf("cshell: unable to create file for writing\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            output_fds[output_count] = fd;
            output_count++;

            i++;
        }
        else if(tokens[i].type == TOKEN_GTGT)
        {
            i++;

            if(i >= token_count || tokens[i].type != TOKEN_WORD)
            {
                printf("cshell: invalid syntax\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            int fd = open(tokens[i].value,
                          O_WRONLY | O_CREAT | O_APPEND,
                          0644);

            if(fd == -1)
            {
                printf("cshell: unable to create file for writing\n");

                closeInputFiles(input_fds, input_count);
                closeOutputFiles(output_fds, output_count);

                return;
            }

            output_fds[output_count] = fd;
            output_count++;

            i++;
        }
        else
        {
            args[argument_count] = tokens[i].value;
            argument_count++;
            i++;
        }
    }

    if(argument_count == 0)
    {
        closeInputFiles(input_fds, input_count);
        closeOutputFiles(output_fds, output_count);
        return;
    }

    char *original = args[0];
    int path_only = 0;

    if(args[0][0] == '%')
    {
        path_only = 1;
        args[0]++;
    }

    args[argument_count] = NULL;

    if(input_count == 0 && output_count == 0)
    {
        int pid = fork();

        if(pid == 0)
            runCommand(args, argument_count, original, path_only);

        else if(pid > 0)
            wait(NULL);

        return;
    }

    int input_pipe[2];
    int output_pipe[2];

    if(input_count > 0)
    {
        if(pipe(input_pipe) != 0)
        {
            closeInputFiles(input_fds, input_count);
            closeOutputFiles(output_fds, output_count);
            return;
        }
    }

    if(output_count > 0)
    {
        if(pipe(output_pipe) != 0)
        {
            if(input_count > 0)
            {
                close(input_pipe[0]);
                close(input_pipe[1]);
            }

            closeInputFiles(input_fds, input_count);
            closeOutputFiles(output_fds, output_count);

            return;
        }
    }

    int command_pid = fork();

    if(command_pid == 0)
    {
        if(input_count > 0)
        {
            close(input_pipe[1]);

            dup2(input_pipe[0], STDIN_FILENO);

            close(input_pipe[0]);
        }

        if(output_count > 0)
        {
            close(output_pipe[0]);

            dup2(output_pipe[1], STDOUT_FILENO);

            close(output_pipe[1]);
        }

        closeInputFiles(input_fds, input_count);
        closeOutputFiles(output_fds, output_count);

        runCommand(args, argument_count, original, path_only);
    }

    int writer_pid = -1;

    if(input_count > 0)
    {
        writer_pid = fork();

        if(writer_pid == 0)
        {
            close(input_pipe[0]);

            for(int i = 0; i < input_count; i++)
            {
                copyFileToPipe(input_fds[i], input_pipe[1]);
                close(input_fds[i]);
            }

            close(input_pipe[1]);
            exit(0);
        }
    }

    if(input_count > 0)
    {
        close(input_pipe[0]);
        close(input_pipe[1]);
    }

    closeInputFiles(input_fds, input_count);

    if(output_count > 0)
    {
        close(output_pipe[1]);

        copyPipeToFiles(output_pipe[0], output_fds, output_count);

        close(output_pipe[0]);
    }

    closeOutputFiles(output_fds, output_count);

    wait(NULL);

    if(writer_pid > 0)
        wait(NULL);
}