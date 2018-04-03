#include "libminiomp.h"

// Called when encountering taskwait and taskgroup constructs

void
GOMP_taskwait (void)
{ 
    // We'll only be here from the main thread, so let's just execute tasks until we're done.
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
