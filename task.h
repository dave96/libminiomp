#include <pthread.h>

/* This structure describes a "task" to be run by a thread.  */
typedef struct {
    void (*fn)(void *);
    void (*data);
    void * buf;
} miniomp_task_t;

typedef struct {
    int max_elements;
    int count;
    int head;
    int tail;
    pthread_mutex_t lock_queue;
    miniomp_task_t **queue;
} miniomp_taskqueue_t;

extern miniomp_taskqueue_t miniomp_taskqueue;

#define MAXELEMENTS_TQ 128

void task_destroy(miniomp_task_t * t);
void task_execute(miniomp_task_t * t);
void wait_no_running_tasks();
void try_execute_task();

bool is_empty(miniomp_taskqueue_t *task_queue);
bool is_full(miniomp_taskqueue_t *task_queue);
bool enqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor); 
miniomp_task_t * dequeue(miniomp_taskqueue_t *task_queue);
void init_task_queue(miniomp_taskqueue_t * queue, int max_elements);
void destroy_task_queue(miniomp_taskqueue_t * queue);
