#include "libminiomp.h"

// This file implements the PARALLEL construct

// Declaration of array for storing pthread identifier from pthread_create function
pthread_t *miniomp_threads;

// Global variable for parallel descriptor
miniomp_parallel_t *miniomp_parallel;

// Declaration of per-thread specific key
pthread_key_t miniomp_specifickey;

// Global control variables
unsigned int miniomp_current_threads;
unsigned int miniomp_threads_arrived;

// This is the prototype for the Pthreads starting function
void * worker(void * args) {
    // insert all necessary code here for:
    pthread_setspecific(miniomp_specifickey, args);
    miniomp_parallel_t * data = (miniomp_parallel_t *) args;
    
    if (data->id == 0) {
        // Parallel_single: only thread 0 executes function.
        data->fn(data->fn_data);
    }
    
    // IMPLICIT BARRIER
    __sync_fetch_and_add(&miniomp_threads_arrived, 1);
    // END IMPLICIT BARRIER
    
    while(miniomp_threads_arrived < miniomp_current_threads || !is_empty(&miniomp_taskqueue)) {
        // dequeue is an "atomic" operation
        miniomp_task_t * t = dequeue(&miniomp_taskqueue);
        if(t != NULL) {
            task_execute(t);
            task_destroy(t);
        }
    }
    
    pthread_exit(NULL);
    
    return (NULL);
}

void GOMP_parallel (void (*fn) (void *), void *data, unsigned num_threads, unsigned int flags) {
    if(!num_threads) num_threads = miniomp_icv.nthreads_var;
    if(num_threads > MAX_THREADS) num_threads = MAX_THREADS;
    pthread_barrier_init(&miniomp_barrier, NULL, num_threads); 
    // Reset control variables
    miniomp_current_threads = num_threads;
    miniomp_threads_arrived = 0;
    
    // Thread creation.    
    for(int i = 0; i < num_threads; ++i) {
        miniomp_parallel[i].fn = fn;
        miniomp_parallel[i].fn_data = data;
        miniomp_parallel[i].id = i;
        
        pthread_create(&(miniomp_threads[i]), NULL, &worker, &(miniomp_parallel[i]));
    }
    
    for(int i = 0; i < num_threads; ++i)
        pthread_join(miniomp_threads[i], NULL);

    pthread_barrier_destroy(&miniomp_barrier);
}
