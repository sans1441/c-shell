#include "jobs.h"
#include "terminal.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <limits.h>

typedef enum { PROCESS_RUNNING, PROCESS_STOPPED, PROCESS_COMPLETED } ProcessState;

typedef struct {
    pid_t pid;
    char *command_name;
    ProcessState state;
} Process;

typedef struct Job {
    int number;
    pid_t pgid;
    pid_t first_pid;
    Process *processes;
    size_t process_count;
    size_t remaining;
    char *command_name;
    char *command_line;
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
static volatile sig_atomic_t interactive_signal;
static volatile sig_atomic_t timeout_expired;

static void childHandler(int signal_number) {
    (void)signal_number;
    int saved_errno = errno;
    ChildEvent event;
    pid_t pid;

    while ((pid = waitpid(-1, &event.status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        event.pid = pid;
        (void)write(notification_pipe[1], &event, sizeof(event));
    }
    errno = saved_errno;
}

static void interactiveSignalHandler(int signal_number) {
    interactive_signal = signal_number;
    (void)write(STDOUT_FILENO, "\n", 1);
}

static void timeoutHandler(int signal_number) {
    (void)signal_number;
    timeout_expired = 1;
}

void jobsInit(void) {
    terminalInit();
    if (pipe(notification_pipe) == -1) return;

    int flags = fcntl(notification_pipe[0], F_GETFL);
    if (flags != -1) fcntl(notification_pipe[0], F_SETFL, flags | O_NONBLOCK);

    struct sigaction action = {0};
    action.sa_handler = childHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &action, NULL);

    struct sigaction interactive_action = {0};
    interactive_action.sa_handler = interactiveSignalHandler;
    sigemptyset(&interactive_action.sa_mask);
    sigaction(SIGINT, &interactive_action, NULL);
    sigaction(SIGTSTP, &interactive_action, NULL);

    struct sigaction timeout_action = {0};
    timeout_action.sa_handler = timeoutHandler;
    sigemptyset(&timeout_action.sa_mask);
    sigaction(SIGALRM, &timeout_action, NULL);
}

void jobsSetPromptCallback(void (*callback)(void)) { prompt_callback = callback; }

int jobsNotificationFd(void) { return notification_pipe[0]; }

int jobsTakeInteractiveSignal(void) {
    int signal_number = interactive_signal;
    interactive_signal = 0;
    return signal_number;
}

void jobsBlockSignals(sigset_t *old_mask) {
    sigset_t blocked;
    sigemptyset(&blocked);
    sigaddset(&blocked, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked, old_mask);
}

void jobsRestoreSignals(const sigset_t *old_mask) { sigprocmask(SIG_SETMASK, old_mask, NULL); }

static int addJob(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count, ProcessState initial_state, int announce_stopped, const char *command_line) {
    Job *job = calloc(1, sizeof(Job));
    if (job == NULL) return 0;

    job->processes = calloc(process_count, sizeof(Process));
    job->command_name = strdup(command_names[0]);
    job->command_line = strdup(command_line);
    if (job->processes == NULL || job->command_name == NULL || job->command_line == NULL) {
        free(job->processes);
        free(job->command_name);
        free(job->command_line);
        free(job);
        return 0;
    }

    for (size_t i = 0; i < process_count; i++) {
        job->processes[i].pid = pids[i];
        job->processes[i].command_name = strdup(command_names[i]);
        job->processes[i].state = initial_state;
        if (job->processes[i].command_name == NULL) {
            for (size_t j = 0; j <= i; j++) free(job->processes[j].command_name);
            free(job->processes);
            free(job->command_name);
            free(job->command_line);
            free(job);
            return 0;
        }
    }

    job->number = next_job_number++;
    job->pgid = pgid;
    job->first_pid = first_pid;
    job->process_count = process_count;
    job->remaining = process_count;

    if (job_list == NULL)
        job_list = job;
    else {
        Job *last = job_list;
        while (last->next != NULL) last = last->next;
        last->next = job;
    }

    if (announce_stopped)
        printf("[%d] + Stopped %s\n", job->number, job->command_line);
    else
        printf("[%d] %d\n", job->number, (int)job->first_pid);
    fflush(stdout);
    return 1;
}

int jobsAdd(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count, const char *command_line) {
    return addJob(pgid, first_pid, pids, command_names, process_count, PROCESS_RUNNING, 0, command_line);
}

int jobsAddStopped(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count, const char *command_line) {
    return addJob(pgid, first_pid, pids, command_names, process_count, PROCESS_STOPPED, 1, command_line);
}

static Job *findJob(pid_t pid, Process **process) {
    for (Job *job = job_list; job != NULL; job = job->next) {
        for (size_t i = 0; i < job->process_count; i++) {
            if (job->processes[i].pid == pid) {
                if (process != NULL) *process = &job->processes[i];
                return job;
            }
        }
    }
    return NULL;
}

void jobsSetForeground(int active) { foreground_active = active; }

int jobsHasStopped(void) {
    for (Job *job = job_list; job != NULL; job = job->next) {
        for (size_t i = 0; i < job->process_count; i++) {
            if (job->processes[i].state == PROCESS_STOPPED) return 1;
        }
    }
    return 0;
}

void jobsTerminateAll(void) {
    for (Job *job = job_list; job != NULL; job = job->next) kill(-job->pgid, SIGHUP);
}

static void removeJob(Job *target) {
    Job *previous = NULL;
    Job *job = job_list;
    while (job != NULL && job != target) {
        previous = job;
        job = job->next;
    }
    if (job == NULL) return;

    if (previous == NULL)
        job_list = job->next;
    else
        previous->next = job->next;
    for (size_t i = 0; i < job->process_count; i++) free(job->processes[i].command_name);
    free(job->processes);
    free(job->command_name);
    free(job->command_line);
    free(job);
}

static void printCompletedJobs(int redraw_prompt) {
    if (foreground_active) return;

    int printed = 0;
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
            removeJob(job);
        }
        job = next;
    }
    if (printed && redraw_prompt && prompt_callback != NULL) prompt_callback();
}

