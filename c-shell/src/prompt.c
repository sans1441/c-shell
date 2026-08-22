#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "prompt.h"

char shell_home[PATH_MAX];

void getHomeShell()
{
    getcwd(shell_home, sizeof(shell_home));
    // printf("shell_home = %s\n", shell_home);
}

void printPath()
{
    char username[256];
    char hostname[256];
    char path[PATH_MAX];

    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);

    strcpy(username, pw->pw_name);
    gethostname(hostname, sizeof(hostname));
    getcwd(path, sizeof(path));

    int home_length = strlen(shell_home);

    if(strcmp(path, shell_home) == 0)
    {
        printf("%s@%s:~$ ", username, hostname);
    }
    else if(strncmp(path, shell_home, home_length) == 0 && path[home_length] == '/')
    {
        printf("%s@%s:~%s$ ", username, hostname, path + home_length);
    }
    else
    {
        printf("%s@%s:%s$ ", username, hostname, path);
    }
}

char *readLine()
{
    char *line = malloc(256);

    char *result = fgets(line, sizeof(line), stdin);

    if(result == NULL)
        return NULL;

    line[strcspn(line, "\n")] = '\0';

    return line;
}