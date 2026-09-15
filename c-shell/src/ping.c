#include "ping.h"
#include "jobs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

static int parseSignalNumber(const char *value, int *signal_number) {
    if (value[0] == '\0' || value[0] == '-') return 0;
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(value, &end, 10);
    if (errno != 0 || *end != '\0') return 0;
    *signal_number = (int)(parsed % 64);
    return 1;
}

void pingRun(const Command *command) {
    int signal_number;
    if (command->argc != 3 || !parseSignalNumber(command->argv[2], &signal_number)) {
        printf("ping: invalid syntax\n");
    } else if (!jobsPing(command->argv[1], signal_number)) {
        printf("ping: no such process found\n");
    } else {
        printf("Sent signal %s to %s\n", command->argv[2], command->argv[1]);
    }
}
