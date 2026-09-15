#include "execute.h"
#include "hop.h"
#include "jobs.h"
#include "locate.h"
#include "peek.h"
#include "reveal.h"
#include "activities.h"
#include "ping.h"
#include "resume.h"
#include "terminal.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

static int isBuiltin(const char *name) { return strcmp(name, "hop") == 0 || strcmp(name, "reveal") == 0 || strcmp(name, "peek") == 0 || strcmp(name, "locate") == 0 || strcmp(name, "activities") == 0 || strcmp(name, "resume") == 0 || strcmp(name, "ping") == 0; }

static char *describePipeline(const Pipeline *pipeline) {
    size_t length = 1;
    for (size_t i = 0; i < pipeline->command_count; i++) {
        if (i > 0) length += 3;
        for (size_t j = 0; j < pipeline->commands[i].argc; j++) length += strlen(pipeline->commands[i].argv[j]) + 1;
    }
    char *description = calloc(length, sizeof(char));
    if (description == NULL) return NULL;
    for (size_t i = 0; i < pipeline->command_count; i++) {
        if (i > 0) strcat(description, " | ");
        for (size_t j = 0; j < pipeline->commands[i].argc; j++) {
            if (j > 0) strcat(description, " ");
            strcat(description, pipeline->commands[i].argv[j]);
        }
    }
    return description;
}

static int runBuiltin(const Command *command, ShellState *state) {
    Token *tokens = calloc(command->argc, sizeof(Token));
    if (tokens == NULL) return 1;

    for (size_t i = 0; i < command->argc; i++) {
        tokens[i].type = TOKEN_WORD;
        tokens[i].value = command->argv[i];
    }

    if (strcmp(command->argv[0], "hop") == 0)
        hop(tokens, (int)command->argc, state);
    else if (strcmp(command->argv[0], "reveal") == 0)
        reveal(tokens, (int)command->argc, state);
    else if (strcmp(command->argv[0], "peek") == 0)
        peek(tokens, (int)command->argc);
    else if (strcmp(command->argv[0], "locate") == 0)
        locate(tokens, (int)command->argc);
    else if (strcmp(command->argv[0], "activities") == 0)
        activitiesRun(command);
    else if (strcmp(command->argv[0], "resume") == 0)
        resumeRun(command);
    else if (strcmp(command->argv[0], "ping") == 0)
        pingRun(command);

    free(tokens);
    return 0;
}

static char *resolveCommand(const char *name) {
    const char *lookup = name;
    if (name[0] == '%') lookup++;

    struct stat info;
    if (strchr(lookup, '/') != NULL) {
        if (stat(lookup, &info) == 0 && S_ISREG(info.st_mode) && access(lookup, X_OK) == 0) return strdup(lookup);
        return NULL;
    }

    if (name[0] != '%' && stat(lookup, &info) == 0 && S_ISREG(info.st_mode) && access(lookup, X_OK) == 0) {
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == NULL) return NULL;
        size_t length = strlen(cwd) + strlen(lookup) + 2;
        char *path = malloc(length);
        if (path != NULL) snprintf(path, length, "%s/%s", cwd, lookup);
        return path;
    }

    const char *path_env = getenv("PATH");
    if (path_env == NULL) return NULL;

    char *path_copy = strdup(path_env);
    if (path_copy == NULL) return NULL;

    char *save = NULL;
    for (char *directory = strtok_r(path_copy, ":", &save); directory != NULL; directory = strtok_r(NULL, ":", &save)) {
        size_t length = strlen(directory) + strlen(lookup) + 2;
        char *candidate = malloc(length);
        if (candidate == NULL) continue;
        snprintf(candidate, length, "%s/%s", directory, lookup);
        if (stat(candidate, &info) == 0 && S_ISREG(info.st_mode) && access(candidate, X_OK) == 0) {
            free(path_copy);
            return candidate;
        }
        free(candidate);
    }

    free(path_copy);
    return NULL;
}

