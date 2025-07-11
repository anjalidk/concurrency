#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

#define QSIZE   10
#define STR_SIZE    50
#define BUCKETS     10

typedef struct {
    int head, tail;
    sem_t sem_prod, sem_cons;
    sem_t semp, semc;
    char str[QSIZE][STR_SIZE];
} Queue_str_t;

typedef struct {
    Queue_str_t queue[BUCKETS];
} Htable_t;

Htable_t    htable;
char* new_str = NULL;

int hash_index (char* key) {
    int hash = 0;
    while (*key) {
        hash = (hash << 5) + *key;
        key++;
    }
    hash = hash % BUCKETS;
    return hash;
}

void init (void) {
    for (int i = 0; i < BUCKETS; i++) {
        htable.queue[i].head = 0;
        htable.queue[i].tail = 0;
        sem_init(&htable.queue[i].sem_prod, 0, 100);
        sem_init(&htable.queue[i].sem_cons, 0, 0);
        sem_init(&htable.queue[i].semp, 0, 1);
        sem_init(&htable.queue[i].semc, 0, 1);
    }
}

void* producer (void* args) {
    int len, tail;
    char* key = (char*) args;
    int index = hash_index(key); 

    sem_wait(&htable.queue[index].sem_prod);
    sem_wait(&htable.queue[index].semp);
    tail = htable.queue[index].tail;
    len = strlen(key) > STR_SIZE-1 ? STR_SIZE-1 : strlen(key);
    strncpy(htable.queue[index].str[tail], key, len);
    printf("Storing key %s at hash index %d, tail %d\n", key, index, tail);
    htable.queue[index].tail = (htable.queue[index].tail + 1) % QSIZE;
    sem_post(&htable.queue[index].semp);
    sem_post(&htable.queue[index].sem_cons);
    return NULL;
}

void* consumer (void* args) {
    int head;
    char* key = (char*) args;
    int index = hash_index(key);

    sem_wait(&htable.queue[index].sem_cons);
    sem_wait(&htable.queue[index].semc);
    head = htable.queue[index].head;
    new_str = strdup(htable.queue[index].str[head]);
    printf("Get key %s from hash index %d, head %d\n", new_str, index, head);
    htable.queue[index].head = (htable.queue[index].head + 1) % QSIZE;
    sem_post(&htable.queue[index].semc);
    sem_post(&htable.queue[index].sem_prod);
    return NULL;
}

int main (void) {
    pthread_t thread1, thread2;
    char key[] = "abcd";
    init();
    pthread_create(&thread1, NULL, *producer, (void*) &key);
    pthread_create(&thread2, NULL, *consumer, (void*) &key);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    if (new_str)
        printf("new_str is %s\n", new_str);
    else
        printf("new_str is NULL\n");
}
