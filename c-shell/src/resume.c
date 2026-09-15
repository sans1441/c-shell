#include "resume.h"
#include "jobs.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parseJobNumber(const char *value, int *number) {
    if (value[0] != '%' || value[1] == '\0') return 0;
    char *end = NULL;
    errno = 0;
    long parsed = strtol(value + 1, &end, 10);
    if (errno != 0 || *end != '\0' || parsed <= 0 || parsed > INT_MAX) return 0;
    *number = (int)parsed;
    return 1;
}

static int parseTimeout(const char *value, unsigned int *seconds) {
    if (value[0] == '\0') return 0;
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(value, &end, 10);
    if (errno != 0 || *end != '\0' || parsed > UINT_MAX) return 0;
    *seconds = (unsigned int)parsed;
    return 1;
}

void resumeRun(const Command *command) {
    int job_number;
    unsigned int timeout = 0;
    int foreground = 0;
    int valid = (command->argc == 3 || command->argc == 5) && parseJobNumber(command->argv[1], &job_number);
    if (valid && strcmp(command->argv[2], "fg") == 0)
        foreground = 1;
    else if (valid && strcmp(command->argv[2], "bg") != 0)
        valid = 0;
    if (valid && command->argc == 5)
        valid = foreground && strcmp(command->argv[3], "--timeout") == 0 && parseTimeout(command->argv[4], &timeout);

    if (!valid) {
        printf("resume: invalid syntax\n");
        return;
    }

    ResumeResult result = jobsResume(job_number, foreground, timeout);
    if (result == RESUME_NO_JOB || result == RESUME_ERROR) printf("resume: no such job\n");
}
