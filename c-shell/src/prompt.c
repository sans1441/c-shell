#include "prompt.h"
#include "jobs.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define LINE_CAPACITY 1026

char shell_home[PATH_MAX];

void getHomeShell() {
    getcwd(shell_home, sizeof(shell_home));
    // printf("shell_home = %s\n", shell_home);
}

void printPath() {
    char username[256];
    char hostname[256];
    char path[PATH_MAX];

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    strcpy(username, pw->pw_name);
    gethostname(hostname, sizeof(hostname));
    getcwd(path, sizeof(path));

    int home_length = strlen(shell_home);

    if (strcmp(path, shell_home) == 0) {
        printf("%s@%s:~$ ", username, hostname);
    } else if (strncmp(path, shell_home, home_length) == 0 && path[home_length] == '/') {
        printf("%s@%s:~%s$ ", username, hostname, path + home_length);
    } else {
        printf("%s@%s:%s$ ", username, hostname, path);
    }

    fflush(stdout);
}

char *readLine() {
    char *line = malloc(LINE_CAPACITY);
    size_t length = 0;
    if (line == NULL) return NULL;

    while (1) {
        struct pollfd descriptors[2];
        descriptors[0].fd = STDIN_FILENO;
        descriptors[0].events = POLLIN;
        descriptors[1].fd = jobsNotificationFd();
        descriptors[1].events = POLLIN;

        int result = poll(descriptors, 2, -1);
        if (result == -1) {
            if (errno == EINTR) continue;
            free(line);
            return NULL;
        }

        if (descriptors[1].revents & POLLIN) {
            jobsProcessNotifications(1);
            if (length > 0) write(STDOUT_FILENO, line, length);
        }

        if (!(descriptors[0].revents & (POLLIN | POLLHUP))) continue;

        char character;
        ssize_t bytes = read(STDIN_FILENO, &character, 1);
        if (bytes == 0) {
            if (length == 0) {
                free(line);
                return NULL;
            }
            break;
        }
        if (bytes == -1) {
            if (errno == EINTR) continue;
            free(line);
            return NULL;
        }
        if (character == '\n') break;
        if (length < LINE_CAPACITY - 1) line[length++] = character;
    }

    line[length] = '\0';
    return line;
}
