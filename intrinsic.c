#include "libminiomp.h"

void omp_set_num_threads (int n) {
    if(n > MAX_THREADS) n = MAX_THREADS;
    if(n <= 0) n = 1;
    miniomp_icv.nthreads_var = n;
}

int omp_get_num_threads (void) {
    return(miniomp_current_threads);
}

int omp_get_thread_num (void) {
    int data = 
        (int) (intptr_t) pthread_getspecific(miniomp_specifickey);
  return(data);
}
