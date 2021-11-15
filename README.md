# C语言实现LRU缓存
使用 C 语言实现 LRU 缓存，从中学习 LRU 缓存的基本概念、C 语言相关编程技巧，双向链表的 C 语言实现以及哈希表的 C 语言实现。

## 用法
```c
#include <stdio.h>
#include <string.h>

#include "lru_cache.h"
#include "lru_cache_impl.h"


int main()
{
    void *LRUCache;
    // 创建缓存
    if (0 == LRUCacheCreate(3, &LRUCache))
        printf("缓存创建成功，容量为: 3\n");
    // 向缓存中添加数据
    if (0 != LRUCacheSet(LRUCache, "key1", "value1"))
        printf("put (key1, value1) failed!\n");
    if (0 != LRUCacheSet(LRUCache, "key2", "value2"))
        printf("put (key2, value2) failed!\n");
    if (0 != LRUCacheSet(LRUCache, "key3", "value3"))
        printf("put (key3, value3) failed!\n");
    if (0 != LRUCacheSet(LRUCache, "key4", "value4"))
        printf("put (key4, value4) failed!\n");
    if (0 != LRUCacheSet(LRUCache, "key5", "value5"))
        printf("put (key5, value5) failed!\n");
    // 输出缓存中的内容
    LRUCachePrint(LRUCache);
    // 获取缓存中的数据
    if (NULL == LRUCacheGet(LRUCache, "key1"))
    {
        printf("key1 所对应的数据未被缓存\n");
    }
    else
    {
        char data[VALUE_SIZE];
        strncmp(data, LRUCacheGet(LRUCache, "key1"), VALUE_SIZE);
        printf("key1所对应的数据为: %s\n", data);
    }
    char data[VALUE_SIZE];
    strncpy(data, LRUCacheGet(LRUCache, "key4"), VALUE_SIZE);
    printf("key4所对应的数据为: %s\n", data);
    // 销毁缓存
    if (0 != LRUCacheDestory(LRUCache))
        printf("destory error\n");
    return 0;
}
```
输出：
```
缓存创建成功，容量为: 3

>>>>>>>>>>>>>>>
cache (key data):
(key5, value5)(key4, value4)(key3, value3)
<<<<<<<<<<<<<<<
key1 所对应的数据未被缓存
key4所对应的数据为: value4

```
## 参考资料
[https://github.com/Stand1210/c-LRU-](https://github.com/Stand1210/c-LRU-)