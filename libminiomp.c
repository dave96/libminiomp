#include <stdlib.h>
#include "libminiomp.h"

pthread_mutex_t miniomp_default_lock;

// Library constructor and desctructor
void init_miniomp(void) __attribute__((constructor));
void fini_miniomp(void) __attribute__((destructor));

void init_miniomp(void) {
    // Parse OMP_NUM_THREADS environment variable to initialize nthreads_var internal control variable
    parse_env();
    
    // Initialize Pthread data structures and thread-specific data
    pthread_key_create(&miniomp_specifickey, NULL);
    pthread_setspecific(miniomp_specifickey, (void *) (intptr_t) (int) 0);
    miniomp_threads = malloc(sizeof(pthread_t) * MAX_THREADS);
    miniomp_parallel = malloc(sizeof(miniomp_parallel_t) * MAX_THREADS);
    
    // Initialize OpenMP default lock and default barrier
    pthread_mutex_init(&miniomp_default_lock, NULL);
    
    // Initialize OpenMP workdescriptors for loop and single and taskqueue
    init_task_queue(&miniomp_taskqueue, MAXELEMENTS_TQ);

    // Initialize counters
    
    in_taskgroup = false;
    taskgroup_count_in_execution = 0;
    tasks_in_execution = 0;
    __sync_synchronize();
}

void fini_miniomp(void) {
    // free structures allocated during library initialization
    pthread_key_delete(miniomp_specifickey);
    free(miniomp_threads);
    free(miniomp_parallel);
    pthread_mutex_destroy(&miniomp_default_lock);
    destroy_task_queue(&miniomp_taskqueue);
}
