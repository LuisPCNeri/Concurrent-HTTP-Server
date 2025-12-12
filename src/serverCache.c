#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serverCache.h"
#include "config.h"

// COLORS :)
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED "\033[31m"

serverConf* conf;

// Using the known hashing algorithm djb2 as found in http://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash_djb2(const char* str){
    unsigned long hash = 5381;
    int c;

    while( (c = *str++) )
        hash = ((hash << 5 ) + hash) + c;   // hash * 33 + c

    return hash;
}

// Get the bucket correspondent to the given PATH
// Returns the bucket index
static int getBucket(cache* c, const char* str){
    return hash_djb2(str) % c->mSize;
}

// Removes the last item from the LRU doubly linked list
static void LRUEvict(cache* c, cacheNode* node) {
    if (node->LRUprev != NULL) {
        node->LRUprev->LRUnext = node->LRUnext;
    } else {
        // This was the head
        c->LRUhead = node->LRUnext;
    }

    if (node->LRUnext != NULL) {
        node->LRUnext->LRUprev = node->LRUprev;
    } else {
        // This was the tail
        c->LRUtail = node->LRUprev;
    }
    node->LRUprev = NULL;
    node->LRUnext = NULL;
}

// ADDS a node to the LRU basically making it the most recently used node
static void LRUAdd(cache* c, cacheNode* node) {
    node->LRUnext = c->LRUhead;
    node->LRUprev = NULL;

    if (c->LRUhead != NULL) {
        c->LRUhead->LRUprev = node;
    }
    c->LRUhead = node;

    if (c->LRUtail == NULL) {
        c->LRUtail = node;
    }
}

// Creates a cacheNode object
// Returns a pointer to a cacheNode object or NULL if there was an error
static cacheNode* createEntry(const char* key, const char* header, const char* data, size_t dataLen, int status){
    cacheNode* entry = (cacheNode*) malloc(sizeof(cacheNode));

    if (!key || !header || !data) {
        printf(RED "BAD STRING >:(" RESET "\n");
        return NULL;
    }

    entry->path = (char*) malloc(strlen(key) + 1);
    memcpy(entry->path, key, strlen(key) + 1);

    entry->header = (char*) malloc(strlen(header) + 1);
    memcpy(entry->header, header, strlen(header) + 1);

    entry->content = (char*) malloc(dataLen);
    memcpy(entry->content, data, dataLen);

    // Change size to the given len and status to the status defined in http respons
    entry->size = dataLen;
    entry->status = status;

    // Initialize ALL pointers
    entry->prev = entry->next = NULL;
    entry->LRUprev = entry->LRUnext = NULL;

    return entry;
}

static void delCacheItem(cacheNode* node){
    free(node->path);
    free(node->header);
    free(node->content);
    free(node);
}

cache* createCache(cache* c){
    if(!conf){
        conf = (serverConf*) malloc(sizeof(serverConf));
        loadConfig("server.conf", conf);
    }

    // Max size for the hash set hard coded to 1009
    c->mSize = 1009;
    c->cSize = 0;
    c->buckets = calloc(c->mSize, sizeof(cacheNode*));

    c->LRUhead = NULL;
    c->LRUtail = NULL;

    return c;
}

int cacheInsert(cache* c, const char* key, const char* header ,const char* body, size_t body_len, int status){
    // Get bucket where key should be stored
    int bucketIndex = getBucket(c, key);

    // cache is BIGGER or equal to 10MB
    while (c->cSize + body_len >= (size_t) conf->CACHE_SIZE_MB * 1024 * 1024 && c->LRUtail != NULL) {
        printf(RED "Cache is full. Evicting least recently used item: %s" RESET "\n", c->LRUtail->path);
        cacheRemove(c, c->LRUtail->path);
    }

    // Create a new node
    cacheNode* node = createEntry(key, header, body, body_len, status);

    c->cSize += sizeof(cacheNode) + strlen(key) + strlen(header) + body_len + 2;

    if( node == NULL ) return -1;

    node->prev = NULL;
    node->next = c->buckets[bucketIndex];

    // If there already is an item in the linked list
    if( c->buckets[bucketIndex] != NULL ) c->buckets[bucketIndex]->prev = node;
    c->buckets[bucketIndex] = node;

    LRUAdd(c, node);

    return 0;
}

void* cacheLookup(cache* c, const char* key){
    int bIndex = getBucket(c, key) % c->mSize;

    // Get the first node in the bucket
    cacheNode* node = c->buckets[bIndex];

    while(node){
        if(strcmp(key, node->path) == 0){
            // FOUND in cache so remove from it's position in LRU and add it to the head
            LRUEvict(c, node);
            LRUAdd(c, node);

            return node;
        }
        // If key did not match go to the next node
        node = node->next;
    }

    // Nothing was found, return a null pointer
    return NULL;
}

int cacheRemove(cache* c, const char* key){
    int bIndex = getBucket(c, key) % c->mSize;
    cacheNode* node = c->buckets[bIndex];

    while(node){
        if(strcmp(key, node->path) == 0){

            if(node->prev)
                node->prev->next = node->next;
            else
                // If node we are removing is the head
                c->buckets[bIndex] = node->next;

            if(node->next)
                node->next->prev = node->prev;

            LRUEvict(c, node);
            // Subtract node's TOTAL size from the doubly linked list
            c->cSize -= (sizeof(cacheNode) + strlen(node->path) + strlen(node->header) + node->size + 2);
            delCacheItem(node);
            return 0; // Item found and removed, we can exit.
        }

        node = node->next;
    }

    return 0;
}

void destroyCache(cache* c){
    // Free all items from all buckets
    for(int i = 0; i < c->mSize; i++){
        cacheNode* n = c->buckets[i];

        while(n){
            cacheNode* next = n->next;
            delCacheItem(n);
            n = next;
        }

        // If node is not null
        if(n) delCacheItem(n);
    }

    if(conf) free(conf);

    free(c->buckets);
    free(c);
}