#include "activities.h"
#include "jobs.h"
#include <stdio.h>

void activitiesRun(const Command *command) {
    if (command->argc != 1)
        printf("activities: invalid syntax\n");
    else
        jobsPrintActivities();
}
