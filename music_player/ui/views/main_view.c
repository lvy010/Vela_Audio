/*
 * Vela 音乐播放器 - 主视图实现
 * 
 * 实现主音乐播放器界面，包括：
 * - 专辑封面显示和动画
 * - 播放控制按钮
 * - 进度条和时间显示
 * - 音量控制
 * - 状态显示
 * 
 * Created by Vela Engineering Team on 2024/12/18
 */

#include "main_view.h"
#include "../../music_player.h"
#include "../../core/event_bus.h"
#include "../../core/state_manager.h"
#include <stdio.h>
#include <string.h>

/*====================
 * 静态变量
 *====================*/

/* UI元素指针 */
static lv_obj_t *main_screen = NULL;
static lv_obj_t *album_cover = NULL;
static lv_obj_t *album_name_label = NULL;
static lv_obj_t *artist_name_label = NULL;
static lv_obj_t *play_btn = NULL;
static lv_obj_t *prev_btn = NULL;
static lv_obj_t *next_btn = NULL;
static lv_obj_t *progress_bar = NULL;
static lv_obj_t *progress_label = NULL;
static lv_obj_t *duration_label = NULL;
static lv_obj_t *volume_bar = NULL;
static lv_obj_t *volume_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *status_label = NULL;

/* 动画和定时器 */
static lv_anim_t album_rotation_anim;
static lv_timer_t *ui_update_timer = NULL;

/* 状态变量 */
static bool view_created = false;
static int16_t current_rotation = 0;

/*====================
 * 前向声明
 *====================*/

static void create_background(lv_obj_t *parent);
static void create_album_display(lv_obj_t *parent);
static void create_control_buttons(lv_obj_t *parent);
static void create_progress_display(lv_obj_t *parent);
static void create_volume_display(lv_obj_t *parent);
static void create_status_display(lv_obj_t *parent);

/* 事件处理 */
static void play_btn_event_cb(lv_event_t *e);
static void prev_btn_event_cb(lv_event_t *e);
static void next_btn_event_cb(lv_event_t *e);
static void volume_bar_event_cb(lv_event_t *e);
static void progress_bar_event_cb(lv_event_t *e);

/* 定时器回调 */
static void ui_update_timer_cb(lv_timer_t *timer);

/* 动画回调 */
static void album_rotation_anim_cb(void *obj, int32_t value);

/* 辅助函数 */
static void format_time(char *buffer, int seconds);
static void update_play_button_icon(void);

/*====================
 * 公共函数实现
 *====================*/

/**
 * 创建主视图
 */
