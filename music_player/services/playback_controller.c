//
// Vela 音乐播放器 - 播放控制器实现
// Created by Vela Engineering Team on 2024/12/18
// 高级播放控制逻辑实现
//

#include "playback_controller.h"
#include "../core/event_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void playback_event_listener(const event_t* event, void* user_data);
static int add_to_recent_tracks(playback_controller_t* controller, const track_info_t* track);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 初始化播放控制器
 */
playback_controller_t* playback_controller_init(audio_engine_t* audio_engine)
{
    if (!audio_engine) {
        printf("❌ 音频引擎实例为空\n");
        return NULL;
    }
    
    printf("🎮 初始化播放控制器...\n");
    
    playback_controller_t* controller = (playback_controller_t*)malloc(sizeof(playback_controller_t));
    if (!controller) {
        printf("❌ 播放控制器内存分配失败\n");
        return NULL;
    }
    
    memset(controller, 0, sizeof(playback_controller_t));
    
    // 设置音频引擎引用
    controller->audio_engine = audio_engine;
    
    // 初始化状态
    controller->state = CONTROLLER_STATE_IDLE;
    controller->play_mode = PLAY_MODE_SEQUENTIAL;
    controller->shuffle_enabled = false;
    controller->repeat_enabled = false;
    controller->crossfade_enabled = false;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&controller->controller_mutex, NULL) != 0) {
        printf("❌ 播放控制器互斥锁初始化失败\n");
        free(controller);
        return NULL;
    }
    
    // 注册事件监听器
    int listener_id = event_bus_register_listener(EVENT_AUDIO_TRACK_ENDED, playback_event_listener, controller);
    if (listener_id >= 0) {
        controller->event_listener_ids[controller->listener_count++] = listener_id;
    }
    
    printf("✅ 播放控制器初始化成功\n");
    return controller;
}

/**
 * @brief 清理播放控制器
 */
void playback_controller_cleanup(playback_controller_t* controller)
{
    if (!controller) {
        return;
    }
    
    printf("🧹 清理播放控制器...\n");
    
    // 停止播放
    playback_controller_stop(controller);
    
    // 注销事件监听器
    for (int i = 0; i < controller->listener_count; i++) {
        event_bus_unregister_listener(controller->event_listener_ids[i]);
    }
    
    // 清理互斥锁
    pthread_mutex_destroy(&controller->controller_mutex);
    
    // 释放实例
    free(controller);
    
    printf("✅ 播放控制器清理完成\n");
}

/**
 * @brief 播放指定音轨
 */
int playback_controller_play_track(playback_controller_t* controller, const track_info_t* track_info)
{
    if (!controller || !track_info) {
        return -1;
    }
    
    printf("🎵 播放音轨：%s - %s\n", track_info->name, track_info->artist);
    
    pthread_mutex_lock(&controller->controller_mutex);
    
    // 停止当前播放
    if (controller->state == CONTROLLER_STATE_PLAYING) {
        audio_engine_stop(controller->audio_engine);
    }
    
    // 更新当前音轨信息
    memcpy(&controller->current_track, track_info, sizeof(track_info_t));
    
    // 加载新文件
    if (audio_engine_load_file(controller->audio_engine, track_info->path) != 0) {
        printf("❌ 音频文件加载失败：%s\n", track_info->path);
        controller->state = CONTROLLER_STATE_ERROR;
        pthread_mutex_unlock(&controller->controller_mutex);
        return -1;
    }
    
    // 开始播放
    if (audio_engine_play(controller->audio_engine) != 0) {
        printf("❌ 音频播放启动失败\n");
        controller->state = CONTROLLER_STATE_ERROR;
        pthread_mutex_unlock(&controller->controller_mutex);
        return -1;
    }
    
    controller->state = CONTROLLER_STATE_PLAYING;
    
    // 添加到最近播放
    add_to_recent_tracks(controller, track_info);
    
    // 更新统计信息
    controller->stats.tracks_played++;
    
    pthread_mutex_unlock(&controller->controller_mutex);
    
    // 更新全局状态
    state_manager_set_current_track(track_info);
    state_manager_set_playback_state(PLAYBACK_STATE_PLAYING);
    
    printf("✅ 音轨播放成功\n");
    return 0;
}

/**
 * @brief 播放/暂停切换
 */
int playback_controller_toggle_play_pause(playback_controller_t* controller)
{
    if (!controller) {
        return -1;
    }
    
    pthread_mutex_lock(&controller->controller_mutex);
    
    int result = 0;
    
    switch (controller->state) {
        case CONTROLLER_STATE_PLAYING:
            result = audio_engine_pause(controller->audio_engine);
            if (result == 0) {
                controller->state = CONTROLLER_STATE_PAUSED;
                state_manager_set_playback_state(PLAYBACK_STATE_PAUSED);
                printf("⏸️ 播放已暂停\n");
            }
            break;
            
        case CONTROLLER_STATE_PAUSED:
            result = audio_engine_resume(controller->audio_engine);
            if (result == 0) {
                controller->state = CONTROLLER_STATE_PLAYING;
                state_manager_set_playback_state(PLAYBACK_STATE_PLAYING);
                printf("▶️ 播放已恢复\n");
            }
            break;
            
        case CONTROLLER_STATE_READY:
            result = audio_engine_play(controller->audio_engine);
            if (result == 0) {
                controller->state = CONTROLLER_STATE_PLAYING;
                state_manager_set_playback_state(PLAYBACK_STATE_PLAYING);
                printf("▶️ 播放已开始\n");
            }
            break;
            
        default:
            printf("⚠️ 当前状态不支持播放/暂停操作：%d\n", controller->state);
            result = -1;
            break;
    }
    
    pthread_mutex_unlock(&controller->controller_mutex);
    return result;
}

