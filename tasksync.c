#include "libminiomp.h"

// Called when encountering taskwait and taskgroup constructs

void
GOMP_taskwait (void)
{ 
    wait_no_running_tasks();
}

void
GOMP_taskgroup_start (void)
{
   in_taskgroup = true; 
}

void
GOMP_taskgroup_end (void)
{
    in_taskgroup = false;
    wait_no_running_tasks_group();
}
