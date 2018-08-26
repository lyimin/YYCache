# YYCache

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

### 4、YYMemoryCache源码分析

YYMemoryCache负责处理容量小的，速度快的内存缓存，API和性能方面与NSCache相似，所有方法都是线程安全。不同点是

- YYMemoryCache是采用LRU算法来移除使用频率小的对象。
- 在清除缓存时，YYMemoryCache提供了三种方式，按照Count, Cost, age清除，而且YYMemoryCache支持手动和自动清除。

``` objective-c
@interface YYMemoryCache : NSObject

#pragma mark - Attribute
// 名称
@property (nullable, copy) NSString *name;
// 缓存总数
@property (readonly) NSUInteger totalCount;
// 缓存内存总大小
@property (readonly) NSUInteger totalCost;

#pragma mark - Limit
// 缓存数量限制
@property NSUInteger countLimit;
// 内存限制
@property NSUInteger costLimit;
// 时间限制
@property NSTimeInterval ageLimit;
// 自动清除时间限制
@property NSTimeInterval autoTrimInterval;
// 当有内存警告时，是否全部清除
@property BOOL shouldRemoveAllObjectsOnMemoryWarning;
@property BOOL shouldRemoveAllObjectsWhenEnteringBackground;

// 收到内存警告回调
@property (nullable, copy) void(^didReceiveMemoryWarningBlock)(YYMemoryCache *cache);
// 退回后台回调
@property (nullable, copy) void(^didEnterBackgroundBlock)(YYMemoryCache *cache);
// 是否在主线程中清除
@property BOOL releaseOnMainThread;
// 是否异步是否对象
@property BOOL releaseAsynchronously;


#pragma mark - Access Methods
// 判断缓存是否存在
- (BOOL)containsObjectForKey:(id)key;
// 获取缓存
- (nullable id)objectForKey:(id)key;
// 写入缓存
- (void)setObject:(nullable id)object forKey:(id)key;
- (void)setObject:(nullable id)object forKey:(id)key withCost:(NSUInteger)cost;

// 删除缓存
- (void)removeObjectForKey:(id)key;
- (void)removeAllObjects;


#pragma mark - Trim
// 根据数量清除缓存
- (void)trimToCount:(NSUInteger)count;
// 根据内存大小清除
- (void)trimToCost:(NSUInteger)cost;
// 根据时间清除
- (void)trimToAge:(NSTimeInterval)age;
@end

```

#### LRU淘汰算法特点

LRU（Least recently used，最近最少使用）算法根据数据的历史访问记录来进行淘汰数据，其核心思想是“如果数据最近被访问过，那么将来被访问的几率也更高”。他的特点是
- 新数据插入到链表头部
- 每次缓存数据被访问，将数据移到链表头部
- 当链表满的时候，将链表尾部的数据丢弃

