#include "libminiomp.h"

miniomp_taskqueue_t miniomp_taskqueue;

unsigned int tasks_in_execution;

// Initializes the task queue
void init_task_queue(miniomp_taskqueue_t * queue, int max_elements) {
     queue->queue = malloc(sizeof(miniomp_task_t *)*max_elements);
     queue->count = 0;
     queue->max_elements = max_elements;
     queue->head = 0;
     queue->tail = -1;
     pthread_mutex_init(&queue->lock_queue, NULL);
     __sync_synchronize();
}

void destroy_task_queue(miniomp_taskqueue_t * queue) {
    queue->max_elements = 0;
    pthread_mutex_destroy(&queue->lock_queue);
    free(queue->queue);
    __sync_synchronize();
}

// Checks if the task queue is empty
bool is_empty(miniomp_taskqueue_t *task_queue) {
    return (task_queue->count == 0);
}

// Checks if the task queue is full
bool is_full(miniomp_taskqueue_t *task_queue) {
    return (task_queue->count >= task_queue->max_elements);
}

// Enqueues the task descriptor at the tail of the task queue
bool enqueue(miniomp_taskqueue_t *task_queue, miniomp_task_t *task_descriptor) {
    if(is_full(task_queue)) return false;
    bool res = false;
    pthread_mutex_lock(&task_queue->lock_queue);
    if (!is_full(task_queue)) {
        task_queue->tail = (task_queue->tail + 1) % task_queue->max_elements;
        task_queue->count++;
        task_queue->queue[task_queue->tail] = task_descriptor;
        res = true;
    }
    pthread_mutex_unlock(&task_queue->lock_queue);
    return res;
}

// Returns the task descriptor at the head of the task queue
miniomp_task_t * dequeue(miniomp_taskqueue_t *task_queue) {
    if(is_empty(task_queue)) return NULL;
    miniomp_task_t * res = NULL;
    pthread_mutex_lock(&task_queue->lock_queue);
    if(!is_empty(task_queue)) {
        __sync_fetch_and_add(&tasks_in_execution, 1);
        res = task_queue->queue[task_queue->head];
        task_queue->head = (task_queue->head + 1) % task_queue->max_elements;
        task_queue->count--;
    }
    pthread_mutex_unlock(&task_queue->lock_queue);
    return res;
}

void task_destroy(miniomp_task_t * t) {
    free(t);
}

void try_execute_task() {
    miniomp_task_t * t = dequeue(&miniomp_taskqueue);
    if(t != NULL) {
        task_execute(t);
        task_destroy(t);
    }
}

void wait_no_running_tasks() {
    while(!is_empty(&miniomp_taskqueue)) try_execute_task();
    while(tasks_in_execution > 0) __sync_synchronize();
}

void task_execute(miniomp_task_t * t) {
    __sync_synchronize(); // Commit all memory operations
    t->fn(t->data);
    __sync_fetch_and_sub(&tasks_in_execution, 1);
    __sync_synchronize(); // Commit all memory operations
}

#define GOMP_TASK_FLAG_UNTIED           (1 << 0)
#define GOMP_TASK_FLAG_FINAL            (1 << 1)
#define GOMP_TASK_FLAG_MERGEABLE        (1 << 2)
#define GOMP_TASK_FLAG_DEPEND           (1 << 3)
#define GOMP_TASK_FLAG_PRIORITY         (1 << 4)
#define GOMP_TASK_FLAG_UP               (1 << 8)
#define GOMP_TASK_FLAG_GRAINSIZE        (1 << 9)
#define GOMP_TASK_FLAG_IF               (1 << 10)
#define GOMP_TASK_FLAG_NOGROUP          (1 << 11)

// Called when encountering an explicit task directive. Arguments are:
//      1. void (*fn) (void *): the generated outlined function for the task body
//      2. void *data: the parameters for the outlined function
//      3. void (*cpyfn) (void *, void *): copy function to replace the default memcpy() from 
//                                         function data to each task's data
//      4. long arg_size: specify the size of data
//      5. long arg_align: alignment of the data
//      6. bool if_clause: the value of if_clause. true --> 1, false -->0; default is set to 1 by compiler
//      7. unsigned flags: untied (1) or not (0) 

void GOMP_task (void (*fn) (void *), void *data, void (*cpyfn) (void *, void *),
           long arg_size, long arg_align, bool if_clause, unsigned flags,
           void **depend, int priority)
{
    miniomp_task_t * task = malloc(sizeof(miniomp_task_t) + (arg_size + arg_align - 1));
    
    task->fn = fn;
    
    task->buf = (void *) (task + 1);

    if (__builtin_expect (cpyfn != NULL, 0))
        {
          task->data = (char *) (((uintptr_t) task->buf + arg_align - 1)
                                & ~(uintptr_t) (arg_align - 1));
          cpyfn (task->data, data);
        }
    else
	{
          task->data = task->buf;
          memcpy (task->buf, data, arg_size);
	}

    while(!enqueue(&miniomp_taskqueue, task)) try_execute_task();
}