bool main_view_create(void)
{
    if (view_created) {
        LV_LOG_WARN("Main view already created");
        return true;
    }

    /* 创建主屏幕 */
    main_screen = lv_obj_create(NULL);
    if (!main_screen) {
        LV_LOG_ERROR("Failed to create main screen");
        return false;
    }

    /* 设置屏幕样式 */
    lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(main_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(main_screen, lv_color_make(20, 20, 30), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(main_screen, LV_GRAD_DIR_VER, LV_PART_MAIN);

    /* 创建UI组件 */
    create_background(main_screen);
    create_album_display(main_screen);
    create_control_buttons(main_screen);
    create_progress_display(main_screen);
    create_volume_display(main_screen);
    create_status_display(main_screen);

    /* 启动UI更新定时器 */
    ui_update_timer = lv_timer_create(ui_update_timer_cb, 1000, NULL);
    lv_timer_set_repeat_count(ui_update_timer, -1);

    /* 初始化专辑旋转动画 */
    lv_anim_init(&album_rotation_anim);
    lv_anim_set_var(&album_rotation_anim, album_cover);
    lv_anim_set_exec_cb(&album_rotation_anim, album_rotation_anim_cb);
    lv_anim_set_time(&album_rotation_anim, 10000); /* 10秒一圈 */
    lv_anim_set_repeat_count(&album_rotation_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_values(&album_rotation_anim, 0, 3600); /* 0-360度 */

    /* 加载屏幕 */
    lv_scr_load(main_screen);

    view_created = true;
    LV_LOG_INFO("Main view created successfully");
    
    return true;
}

/**
 * 更新播放进度
 */
void main_view_update_progress(int current_pos, int duration)
{
    if (!view_created || !progress_bar || !progress_label || !duration_label) {
        return;
    }

    /* 更新进度条 */
    if (duration > 0) {
        int progress = (current_pos * 100) / duration;
        lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
    }

    /* 更新时间显示 */
    char current_time[16], total_time[16];
    format_time(current_time, current_pos);
    format_time(total_time, duration);
    
    lv_label_set_text(progress_label, current_time);
    lv_label_set_text(duration_label, total_time);
}

/**
 * 更新专辑信息
 */
void main_view_update_album_info(const album_info_t *album)
{
    if (!view_created || !album) {
        return;
    }

    /* 更新专辑名 */
    if (album_name_label && album->name) {
        lv_label_set_text(album_name_label, album->name);
    }

    /* 更新艺术家名 */
    if (artist_name_label && album->artist) {
        lv_label_set_text(artist_name_label, album->artist);
    }

    /* 更新专辑封面 */
    if (album_cover && album->cover_path) {
        lv_img_set_src(album_cover, album->cover_path);
    }

    LV_LOG_INFO("Album info updated: %s - %s", 
                album->artist ? album->artist : "Unknown",
                album->name ? album->name : "Unknown");
}

/**
 * 更新播放状态
 */
void main_view_update_play_status(play_status_t status)
{
    if (!view_created) {
        return;
    }

    /* 更新播放按钮图标 */
    update_play_button_icon();

    /* 控制专辑旋转动画 */
    if (status == PLAY_STATUS_PLAYING) {
        if (lv_anim_get(&album_rotation_anim, album_rotation_anim_cb) == NULL) {
            lv_anim_start(&album_rotation_anim);
        }
    } else {
        lv_anim_del(album_cover, album_rotation_anim_cb);
    }

    /* 更新状态显示 */
    if (status_label) {
        const char *status_text = "";
        switch (status) {
            case PLAY_STATUS_PLAYING:
                status_text = "Playing";
                break;
            case PLAY_STATUS_PAUSED:
                status_text = "Paused";
                break;
            case PLAY_STATUS_STOPPED:
                status_text = "Stopped";
                break;
            default:
                status_text = "Unknown";
                break;
        }
        lv_label_set_text(status_label, status_text);
    }

    LV_LOG_INFO("Play status updated: %d", status);
}

/**
 * 更新音量显示
 */
void main_view_update_volume(int volume)
{
    if (!view_created) {
        return;
    }

    /* 更新音量条 */
    if (volume_bar) {
        lv_bar_set_value(volume_bar, volume, LV_ANIM_ON);
    }

    /* 更新音量标签 */
    if (volume_label) {
        char volume_text[16];
        snprintf(volume_text, sizeof(volume_text), "%d%%", volume);
        lv_label_set_text(volume_label, volume_text);
    }

    LV_LOG_INFO("Volume updated: %d%%", volume);
}

/**
 * 更新日期时间
 */
void main_view_update_datetime(const char *date, const char *time)
{
    if (!view_created) {
        return;
    }

    if (date_label && date) {
        lv_label_set_text(date_label, date);
    }

    if (time_label && time) {
        lv_label_set_text(time_label, time);
    }
}

/*====================
 * 静态函数实现
 *====================*/

/**
 * 创建背景
 */
static void create_background(lv_obj_t *parent)
{
    /* 背景已在主屏幕设置中处理 */
    LV_UNUSED(parent);
}

/**
 * 创建专辑显示区域
 */
static void create_album_display(lv_obj_t *parent)
{
    /* 专辑封面 */
    album_cover = lv_img_create(parent);
    lv_obj_set_size(album_cover, 200, 200);
    lv_obj_align(album_cover, LV_ALIGN_CENTER, 0, -50);
    
    /* 设置默认封面 */
    if (R && R->cover_default) {
        lv_img_set_src(album_cover, R->cover_default);
    }
    
    /* 专辑封面样式 */
    lv_obj_set_style_radius(album_cover, 100, LV_PART_MAIN);
    lv_obj_set_style_border_width(album_cover, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(album_cover, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(album_cover, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(album_cover, lv_color_black(), LV_PART_MAIN);

    /* 专辑名称 */
    album_name_label = lv_label_create(parent);
    lv_obj_align_to(album_name_label, album_cover, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_label_set_text(album_name_label, "Unknown Album");
    
    /* 专辑名称样式 */
    if (R && R->font_large) {
        lv_obj_set_style_text_font(album_name_label, R->font_large, LV_PART_MAIN);
    }
    lv_obj_set_style_text_color(album_name_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_align(album_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

    /* 艺术家名称 */
    artist_name_label = lv_label_create(parent);
    lv_obj_align_to(artist_name_label, album_name_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_label_set_text(artist_name_label, "Unknown Artist");
    
    /* 艺术家名称样式 */
    if (R && R->font_medium) {
        lv_obj_set_style_text_font(artist_name_label, R->font_medium, LV_PART_MAIN);
    }
    lv_obj_set_style_text_color(artist_name_label, lv_color_make(180, 180, 180), LV_PART_MAIN);
    lv_obj_set_style_text_align(artist_name_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
}

/**
 * 创建控制按钮
 */
static void create_control_buttons(lv_obj_t *parent)
{
    /* 按钮容器 */
    lv_obj_t *btn_container = lv_obj_create(parent);
    lv_obj_set_size(btn_container, LV_SIZE_CONTENT, 80);
    lv_obj_align_to(btn_container, artist_name_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 30);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* 上一曲按钮 */
    prev_btn = lv_btn_create(btn_container);
    lv_obj_set_size(prev_btn, 60, 60);
    lv_obj_add_event_cb(prev_btn, prev_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *prev_icon = lv_label_create(prev_btn);
    lv_label_set_text(prev_icon, LV_SYMBOL_PREV);
    lv_obj_center(prev_icon);

    /* 播放/暂停按钮 */
    play_btn = lv_btn_create(btn_container);
    lv_obj_set_size(play_btn, 80, 80);
    lv_obj_add_event_cb(play_btn, play_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_radius(play_btn, 40, LV_PART_MAIN);
    
    /* 播放按钮样式 */
    lv_obj_set_style_bg_color(play_btn, lv_color_make(255, 100, 100), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(play_btn, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(play_btn, lv_color_make(255, 100, 100), LV_PART_MAIN);

    /* 下一曲按钮 */
    next_btn = lv_btn_create(btn_container);
    lv_obj_set_size(next_btn, 60, 60);
    lv_obj_add_event_cb(next_btn, next_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *next_icon = lv_label_create(next_btn);
    lv_label_set_text(next_icon, LV_SYMBOL_NEXT);
    lv_obj_center(next_icon);

    /* 初始化播放按钮图标 */
    update_play_button_icon();
}

/**
 * 创建进度显示
 */
static void create_progress_display(lv_obj_t *parent)
{
    /* 进度条容器 */
    lv_obj_t *progress_container = lv_obj_create(parent);
    lv_obj_set_size(progress_container, lv_pct(80), 60);
    lv_obj_align(progress_container, LV_ALIGN_BOTTOM_MID, 0, -120);
    lv_obj_set_style_bg_opa(progress_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(progress_container, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 当前时间标签 */
    progress_label = lv_label_create(progress_container);
    lv_obj_align(progress_label, LV_ALIGN_LEFT_MID, 0, 0);
    lv_label_set_text(progress_label, "00:00");
    lv_obj_set_style_text_color(progress_label, lv_color_white(), LV_PART_MAIN);

    /* 总时间标签 */
    duration_label = lv_label_create(progress_container);
    lv_obj_align(duration_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_text(duration_label, "00:00");
    lv_obj_set_style_text_color(duration_label, lv_color_white(), LV_PART_MAIN);

    /* 进度条 */
    progress_bar = lv_bar_create(progress_container);
    lv_obj_set_size(progress_bar, lv_pct(60), 8);
    lv_obj_center(progress_bar);
    lv_bar_set_range(progress_bar, 0, 100);
    lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(progress_bar, progress_bar_event_cb, LV_EVENT_CLICKED, NULL);
    
    /* 进度条样式 */
    lv_obj_set_style_bg_color(progress_bar, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(progress_bar, lv_color_make(255, 100, 100), LV_PART_INDICATOR);
    lv_obj_set_style_radius(progress_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 4, LV_PART_INDICATOR);
}

/**
 * 创建音量显示
 */
static void create_volume_display(lv_obj_t *parent)
{
    /* 音量容器 */
    lv_obj_t *volume_container = lv_obj_create(parent);
    lv_obj_set_size(volume_container, 200, 40);
    lv_obj_align(volume_container, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_bg_opa(volume_container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(volume_container, LV_OPA_TRANSP, LV_PART_MAIN);

    /* 音量图标 */
    lv_obj_t *volume_icon = lv_label_create(volume_container);
    lv_label_set_text(volume_icon, LV_SYMBOL_VOLUME_MAX);
    lv_obj_align(volume_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(volume_icon, lv_color_white(), LV_PART_MAIN);

    /* 音量条 */
    volume_bar = lv_bar_create(volume_container);
    lv_obj_set_size(volume_bar, 120, 8);
    lv_obj_align(volume_bar, LV_ALIGN_CENTER, 10, 0);
    lv_bar_set_range(volume_bar, 0, 100);
    lv_bar_set_value(volume_bar, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(volume_bar, volume_bar_event_cb, LV_EVENT_CLICKED, NULL);

    /* 音量条样式 */
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(50, 50, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(100, 200, 100), LV_PART_INDICATOR);
    lv_obj_set_style_radius(volume_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(volume_bar, 4, LV_PART_INDICATOR);

    /* 音量百分比 */
    volume_label = lv_label_create(volume_container);
    lv_obj_align(volume_label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_label_set_text(volume_label, "50%");
    lv_obj_set_style_text_color(volume_label, lv_color_white(), LV_PART_MAIN);
}

/**
 * 创建状态显示
 */
static void create_status_display(lv_obj_t *parent)
{
    /* 日期标签 */
    date_label = lv_label_create(parent);
    lv_obj_align(date_label, LV_ALIGN_TOP_LEFT, 20, 20);
    lv_label_set_text(date_label, "2024-12-18");
    lv_obj_set_style_text_color(date_label, lv_color_white(), LV_PART_MAIN);

    /* 时间标签 */
    time_label = lv_label_create(parent);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_label_set_text(time_label, "12:00");
    lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
    if (R && R->font_large) {
        lv_obj_set_style_text_font(time_label, R->font_large, LV_PART_MAIN);
    }

    /* 状态标签 */
    status_label = lv_label_create(parent);
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_label_set_text(status_label, "Ready");
    lv_obj_set_style_text_color(status_label, lv_color_make(150, 150, 150), LV_PART_MAIN);
}

/**
 * 播放按钮事件处理
 */
static void play_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    
    /* 发送播放/暂停事件 */
    event_data_t event_data = {0};
    event_bus_post(EVENT_PLAYBACK_TOGGLE_REQUESTED, &event_data);
    
    LV_LOG_INFO("Play/Pause button clicked");
}

/**
 * 上一曲按钮事件处理
 */
static void prev_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    
    /* 发送上一曲事件 */
    event_data_t event_data = {0};
    event_bus_post(EVENT_TRACK_PREVIOUS_REQUESTED, &event_data);
    
    LV_LOG_INFO("Previous button clicked");
}

/**
 * 下一曲按钮事件处理
 */
static void next_btn_event_cb(lv_event_t *e)
{
    LV_UNUSED(e);
    
    /* 发送下一曲事件 */
    event_data_t event_data = {0};
    event_bus_post(EVENT_TRACK_NEXT_REQUESTED, &event_data);
    
    LV_LOG_INFO("Next button clicked");
}

/**
 * 音量条事件处理
 */
static void volume_bar_event_cb(lv_event_t *e)
{
    lv_obj_t *bar = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_get_act();
    
    if (indev) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        
        /* 计算点击位置对应的音量值 */
        lv_coord_t bar_width = lv_obj_get_width(bar);
        lv_coord_t bar_x = lv_obj_get_x(bar);
        int volume = ((point.x - bar_x) * 100) / bar_width;
        
        /* 限制范围 */
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;
        
        /* 发送音量变化事件 */
        event_data_t event_data = {0};
        event_data.volume_data.volume = volume;
        event_bus_post(EVENT_VOLUME_CHANGE_REQUESTED, &event_data);
        
        LV_LOG_INFO("Volume changed: %d%%", volume);
    }
}

/**
 * 进度条事件处理
 */
static void progress_bar_event_cb(lv_event_t *e)
{
    lv_obj_t *bar = lv_event_get_target(e);
    lv_indev_t *indev = lv_indev_get_act();
    
    if (indev) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        
        /* 计算点击位置对应的进度百分比 */
        lv_coord_t bar_width = lv_obj_get_width(bar);
        lv_coord_t bar_x = lv_obj_get_x(bar);
        int progress = ((point.x - bar_x) * 100) / bar_width;
        
        /* 限制范围 */
        if (progress < 0) progress = 0;
        if (progress > 100) progress = 100;
        
        /* 发送跳转事件 */
        event_data_t event_data = {0};
        event_data.seek_data.position = progress;
        event_bus_post(EVENT_SEEK_REQUESTED, &event_data);
        
        LV_LOG_INFO("Seek requested: %d%%", progress);
    }
}

/**
 * UI更新定时器回调
 */
static void ui_update_timer_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);
    
    /* 更新日期时间 */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    if (tm_info) {
        char date_str[32], time_str[32];
        strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm_info);
        strftime(time_str, sizeof(time_str), "%H:%M", tm_info);
        
        main_view_update_datetime(date_str, time_str);
    }
    
    /* 请求播放进度更新 */
    event_data_t event_data = {0};
    event_bus_post(EVENT_PROGRESS_UPDATE_REQUESTED, &event_data);
}

/**
 * 专辑旋转动画回调
 */
static void album_rotation_anim_cb(void *obj, int32_t value)
{
    lv_obj_t *img = (lv_obj_t *)obj;
    current_rotation = value / 10; /* 转换为度数 */
    lv_img_set_angle(img, current_rotation);
}

/**
 * 格式化时间
 */
static void format_time(char *buffer, int seconds)
{
    if (!buffer) return;
    
    int minutes = seconds / 60;
    int secs = seconds % 60;
    snprintf(buffer, 16, "%02d:%02d", minutes, secs);
}

/**
 * 更新播放按钮图标
 */
static void update_play_button_icon(void)
{
    if (!play_btn) return;
    
    /* 获取当前播放状态 */
    playback_state_t state = state_manager_get_playback_state();
    
    /* 查找按钮中的图标标签 */
    lv_obj_t *icon = lv_obj_get_child(play_btn, 0);
    if (icon && lv_obj_check_type(icon, &lv_label_class)) {
        if (state == PLAYBACK_STATE_PLAYING) {
            lv_label_set_text(icon, LV_SYMBOL_PAUSE);
        } else {
            lv_label_set_text(icon, LV_SYMBOL_PLAY);
        }
    }
}
