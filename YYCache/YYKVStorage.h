//
//  YYKVStorage.h
//  YYCache <https://github.com/ibireme/YYCache>
//
//  Created by ibireme on 15/4/22.
//  Copyright (c) 2015 ibireme.
//
//  This source code is licensed under the MIT-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

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

typedef NS_ENUM(NSUInteger, YYKVStorageType) {
    
    /// The `value` is stored as a file in file system.
    // 发生了错误
    YYKVStorageTypeFile = 0,
    
    // mysql存储
    YYKVStorageTypeSQLite = 1,
    
    // 文件存储
    YYKVStorageTypeMixed = 2,
};



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

NS_ASSUME_NONNULL_END
