//
// Vela 音乐播放器 - 媒体库管理
// Created by Vela Engineering Team on 2024/12/18
// 统一媒体文件管理，支持扫描、索引和元数据提取
//

#ifndef MEDIA_LIBRARY_H
#define MEDIA_LIBRARY_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/state_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define MAX_MEDIA_PATHS 16
#define MAX_TRACKS_IN_LIBRARY 1000
#define MEDIA_SCAN_BATCH_SIZE 50

/*********************
 *      TYPEDEFS
 *********************/

// 媒体扫描状态
typedef enum {
    SCAN_STATE_IDLE = 0,
    SCAN_STATE_SCANNING,
    SCAN_STATE_INDEXING,
    SCAN_STATE_COMPLETED,
    SCAN_STATE_ERROR
} scan_state_t;

// 媒体库统计信息
typedef struct {
    uint32_t total_tracks;
    uint32_t total_artists;
    uint32_t total_albums;
    uint32_t total_duration_ms;
    uint64_t total_file_size;
    uint32_t last_scan_time;
} library_stats_t;

// 媒体搜索过滤器
typedef struct {
    char name_filter[128];
    char artist_filter[128];
    char album_filter[128];
    audio_format_t format_filter;
    uint32_t min_duration_ms;
    uint32_t max_duration_ms;
    bool favorites_only;
} media_filter_t;

// 媒体库实例
typedef struct {
    // 音轨数据库
    track_info_t* tracks;
    uint32_t track_count;
    uint32_t track_capacity;
    
    // 扫描路径
    char scan_paths[MAX_MEDIA_PATHS][256];
    int scan_path_count;
    
    // 扫描状态
    scan_state_t scan_state;
    uint32_t scan_progress;
    uint32_t scan_total;
    
    // 索引和缓存
    uint32_t* name_index;     // 按名称排序的索引
    uint32_t* artist_index;   // 按艺术家排序的索引
    uint32_t* duration_index; // 按时长排序的索引
    bool indexes_valid;
    
    // 统计信息
    library_stats_t stats;
    
    // 事件监听器
    int event_listener_ids[4];
    int listener_count;
    
    // 线程管理
    pthread_t scan_thread;
    pthread_mutex_t library_mutex;
    bool scan_thread_running;
    
} media_library_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 初始化媒体库
 * @return 媒体库实例指针，NULL表示失败
 */
media_library_t* media_library_init(void);

/**
 * @brief 清理媒体库
 * @param library 媒体库实例
 */
void media_library_cleanup(media_library_t* library);

/**
 * @brief 添加扫描路径
 * @param library 媒体库实例
 * @param path 扫描路径
 * @return 0 成功, -1 失败
 */
int media_library_add_scan_path(media_library_t* library, const char* path);

/**
 * @brief 开始扫描媒体文件
 * @param library 媒体库实例
 * @param recursive 是否递归扫描
 * @return 0 成功, -1 失败
 */
int media_library_start_scan(media_library_t* library, bool recursive);

/**
 * @brief 停止扫描
 * @param library 媒体库实例
 * @return 0 成功, -1 失败
 */
int media_library_stop_scan(media_library_t* library);

/**
 * @brief 获取扫描进度
 * @param library 媒体库实例
 * @param current 当前进度（输出）
 * @param total 总进度（输出）
 * @return 扫描状态
 */
scan_state_t media_library_get_scan_progress(media_library_t* library, uint32_t* current, uint32_t* total);

/**
 * @brief 根据索引获取音轨
 * @param library 媒体库实例
 * @param index 音轨索引
 * @return 音轨信息指针，NULL表示失败
 */
const track_info_t* media_library_get_track(media_library_t* library, uint32_t index);

/**
 * @brief 根据路径查找音轨
 * @param library 媒体库实例
 * @param path 文件路径
 * @return 音轨索引，-1表示未找到
 */
int media_library_find_track_by_path(media_library_t* library, const char* path);

