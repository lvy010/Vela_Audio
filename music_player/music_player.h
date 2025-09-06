//
// Vela的音乐播放器头文件 - 现代化音乐播放器
// Created by Vela on 2025/7/15
// 项目初始设计和核心数据结构定义
//

#ifndef LVGL_APP_H
#define LVGL_APP_H

#include "audio_ctl.h"
#include "lvgl.h"
#include "wifi.h"

#define RES_ROOT CONFIG_LVX_MUSIC_PLAYER_DATA_ROOT "/res"
#define FONTS_ROOT RES_ROOT "/fonts"
#define ICONS_ROOT RES_ROOT "/icons"
#define MUSICS_ROOT RES_ROOT "/musics"

typedef struct _album_info_t {
    const char* name;
    const char* artist;
    char path[LV_FS_MAX_PATH_LENGTH];
    char cover[LV_FS_MAX_PATH_LENGTH];
    uint64_t total_time; /**< in milliseconds */
    lv_color_t color;
} album_info_t;

typedef enum _switch_album_mode_t {
    SWITCH_ALBUM_MODE_PREV,
    SWITCH_ALBUM_MODE_NEXT,
} switch_album_mode_t;

typedef enum _play_status_t {
    PLAY_STATUS_STOP,
    PLAY_STATUS_PLAY,
    PLAY_STATUS_PAUSE,
} play_status_t;

struct resource_s {
    struct {
        lv_obj_t* time;
        lv_obj_t* date;

        lv_obj_t* player_group;

        lv_obj_t* volume_bar;
        lv_obj_t* volume_bar_indic;
        lv_obj_t* audio;
        lv_obj_t* playlist_base;

        lv_obj_t* album_cover_container;  // 圆形封面容器
        lv_obj_t* album_cover;           // 封面图片
        lv_obj_t* vinyl_ring;            // 唱片外环
        lv_obj_t* vinyl_center;          // 唱片中心孔
        lv_obj_t* album_name;
        lv_obj_t* album_artist;

        lv_obj_t* play_btn;
        lv_obj_t* playback_group;
        lv_obj_t* playback_progress;
        lv_span_t* playback_current_time;
        lv_span_t* playback_total_time;

        lv_obj_t* playlist;
        lv_obj_t* frosted_bg;            // 磨砂玻璃背景
        
        // v1.1.1: 新增进度控制按钮
        lv_obj_t* backward_10s_btn;      // 快退10秒按钮
        lv_obj_t* forward_10s_btn;       // 快进10秒按钮
        
        // v1.1.2: WiFi状态显示
        lv_obj_t* wifi_status_label;     // WiFi状态标签
    } ui;

    struct {
        struct {
            const lv_font_t* normal;
        } size_16;
        struct {
            const lv_font_t* bold;
        } size_22;
        struct {
            const lv_font_t* normal;
        } size_24;
        struct {
            const lv_font_t* normal;
        } size_28;
        struct {
            const lv_font_t* bold;
        } size_60;
    } fonts;

    struct {
        lv_style_t button_default;
        lv_style_t button_pressed;
        lv_style_t circular_cover;           // 圆形封面样式
        lv_style_t vinyl_ring;               // 唱片外环样式
        lv_style_t vinyl_center;             // 唱片中心孔样式
        lv_style_t gradient_progress;        // 渐变进度条样式
        lv_style_t frosted_glass;           // 磨砂玻璃样式
        lv_style_t modern_card;             // 现代卡片样式
        lv_style_transition_dsc_t button_transition_dsc;
        lv_style_transition_dsc_t transition_dsc;
        lv_style_transition_dsc_t cover_rotation;     // 封面旋转动画
    } styles;

    struct {
        const char* playlist;
        const char* previous;
        const char* play;
        const char* pause;
        const char* next;
        const char* audio;
        const char* mute;
        const char* music;
        const char* nocover;
        const char* background;  // 背景图片
    } images;

    album_info_t* albums;
    uint8_t album_count;
};

struct ctx_s {
    bool resource_healthy_check;

    album_info_t* current_album;
    lv_obj_t* current_album_related_obj;

    uint16_t volume;

    play_status_t play_status_prev;
    play_status_t play_status;
    uint64_t current_time;

    struct {
        lv_timer_t* volume_bar_countdown;
        lv_timer_t* playback_progress_update;
        lv_timer_t* refresh_date_time;       // 时间日期更新计时器
        lv_timer_t* cover_rotation;          // 封面旋转计时器
    } timers;

    struct {
        lv_anim_t cover_rotation_anim;       // 封面旋转动画
        bool is_rotating;                    // 是否正在旋转
        int16_t rotation_angle;              // 当前旋转角度
    } animations;

    audioctl_s* audioctl;
};

struct conf_s {
#if WIFI_ENABLED
    wifi_conf_t wifi;
#endif
};

void app_create(void);
void splash_screen_create(void);  // 启动页面创建函数

// 播放列表管理器函数 (原版)
void playlist_manager_create(lv_obj_t* parent);
void playlist_manager_refresh(void);
void playlist_manager_close(void);
bool playlist_manager_is_open(void);

// app control API for external modules
void app_set_play_status(play_status_t status);
void app_switch_to_album(int index);

// 简化版播放列表管理器函数 (第一版本核心功能)
void simple_playlist_manager_create(lv_obj_t* parent);
void simple_playlist_manager_refresh(void);
void simple_playlist_manager_close(void);
bool simple_playlist_manager_is_open(void);

// 简化版本：移除复杂的进度控制功能

// WiFi优化函数 (v1.1.2新增)
int wifi_manager_optimized_init(void);
int wifi_connect_optimized(const char* ssid, const char* password);
void wifi_start_connection_monitor(void);
void wifi_set_auto_reconnect(bool enabled);
void wifi_create_settings_ui(lv_obj_t* parent);
void wifi_manager_optimized_cleanup(void);

// 内部函数声明
void app_switch_to_album(int index);

#endif // LVGL_APP_H
