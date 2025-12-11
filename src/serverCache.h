#ifndef _SERVER_CACHE_H_
#define _SERVER_CACHE_H_

#include <stdlib.h>

typedef struct cacheNode{
    char* path;             // KEY
    char* header;
    char* content;          // DATA
    size_t size;

    struct cacheNode* prev;        // Previous node to make each bucket contain a doubly linked list
    struct cacheNode* next;        // Next node to make each bucket contain a doubly linked list

} cacheNode;

typedef struct{

    cacheNode** buckets;

    int mSize;
    size_t cSize;

} cache;

// Creates a cache object
cache* createCache(cache* cache);

// Inserts value onto CACHE cache associated to KEY key
// Returns 0 on success and -1 on failure;
int cacheInsert(cache* cache, const char* key, const char* head, const char* body);

// Looks for a given key in the CACHE cache hash table
// If a value is found return a pointer to it, if not return NULL
void* cacheLookup(cache* cache, const char* key);

// Removes a given key value pair from the cache hash table
// Returns 0 on success and -1 on failure
int cacheRemove(cache* cache, const char* key);

// Frees memory occupied by a cache struct and all it's components
void destroyCache(cache* cache);

#endif