/**
 * @brief 搜索音轨
 * @param library 媒体库实例
 * @param filter 搜索过滤器
 * @param results 结果数组（输出）
 * @param max_results 最大结果数
 * @return 实际结果数量
 */
int media_library_search_tracks(media_library_t* library, const media_filter_t* filter,
                               uint32_t* results, int max_results);

/**
 * @brief 获取所有艺术家
 * @param library 媒体库实例
 * @param artists 艺术家数组（输出）
 * @param max_artists 最大数量
 * @return 实际艺术家数量
 */
int media_library_get_artists(media_library_t* library, char artists[][MAX_ARTIST_NAME_LEN], int max_artists);

/**
 * @brief 获取艺术家的所有音轨
 * @param library 媒体库实例
 * @param artist 艺术家名称
 * @param track_indexes 音轨索引数组（输出）
 * @param max_tracks 最大数量
 * @return 实际音轨数量
 */
int media_library_get_tracks_by_artist(media_library_t* library, const char* artist,
                                      uint32_t* track_indexes, int max_tracks);

/**
 * @brief 获取库统计信息
 * @param library 媒体库实例
 * @return 统计信息指针
 */
const library_stats_t* media_library_get_stats(media_library_t* library);

/**
 * @brief 重建索引
 * @param library 媒体库实例
 * @return 0 成功, -1 失败
 */
int media_library_rebuild_indexes(media_library_t* library);

/**
 * @brief 保存媒体库到文件
 * @param library 媒体库实例
 * @param filepath 文件路径
 * @return 0 成功, -1 失败
 */
int media_library_save_to_file(media_library_t* library, const char* filepath);

/**
 * @brief 从文件加载媒体库
 * @param library 媒体库实例
 * @param filepath 文件路径
 * @return 0 成功, -1 失败
 */
int media_library_load_from_file(media_library_t* library, const char* filepath);

/**
 * @brief 添加单个音轨到库
 * @param library 媒体库实例
 * @param track_info 音轨信息
 * @return 0 成功, -1 失败
 */
int media_library_add_track(media_library_t* library, const track_info_t* track_info);

/**
 * @brief 从库中移除音轨
 * @param library 媒体库实例
 * @param track_index 音轨索引
 * @return 0 成功, -1 失败
 */
int media_library_remove_track(media_library_t* library, uint32_t track_index);

/**
 * @brief 更新音轨元数据
 * @param library 媒体库实例
 * @param track_index 音轨索引
 * @param updated_track 更新的音轨信息
 * @return 0 成功, -1 失败
 */
int media_library_update_track(media_library_t* library, uint32_t track_index, 
                              const track_info_t* updated_track);

/*********************
 * CALLBACK TYPES
 *********************/

// 扫描进度回调
typedef void (*scan_progress_cb_t)(media_library_t* library, uint32_t current, uint32_t total, void* user_data);

// 音轨发现回调
typedef void (*track_discovered_cb_t)(media_library_t* library, const track_info_t* track, void* user_data);

/**
 * @brief 设置扫描进度回调
 * @param library 媒体库实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int media_library_set_scan_progress_callback(media_library_t* library, 
                                            scan_progress_cb_t callback, void* user_data);

/**
 * @brief 设置音轨发现回调
 * @param library 媒体库实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int media_library_set_track_discovered_callback(media_library_t* library,
                                               track_discovered_cb_t callback, void* user_data);

/*********************
 * METADATA FUNCTIONS
 *********************/

/**
 * @brief 提取音频文件元数据
 * @param file_path 文件路径
 * @param track_info 音轨信息（输出）
 * @return 0 成功, -1 失败
 */
int media_library_extract_metadata(const char* file_path, track_info_t* track_info);

/**
 * @brief 生成音轨缩略图
 * @param track_info 音轨信息
 * @param output_path 输出路径
 * @return 0 成功, -1 失败
 */
int media_library_generate_thumbnail(const track_info_t* track_info, const char* output_path);

#ifdef __cplusplus
}
#endif

#endif // MEDIA_LIBRARY_H