/**
 * @brief 停止播放
 */
int playback_controller_stop(playback_controller_t* controller)
{
    if (!controller) {
        return -1;
    }
    
    printf("⏹️ 停止播放...\n");
    
    pthread_mutex_lock(&controller->controller_mutex);
    
    int result = audio_engine_stop(controller->audio_engine);
    if (result == 0) {
        controller->state = CONTROLLER_STATE_IDLE;
        state_manager_set_playback_state(PLAYBACK_STATE_STOPPED);
        state_manager_set_playback_position(0);
    }
    
    pthread_mutex_unlock(&controller->controller_mutex);
    
    return result;
}

/**
 * @brief 设置音量
 */
int playback_controller_set_volume(playback_controller_t* controller, uint16_t volume)
{
    if (!controller) {
        return -1;
    }
    
    int result = audio_engine_set_volume(controller->audio_engine, volume);
    if (result == 0) {
        state_manager_set_volume(volume);
        printf("🔊 音量设置：%d%%\n", volume);
    }
    
    return result;
}

/**
 * @brief 获取控制器状态
 */
controller_state_t playback_controller_get_state(playback_controller_t* controller)
{
    if (!controller) {
        return CONTROLLER_STATE_ERROR;
    }
    
    return controller->state;
}

/**
 * @brief 获取播放统计信息
 */
const playback_stats_t* playback_controller_get_stats(playback_controller_t* controller)
{
    if (!controller) {
        return NULL;
    }
    
    return &controller->stats;
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 播放事件监听器
 */
static void playback_event_listener(const event_t* event, void* user_data)
{
    playback_controller_t* controller = (playback_controller_t*)user_data;
    if (!controller || !event) {
        return;
    }
    
    switch (event->type) {
        case EVENT_AUDIO_TRACK_ENDED:
            printf("🎵 音轨播放结束，准备播放下一首\n");
            
            // 根据播放模式决定下一步动作
            switch (controller->play_mode) {
                case PLAY_MODE_REPEAT_ONE:
                    // 重复播放当前音轨
                    playback_controller_play_track(controller, &controller->current_track);
                    break;
                    
                case PLAY_MODE_SEQUENTIAL:
                case PLAY_MODE_REPEAT_ALL:
                case PLAY_MODE_SHUFFLE:
                    // 播放下一首（需要从播放列表服务获取）
                    playback_controller_next_track(controller);
                    break;
            }
            break;
            
        default:
            break;
    }
}

/**
 * @brief 添加到最近播放历史
 */
static int add_to_recent_tracks(playback_controller_t* controller, const track_info_t* track)
{
    if (!controller || !track) {
        return -1;
    }
    
    // 检查是否已存在
    for (int i = 0; i < controller->recent_track_count; i++) {
        if (controller->recent_tracks[i].track_id == track->track_id) {
            // 已存在，移动到最前面
            track_info_t temp = controller->recent_tracks[i];
            memmove(&controller->recent_tracks[1], &controller->recent_tracks[0], i * sizeof(track_info_t));
            controller->recent_tracks[0] = temp;
            return 0;
        }
    }
    
    // 添加新音轨到最前面
    if (controller->recent_track_count < MAX_RECENT_TRACKS) {
        memmove(&controller->recent_tracks[1], &controller->recent_tracks[0], 
                controller->recent_track_count * sizeof(track_info_t));
        controller->recent_track_count++;
    } else {
        memmove(&controller->recent_tracks[1], &controller->recent_tracks[0], 
                (MAX_RECENT_TRACKS - 1) * sizeof(track_info_t));
    }
    
    controller->recent_tracks[0] = *track;
    
    printf("📚 添加到最近播放：%s\n", track->name);
    return 0;
}

/**
 * @brief 播放下一首（简化实现）
 */
int playback_controller_next_track(playback_controller_t* controller)
{
    if (!controller) {
        return -1;
    }
    
    printf("⏭️ 播放下一首音轨\n");
    
    // 发布下一首事件，让播放列表服务处理
    playlist_event_data_t event_data = {
        .track_index = controller->current_track_index,
        .play_mode = controller->play_mode
    };
    
    event_bus_publish(EVENT_PLAYLIST_NEXT_TRACK, &event_data, sizeof(event_data));
    
    return 0;
}

/**
 * @brief 播放上一首（简化实现）
 */
int playback_controller_previous_track(playback_controller_t* controller)
{
    if (!controller) {
        return -1;
    }
    
    printf("⏮️ 播放上一首音轨\n");
    
    // 发布上一首事件，让播放列表服务处理
    playlist_event_data_t event_data = {
        .track_index = controller->current_track_index,
        .play_mode = controller->play_mode
    };
    
    event_bus_publish(EVENT_PLAYLIST_PREV_TRACK, &event_data, sizeof(event_data));
    
    return 0;
}
