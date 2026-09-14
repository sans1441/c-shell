#ifndef JOBS_H
#define JOBS_H

#include <signal.h>
#include <stddef.h>
#include <sys/types.h>

void jobsInit(void);
void jobsSetPromptCallback(void (*callback)(void));
int jobsNotificationFd(void);
void jobsBlockSignals(sigset_t *old_mask);
void jobsRestoreSignals(const sigset_t *old_mask);
int jobsAdd(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count);
void jobsSetForeground(int active);
void jobsProcessNotifications(int redraw_prompt);
void jobsPrintActivities(void);

#endif