void jobsProcessNotifications(int redraw_prompt) {
    ChildEvent event;
    ssize_t bytes;
    while ((bytes = read(notification_pipe[0], &event, sizeof(event))) == (ssize_t)sizeof(event)) {
        Process *process = NULL;
        Job *job = findJob(event.pid, &process);
        if (job == NULL || process == NULL) continue;

        if (WIFSTOPPED(event.status)) {
            process->state = PROCESS_STOPPED;
        } else if (WIFCONTINUED(event.status)) {
            process->state = PROCESS_RUNNING;
        } else if ((WIFEXITED(event.status) || WIFSIGNALED(event.status)) && process->state != PROCESS_COMPLETED) {
            process->state = PROCESS_COMPLETED;
            if (event.pid == job->first_pid) job->first_status = event.status;
            job->remaining--;
            if (job->remaining == 0) job->completed = 1;
        }
    }

    if (bytes == -1 && errno != EAGAIN && errno != EINTR) return;
    printCompletedJobs(redraw_prompt);
}

void jobsPrintActivities(void) {
    jobsProcessNotifications(0);
    for (Job *job = job_list; job != NULL; job = job->next) {
        int active = 0;
        for (size_t i = 0; i < job->process_count; i++) {
            if (job->processes[i].state != PROCESS_COMPLETED) active = 1;
        }
        if (!active) continue;

        printf("[%d] pgid %d\n", job->number, (int)job->pgid);
        for (size_t i = 0; i < job->process_count; i++) {
            Process *process = &job->processes[i];
            if (process->state == PROCESS_COMPLETED) continue;
            const char *state = process->state == PROCESS_STOPPED ? "Stopped" : "Running";
            printf("  %d %s %s\n", (int)process->pid, process->command_name, state);
        }
    }
    fflush(stdout);
}

