# YYCache



### [简介](https://github.com/lyimin/YYCache#1、简介)
### [原理](https://github.com/lyimin/YYCache#2、原理)
### [YYCache源码分析](https://github.com/lyimin/YYCache#3、YYCache源码分析)
### [YYMemoryCache源码分析](https://github.com/lyimin/YYCache#4、YYMemoryCache源码分析)

### 1、简介

[YYCache](https://github.com/ibireme/YYCache) 是2015年由[ibireme](https://github.com/ibireme)发布到Github的一个高性能、线程安全的缓存框架。笔者在阅读YYCache源码时并没有太大阻碍，
代码质量很高，逻辑清晰，性能还非常好。下图时作者的测试性能数据：
![](https://github.com/lyimin/YYCache/blob/master/res/yymemorycache.png)
![](https://github.com/lyimin/YYCache/blob/master/res/yydiskcache.png)

### 2、原理

先看看YYCache大致框架
![](https://github.com/lyimin/YYCache/blob/master/res/yycache.png)
	
- **YYCache**: 提供对外开发的相关接口。
- **YYMemoryCache**: 提供容量小的内存缓存接口。
	- **_YYLinkedMap**: 双向链表类，负责管理链表的增删改查。
	- **_YYLinkedMapNode**: 链表的一个节点，存储缓存内容的key，value，以及指向上一个节点和下一个节点的指针等
- **YYDiskCache**: 提供容量大的磁盘缓存接口。
	- **YYKVStorage**: 磁盘缓存底层实现类，封装了增删改查给YYDiskCache层调用。
	- **YYKVStorageItem**: 磁盘存储的缓存对象。

笔者在阅读源码时，学习到作者用了**双向链表**和**hash**的方案来处理容量比较少的内存缓存，也就是YYMemoryCache。而相对于容量大的数据，作者处理的方案是**20kb**内直接用sqlite存储，大于20kb的数据先用文件存储，再把文件路径存储到sqlite

### 3、YYCache源码分析

#### YYCache提供的接口

```objective-c

@interface YYCache : NSObject

// 缓存名称
@property (copy, readonly) NSString *name;
// 内存缓存对象
@property (strong, readonly) YYMemoryCache *memoryCache;
// 磁盘缓存对象
@property (strong, readonly) YYDiskCache *diskCache;

// 根据名称创建
- (nullable instancetype)initWithName:(NSString *)name;
// 根据路径创建
- (nullable instancetype)initWithPath:(NSString *)path NS_DESIGNATED_INITIALIZER;

+ (nullable instancetype)cacheWithName:(NSString *)name;
+ (nullable instancetype)cacheWithPath:(NSString *)path;

- (instancetype)init UNAVAILABLE_ATTRIBUTE;
+ (instancetype)new UNAVAILABLE_ATTRIBUTE;

// 根据key判断缓存内容是否存在
- (BOOL)containsObjectForKey:(NSString *)key;
- (void)containsObjectForKey:(NSString *)key withBlock:(nullable void(^)(NSString *key, BOOL contains))block;

// 根据key获取缓存内容
- (nullable id<NSCoding>)objectForKey:(NSString *)key;
- (void)objectForKey:(NSString *)key withBlock:(nullable void(^)(NSString *key, id<NSCoding> object))block;

// 添加缓存对象
- (void)setObject:(nullable id<NSCoding>)object forKey:(NSString *)key;
- (void)setObject:(nullable id<NSCoding>)object forKey:(NSString *)key withBlock:(nullable void(^)(void))block;

// 删除缓存对象
- (void)removeObjectForKey:(NSString *)key;
- (void)removeObjectForKey:(NSString *)key withBlock:(nullable void(^)(NSString *key))block;
- (void)removeAllObjects;
- (void)removeAllObjectsWithBlock:(void(^)(void))block;
- (void)removeAllObjectsWithProgressBlock:(nullable void(^)(int removedCount, int totalCount))progress
                                 endBlock:(nullable void(^)(BOOL error))end;

@end
```

可以看出YYCache提供的接口还是相当友好的，与**NSCache**相似。
下面抽出几个方法看看具体实现

### YYCache接口相关实现
```objective-c
// 取值
- (id<NSCoding>)objectForKey:(NSString *)key {
    // 从内存缓存中获取值
    id<NSCoding> object = [_memoryCache objectForKey:key];
    if (!object) {
        // 如果内存缓存没有，再从磁盘缓存中获取
        object = [_diskCache objectForKey:key];
        if (object) {
            // 如果磁盘缓存中有，缓存到内存
            [_memoryCache setObject:object forKey:key];
        }
    }
    return object;
}

// 存储
- (void)setObject:(id<NSCoding>)object forKey:(NSString *)key {
    // 缓存到内存
    [_memoryCache setObject:object forKey:key];
    // 缓存到磁盘
    [_diskCache setObject:object forKey:key];
}

// 删除
- (void)removeObjectForKey:(NSString *)key {
    [_memoryCache removeObjectForKey:key];
    [_diskCache removeObjectForKey:key];
}
```

可以看出YYCache的接口都是先读内存，如果内存没有再去读磁盘。还有一个细节是，当内存中没有值，而磁盘中有值时，作者会把这个值再缓存去内存中，提高下次读取速度。

