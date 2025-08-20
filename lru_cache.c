#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "uthash.h"
 
typedef struct DList {
    int dkey;
    int val;
    struct DList* next;
    struct DList* prev;
} DList_t;

typedef struct {
    int hkey;
    DList_t* node_addr;
    UT_hash_handle hh;
} HashNode_t;

typedef struct {
    DList_t* dll_head;
    DList_t* dll_tail;
    int capacity;
    int count;
    pthread_mutex_t mtx;
    HashNode_t* hasht_head;
} LRUCache;

DList_t* alloc_dllnode (int key, int val) {
    DList_t* node = (DList_t*) malloc(sizeof(DList_t) * 1);
    if (!node)
        return NULL;
    node->dkey = key;
    node->val = val;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

HashNode_t* alloc_hash_node (int key, DList_t* node_addr) {
    HashNode_t* hn = (HashNode_t*) malloc(sizeof(HashNode_t) * 1);
    if (!hn)
        return NULL;
    hn->node_addr = node_addr;
    hn->hkey = key;
    return hn;
}

void insert_dll_node_front (LRUCache* cache, DList_t* node) {
    node->prev = cache->dll_head;
    node->next = cache->dll_head->next;
    cache->dll_head->next = node;
    node->next->prev = node;
}

void unlink_dll_node(DList_t* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

int GetCache (LRUCache* cache, int key, int val, int update) {
    HashNode_t* hn = NULL;

    pthread_mutex_lock(&cache->mtx);
    HASH_FIND_INT(cache->hasht_head, &key, hn);
    if (!hn) {
        pthread_mutex_unlock(&cache->mtx);
        return -1;
    }
    unlink_dll_node(hn->node_addr);
    insert_dll_node_front(cache, hn->node_addr);
    if (update)
        hn->node_addr->val = val;
    pthread_mutex_unlock(&cache->mtx);
    return hn->node_addr->val;
}

int lRUCacheGet(LRUCache* obj, int key) {
    return GetCache(obj, key, 0, 0);
}

int EvictCache (LRUCache* cache) {
    HashNode_t* hn = NULL;
    if (cache->count <= 0)
        return -1;
    HASH_FIND_INT(cache->hasht_head, &cache->dll_tail->prev->dkey, hn);
    if (hn) {
        unlink_dll_node(cache->dll_tail->prev);
        HASH_DEL(cache->hasht_head, hn);
        free(hn->node_addr);
        free(hn);
        cache->count--;
        return 0;
    }
    return -1;
}

int lRUCachePut (LRUCache* cache, int key, int val) {
    if (GetCache(cache, key, val, 1) != -1)
        return 0;
    DList_t* dll_node = alloc_dllnode(key, val);
    if (!dll_node)
        return -1;

    HashNode_t* hn = alloc_hash_node(key, dll_node);
    if (!hn)
        return -1;

    pthread_mutex_lock(&cache->mtx);
    if (cache->count >= cache->capacity) {
        if (EvictCache(cache) != 0) {
            free(dll_node);
            free(hn);
            pthread_mutex_unlock(&cache->mtx);
        }
    }
    insert_dll_node_front(cache, dll_node);
    HASH_ADD_INT(cache->hasht_head, hkey, hn);
    cache->count++;
    pthread_mutex_unlock(&cache->mtx);
    return 0;
}

void InitDll (LRUCache* cache) {
    cache->dll_head = alloc_dllnode (-1, -1);
    cache->dll_tail = alloc_dllnode (-1, -1);
    cache->dll_head->next = cache->dll_tail;
    cache->dll_tail->prev = cache->dll_head;
}

LRUCache* lRUCacheCreate(int capacity) {
    LRUCache* obj = (LRUCache*) malloc(sizeof(LRUCache) * 1);
    obj->capacity = capacity;
    obj->count = 0;
    pthread_mutex_init(&obj->mtx, NULL);
    obj->hasht_head = NULL;
    InitDll(obj);
    return obj;
}

void lRUCacheFree(LRUCache* obj) {
}
