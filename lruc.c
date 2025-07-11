#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define BUCKETS    5 

typedef struct __node {
    int key;
    int val;
    struct __node* next;
} Node_t;

typedef struct {
    Node_t* head;
    int items;
    pthread_mutex_t mtx;
} List_t;

typedef struct {
    List_t lists[BUCKETS];
    int capacity;
} LRUCache;

void lRUInit(LRUCache* lru, int c) {
    lru->capacity = c;
    for (int i = 0; i < BUCKETS; i++) {
        lru->lists[i].head = NULL;
        lru->lists[i].items = 0;
        pthread_mutex_init(&lru->lists[i].mtx, NULL);
    }
}

int lRUEvict (LRUCache* lru, int index) {
    Node_t *head, *prev;

    head = lru->lists[index].head;
    if (head == NULL) {
        printf("Nothing to evict\n");
        return -1;
    }
    prev = NULL;
    while (head->next) {
        prev = head;
        head = head->next;
    }
    if (prev == NULL) {
        lru->lists[index].head = NULL;
    } else {
        prev->next = NULL;
    }
    printf("Evicting element %d\n", head->val);
    free(head);
    lru->lists[index].items--;
    return 0;
}

Node_t* alloc_node (int key, int val) {
    Node_t* node = (Node_t*) malloc(sizeof(Node_t) * 1);
    node->key = key;
    node->val = val;
    node->next = NULL;
    return node;
}

int lRUCachePut (LRUCache* lru, int key, int val) {
    Node_t* node;

    int index = key % BUCKETS;
    node = alloc_node(key, val);
    if (node == NULL) {
        printf("fail in alloc_node, index %d\n", index);
        return -1;
    }
    pthread_mutex_lock(&lru->lists[index].mtx);
    if (lru->lists[index].items >= lru->capacity/BUCKETS) {
        if (lRUEvict(lru, index) != 0) {
            printf("Insert of %d at key %d failed\n", val, key);
            free(node);
            pthread_mutex_lock(&lru->lists[index].mtx);
            return -2;
        }
    }
    node->next = lru->lists[index].head;
    lru->lists[index].head = node;
    lru->lists[index].items++;
    pthread_mutex_unlock(&lru->lists[index].mtx);
    return 0;
}

int lRUCacheGet(LRUCache* lru, int key) {
    Node_t *head, *prev;

    int index = key % BUCKETS;
    pthread_mutex_lock(&lru->lists[index].mtx);
    head = lru->lists[index].head;
    prev = NULL;
    while (head) {
        if (head->key == key) {
            break;
        }
        prev = head;
        head = head->next;
    }
    if (head == NULL) {
        printf("Key %d not found\n", key);
        pthread_mutex_unlock(&lru->lists[index].mtx);
        return -1;
    }
    if (prev != NULL) {
        prev->next = head->next;
        head->next = lru->lists[index].head;
        lru->lists[index].head = head;
    }
    pthread_mutex_unlock(&lru->lists[index].mtx);
    return head->val;
}

void walk_cache(LRUCache* lru, int index) {
    pthread_mutex_lock(&lru->lists[index].mtx);
    Node_t *head = lru->lists[index].head;
    printf("Walking Index %d:\n", index);
    int count = 0;
    while (head != NULL) {
        printf("%d->",head->val);
        head = head->next;
        count++;
        if (count > 4)
            break;
    }
    pthread_mutex_unlock(&lru->lists[index].mtx);
    printf("\n");
}

int main (void) {
    int ret;
    LRUCache lru;
    lRUInit(&lru, 10);
    if (lRUCachePut(&lru, 1, 1) == -1)
        perror("lRUCachePut");
    if (lRUCachePut(&lru, 2, 2) == -1)
        perror("lRUCachePut");
    printf("After adding 1 & 2, cache is:\n");
    walk_cache(&lru, 1);
    walk_cache(&lru, 2);
    ret = lRUCacheGet(&lru, 1);
    printf("lRUCacheGet(1) returned %d\n", ret);
    
    if (lRUCachePut(&lru, 11, 11) == -1)
        perror("lRUCachePut");
    printf("After adding 11, bucket[1] is\n");
    walk_cache(&lru, 1);

    ret = lRUCacheGet(&lru, 1);
    printf("lRUCacheGet(1) returned %d\n", ret);
    walk_cache(&lru, 1);

    if (lRUCachePut(&lru, 21, 21) == -1)
        perror("lRUCachePut");
    printf("After adding 21, bucket[1] is\n");
    walk_cache(&lru, 1);
    ret = lRUCacheGet(&lru, 1);
    printf("lRUCacheGet(1) returned %d\n", ret);
    walk_cache(&lru, 1);
    ret = lRUCacheGet(&lru, 21);
    printf("lRUCacheGet(21) returned %d\n", ret);
    ret = lRUCacheGet(&lru, 2);
    printf("lRUCacheGet(2) returned %d\n", ret);
}
