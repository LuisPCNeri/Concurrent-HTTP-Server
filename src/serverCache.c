#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "serverCache.h"

// Using the known hashing algorithm djb2 as found in http://www.cse.yorku.ca/~oz/hash.html
static unsigned long hash_djb2(unsigned char* str){
    unsigned long hash = 5381;
    int c;

    while(c = *str++)
        hash = ((hash << 5 ) + hash) + c;   // hash * 33 + c

    return hash;
}

static int getBucket(cache* c, char* str){
    return hash_djb2(str) % c->mSize;
}

static cacheNode* createEntry(char* key, char* data){
    cacheNode* entry = (cacheNode*) malloc(sizeof(cacheNode));
    entry->path = (char*) malloc(sizeof(key));
    if(!entry->path) return -1;

    strcpy(entry->path, key);

    entry->content = (char*) malloc(sizeof(data));
    if(!entry->content) return -1;
    
    strcpy(entry->content, data);

    return entry;
}

static void delCacheItem(cacheNode* node){
    free(node->path);
    free(node->content);
    free(node);
}

cache* createCache(size_t maxSizeMB){
    // By default maxSizeMB oughta be 10
    size_t mSizeB = maxSizeMB*1024*1024;
    cache* c = (cache*) malloc(mSizeB);

    // Max size for the hash set hard coded to 20
    c->mSize = 10;
    c->cSize = 0;
    c->buckets = calloc(c->mSize, sizeof(cacheNode*));

    return c;
}

int cacheInsert(cache* c, const char* key, const char* data){
    // Get bucket where key should be stored
    int bucketIndex = getBucket(c, key);

    // Create a new node
    cacheNode* node = createEntry(key, data);

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

    // If that bucket was already empty don't loop through an empty linked list
    if(node == NULL) return NULL;

    while(node){
        if(strcmp(key, node->path) == 0) 
            return node->content;

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
    for(int i = 0; i < c->cSize; i++){
        cacheNode* n = c->buckets[i];

        // If node is not null
        if(n) delCacheItem(n);
    }

    free(c->buckets);
    free(c);
}