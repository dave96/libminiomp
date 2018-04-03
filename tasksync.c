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
    // Nothing to do
}

void
GOMP_taskgroup_end (void)
{
    GOMP_taskwait();
}
