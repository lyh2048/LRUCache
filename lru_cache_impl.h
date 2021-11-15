#ifndef LRUCACHEIMPL_H
#define LRUCACHEIMPL_H

#include <semaphore.h>

#define KEY_SIZE 50
#define VALUE_SIZE 100

/*定义LRU缓存的缓存单元*/
typedef struct cacheEntry {
    char key[KEY_SIZE];
    char data[VALUE_SIZE];

    sem_t entry_lock;
    // 缓存哈希表指针，指向哈希链表的前一个元素
    struct cacheEntry *hashListPrev;
    // 缓存哈希表指针，指向哈希链表的后一个元素
    struct cacheEntry *hashListNext;
    // 缓存双向链表指针，指向链表的前一个元素
    struct cacheEntry *lruListPrev;
    // 缓存双向链表指针，指向链表的后一个元素
    struct cacheEntry *lruListNext;

}cacheEntry;

/*定义LRU缓存*/
typedef struct LRUCache
{
    sem_t cache_lock;
    // 缓存容量
    int cacheCapacity;
    // 缓存的双向链表结点个数
    int lruListSize;
    // 缓存的哈希表
    cacheEntry **hashMap;
    // 缓存的双向链表表头
    cacheEntry *lruListHead;
    // 缓存的双向链表表尾
    cacheEntry *lruListTail;

}LRUCache;


#endif
