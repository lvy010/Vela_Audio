//
// Vela 音乐播放器 - 播放控制器
// Created by Vela Engineering Team on 2024/12/18
// 高级播放控制逻辑，协调音频引擎和播放列表服务
//

#ifndef PLAYBACK_CONTROLLER_H
#define PLAYBACK_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/state_manager.h"
#include "../core/audio_engine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define MAX_RECENT_TRACKS 50
#define CROSSFADE_DURATION_MS 2000

/*********************
 *      TYPEDEFS
 *********************/

// 播放控制器状态
typedef enum {
    CONTROLLER_STATE_IDLE = 0,
    CONTROLLER_STATE_LOADING,
    CONTROLLER_STATE_READY,
    CONTROLLER_STATE_PLAYING,
    CONTROLLER_STATE_PAUSED,
    CONTROLLER_STATE_SEEKING,
    CONTROLLER_STATE_ERROR
} controller_state_t;

// 播放统计信息
typedef struct {
    uint32_t total_play_time_ms;
    uint32_t tracks_played;
    uint32_t skip_count;
    uint32_t seek_count;
    uint32_t error_count;
} playback_stats_t;

// 播放控制器实例
typedef struct {
    controller_state_t state;
    
    // 音频引擎引用
    audio_engine_t* audio_engine;
    
    // 当前播放信息
    track_info_t current_track;
    uint32_t current_playlist_id;
    uint32_t current_track_index;
    
    // 播放控制
    play_mode_t play_mode;
    bool shuffle_enabled;
    bool repeat_enabled;
    bool crossfade_enabled;
    
    // 最近播放历史
    track_info_t recent_tracks[MAX_RECENT_TRACKS];
    int recent_track_count;
    int recent_track_index;
    
    // 统计信息
    playback_stats_t stats;
    
    // 事件监听器ID
    int event_listener_ids[8];
    int listener_count;
    
    // 线程同步
    pthread_mutex_t controller_mutex;
    
} playback_controller_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 初始化播放控制器
 * @param audio_engine 音频引擎实例
 * @return 播放控制器实例指针，NULL表示失败
 */
playback_controller_t* playback_controller_init(audio_engine_t* audio_engine);

/**
 * @brief 清理播放控制器
 * @param controller 播放控制器实例
 */
void playback_controller_cleanup(playback_controller_t* controller);

/**
 * @brief 播放指定音轨
 * @param controller 播放控制器实例
 * @param track_info 音轨信息
 * @return 0 成功, -1 失败
 */
int playback_controller_play_track(playback_controller_t* controller, const track_info_t* track_info);

/**
 * @brief 播放/暂停切换
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_toggle_play_pause(playback_controller_t* controller);

/**
 * @brief 停止播放
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_stop(playback_controller_t* controller);

/**
 * @brief 播放下一首
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_next_track(playback_controller_t* controller);

/**
 * @brief 播放上一首
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_previous_track(playback_controller_t* controller);

/**
 * @brief 跳转到指定位置
 * @param controller 播放控制器实例
 * @param position_ms 位置（毫秒）
 * @return 0 成功, -1 失败
 */
int playback_controller_seek(playback_controller_t* controller, uint32_t position_ms);

/**
 * @brief 设置音量
 * @param controller 播放控制器实例
 * @param volume 音量值 (0-100)
 * @return 0 成功, -1 失败
 */
int playback_controller_set_volume(playback_controller_t* controller, uint16_t volume);

/**
 * @brief 设置播放模式
 * @param controller 播放控制器实例
 * @param mode 播放模式
 * @return 0 成功, -1 失败
 */
int playback_controller_set_play_mode(playback_controller_t* controller, play_mode_t mode);

/**
 * @brief 启用/禁用随机播放
 * @param controller 播放控制器实例
 * @param enabled 是否启用
 * @return 0 成功, -1 失败
 */
int playback_controller_set_shuffle(playback_controller_t* controller, bool enabled);

/**
 * @brief 启用/禁用循环播放
 * @param controller 播放控制器实例
 * @param enabled 是否启用
 * @return 0 成功, -1 失败
 */
int playback_controller_set_repeat(playback_controller_t* controller, bool enabled);

/**
 * @brief 启用/禁用交叉淡化
 * @param controller 播放控制器实例
 * @param enabled 是否启用
 * @return 0 成功, -1 失败
 */
int playback_controller_set_crossfade(playback_controller_t* controller, bool enabled);

/**
 * @brief 获取当前播放位置
 * @param controller 播放控制器实例
 * @return 当前位置（毫秒）
 */
uint32_t playback_controller_get_position(playback_controller_t* controller);

/**
 * @brief 获取当前音轨时长
 * @param controller 播放控制器实例
 * @return 音轨时长（毫秒）
 */
uint32_t playback_controller_get_duration(playback_controller_t* controller);

/**
 * @brief 获取控制器状态
 * @param controller 播放控制器实例
 * @return 控制器状态
 */
controller_state_t playback_controller_get_state(playback_controller_t* controller);

/**
 * @brief 获取最近播放的音轨
 * @param controller 播放控制器实例
 * @param tracks 音轨数组（输出）
 * @param max_count 最大数量
 * @return 实际音轨数量
 */
int playback_controller_get_recent_tracks(playback_controller_t* controller, 
                                         track_info_t* tracks, int max_count);

/**
 * @brief 获取播放统计信息
 * @param controller 播放控制器实例
 * @return 统计信息指针
 */
const playback_stats_t* playback_controller_get_stats(playback_controller_t* controller);

/**
 * @brief 重置播放统计信息
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_reset_stats(playback_controller_t* controller);

/**
 * @brief 设置当前播放列表
 * @param controller 播放控制器实例
 * @param playlist_id 播放列表ID
 * @return 0 成功, -1 失败
 */
int playback_controller_set_playlist(playback_controller_t* controller, uint32_t playlist_id);

/**
 * @brief 预加载下一首音轨（性能优化）
 * @param controller 播放控制器实例
 * @return 0 成功, -1 失败
 */
int playback_controller_preload_next(playback_controller_t* controller);

/*********************
 * CALLBACK TYPES
 *********************/

// 播放事件回调类型
typedef void (*playback_event_cb_t)(playback_controller_t* controller, 
                                   controller_state_t old_state, 
                                   controller_state_t new_state,
                                   void* user_data);

// 进度更新回调类型
typedef void (*progress_update_cb_t)(playback_controller_t* controller,
                                    uint32_t position_ms,
                                    uint32_t duration_ms,
                                    void* user_data);

/**
 * @brief 设置播放事件回调
 * @param controller 播放控制器实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int playback_controller_set_event_callback(playback_controller_t* controller,
                                          playback_event_cb_t callback,
                                          void* user_data);

/**
 * @brief 设置进度更新回调
 * @param controller 播放控制器实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int playback_controller_set_progress_callback(playback_controller_t* controller,
                                             progress_update_cb_t callback,
                                             void* user_data);

#ifdef __cplusplus
}
#endif

#endif // PLAYBACK_CONTROLLER_H
