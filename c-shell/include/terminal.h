#ifndef TERMINAL_H
#define TERMINAL_H

#include <sys/types.h>

void terminalInit(void);
void terminalGiveTo(pid_t pgid);
void terminalReclaim(void);
void terminalPrepareChild(void);

#endif
