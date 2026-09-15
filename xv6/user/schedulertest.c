#include "kernel/types.h"
#include "user/user.h"

static void
busy_for(int duration)
{
  uint start = uptime();
  volatile uint work = 0;

  while (uptime() - start < (uint)duration)
    work++;

  (void)work;
}

static void
run_workload(char *name, int cpu_burst, int sleep_ticks, int rounds)
{
  printf("schedulertest: %s pid=%d started\n", name, getpid());

  for (int round = 0; round < rounds; round++) {
    busy_for(cpu_burst);
    if (sleep_ticks > 0)
      pause(sleep_ticks);
  }

  printf("schedulertest: %s pid=%d finished\n", name, getpid());

  struct schedstats stats;
  uint finish = uptime();
  if (schedstats(&stats) == 0) {
    uint turnaround = finish - (uint)stats.arrival_tick;
    uint response = (uint)stats.first_run_tick - (uint)stats.arrival_tick;
    printf("schedulertest: metrics pid=%d turnaround=%d waiting=%d response=%d running=%d sleeping=%d\n",
           getpid(), turnaround, (uint)stats.waiting_ticks, response,
           (uint)stats.running_ticks, (uint)stats.sleeping_ticks);
  }
  exit(0);
}

int
main(void)
{
  int pid;

  pid = fork();
  if (pid == 0)
    run_workload("short", 3, 0, 4);

  pid = fork();
  if (pid == 0)
    run_workload("cpu-bound", 20, 0, 4);

  pid = fork();
  if (pid == 0)
    run_workload("io-bound", 2, 4, 12);

  pid = fork();
  if (pid == 0)
    run_workload("mixed", 8, 2, 8);

  for (int i = 0; i < 4; i++)
    wait(0);

  printf("schedulertest: all workloads finished\n");
  exit(0);
}
