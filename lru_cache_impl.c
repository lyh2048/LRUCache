#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>
#include "lru_cache.h"
#include "lru_cache_impl.h"

/**
 * 
 * 封装 Posix semaphores
 */
void unix_error(char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
void Sem_init(sem_t *sem, int pshared, unsigned int value)
{
    if (sem_init(sem, pshared, value) < 0)
    {
        unix_error("Sem_init error");
    }
}
void P(sem_t *sem)
{
    if (sem_wait(sem) < 0)
    {
        unix_error("P error");
    }
}
void V(sem_t *sem)
{
    if (sem_post(sem) < 0)
    {
        unix_error("V error");
    }
}

/**
 * 双向链表
 */

// 从双向链表中删除指定的结点
static void removeFromList(LRUCache *cache, cacheEntry *entry)
{
    // 链表为空
    if (cache->lruListSize == 0)
    {
        return;
    }
    if (entry == cache->lruListHead && entry == cache->lruListTail)
    {
        // 链表只有一个结点的情况
        P(&cache->cache_lock);
        cache->lruListHead = cache->lruListTail = NULL;
        V(&cache->cache_lock);
    } 
    else if (entry == cache->lruListHead)
    {
        // 要删除的结点是头结点
        P(&cache->cache_lock);
        cache->lruListHead = entry->lruListNext;
        cache->lruListHead->lruListPrev = NULL;
        V(&cache->cache_lock);
    }
    else if (entry == cache->lruListTail)
    {
        // 要删除的结点是尾结点
        P(&cache->cache_lock);
        cache->lruListTail = entry->lruListPrev;
        cache->lruListTail->lruListNext = NULL;
        V(&cache->cache_lock);
    }
    else
    {
        P(&cache->cache_lock);
        entry->lruListPrev->lruListNext = entry->lruListNext;
        entry->lruListNext->lruListPrev = entry->lruListPrev;
        V(&cache->cache_lock);
    }
    // 链表长度减一
    P(&cache->cache_lock);
    --cache->lruListSize;
    V(&cache->cache_lock);
}
// 将结点插入到双向链表的表头
static cacheEntry* insertToListHead(LRUCache *cache, cacheEntry *entry)
{
    cacheEntry * removedEntry = NULL;
    P(&cache->cache_lock);
    ++cache->lruListSize;
    V(&cache->cache_lock);

    // 缓存满了
    if (cache->cacheCapacity < cache->lruListSize)
    {
        // 需要删除尾结点，即淘汰最久没有访问到的缓存单元
        removedEntry = cache->lruListTail;
        removeFromList(cache, cache->lruListTail);
    }

    // 如果此时双向链表为空
    if (cache->lruListHead == NULL && cache->lruListTail == NULL)
    {
        P(&cache->cache_lock);
        cache->lruListHead = cache->lruListTail = entry;
        V(&cache->cache_lock);
    }
    else
    {
        // 此时双向链表非空，应该插入表头
        P(&cache->cache_lock);
        entry->lruListNext = cache->lruListHead;
        entry->lruListPrev = NULL;
        cache->lruListHead->lruListPrev = entry;
        cache->lruListHead = entry;
        V(&cache->cache_lock);
    }
    return removedEntry;
}
// 释放整个链表
static void freeList(LRUCache *cache)
{
    // 链表为空
    if (0 == cache->lruListSize)
    {
        return;
    }
    // 遍历链表释放所有结点
    cacheEntry *entry = cache->lruListHead;
    while (entry)
    {
        cacheEntry *temp = entry->hashListNext;
        free(entry);
        entry = temp;
    }
    cache->lruListSize = 0;
}
// 保证最近访问的结点总是位于链表的表头
static void updateLRUList(LRUCache *cache, cacheEntry *entry)
{
    // 将结点从链表中删除
    removeFromList(cache, entry);
    // 将结点插入链表的表头
    insertToListHead(cache, entry);
}

/**
 * 哈希表
 */

// 哈希函数
static unsigned int hashKey(LRUCache *cache, char *key)
{
    unsigned int len = strlen(key);
    unsigned int b = 378551;
    unsigned int a = 63689;
    unsigned int hash = 0;
    unsigned int i = 0;
    for (i = 0; i < len; key++, i++)
    {
        hash = hash * a + (unsigned int)(*key);
        a = a * b;       
    }
    return hash % (cache->cacheCapacity);
}
// 从哈希表中获取缓存单元
static cacheEntry *getValueFromHashMap(LRUCache *cache, char *key)
{
    // 1. 使用哈希函数定位数据存放于哪个槽
    cacheEntry *entry = cache->hashMap[hashKey(cache, key)];
    // 2. 遍历查询槽内链表，找到对应的数据项
    while (entry)
    {
        if (!strncmp(entry->key, key, KEY_SIZE))
        {
            break;
        }
        entry = entry->hashListNext;
    }
    return entry;
}
// 向哈希表中插入缓存单元
static void insertEntryToHashMap(LRUCache *cache, cacheEntry *entry)
{
    // 1. 使用哈希函数定位数据位于哪个槽
    cacheEntry *n = cache->hashMap[hashKey(cache, entry->key)];
    P(&cache->cache_lock);
    if (n!=NULL)
    {
        // 如果槽内已有其他数据项，将槽内的数据项与当前要加入的项链成链表
        // 当前要加入的数据项是表头
        entry->hashListNext = n;
        n->hashListPrev = entry;
    }
    // 3. 将数据加入槽内
    cache->hashMap[hashKey(cache, entry->key)] = entry;
    V(&cache->cache_lock);
}
// 从哈希表中删除缓存单元
static void removeEntryFromHashMap(LRUCache *cache, cacheEntry *entry)
{
    if (NULL == entry || NULL == cache || NULL == cache->hashMap)
    {
        return;
    }
    // 使用hash函数定义数据项位于哪个槽
    cacheEntry *n = cache->hashMap[hashKey(cache, entry->key)];
    // 遍历槽内链表，找到要删除的结点，将该结点删除
    while (n)
    {
        if (n->key == entry->key)
        {
            // 找到要删除的结点
            if (n->hashListPrev)
            {
                P(&cache->cache_lock);
                n->hashListPrev->hashListNext = n->hashListNext;
                V(&cache->cache_lock);
            }
            else
            {
                P(&cache->cache_lock);
                cache->hashMap[hashKey(cache, entry->key)] = n->hashListNext;
                V(&cache->cache_lock);
            }
            if (n->hashListNext)
            {
                P(&cache->cache_lock);
                n->hashListNext->hashListPrev = n->hashListPrev;
                V(&cache->cache_lock);
            }
            return;
        }
        n = n->hashListNext;
    }
}


/**
 * LRU缓存及缓存单元
 * 
 */

// 创建一个缓存单元
static cacheEntry* newCacheEntry(char *key, char *data)
{
    cacheEntry* entry = NULL;
    if (NULL == (entry = malloc(sizeof(*entry))))
    {
        perror("malloc error");
        return NULL;
    }
    memset(entry, 0, sizeof(*entry));
    strncpy(entry->key, key, KEY_SIZE);
    strncpy(entry->data, data, VALUE_SIZE);
    Sem_init(&(entry->entry_lock), 0, 1);
    return entry;
}
// 释放一个缓存单元
static void freeCacheEntry(cacheEntry* entry)
{
    if (NULL == entry)
    {
        return;
    }
    free(entry);
}
// 创建一个LRU缓存
int LRUCacheCreate(int capacity, void **lruCache)
{
    LRUCache* cache = NULL;
    if (NULL == (cache=malloc(sizeof(*cache))))
    {
        perror("malloc error");
        return -1;
    }
    memset(cache, 0, sizeof(*cache));
    cache->cacheCapacity = capacity;
    Sem_init(&(cache->cache_lock), 0, 1);
    cache->hashMap = malloc(sizeof(cacheEntry*)*capacity);
    if (NULL == cache->hashMap)
    {
        free(cache);
        perror("malloc error");
        return -1;
    }
    memset(cache->hashMap, 0, sizeof(cacheEntry*)*capacity);
    *lruCache = cache;
    return 0;
}
// 释放一个LRU缓存
int LRUCacheDestory(void *lruCache)
{
    LRUCache *cache = (LRUCache*) lruCache;
    if (NULL == cache)
    {
        return 0;
    }
    if (cache->hashMap)
    {
        free(cache->hashMap);
    }
    freeList(cache);
    free(cache);
    return 0;
}
// 将数据放入LRU缓存中
int LRUCacheSet(void *lruCache, char *key, char *data)
{
    LRUCache* cache = (LRUCache*)lruCache;
    // 从哈希表中查找数据是否已经在缓存中
    cacheEntry *entry = getValueFromHashMap(cache, key);
    // 数据在缓存中
    if (entry != NULL)
    {
        // 更新数据
        P(&entry->entry_lock);
        strncpy(entry->data, data, VALUE_SIZE);
        V(&entry->entry_lock);
        updateLRUList(cache, entry);
    }
    else
    {
        // 数据没在缓存中
        // 新建缓存单元
        entry =newCacheEntry(key, data);
        // 将新建的缓存单元插入双向链表的表头
        cacheEntry *removedEntry = insertToListHead(cache, entry);
        if (NULL != removedEntry)
        {
            // 插入过程中，缓存满了
            // 淘汰最久没有访问的缓存单元
            removeEntryFromHashMap(cache, removedEntry);
            freeCacheEntry(removedEntry);
        }
        // 将新建缓存单元插入哈希表
        insertEntryToHashMap(cache, entry);
    }
    return 0;
}
// 从缓存中获取数据
char *LRUCacheGet(void *lruCache, char *key)
{
    LRUCache* cache = (LRUCache*)lruCache;
    // 从哈希表中查找数据是否已经在缓存中
    cacheEntry *entry = getValueFromHashMap(cache, key);
    if (NULL != entry)
    {
        // 更新至链表的表头并返回数据
        updateLRUList(cache, entry);
        return entry->data;
    }
    else 
    {
        // 不存在该数据
        return NULL;
    }
}
// 遍历缓存，打印缓存中的数据，按访问时间从新到旧顺序输出
void LRUCachePrint(void *lruCache)
{
    LRUCache *cache = (LRUCache*)lruCache;
    if (NULL == cache || 0 == cache->lruListSize)
    {
        return;
    }
    fprintf(stdout, "\n>>>>>>>>>>>>>>>\n");
    fprintf(stdout, "cache (key data):\n");
    cacheEntry* entry = cache->lruListHead;
    while (entry)
    {
        fprintf(stdout, "(%s, %s)", entry->key, entry->data);
        entry = entry->lruListNext;
    }
    fprintf(stdout, "\n<<<<<<<<<<<<<<<\n");
}