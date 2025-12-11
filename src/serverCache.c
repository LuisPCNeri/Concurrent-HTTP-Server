#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serverCache.h"

// Using the known hashing algorithm djb2 as found in http://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash_djb2(const char* str){
    unsigned long hash = 5381;
    int c;

    while( (c = *str++) )
        hash = ((hash << 5 ) + hash) + c;   // hash * 33 + c

    return hash;
}

static int getBucket(cache* c, const char* str){
    return hash_djb2(str) % c->mSize;
}

static cacheNode* createEntry(const char* key, const char* header, const char* data){
    cacheNode* entry = (cacheNode*) malloc(sizeof(cacheNode));

    if (!key || !header || !data) {
        printf("BAD STRING >:(\n");
        return NULL;
    }

    entry->path = (char*) malloc(strlen(key) + 1);
    memcpy(entry->path, key, strlen(key) + 1);

    entry->header = (char*) malloc(strlen(header) + 1);
    memcpy(entry->header, header, strlen(header) + 1);

    entry->content = (char*) malloc(strlen(data) + 1);
    memcpy(entry->content, data, strlen(data) + 1);

    return entry;
}

static void delCacheItem(cacheNode* node){
    free(node->path);
    free(node->header);
    free(node->content);
    free(node);
}

cache* createCache(cache* c){
    // By default maxSizeMB oughta be 10

    // Max size for the hash set hard coded to 20
    c->mSize = 10;
    c->cSize = 0;
    c->buckets = calloc(c->mSize, sizeof(cacheNode*));

    return c;
}

int cacheInsert(cache* c, const char* key, const char* header ,const char* body){
    // Get bucket where key should be stored
    int bucketIndex = getBucket(c, key);

    // Create a new node
    cacheNode* node = createEntry(key, header, body);
    // TODO Checking of current cache size and comparing to 10MB

    

    c->cSize += sizeof(cacheNode) + strlen(key) + strlen(header) + strlen(body) + 3;

    // If node creation failed
    if( node == NULL ) return -1;

    // Put new node in the front of the doubly linked list
    node->prev = NULL;
    node->next = c->buckets[bucketIndex];

    // If there already is an item in the linked list
    if( c->buckets[bucketIndex] != NULL ) c->buckets[bucketIndex]->prev = node;

    // Set new linked list head to the new node created
    c->buckets[bucketIndex] = node;

    return 0;
}

void* cacheLookup(cache* c, const char* key){
    int bIndex = getBucket(c, key) % c->mSize;

    // Get the first node in the bucket
    cacheNode* node = c->buckets[bIndex];

    printf("LOOKING: %s\n", key); 

    while(node){
        printf("NODE: %s\n", node->path);
        if(strcmp(key, node->path) == 0){
            printf("FOUND IT\n");
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

            delCacheItem(node);
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

    free(c->buckets);
    free(c);
}