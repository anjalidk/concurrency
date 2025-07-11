#include <stdio.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>

#define BUCKETS         4
#define PAGE_SIZE       1024 
#define MIN_SIZE        128

typedef struct __node {
    void* addr;
    int size;
    struct __node* next;
} Node_t;

typedef struct {
    Node_t* head;
    void* start_addr;
    int num_free_elem;
    pthread_mutex_t mtx;
} List_t;

typedef struct {
    List_t free_list[BUCKETS];
    pthread_mutex_t glb_mtx;
} MemCache_t;

MemCache_t memc;

int size_to_bucket (int size) {
    if (size > ((pow(2, (BUCKETS-1))) * MIN_SIZE))
        return -1;
    return (int)log2(size/MIN_SIZE);
}

int bucket_to_size (int bucket) {
    return ((pow(2, bucket)) * MIN_SIZE);
}

Node_t* alloc_node (int index, int i) {
    Node_t* node = (Node_t*) malloc(sizeof(Node_t)*1);
    if (node == NULL)
        return NULL;
    node->size = bucket_to_size(index);
    node->addr = memc.free_list[index].start_addr + (i * node->size); 
    node->next = NULL;
    return node;
}

int init_free_list (int index) {
    List_t* list = &memc.free_list[index];
    Node_t* prev = NULL, *curr;

    for (int i = 0; i < list->num_free_elem; i++) {
        curr = alloc_node(index, i);
        if (curr == NULL)
            return -1;
        if (prev == NULL) {
            list->head = curr;
        } else {
            prev->next = curr;
        }
        //printf("%p-->", curr->addr);
        prev = curr;
    }
    //printf("\n");
    return 0;
}

int init_cache (void) {
    pthread_mutex_init(&memc.glb_mtx, NULL);
    pthread_mutex_lock(&memc.glb_mtx);
    for (int i = 0; i < BUCKETS; i++) {
        memc.free_list[i].start_addr = 
            mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
        if (memc.free_list[i].start_addr == (void*)-1) {
            printf("Mmap failed, errno %d\n", errno);
            return -1;
        }
        memc.free_list[i].num_free_elem = PAGE_SIZE/bucket_to_size(i); 
        printf("start_addr for index %d is %p, num_free_elems %d\n", i, 
                                            memc.free_list[i].start_addr,
                                            memc.free_list[i].num_free_elem);
        init_free_list(i);
        pthread_mutex_init(&memc.free_list[i].mtx, NULL);
    }
    pthread_mutex_unlock(&memc.glb_mtx);
    return 0;
}

Node_t* malloc_size (int size) {
    int index = size_to_bucket(size);
    pthread_mutex_lock(&memc.free_list[index].mtx);
    Node_t* node = memc.free_list[index].head;
    if (node == NULL) {
        printf("Out of memory for allocating size %d\n", size);
        pthread_mutex_unlock(&memc.free_list[index].mtx);
        return NULL;
    }
    memc.free_list[index].head = node->next;
    node->next = NULL;
    pthread_mutex_unlock(&memc.free_list[index].mtx);
    return node;
}

void free_size (Node_t* data) {
    int index = size_to_bucket(data->size);
    pthread_mutex_lock(&memc.free_list[index].mtx);
    data->next = memc.free_list[index].head;
    memc.free_list[index].head = data;
    printf("Freeing data %lx\n", *(unsigned long*)data);
    pthread_mutex_unlock(&memc.free_list[index].mtx);
}

int main (void) {
    Node_t* ms, *ms1;
    char* ch_addr;

    init_cache();
    ms = malloc_size(128);
    if (ms != NULL)
        printf("Malloc'ed size 128, addr %lx\n", *(unsigned long*)ms);
    ms = malloc_size(512);
    if (ms != NULL)
        printf("Malloc'ed size 512, addr %lx\n", *(unsigned long*)ms);
    ms = malloc_size(128);
    if (ms != NULL)
        printf("Malloc'ed size 128, addr %lx\n", *(unsigned long*)ms);
    ms = malloc_size(256);
    if (ms != NULL)
        printf("Malloc'ed size 256, addr %lx\n", *(unsigned long*)ms);
    ms = malloc_size(1024);
    if (ms != NULL) {
        printf("Malloc'ed size 1024, addr %lx\n", *(unsigned long*)ms);
        ch_addr = (char*) *(unsigned long*)ms;
        *ch_addr = 'a';
        printf("Wrote %c to %lx\n", *ch_addr, *(unsigned long*)ms); 
    }
    ms1 = malloc_size(1024);
    if (ms1 != NULL)
        printf("Malloc'ed size 1024, addr %lx\n", *(unsigned long*)ms1);
    if (ms)
        free_size(ms);
    ms = malloc_size(1024);
    if (ms != NULL)
        printf("Malloc'ed size 1024, addr %lx\n", *(unsigned long*)ms);
}

