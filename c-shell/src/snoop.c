#include "snoop.h"
#include "path_utils.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

typedef struct {
    long number;
    unsigned long calls;
    double total_time;
    unsigned long first_seen;
} SyscallSummary;

static const char *syscallName(long number) {
    static const char *names[] = {
        [0] = "read", [1] = "write", [2] = "open", [3] = "close", [4] = "stat", [5] = "fstat", [6] = "lstat",
        [7] = "poll", [9] = "mmap", [10] = "mprotect", [11] = "munmap", [12] = "brk", [13] = "rt_sigaction",
        [14] = "rt_sigprocmask", [16] = "ioctl", [17] = "pread64", [18] = "pwrite64", [21] = "access", [28] = "madvise",
        [35] = "nanosleep", [39] = "getpid", [41] = "socket", [56] = "clone", [57] = "fork", [59] = "execve",
        [60] = "exit", [61] = "wait4", [62] = "kill", [63] = "uname", [72] = "fcntl", [79] = "getcwd",
        [80] = "chdir", [89] = "readlink", [96] = "gettimeofday", [131] = "sigaltstack", [158] = "arch_prctl",
        [202] = "futex", [217] = "getdents64", [218] = "set_tid_address", [219] = "restart_syscall", [228] = "clock_gettime", [230] = "clock_nanosleep",
        [231] = "exit_group", [234] = "tgkill", [257] = "openat", [262] = "newfstatat", [273] = "set_robust_list",
        [302] = "prlimit64", [318] = "getrandom", [334] = "rseq"
    };
    static char unknown[32];
    if (number >= 0 && (size_t)number < sizeof(names) / sizeof(names[0]) && names[number] != NULL) return names[number];
    snprintf(unknown, sizeof(unknown), "syscall_%ld", number);
    return unknown;
}

static double elapsed(const struct timespec *start, const struct timespec *end) {
    return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static int addSummary(SyscallSummary **summaries, size_t *count, size_t *capacity, long number, double duration, unsigned long order) {
    for (size_t i = 0; i < *count; i++) {
        if ((*summaries)[i].number == number) {
            (*summaries)[i].calls++;
            (*summaries)[i].total_time += duration;
            return 1;
        }
    }
    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 32 : *capacity * 2;
        SyscallSummary *expanded = realloc(*summaries, new_capacity * sizeof(SyscallSummary));
        if (expanded == NULL) return 0;
        *summaries = expanded;
        *capacity = new_capacity;
    }
    (*summaries)[*count] = (SyscallSummary){number, 1, duration, order};
    (*count)++;
    return 1;
}

static int compareSummaries(const void *left, const void *right) {
    const SyscallSummary *a = left;
    const SyscallSummary *b = right;
    if (a->calls < b->calls) return 1;
    if (a->calls > b->calls) return -1;
    if (a->first_seen > b->first_seen) return 1;
    if (a->first_seen < b->first_seen) return -1;
    return 0;
}

static void printSummary(SyscallSummary *summaries, size_t count) {
    qsort(summaries, count, sizeof(SyscallSummary), compareSummaries);
    printf("syscall       calls   time\n");
    for (size_t i = 0; i < count; i++)
        printf("%-13s %-7lu %.3fs\n", syscallName(summaries[i].number), summaries[i].calls, summaries[i].total_time);
}

static void recordPendingSyscall(SyscallSummary **summaries, size_t *count, size_t *capacity, int entering, long number,
                                 const struct timespec *start, unsigned long *order) {
    if (!entering && number >= 0) {
        struct timespec end;
        clock_gettime(CLOCK_MONOTONIC, &end);
        addSummary(summaries, count, capacity, number, elapsed(start, &end), (*order)++);
    }
}

static int traceProcess(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) return 0;
    if (!WIFSTOPPED(status)) return 0;
    if (ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)PTRACE_O_TRACESYSGOOD) == -1) return 0;

    SyscallSummary *summaries = NULL;
    size_t count = 0;
    size_t capacity = 0;
    unsigned long order = 0;
    long current_syscall = -1;
    struct timespec start;
    int entering = 1;
    int deliver_signal = 0;
    while (1) {
        if (ptrace(PTRACE_SYSCALL, pid, NULL, (void *)(long)deliver_signal) == -1) break;
        deliver_signal = 0;
        if (waitpid(pid, &status, 0) == -1) break;
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            recordPendingSyscall(&summaries, &count, &capacity, entering, current_syscall, &start, &order);
            printSummary(summaries, count);
            free(summaries);
            return 1;
        }
        if (!WIFSTOPPED(status)) continue;
        int stop_signal = WSTOPSIG(status);
        if (stop_signal != (SIGTRAP | 0x80)) {
            deliver_signal = stop_signal == SIGTRAP ? 0 : stop_signal;
            continue;
        }

        struct user_regs_struct registers;
        if (ptrace(PTRACE_GETREGS, pid, NULL, &registers) == -1) break;
        if (entering) {
            current_syscall = (long)registers.orig_rax;
            clock_gettime(CLOCK_MONOTONIC, &start);
        } else {
            struct timespec end;
            clock_gettime(CLOCK_MONOTONIC, &end);
            if (!addSummary(&summaries, &count, &capacity, current_syscall, elapsed(&start, &end), order++)) break;
        }
        entering = !entering;
    }
    free(summaries);
    return 0;
}

static int parsePid(const char *value, pid_t *pid) {
    char *end = NULL;
    errno = 0;
    long parsed = strtol(value, &end, 10);
    if (errno != 0 || *value == '\0' || *end != '\0' || parsed <= 0) return 0;
    *pid = (pid_t)parsed;
    return 1;
}

void snoopRun(const Command *command) {
    if (command->argc >= 2 && strcmp(command->argv[1], "-p") == 0) {
        if (command->argc != 3) {
            printf("snoop: invalid syntax\n");
            return;
        }
        pid_t pid;
        if (!parsePid(command->argv[2], &pid) || kill(pid, 0) == -1 || ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
            printf("snoop: no such process\n");
            return;
        }
        if (!traceProcess(pid)) printf("snoop: no such process\n");
        ptrace(PTRACE_DETACH, pid, NULL, NULL);
        return;
    }
    if (command->argc < 2) {
        printf("snoop: invalid syntax\n");
        return;
    }

    char *path = resolveCommand(command->argv[1]);
    if (path == NULL) {
        printf("snoop: command not found\n");
        return;
    }
    pid_t pid = fork();
    if (pid == -1) {
        free(path);
        printf("snoop: command not found\n");
        return;
    }
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        raise(SIGSTOP);
        execve(path, &command->argv[1], environ);
        _exit(127);
    }
    free(path);
    traceProcess(pid);
}
