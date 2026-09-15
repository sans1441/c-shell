#include "terminal.h"
#include <signal.h>
#include <unistd.h>

static pid_t shell_pgid;
static int interactive_terminal;

void terminalInit(void) {
    interactive_terminal = isatty(STDIN_FILENO);
    shell_pgid = getpid();
    if (!interactive_terminal) return;

    signal(SIGTTOU, SIG_IGN);
    setpgid(shell_pgid, shell_pgid);
    tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void terminalGiveTo(pid_t pgid) {
    if (interactive_terminal) tcsetpgrp(STDIN_FILENO, pgid);
}

void terminalReclaim(void) {
    if (interactive_terminal) tcsetpgrp(STDIN_FILENO, shell_pgid);
}

void terminalPrepareChild(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}
