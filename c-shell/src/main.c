#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <limits.h>

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

    char *home = pw->pw_dir;

    if(strncmp(path, home, strlen(home)) == 0)
    {
        printf("%s@%s:~%s$ ", username, hostname, path + strlen(home));
    }
    else
    {
        printf("%s@%s:%s$ ", username, hostname, path);
    }
}

int main()
{
    while(1)
    {
        printPath();
        char str[256];
        fgets(str, sizeof(str), stdin);
    }
}