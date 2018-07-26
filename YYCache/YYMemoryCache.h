//
//  YYMemoryCache.h
//  YYCache <https://github.com/ibireme/YYCache>
//
//  Created by ibireme on 15/2/7.
//  Copyright (c) 2015 ibireme.
//
//  This source code is licensed under the MIT-style license found in the
//  LICENSE file in the root directory of this source tree.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

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

NS_ASSUME_NONNULL_END
