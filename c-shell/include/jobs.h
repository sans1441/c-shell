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
int jobsAdd(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count, const char *command_line);
void jobsSetForeground(int active);
void jobsProcessNotifications(int redraw_prompt);
void jobsPrintActivities(void);
int jobsTakeInteractiveSignal(void);
int jobsHasStopped(void);
int jobsAddStopped(pid_t pgid, pid_t first_pid, const pid_t *pids, const char *const *command_names, size_t process_count, const char *command_line);
void jobsTerminateAll(void);
typedef enum { RESUME_OK, RESUME_NO_JOB, RESUME_ERROR } ResumeResult;
ResumeResult jobsResume(int job_number, int foreground, unsigned int timeout_seconds);
int jobsPing(const char *target, int signal_number);

#endif
