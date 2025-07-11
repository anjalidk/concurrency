/* In this problem, we are presented with a multi-buffered system where 
 * multiple master threads produce data, and multiple worker threads consume 
 * this data. The unique aspect of the problem is that while each master 
 * thread is dedicated to its own buffer, the worker threads have the 
 * flexibility to consume data from any buffer that has data available. 
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>

#define NO_PRODUCERS    10
#define NO_CONSUMERS    10
#define QEMPTY          -1
#define QFULL           -2
#define QSIZE           10

typedef struct {
    int key;
    int val;
} Node_t;

typedef struct {
    int head, tail;
    Node_t data[QSIZE];
    pthread_mutex_t mtx;
} Queue_t;

typedef struct {
    Queue_t queues[NO_PRODUCERS];
    int no_prod, no_cons;
} Multi_t;

Multi_t mt;
volatile int data = -1;

void init_queue (Queue_t* q) {
    q->head = -1;
    q->tail = -1;
    pthread_mutex_init(&q->mtx, NULL);
}

void init_multi (Multi_t* mt) {
    mt->no_prod = NO_PRODUCERS;
    mt->no_cons = NO_CONSUMERS;
    for (int i = 0; i < NO_PRODUCERS; i++) {
        init_queue(&mt->queues[i]);
    }
}

void* producer (void* args) {
    int thread_id = *((int*) args);
    thread_id = thread_id % NO_PRODUCERS;
    pthread_mutex_lock(&mt.queues[thread_id].mtx);
    int tail = mt.queues[thread_id].tail;
    /* Q Full */
    if ((tail+1 % QSIZE) == mt.queues[thread_id].head) {
        printf("Q is full\n");
        return NULL;
    }

    mt.queues[thread_id].tail = (tail + 1) % QSIZE;
    tail = mt.queues[thread_id].tail;
    mt.queues[thread_id].data[tail].val = thread_id;
    mt.queues[thread_id].data[tail].key = thread_id;
    printf("Storing %d at tail %d, Q no %d\n", thread_id, tail, thread_id);
    if (mt.queues[thread_id].head == -1)
        mt.queues[thread_id].head = 0;
    pthread_mutex_unlock(&mt.queues[thread_id].mtx);
    return NULL;
}

void* consumer (void* args) {
    int ret;
    while (1) {
        for (int i = 0; i < NO_PRODUCERS; i++) {
            ret = pthread_mutex_trylock(&mt.queues[i].mtx);
            if (ret == 0) {
                /* Q Empty */
                if (mt.queues[i].head == -1) {
                    pthread_mutex_unlock(&mt.queues[i].mtx);
                    continue;
                }
                data = mt.queues[i].data[mt.queues[i].head].val; 
                if (mt.queues[i].head == mt.queues[i].tail) {
                    mt.queues[i].head = -1;
                    mt.queues[i].tail = -1;
                    printf("Q is now empty, head = tail = -1\n");
                } else {
                    mt.queues[i].head = (mt.queues[i].head + 1) % QSIZE;
                }
                pthread_mutex_unlock(&mt.queues[i].mtx);
                return NULL;
            }
        }
    }
}


int main (void) {
    pthread_t thread1, thread2;
    int key = 6;

    init_multi(&mt);
    pthread_create(&thread1, NULL, *producer, (void*) &key);
    pthread_create(&thread2, NULL, *consumer, (void*) &key);
    pthread_join(thread1, NULL);
    printf("data is %d\n", data);
}
