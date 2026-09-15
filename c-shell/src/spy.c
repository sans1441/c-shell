#include "spy.h"
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char **paths;
    size_t count;
    size_t capacity;
} PathList;

static char *readLinkPath(const char *path) {
    size_t capacity = 256;
    char *value = malloc(capacity);
    if (value == NULL) return NULL;

    while (1) {
        ssize_t length = readlink(path, value, capacity - 1);
        if (length == -1) {
            free(value);
            return NULL;
        }
        if ((size_t)length < capacity - 1) {
            value[length] = '\0';
            return value;
        }
        capacity *= 2;
        char *expanded = realloc(value, capacity);
        if (expanded == NULL) {
            free(value);
            return NULL;
        }
        value = expanded;
    }
}

static const char *fileType(const char *path) {
    if (strncmp(path, "pipe:[", 6) == 0) return "FIFO";
    if (strncmp(path, "socket:[", 8) == 0) return "SOCK";
    struct stat info;
    if (stat(path, &info) == -1) return "UNKNOWN";
    if (S_ISREG(info.st_mode)) return "REG";
    if (S_ISDIR(info.st_mode)) return "DIR";
    if (S_ISCHR(info.st_mode)) return "CHR";
    if (S_ISBLK(info.st_mode)) return "BLK";
    if (S_ISFIFO(info.st_mode)) return "FIFO";
    if (S_ISSOCK(info.st_mode)) return "SOCK";
    return "UNKNOWN";
}

static void printEntry(pid_t pid, const char *fd, const char *path) {
    printf("%-6d %-5s %-6s %s\n", (int)pid, fd, fileType(path), path);
}

static int pathListContains(const PathList *list, const char *path) {
    for (size_t i = 0; i < list->count; i++)
        if (strcmp(list->paths[i], path) == 0) return 1;
    return 0;
}

static int pathListAdd(PathList *list, const char *path) {
    if (pathListContains(list, path)) return 1;
    if (list->count == list->capacity) {
        size_t capacity = list->capacity == 0 ? 8 : list->capacity * 2;
        char **paths = realloc(list->paths, capacity * sizeof(char *));
        if (paths == NULL) return 0;
        list->paths = paths;
        list->capacity = capacity;
    }
    list->paths[list->count] = strdup(path);
    if (list->paths[list->count] == NULL) return 0;
    list->count++;
    return 1;
}

static void freePathList(PathList *list) {
    for (size_t i = 0; i < list->count; i++) free(list->paths[i]);
    free(list->paths);
}

static void printSpecialEntry(pid_t pid, const char *proc_dir, const char *name) {
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", proc_dir, name);
    char *target = readLinkPath(path);
    if (target != NULL) {
        printEntry(pid, strcmp(name, "exe") == 0 ? "txt" : name, target);
        free(target);
    }
}

static void printMemoryEntries(pid_t pid, const char *proc_dir) {
    char maps_path[PATH_MAX];
    snprintf(maps_path, sizeof(maps_path), "%s/maps", proc_dir);
    FILE *maps = fopen(maps_path, "r");
    if (maps == NULL) return;

    PathList paths = {0};
    char line[PATH_MAX * 2];
    while (fgets(line, sizeof(line), maps) != NULL) {
        char mapped_path[PATH_MAX];
        mapped_path[0] = '\0';
        if (sscanf(line, "%*s %*s %*s %*s %*s %4095[^\n]", mapped_path) != 1) continue;
        if (mapped_path[0] != '/' || pathListContains(&paths, mapped_path)) continue;
        if (!pathListAdd(&paths, mapped_path)) break;
        printEntry(pid, "mem", mapped_path);
    }
    freePathList(&paths);
    fclose(maps);
}

static int isNumericName(const char *name) {
    if (*name == '\0') return 0;
    for (const char *character = name; *character != '\0'; character++)
        if (*character < '0' || *character > '9') return 0;
    return 1;
}

static void printDescriptors(pid_t pid, const char *proc_dir) {
    char fd_dir_path[PATH_MAX];
    snprintf(fd_dir_path, sizeof(fd_dir_path), "%s/fd", proc_dir);
    DIR *fd_dir = opendir(fd_dir_path);
    if (fd_dir == NULL) return;

    struct dirent *entry;
    while ((entry = readdir(fd_dir)) != NULL) {
        if (!isNumericName(entry->d_name)) continue;
        char link_path[PATH_MAX + NAME_MAX + 2];
        snprintf(link_path, sizeof(link_path), "%s/%s", fd_dir_path, entry->d_name);
        char *target = readLinkPath(link_path);
        if (target != NULL) {
            printEntry(pid, entry->d_name, target);
            free(target);
        }
    }
    closedir(fd_dir);
}

void spyRun(const Command *command) {
    if (command->argc > 2) {
        printf("spy: invalid syntax\n");
        return;
    }

    pid_t pid = getpid();
    if (command->argc == 2) {
        char *end = NULL;
        errno = 0;
        long parsed = strtol(command->argv[1], &end, 10);
        if (errno != 0 || *end != '\0' || parsed <= 0 || parsed > INT_MAX) {
            printf("spy: no such process\n");
            return;
        }
        pid = (pid_t)parsed;
    }

    char proc_dir[PATH_MAX];
    snprintf(proc_dir, sizeof(proc_dir), "/proc/%d", (int)pid);
    struct stat info;
    if (stat(proc_dir, &info) == -1 || !S_ISDIR(info.st_mode)) {
        printf("spy: no such process\n");
        return;
    }

    printf("PID    FD    TYPE   PATH\n");
    printSpecialEntry(pid, proc_dir, "cwd");
    printSpecialEntry(pid, proc_dir, "exe");
    printMemoryEntries(pid, proc_dir);
    printDescriptors(pid, proc_dir);
}
