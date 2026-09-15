#include "path_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *resolveCommand(const char *name) {
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