static Job *findJobNumber(int number) {
    for (Job *job = job_list; job != NULL; job = job->next)
        if (job->number == number) return job;
    return NULL;
}

static void markJobRunning(Job *job) {
    for (size_t i = 0; i < job->process_count; i++)
        if (job->processes[i].state != PROCESS_COMPLETED) job->processes[i].state = PROCESS_RUNNING;
    job->completed = 0;
}

ResumeResult jobsResume(int job_number, int foreground, unsigned int timeout_seconds) {
    jobsProcessNotifications(0);
    Job *job = findJobNumber(job_number);
    if (job == NULL) return RESUME_NO_JOB;

    sigset_t old_mask;
    jobsBlockSignals(&old_mask);
    if (kill(-job->pgid, SIGCONT) == -1) {
        jobsRestoreSignals(&old_mask);
        return RESUME_ERROR;
    }

    markJobRunning(job);
    if (!foreground) {
        printf("[%d] + Running %s\n", job->number, job->command_line);
        fflush(stdout);
        jobsRestoreSignals(&old_mask);
        return RESUME_OK;
    }

    printf("%s\n", job->command_line);
    fflush(stdout);

    jobsSetForeground(1);
    terminalGiveTo(job->pgid);
    timeout_expired = 0;
    if (timeout_seconds > 0) alarm(timeout_seconds);

    int stopped = 0;
    for (size_t i = 0; i < job->process_count; i++) {
        Process *process = &job->processes[i];
        if (process->state == PROCESS_COMPLETED) continue;
        int status;
        pid_t result;
        do {
            result = waitpid(process->pid, &status, WUNTRACED);
        } while (result == -1 && errno == EINTR && !timeout_expired);
        if (timeout_expired) break;
        if (result == -1) continue;
        if (WIFSTOPPED(status)) {
            process->state = PROCESS_STOPPED;
            stopped = 1;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            process->state = PROCESS_COMPLETED;
            if (job->remaining > 0) job->remaining--;
        }
    }
    alarm(0);

    if (timeout_expired) {
        kill(-job->pgid, SIGTERM);
        for (size_t i = 0; i < job->process_count; i++) {
            if (job->processes[i].state != PROCESS_COMPLETED) {
                int status;
                waitpid(job->processes[i].pid, &status, 0);
                job->processes[i].state = PROCESS_COMPLETED;
            }
        }
        printf("resume: job timed out\n");
        terminalReclaim();
        jobsSetForeground(0);
        jobsRestoreSignals(&old_mask);
        removeJob(job);
        return RESUME_OK;
    }

    if (stopped) {
        for (size_t i = 0; i < job->process_count; i++)
            if (job->processes[i].state != PROCESS_COMPLETED) job->processes[i].state = PROCESS_STOPPED;
        printf("[%d] + Stopped %s\n", job->number, job->command_line);
    } else {
        removeJob(job);
    }
    terminalReclaim();
    jobsSetForeground(0);
    jobsRestoreSignals(&old_mask);
    fflush(stdout);
    return RESUME_OK;
}

int jobsPing(const char *target, int signal_number) {
    jobsProcessNotifications(0);
    if (target[0] == '%') {
        char *end = NULL;
        errno = 0;
        long number = strtol(target + 1, &end, 10);
        if (target[1] == '\0' || errno != 0 || *end != '\0' || number <= 0 || number > INT_MAX) return 0;
        Job *job = findJobNumber((int)number);
        if (job == NULL || kill(-job->pgid, signal_number) == -1) return 0;
        return 1;
    }

    char *end = NULL;
    errno = 0;
    long pid_value = strtol(target, &end, 10);
    if (*target == '\0' || errno != 0 || *end != '\0' || pid_value <= 0 || pid_value > INT_MAX) return 0;
    Process *process = NULL;
    if (findJob((pid_t)pid_value, &process) == NULL || process == NULL) return 0;
    return kill((pid_t)pid_value, signal_number) == 0;
}
