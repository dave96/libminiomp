#include "libminiomp.h"

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

/* Called when encountering a taskloop directive. */

void
GOMP_taskloop (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
               long arg_size, long arg_align, unsigned flags,
               unsigned long num_tasks, int priority,
               long start, long end, long step)
{
    long it_number;
    long total_count = end - start;

    it_number = total_count/step;

    long tasknum = 0;
    if (flags & GOMP_TASK_FLAG_GRAINSIZE) {
        tasknum = it_number / num_tasks;
        if (it_number % num_tasks != 0) tasknum++;
    } else {
        if (num_tasks == 0) num_tasks = omp_get_num_threads();
        tasknum = num_tasks;
    }

    if (it_number < tasknum) tasknum = it_number;

    long it_size = it_number / tasknum;
    long it_step = it_size * step;

    for(unsigned long i = 0; i < tasknum; ++i) {
        // Create task
        miniomp_task_t * task = malloc(sizeof(miniomp_task_t) + (arg_size + arg_align - 1));
        task->fn = fn;

        long taskEnd;

        if (__builtin_expect (i == tasknum-1, 0)) {
            taskEnd = end;
        } else {
            taskEnd = start + it_step;
        }

        task->buf = (void *) (task + 1);

        if (__builtin_expect (cpyfn != NULL, 0)) {
            task->data = (char *) (((uintptr_t) task->buf + arg_align - 1)
                    & ~(uintptr_t) (arg_align - 1));
            cpyfn (task->data, data);
        } else {
            task->data = task->buf;
            memcpy (task->buf, data, arg_size);
        }
        ((long *)task->data)[0] = start;
        ((long *)task->data)[1] = taskEnd;

        start += it_step;
        while(!enqueue(&miniomp_taskqueue, task)) try_execute_task();
    }

    wait_no_running_tasks(); // Taskwait implicit.
}