![](https://github.com/lyimin/YYCache/blob/master/res/lru.png)

#### _YYLinkedMapNode 和 _YYLinkedMap
上文中说到了LRU算法，这里的**_YYLinkedMap**相当于一个链表，而**_YYLinkedMapNode**其实就是链表中的一个节点。**_YYLinkedMap**负责节点的增删改查，而**_YYLinkedMapNode**则负责保存数据。

```objective-c

/**
    相当于_YYLinkedMap的一个节点
 */
@interface _YYLinkedMapNode : NSObject {
    @package
    __unsafe_unretained _YYLinkedMapNode *_prev; // retained by dic
    __unsafe_unretained _YYLinkedMapNode *_next; // retained by dic
    id _key;
    id _value;
    NSUInteger _cost; // 记录缓存对象大小
    NSTimeInterval _time; // 记录缓存对象时间
}
@end

@implementation _YYLinkedMapNode
@end


/**
    YYMemoryCache 内的一个链表
 */
@interface _YYLinkedMap : NSObject {
    @package
    CFMutableDictionaryRef _dic; 
    NSUInteger _totalCost;
    NSUInteger _totalCount;
    _YYLinkedMapNode *_head; // 链表
    _YYLinkedMapNode *_tail; // 链尾
    BOOL _releaseOnMainThread; // 是否在主线程清除
    BOOL _releaseAsynchronously; // 是否在子线程清除对象
}

// 插入数据到链表
- (void)insertNodeAtHead:(_YYLinkedMapNode *)node;
// 移动数据到链表
- (void)bringNodeToHead:(_YYLinkedMapNode *)node;
// 删除数据
- (void)removeNode:(_YYLinkedMapNode *)node;
// 删除链尾数据
- (_YYLinkedMapNode *)removeTailNode;
// 删除所有数据
- (void)removeAll;

@end

```

#### YYMemoryCache线程安全
```objective-c
@implementation YYMemoryCache {
    pthread_mutex_t _lock; // 线程锁
    _YYLinkedMap *_lru; // 链表
    dispatch_queue_t _queue; // 串行队列
}

- (instancetype)init {
    self = super.init;
    pthread_mutex_init(&_lock, NULL);
    _lru = [_YYLinkedMap new];
    _queue = dispatch_queue_create("com.ibireme.cache.memory", DISPATCH_QUEUE_SERIAL);
    
    ···
}
```

作者这里用了**pthread_mutex_t**和串行队列来，可以看到在**_YYLinkedMap**的增删改查上，作者都加上了线程锁来保证线程安全。

#### 异步释放对象

```objective-c
- (void)_trimToCount:(NSUInteger)countLimit {
    '''
    if (holder.count) {
        dispatch_queue_t queue = _lru->_releaseOnMainThread ? dispatch_get_main_queue() : YYMemoryCacheGetReleaseQueue();
        dispatch_async(queue, ^{
            [holder count]; // release in queue
        });
    }
}

- (void)_trimToCost:(NSUInteger)costLimit {
    '''
    if (holder.count) {
        dispatch_queue_t queue = _lru->_releaseOnMainThread ? dispatch_get_main_queue() : YYMemoryCacheGetReleaseQueue();
        dispatch_async(queue, ^{
            [holder count]; // release in queue
        });
    }
}


- (void)_trimToCount:(NSUInteger)countLimit {
    '''
    if (holder.count) {
        dispatch_queue_t queue = _lru->_releaseOnMainThread ? dispatch_get_main_queue() : YYMemoryCacheGetReleaseQueue();
        dispatch_async(queue, ^{
            [holder count]; // release in queue
        });
    }
}
```

作者在处理完要删除的数据后，都新开辟了一个异步线程调用**[holder count]**，按道理，holder如果不调用count方法的话，应该在trimToXXX方法调用完，系统就会回收内存，作者为了性能考虑，将内存的释放放到了子线程去处理，因为block中引用了holder，所以holder会在block里面释放而不是在trimToXXX方法中释放。这一点我很佩服作者对性能的要求。

### 5、YYDiskCache 源码分析

YYDiskCache提供的API跟YYMemoryCache相似，还是一如既往的整洁，不同的是YYDiskCache是基于**file**和**sqlite**结合来做磁盘缓存

```objective-c

// 磁盘缓存
@interface YYDiskCache : NSObject

#pragma mark - Attribute

@property (nullable, copy) NSString *name; // 名称
@property (readonly) NSString *path;  // 缓存路径
@property (readonly) NSUInteger inlineThreshold; // 数据大于这个值就是文件存储，默认是20kb
@property (nullable, copy) NSData *(^customArchiveBlock)(id object);

@property (nullable, copy) id (^customUnarchiveBlock)(NSData *data);

@property (nullable, copy) NSString *(^customFileNameBlock)(NSString *key);

#pragma mark - Limit

@property NSUInteger countLimit; // 数量限制
@property NSUInteger costLimit; // 大小限制
@property NSTimeInterval ageLimit; // 时间限制

@property NSUInteger freeDiskSpaceLimit;

@property NSTimeInterval autoTrimInterval;

@property BOOL errorLogsEnabled;

#pragma mark - Initializer

- (instancetype)init UNAVAILABLE_ATTRIBUTE;
+ (instancetype)new UNAVAILABLE_ATTRIBUTE;

- (nullable instancetype)initWithPath:(NSString *)path;

- (nullable instancetype)initWithPath:(NSString *)path
                      inlineThreshold:(NSUInteger)threshold NS_DESIGNATED_INITIALIZER;


#pragma mark - Access Methods

- (BOOL)containsObjectForKey:(NSString *)key;
- (void)containsObjectForKey:(NSString *)key withBlock:(void(^)(NSString *key, BOOL contains))block;

- (nullable id<NSCoding>)objectForKey:(NSString *)key;
- (void)objectForKey:(NSString *)key withBlock:(void(^)(NSString *key, id<NSCoding> _Nullable object))block;

- (void)setObject:(nullable id<NSCoding>)object forKey:(NSString *)key;
- (void)setObject:(nullable id<NSCoding>)object forKey:(NSString *)key withBlock:(void(^)(void))block;

- (void)removeObjectForKey:(NSString *)key;
- (void)removeObjectForKey:(NSString *)key withBlock:(void(^)(NSString *key))block;
- (void)removeAllObjects;
- (void)removeAllObjectsWithBlock:(void(^)(void))block;
- (void)removeAllObjectsWithProgressBlock:(nullable void(^)(int removedCount, int totalCount))progress
                                 endBlock:(nullable void(^)(BOOL error))end;

- (NSInteger)totalCount;
- (void)totalCountWithBlock:(void(^)(NSInteger totalCount))block;
- (NSInteger)totalCost;
- (void)totalCostWithBlock:(void(^)(NSInteger totalCost))block;


#pragma mark - Trim
- (void)trimToCount:(NSUInteger)count;
- (void)trimToCount:(NSUInteger)count withBlock:(void(^)(void))block;

- (void)trimToCost:(NSUInteger)cost;
- (void)trimToCost:(NSUInteger)cost withBlock:(void(^)(void))block;

- (void)trimToAge:(NSTimeInterval)age;
- (void)trimToAge:(NSTimeInterval)age withBlock:(void(^)(void))block;

#pragma mark - Extended Data
+ (nullable NSData *)getExtendedDataFromObject:(id)object;
+ (void)setExtendedData:(nullable NSData *)extendedData toObject:(id)object;
@end

```

#### YYKVStorageItem 与 YYKVStorage

YYDiskCache不会直接操作缓存对象，而是通过YYKVStorage来间接操作缓存对象。通过用YYKVStorageItem来存储键值对和元数据，而YYKVSotage来管理YYKVStorageItem，提供增删改查方法给外部调用。

```objective-c

// YYKVStorage 中用来存储键值对和元数据的类
@interface YYKVStorageItem : NSObject
@property (nonatomic, strong) NSString *key;                ///< key
@property (nonatomic, strong) NSData *value;                ///< value
@property (nullable, nonatomic, strong) NSString *filename; ///< filename (nil if inline)
@property (nonatomic) int size;                             ///< value's size in bytes
@property (nonatomic) int modTime;                          ///< modification unix timestamp
@property (nonatomic) int accessTime;                       ///< last access unix timestamp
@property (nullable, nonatomic, strong) NSData *extendedData; ///< extended data (nil if no extended data)
@end


//  YYKVSotage用来管理YYKVStorageItem，提供增删改查方法给外部调用
@interface YYKVStorage : NSObject

#pragma mark - Attribute
@property (nonatomic, readonly) NSString *path;        ///< The path of this storage.
@property (nonatomic, readonly) YYKVStorageType type;  ///< The type of this storage.
@property (nonatomic) BOOL errorLogsEnabled;           ///< Set `YES` to enable error logs for debug.

#pragma mark - Initializer
- (instancetype)init UNAVAILABLE_ATTRIBUTE;
+ (instancetype)new UNAVAILABLE_ATTRIBUTE;
- (nullable instancetype)initWithPath:(NSString *)path type:(YYKVStorageType)type NS_DESIGNATED_INITIALIZER;


#pragma mark - Save Items

- (BOOL)saveItem:(YYKVStorageItem *)item;
- (BOOL)saveItemWithKey:(NSString *)key value:(NSData *)value;
- (BOOL)saveItemWithKey:(NSString *)key
                  value:(NSData *)value
               filename:(nullable NSString *)filename
           extendedData:(nullable NSData *)extendedData;

#pragma mark - Remove Items

- (BOOL)removeItemForKey:(NSString *)key;
- (BOOL)removeItemForKeys:(NSArray<NSString *> *)keys;
- (BOOL)removeItemsLargerThanSize:(int)size;
- (BOOL)removeItemsEarlierThanTime:(int)time;
- (BOOL)removeItemsToFitSize:(int)maxSize;
- (BOOL)removeItemsToFitCount:(int)maxCount;
- (BOOL)removeAllItems;
- (void)removeAllItemsWithProgressBlock:(nullable void(^)(int removedCount, int totalCount))progress
                               endBlock:(nullable void(^)(BOOL error))end;


#pragma mark - Get Items

- (nullable YYKVStorageItem *)getItemForKey:(NSString *)key;
- (nullable YYKVStorageItem *)getItemInfoForKey:(NSString *)key;
- (nullable NSData *)getItemValueForKey:(NSString *)key;
- (nullable NSArray<YYKVStorageItem *> *)getItemForKeys:(NSArray<NSString *> *)keys;
- (nullable NSArray<YYKVStorageItem *> *)getItemInfoForKeys:(NSArray<NSString *> *)keys;
- (nullable NSDictionary<NSString *, NSData *> *)getItemValueForKeys:(NSArray<NSString *> *)keys;

#pragma mark - Get Storage Status
- (BOOL)itemExistsForKey:(NSString *)key;
- (int)getItemsCount;
- (int)getItemsSize;

@end
```

#### YYDiskCache线程安全

作者在做YYDiskCache线程安全时采用的方案是信号量

```objective-c
/// weak reference for all instances
static NSMapTable *_globalInstances;
static dispatch_semaphore_t _globalInstancesLock;

static void _YYDiskCacheInitGlobal() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        _globalInstancesLock = dispatch_semaphore_create(1);
        _globalInstances = [[NSMapTable alloc] initWithKeyOptions:NSPointerFunctionsStrongMemory valueOptions:NSPointerFunctionsWeakMemory capacity:0];
    });
}

static YYDiskCache *_YYDiskCacheGetGlobal(NSString *path) {
    if (path.length == 0) return nil;
    _YYDiskCacheInitGlobal();
    dispatch_semaphore_wait(_globalInstancesLock, DISPATCH_TIME_FOREVER);
    id cache = [_globalInstances objectForKey:path];
    dispatch_semaphore_signal(_globalInstancesLock);
    return cache;
}

static void _YYDiskCacheSetGlobal(YYDiskCache *cache) {
    if (cache.path.length == 0) return;
    _YYDiskCacheInitGlobal();
    dispatch_semaphore_wait(_globalInstancesLock, DISPATCH_TIME_FOREVER);
    [_globalInstances setObject:cache forKey:cache.path];
    dispatch_semaphore_signal(_globalInstancesLock);
}
```
