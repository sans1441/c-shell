#include "jobs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct Job {
    int number;
    pid_t pgid;
    pid_t first_pid;
    pid_t *pids;
    size_t process_count;
    size_t remaining;
    char *command_name;
    int completed;
    int first_status;
    struct Job *next;
} Job;

typedef struct {
    pid_t pid;
    int status;
} ChildEvent;

static Job *job_list;
static int next_job_number = 1;
static int notification_pipe[2] = {-1, -1};
static int foreground_active;
static void (*prompt_callback)(void);

static void childHandler(int signal_number) {
    (void)signal_number;
    int saved_errno = errno;
    ChildEvent event;
    pid_t pid;

    while ((pid = waitpid(-1, &event.status, WNOHANG)) > 0) {
        event.pid = pid;
        (void)write(notification_pipe[1], &event, sizeof(event));
    }
    errno = saved_errno;
}

void jobsInit(void) {
    if (pipe(notification_pipe) == -1) return;

    int flags = fcntl(notification_pipe[0], F_GETFL);
    if (flags != -1) fcntl(notification_pipe[0], F_SETFL, flags | O_NONBLOCK);

    struct sigaction action = {0};
    action.sa_handler = childHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &action, NULL);
}

void jobsSetPromptCallback(void (*callback)(void)) { prompt_callback = callback; }

int jobsNotificationFd(void) { return notification_pipe[0]; }

void jobsBlockSignals(sigset_t *old_mask) {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked, old_mask);
}

void jobsRestoreSignals(const sigset_t *old_mask) { sigprocmask(SIG_SETMASK, old_mask, NULL); }

int jobsAdd(pid_t pgid, pid_t first_pid, const pid_t *pids, size_t process_count, const char *command_name) {
    Job *job = calloc(1, sizeof(Job));
    if (job == NULL) return 0;

    job->pids = malloc(sizeof(pid_t) * process_count);
    job->command_name = strdup(command_name);
    if (job->pids == NULL || job->command_name == NULL) {
        free(job->pids);
        free(job->command_name);
        free(job);
        return 0;
    }

    memcpy(job->pids, pids, sizeof(pid_t) * process_count);
    job->number = next_job_number++;
    job->pgid = pgid;
    job->first_pid = first_pid;
    job->process_count = process_count;
    job->remaining = process_count;
    job->next = job_list;
    job_list = job;

    printf("[%d] %d\n", job->number, (int)job->first_pid);
    fflush(stdout);
    return 1;
}

static Job *findJob(pid_t pid, Job **previous) {
    Job *before = NULL;
    for (Job *job = job_list; job != NULL; job = job->next) {
        for (size_t i = 0; i < job->process_count; i++) {
            if (job->pids[i] == pid) {
                if (previous != NULL) *previous = before;
                return job;
            }
        }
        before = job;
    }
    return NULL;
}

void jobsSetForeground(int active) { foreground_active = active; }

void jobsProcessNotifications(int redraw_prompt) {
    ChildEvent event;
    ssize_t bytes;
    while ((bytes = read(notification_pipe[0], &event, sizeof(event))) == (ssize_t)sizeof(event)) {
        Job *job = findJob(event.pid, NULL);
        if (job == NULL) continue;

        if (event.pid == job->first_pid) job->first_status = event.status;
        job->remaining--;
        if (job->remaining != 0) continue;

        job->completed = 1;
    }

    if (bytes == -1 && errno != EAGAIN && errno != EINTR) return;

    if (foreground_active) return;

    int printed = 0;
    Job *previous = NULL;
    Job *job = job_list;
    while (job != NULL) {
        Job *next = job->next;
        if (job->completed) {
            if (redraw_prompt && !printed) printf("\n");
            if (WIFEXITED(job->first_status))
                printf("%s with pid %d exited normally\n", job->command_name, (int)job->first_pid);
            else if (WIFSIGNALED(job->first_status))
                printf("%s with pid %d exited abnormally\n", job->command_name, (int)job->first_pid);
            printed = 1;

            if (previous == NULL)
                job_list = next;
            else
                previous->next = next;
            free(job->pids);
            free(job->command_name);
            free(job);
        } else
            previous = job;
        job = next;
    }

    if (printed && redraw_prompt && prompt_callback != NULL) prompt_callback();
}