static void copyFilesToPipe(const int *files, size_t count, int write_fd) {
    char buffer[4096];
    for (size_t i = 0; i < count; i++) {
        ssize_t bytes;
        while ((bytes = read(files[i], buffer, sizeof(buffer))) > 0) {
            ssize_t written = 0;
            while (written < bytes) {
                ssize_t result = write(write_fd, buffer + written, (size_t)(bytes - written));
                if (result <= 0) return;
                written += result;
            }
        }
        close(files[i]);
    }
}

static void copyPipeToFiles(int read_fd, const int *files, size_t count) {
    char buffer[4096];
    ssize_t bytes;
    while ((bytes = read(read_fd, buffer, sizeof(buffer))) > 0) {
        for (size_t i = 0; i < count; i++) {
            ssize_t written = 0;
            while (written < bytes) {
                ssize_t result = write(files[i], buffer + written, (size_t)(bytes - written));
                if (result <= 0) return;
                written += result;
            }
        }
    }
}

static void closeFiles(int *files, int count) {
    for (int i = 0; i < count; i++) close(files[i]);
}

static void reportLaunchFailure(int error_fd) {
    char failure = 1;
    (void)write(error_fd, &failure, sizeof(failure));
}

static void childExecute(const Command *command, ShellState *state, int input_fd, int output_fd, int **pipe_fds, size_t pipe_count, int *input_files, int input_count, int *output_files,
                         int output_count, int error_fd, pid_t pgid, int background) {
    terminalPrepareChild();
    setpgid(0, pgid);
    if (input_count > 0) {
        if (input_count == 1)
            dup2(input_files[0], STDIN_FILENO);
        else
            dup2(input_fd, STDIN_FILENO);
    } else if (input_fd != -1)
        dup2(input_fd, STDIN_FILENO);
    else if (background) {
        int null_fd = open("/dev/null", O_RDONLY);
        if (null_fd != -1) {
            dup2(null_fd, STDIN_FILENO);
            close(null_fd);
        }
    }

    if (output_count == 1)
        dup2(output_files[0], STDOUT_FILENO);
    else if (output_count > 1)
        dup2(output_fd, STDOUT_FILENO);
    else if (output_fd != -1)
        dup2(output_fd, STDOUT_FILENO);

    for (size_t i = 0; i < pipe_count; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    closeFiles(input_files, input_count);
    closeFiles(output_files, output_count);
    if (input_fd != -1) close(input_fd);
    if (output_fd != -1) close(output_fd);

    if (isBuiltin(command->argv[0])) {
        runBuiltin(command, state);
        close(error_fd);
        exit(0);
    }

    char *path = resolveCommand(command->argv[0]);
    if (path == NULL) {
        reportLaunchFailure(error_fd);
        close(error_fd);
        printf("cshell: command not found (%s)\n", command->argv[0]);
        exit(1);
    }
    execve(path, command->argv, environ);
    free(path);
    reportLaunchFailure(error_fd);
    close(error_fd);
    printf("cshell: command not found (%s)\n", command->argv[0]);
    exit(1);
}

static int executePipeline(Pipeline *pipeline, ShellState *state) {
    size_t count = pipeline->command_count;
    size_t max_redirections = 1;
    for (size_t i = 0; i < count; i++) {
        if (pipeline->commands[i].redirection_count > max_redirections) max_redirections = pipeline->commands[i].redirection_count;
    }
    int **pipes = calloc(count > 0 ? count - 1 : 0, sizeof(int *));
    pid_t *pids = calloc(count * 3 + 1, sizeof(pid_t));
    pid_t *command_pids = calloc(count, sizeof(pid_t));
    const char **command_names = calloc(count, sizeof(char *));
    int (*error_pipes)[2] = calloc(count, sizeof(*error_pipes));
    if (pids == NULL || command_pids == NULL || command_names == NULL || (count > 1 && pipes == NULL) || error_pipes == NULL) return EXECUTION_LAUNCH_FAILED;

    sigset_t old_mask;
    jobsBlockSignals(&old_mask);

    for (size_t i = 0; i + 1 < count; i++) {
        pipes[i] = malloc(sizeof(int) * 2);
        if (pipes[i] == NULL || pipe(pipes[i]) == -1) return EXECUTION_LAUNCH_FAILED;
    }
    for (size_t i = 0; i < count; i++) {
        if (pipe(error_pipes[i]) == -1) return EXECUTION_LAUNCH_FAILED;
        int flags = fcntl(error_pipes[i][1], F_GETFD);
        fcntl(error_pipes[i][1], F_SETFD, flags | FD_CLOEXEC);
    }

    int (*input_files)[max_redirections] = calloc(count, sizeof(*input_files));
    int (*output_files)[max_redirections] = calloc(count, sizeof(*output_files));
    int *input_counts = calloc(count, sizeof(int));
    int *output_counts = calloc(count, sizeof(int));
    int *input_streams = calloc(count, sizeof(int));
    int *output_streams = calloc(count, sizeof(int));
    if (input_files == NULL || output_files == NULL || input_counts == NULL || output_counts == NULL || input_streams == NULL || output_streams == NULL) return EXECUTION_LAUNCH_FAILED;
    for (size_t i = 0; i < count; i++) {
        input_streams[i] = -1;
        output_streams[i] = -1;
        input_counts[i] = 0;
        output_counts[i] = 0;
        for (size_t j = 0; j < pipeline->commands[i].redirection_count; j++) {
            Redirection *r = &pipeline->commands[i].redirections[j];
            int flags = r->type == REDIR_INPUT ? O_RDONLY : O_WRONLY | O_CREAT | (r->type == REDIR_OUTPUT ? O_TRUNC : O_APPEND);
            int fd = open(r->filename, flags, 0644);
            if (fd == -1) {
                printf("cshell: %s\n", r->type == REDIR_INPUT ? "no such file or directory" : "unable to create file for writing");
                for (size_t k = 0; k < count; k++) {
                    closeFiles(input_files[k], input_counts[k]);
                    closeFiles(output_files[k], output_counts[k]);
                }
                for (size_t k = 0; k + 1 < count; k++) {
                    close(pipes[k][0]);
                    close(pipes[k][1]);
                }
                for (size_t k = 0; k < count; k++) {
                    close(error_pipes[k][0]);
                    close(error_pipes[k][1]);
                }
                return EXECUTION_LAUNCH_FAILED;
            }
            if (r->type == REDIR_INPUT)
                input_files[i][input_counts[i]++] = fd;
            else
                output_files[i][output_counts[i]++] = fd;
        }
    }

    size_t pid_count = 0;
    for (size_t i = 0; i < count; i++) {
        command_names[i] = pipeline->commands[i].argv[0];
        if (input_counts[i] > 1) {
            int feeder[2];
            pipe(feeder);
            pid_t pid = fork();
            if (pid == 0) {
                close(feeder[0]);
                copyFilesToPipe(input_files[i], (size_t)input_counts[i], feeder[1]);
                close(feeder[1]);
                exit(0);
            }
            pids[pid_count++] = pid;
            close(feeder[1]);
            input_streams[i] = feeder[0];
        }
        if (output_counts[i] > 1) {
            int collector[2];
            pipe(collector);
            pid_t pid = fork();
            if (pid == 0) {
                close(collector[1]);
                copyPipeToFiles(collector[0], output_files[i], (size_t)output_counts[i]);
                close(collector[0]);
                exit(0);
            }
            pids[pid_count++] = pid;
            close(collector[0]);
            output_streams[i] = collector[1];
        }
    }

    size_t command_start = pid_count;
    for (size_t i = 0; i < count; i++) {
        int input = input_streams[i] != -1 ? input_streams[i] : (i > 0 ? pipes[i - 1][0] : -1);
        int output = output_streams[i] != -1 ? output_streams[i] : (i + 1 < count ? pipes[i][1] : -1);
        pid_t pid = fork();
        if (pid == 0) {
            close(error_pipes[i][0]);
            childExecute(&pipeline->commands[i], state, input, output, pipes, count - 1, input_files[i], input_counts[i], output_files[i], output_counts[i], error_pipes[i][1],
                         i == 0 ? 0 : command_pids[0], pipeline->background);
        }
        if (i == 0) command_pids[0] = pid;
        setpgid(pid, command_pids[0]);
        close(error_pipes[i][1]);
        command_pids[i] = pid;
        pids[pid_count++] = pid;
    }

    for (size_t i = 0; i + 1 < count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
        free(pipes[i]);
    }
    for (size_t i = 0; i < count; i++) {
        if (input_streams[i] != -1) close(input_streams[i]);
        if (output_streams[i] != -1) close(output_streams[i]);
        closeFiles(input_files[i], input_counts[i]);
        closeFiles(output_files[i], output_counts[i]);
    }
    if (pipeline->background) {
        char *description = describePipeline(pipeline);
        int added = description != NULL && jobsAdd(command_pids[0], command_pids[0], command_pids, command_names, count, description);
        free(description);
        for (size_t i = 0; i < count; i++) close(error_pipes[i][0]);
        jobsRestoreSignals(&old_mask);
        free(command_pids);
        free(command_names);
        free(input_files);
        free(output_files);
        free(input_counts);
        free(output_counts);
        free(input_streams);
        free(output_streams);
        free(pids);
        free(pipes);
        free(error_pipes);
        return added ? EXECUTION_OK : EXECUTION_LAUNCH_FAILED;
    }
    jobsSetForeground(1);
    terminalGiveTo(command_pids[0]);
    int stopped = 0;
    int *command_statuses = calloc(count, sizeof(int));
    for (size_t i = 0; i < count; i++) {
        waitpid(command_pids[i], &command_statuses[i], WUNTRACED);
        if (WIFSTOPPED(command_statuses[i])) stopped = 1;
    }
    if (!stopped) {
        for (size_t i = 0; i < command_start; i++) waitpid(pids[i], NULL, 0);
    }
    terminalReclaim();
    jobsSetForeground(0);
    jobsRestoreSignals(&old_mask);
    if (stopped) {
        printf("\n");
        char *description = describePipeline(pipeline);
        if (description != NULL) {
            jobsAddStopped(command_pids[0], command_pids[0], command_pids, command_names, count, description);
            free(description);
        }
        for (size_t i = 0; i < count; i++) close(error_pipes[i][0]);
        free(command_statuses);
        free(input_files);
        free(output_files);
        free(input_counts);
        free(output_counts);
        free(input_streams);
        free(output_streams);
        free(pids);
        free(command_pids);
        free(command_names);
        free(pipes);
        free(error_pipes);
        return EXECUTION_OK;
    }
    for (size_t i = 0; i < count; i++) {
        if (WIFSIGNALED(command_statuses[i]) && WTERMSIG(command_statuses[i]) == SIGINT) {
            printf("\n");
            break;
        }
    }
    free(command_statuses);
    jobsProcessNotifications(0);
    ExecutionResult result = EXECUTION_OK;
    for (size_t i = 0; i < count; i++) {
        char failure;
        if (read(error_pipes[i][0], &failure, sizeof(failure)) > 0) result = EXECUTION_LAUNCH_FAILED;
        close(error_pipes[i][0]);
    }
    free(input_files);
    free(output_files);
    free(input_counts);
    free(output_counts);
    free(input_streams);
    free(output_streams);
    free(pids);
    free(command_pids);
    free(command_names);
    free(pipes);
    free(error_pipes);
    return result;
}

void executeLine(CommandLine *line, ShellState *state) {
    for (size_t i = 0; i < line->pipeline_count; i++) {
        Pipeline *pipeline = &line->pipelines[i];
        Command *first = &pipeline->commands[0];
    if (!pipeline->background && pipeline->command_count == 1 && first->redirection_count == 0 && isBuiltin(first->argv[0]))
            runBuiltin(first, state);
        else if (executePipeline(pipeline, state) == EXECUTION_LAUNCH_FAILED && pipeline->command_count == 1)
            break;
    }
}
