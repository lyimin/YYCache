//
//  YYDiskCache.h
//  YYCache <https://github.com/ibireme/YYCache>
//
//  Created by ibireme on 15/2/11.
//  Copyright (c) 2015 ibireme.
//
//  This source code is licensed under the MIT-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

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

NS_ASSUME_NONNULL_END
