//
// Vela的音乐播放器 - 基于LVGL的现代化音乐播放器
// Created by Vela on 2025/7/28
// 核心音乐播放器功能实现，包含UI设计、动画效果、音频控制
//

/*
 * UI:
 *
 * TIME GROUP:
 *      TIME: 00:00:00
 *      DATE: 2025/07/28
 *
 * PLAYER GROUP:
 *      ALBUM GROUP:
 *          ALBUM PICTURE
 *          ALBUM INFO:
 *              ALBUM NAME
 *              ALBUM ARTIST
 *      PROGRESS GROUP:
 *          CURRENT TIME: 00:00/00:00
 *          PLAYBACK PROGRESS BAR
 *      CONTROL GROUP:
 *          PLAYLIST
 *          PREVIOUS
 *          PLAY/PAUSE
 *          NEXT
 *          AUDIO
 *
 * TOP Layer:
 *      VOLUME BAR
 *      PLAYLIST GROUP:
 *          TITLE
 *          LIST:
 *              ICON
 *              ALBUM NAME
 *              ALBUM ARTIST
 */

/*********************
 *      INCLUDES
 *********************/

#include "music_player.h"
#include "playlist_manager.h"
#include "font_config.h"
#include <stdlib.h>
#include <netutils/cJSON.h>
#include <time.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 * MODERN UI CONSTANTS
 **********************/
#define MODERN_BACKGROUND_COLOR      lv_color_hex(0x121212)
#define MODERN_CARD_COLOR           lv_color_hex(0x1E1E1E)
#define MODERN_PRIMARY_COLOR        lv_color_hex(0x00BFFF)
#define MODERN_SECONDARY_COLOR      lv_color_hex(0xFF6B6B)
#define MODERN_TEXT_PRIMARY         lv_color_hex(0xFFFFFF)
#define MODERN_TEXT_SECONDARY       lv_color_hex(0xBBBBBB)
#define MODERN_ACCENT_COLOR         lv_color_hex(0x4ECDC4)

#define COVER_SIZE                  200
#define COVER_ROTATION_DURATION     8000  // 8秒转一圈，更接近真实唱片转速 (33 RPM ≈ 1.8秒/圈，45 RPM ≈ 1.3秒/圈，8秒为慢速视觉效果)

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/* Init functions */
static void read_configs(void);
static bool init_resource(void);
static void reload_music_config(void);
static void app_create_error_page(void);
static void app_create_main_page(void);
static void app_create_top_layer(void);

/* Timer starting functions */
static void app_start_updating_date_time(void);

/* Animation functions */
static void app_start_cover_rotation_animation(void);
static void app_stop_cover_rotation_animation(void);
static void app_cover_rotation_anim_cb(void* obj, int32_t value);

/* Album operations */
static int32_t app_get_album_index(album_info_t* album);

/* Album operations */
void app_set_play_status(play_status_t status);
static void app_set_playback_time(uint32_t current_time);
static void app_set_volume(uint16_t volume);

/* UI refresh functions */
static void app_refresh_album_info(void);
static void app_refresh_date_time(void);
static void app_refresh_play_status(void);
static void app_refresh_playback_progress(void);
static void app_refresh_playlist(void);
static void app_refresh_volume_bar(void);
static void app_refresh_volume_countdown_timer(void);

/* Event handler functions */
static void app_audio_event_handler(lv_event_t* e);
static void app_play_status_event_handler(lv_event_t* e);
// static void app_playlist_btn_event_handler(lv_event_t* e);  // 已移除，使用playlist_manager系统
static void app_playlist_event_handler(lv_event_t* e);
static void app_switch_album_event_handler(lv_event_t* e);
static void app_volume_bar_event_handler(lv_event_t* e);
static void app_playback_progress_bar_event_handler(lv_event_t* e);

/* Timer callback functions */
static void app_refresh_date_time_timer_cb(lv_timer_t* timer);
static void app_playback_progress_update_timer_cb(lv_timer_t* timer);
static void app_volume_bar_countdown_timer_cb(lv_timer_t* timer);

/**********************
 *  STATIC VARIABLES
 **********************/

// clang-format off
struct resource_s   R;  /**< Resources */
struct ctx_s        C;  /**< Context */
struct conf_s       CF; /**< Configuration */
// clang-format on

/* Week days mapping - 完整格式显示星期 */
const char* WEEK_DAYS[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

/* Transition properties for the objects */
const lv_style_prop_t transition_props[] = {
    LV_STYLE_OPA,
    LV_STYLE_BG_OPA,
    LV_STYLE_Y,
    LV_STYLE_HEIGHT,
    LV_STYLE_PROP_FLAG_NONE
};

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void app_create(void)
{
    // Init resource and context structure
    lv_memzero(&R, sizeof(R));
    lv_memzero(&C, sizeof(C));
    lv_memzero(&CF, sizeof(CF));

    // 初始化字体系统
    font_system_init();

    read_configs();

#if WIFI_ENABLED
    CF.wifi.conn_delay = 2000000;
    wifi_connect(&CF.wifi);
#endif

    C.resource_healthy_check = init_resource();

    if (!C.resource_healthy_check) {
        app_create_error_page();
        return;
    }

    app_create_main_page();
    app_set_play_status(PLAY_STATUS_STOP);
    app_switch_to_album(0);
    app_set_volume(30);

    app_refresh_album_info();
    app_refresh_playlist();
    app_refresh_volume_bar();
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t app_get_album_index(album_info_t* album)
{
    for (int i = 0; i < R.album_count; i++) {
        if (album == &R.albums[i]) {
            return i;
        }
    }
    return -1;
}

static void app_set_volume(uint16_t volume)
{
    C.volume = volume;
    audio_ctl_set_volume(C.audioctl, C.volume);
}

void app_set_play_status(play_status_t status)
{
    C.play_status_prev = C.play_status;
    C.play_status = status;
    app_refresh_play_status();
}

void app_switch_to_album(int index)
{
    if (R.album_count == 0 || index < 0 || index >= R.album_count || C.current_album == &R.albums[index])
        return;

    C.current_album = &R.albums[index];
    app_refresh_album_info();
    app_refresh_playlist();
    app_set_playback_time(0);

    if (C.play_status == PLAY_STATUS_STOP) {
        return;
    }

    app_set_play_status(PLAY_STATUS_STOP);
    app_set_play_status(PLAY_STATUS_PLAY);
}

static void app_set_playback_time(uint32_t current_time)
{
    C.current_time = current_time;

    audio_ctl_seek(C.audioctl, C.current_time / 1000);
    app_refresh_playback_progress();
}

static void app_refresh_date_time(void)
{
    // 检查UI组件是否存在
    if (!R.ui.time || !R.ui.date) {
        LV_LOG_WARN("Time/Date UI components not initialized");
        return;
    }

    time_t now = time(NULL);
    LV_LOG_USER("Current timestamp: %ld", (long)now);
    
    // 如果时间获取失败，使用静态时间显示
    if (now <= 0) {
        lv_label_set_text(R.ui.time, "12:34");
        lv_label_set_text(R.ui.date, "Friday");
        LV_LOG_WARN("Using fallback time display");
        return;
    }
    
    struct tm* timeinfo = localtime(&now);
    if (!timeinfo) {
        // 如果localtime失败，使用gmtime
        timeinfo = gmtime(&now);
        if (!timeinfo) {
            lv_label_set_text(R.ui.time, "12:34");
            lv_label_set_text(R.ui.date, "Friday");
            LV_LOG_ERROR("Failed to get current time, using fallback");
            return;
        }
    }

    // 更新时间 (HH:MM格式)
    char time_str[6];
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    lv_label_set_text(R.ui.time, time_str);

    // 更新星期 (完整格式: Monday, Tuesday等)
    char date_str[12];
    lv_snprintf(date_str, sizeof(date_str), "%s", WEEK_DAYS[timeinfo->tm_wday]);
    lv_label_set_text(R.ui.date, date_str);

    LV_LOG_USER("Time updated: %s %s (wday: %d)", time_str, date_str, timeinfo->tm_wday);
}

static void app_refresh_volume_bar(void)
{
    int32_t volume_bar_indic_height = C.volume;

    lv_obj_set_height(R.ui.volume_bar_indic, volume_bar_indic_height);

    lv_obj_refr_size(R.ui.volume_bar_indic);
    lv_obj_update_layout(R.ui.volume_bar_indic);

    if (C.volume > 0) {
        lv_image_set_src(R.ui.audio, R.images.audio);
    } else {
        lv_image_set_src(R.ui.audio, R.images.mute);
    }
}

static void app_refresh_album_info(void)
{
    if (C.current_album) {
        // 🖼️ 智能图片加载 - 支持PNG格式并保持正确比例
        if (access(C.current_album->cover, F_OK) == 0) {
            lv_image_set_src(R.ui.album_cover, C.current_album->cover);
            LV_LOG_USER("📷 加载专辑封面: %s", C.current_album->cover);
            
            // 确保PNG图片保持正确的宽高比
            lv_image_set_scale(R.ui.album_cover, 256);  // 保持适当缩放
            lv_image_set_inner_align(R.ui.album_cover, LV_IMAGE_ALIGN_CENTER);
            
        } else {
            lv_image_set_src(R.ui.album_cover, R.images.nocover);
            LV_LOG_WARN("📷 专辑封面文件不存在，使用默认封面: %s", C.current_album->cover);
        }
        
        // 🎵 更新歌曲信息 - 支持UTF-8编码
        const char* display_name = (C.current_album->name && strlen(C.current_album->name) > 0) ? 
                                  C.current_album->name : "未知歌曲";
        const char* display_artist = (C.current_album->artist && strlen(C.current_album->artist) > 0) ? 
                                    C.current_album->artist : "未知艺术家";
        
        // 使用UTF-8字体配置系统
        set_label_utf8_text(R.ui.album_name, display_name, get_font_by_size(28));
        set_label_utf8_text(R.ui.album_artist, display_artist, get_font_by_size(22));
        
        LV_LOG_USER("🎵 专辑信息已更新: %s - %s", 
                   C.current_album->name ? C.current_album->name : "未知歌曲",
                   C.current_album->artist ? C.current_album->artist : "未知艺术家");
    }
}

static void app_refresh_play_status(void)
{
    if (C.timers.playback_progress_update == NULL) {
        C.timers.playback_progress_update = lv_timer_create(app_playback_progress_update_timer_cb, 1000, NULL);
    }

    switch (C.play_status) {
    case PLAY_STATUS_STOP:
        lv_image_set_src(R.ui.play_btn, R.images.play);
        lv_timer_pause(C.timers.playback_progress_update);
        app_stop_cover_rotation_animation();  // 停止封面旋转
        if (C.audioctl) {
            audio_ctl_stop(C.audioctl);
            audio_ctl_uninit_nxaudio(C.audioctl);
            C.audioctl = NULL;
        }
        break;
    case PLAY_STATUS_PLAY:
        lv_image_set_src(R.ui.play_btn, R.images.pause);
        lv_timer_resume(C.timers.playback_progress_update);
        app_start_cover_rotation_animation();  // 开始封面旋转
        if (C.play_status_prev == PLAY_STATUS_PAUSE)
            audio_ctl_resume(C.audioctl);
        else if (C.play_status_prev == PLAY_STATUS_STOP) {
            C.audioctl = audio_ctl_init_nxaudio(C.current_album->path);
            audio_ctl_start(C.audioctl);
        }
        break;
    case PLAY_STATUS_PAUSE:
        lv_image_set_src(R.ui.play_btn, R.images.play);
        lv_timer_pause(C.timers.playback_progress_update);
        app_stop_cover_rotation_animation();  // 暂停时停止旋转
        audio_ctl_pause(C.audioctl);
        break;
    default:
        break;
    }
}

static void app_refresh_playback_progress(void)
{
    uint64_t total_time = C.current_album->total_time;

    if (C.current_time > total_time) {
        app_set_play_status(PLAY_STATUS_STOP);
        C.current_time = 0;
        return;
    }

    lv_bar_set_range(R.ui.playback_progress, 0, (int32_t)total_time);
    lv_bar_set_value(R.ui.playback_progress, (int32_t)C.current_time, LV_ANIM_ON);

    char buff[256];

    uint32_t current_time_min = C.current_time / 60000;
    uint32_t current_time_sec = (C.current_time % 60000) / 1000;
    uint32_t total_time_min = total_time / 60000;
    uint32_t total_time_sec = (total_time % 60000) / 1000;

    lv_snprintf(buff, sizeof(buff), "%02d:%02d", current_time_min, current_time_sec);
    lv_span_set_text(R.ui.playback_current_time, buff);
    lv_snprintf(buff, sizeof(buff), "%02d:%02d", total_time_min, total_time_sec);
    lv_span_set_text(R.ui.playback_total_time, buff);
}

static void app_refresh_playlist(void)
{
    // 使用新的播放列表管理器刷新
    // 如果播放列表管理器已打开，则刷新其内容
    if (playlist_manager_is_open()) {
        playlist_manager_refresh();
    }
    
    // 保留此函数以兼容现有调用，但实际逻辑已转移到playlist_manager
    LV_LOG_USER("Playlist refresh triggered - using new playlist manager system");
}

static void app_volume_bar_countdown_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);
    lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, true);
    lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, false);
}

static void app_playback_progress_update_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);

    C.current_time = audio_ctl_get_position(C.audioctl) * 1000;
    app_refresh_playback_progress();
}

static void app_refresh_date_time_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);

    app_refresh_date_time();
}

static void app_refresh_volume_countdown_timer(void)
{
    if (C.timers.volume_bar_countdown) {
        lv_timer_set_repeat_count(C.timers.volume_bar_countdown, 1);
        lv_timer_reset(C.timers.volume_bar_countdown);
        lv_timer_resume(C.timers.volume_bar_countdown);
    } else {
        C.timers.volume_bar_countdown = lv_timer_create(app_volume_bar_countdown_timer_cb, 3000, NULL);
        lv_timer_set_auto_delete(C.timers.volume_bar_countdown, false);
    }
}

static void app_playlist_event_handler(lv_event_t* e)
{
    LV_UNUSED(e);

    // 使用优化版播放列表管理器
    if (playlist_manager_is_open()) {
        // 如果播放列表已打开，则关闭它
        playlist_manager_close();
    } else {
        // 创建优化版播放列表管理器 - 使用顶层容器确保正确层级
        lv_obj_t* top_layer = lv_layer_top();
        playlist_manager_create(top_layer);
    }
}

static void app_volume_bar_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (!(code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_PRESS_LOST)) {
        return;
    }

    lv_point_t point;
    lv_indev_t* indev = lv_indev_active();
    lv_indev_get_vect(indev, &point);

    int32_t volume_bar_height = lv_obj_get_height(R.ui.volume_bar);
    int32_t volume_bar_indic_height = lv_obj_get_height(R.ui.volume_bar_indic);

    if (volume_bar_indic_height < 0) {
        volume_bar_indic_height = 0;
    } else if (volume_bar_indic_height > volume_bar_height) {
        volume_bar_indic_height = volume_bar_height;
    }

    int32_t volume = volume_bar_indic_height - point.y;
    if (volume < 0)
        volume = 0;

    app_set_volume(volume);

    app_refresh_volume_bar();
    app_refresh_volume_countdown_timer();
}

static void app_audio_event_handler(lv_event_t* e)
{
    LV_UNUSED(e);

    if (lv_obj_has_state(R.ui.volume_bar, LV_STATE_USER_1)) {
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, true);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, false);
    } else {
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_DEFAULT, false);
        lv_obj_set_state(R.ui.volume_bar, LV_STATE_USER_1, true);
        app_refresh_volume_countdown_timer();
    }
}

// 注意：app_playlist_btn_event_handler已移除，因为现在使用playlist_manager.c中的新事件处理系统

static void app_switch_album_event_handler(lv_event_t* e)
{
    switch_album_mode_t direction = (switch_album_mode_t)(lv_uintptr_t)lv_event_get_user_data(e);

    int32_t album_index = app_get_album_index(C.current_album);
    if (album_index < 0) {
        return;
    }

    switch (direction) {
    case SWITCH_ALBUM_MODE_PREV:
        album_index--;
        break;
    case SWITCH_ALBUM_MODE_NEXT:
        album_index++;
        break;
    default:
        break;
    }

    album_index = (album_index + R.album_count) % R.album_count;

    app_switch_to_album(album_index);
}

static void app_play_status_event_handler(lv_event_t* e)
{
    LV_UNUSED(e);

    switch (C.play_status) {
    case PLAY_STATUS_STOP:
        app_set_play_status(PLAY_STATUS_PLAY);
        break;
    case PLAY_STATUS_PLAY:
        app_set_play_status(PLAY_STATUS_PAUSE);
        break;
    case PLAY_STATUS_PAUSE:
        app_set_play_status(PLAY_STATUS_PLAY);
        break;
    default:
        break;
    }
}

static void app_playback_progress_bar_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    switch (code) {
    case LV_EVENT_LONG_PRESSED:
        lv_obj_set_height(R.ui.playback_progress, 8);
        lv_obj_set_style_margin_ver(R.ui.playback_progress, 0, LV_PART_MAIN);
        lv_obj_set_state(R.ui.playback_progress, LV_STATE_CHECKED, true);
        break;
    case LV_EVENT_PRESS_LOST:
    case LV_EVENT_RELEASED:
        lv_obj_set_height(R.ui.playback_progress, 4);
        lv_obj_set_style_margin_ver(R.ui.playback_progress, 2, LV_PART_MAIN);
        lv_obj_set_state(R.ui.playback_progress, LV_STATE_CHECKED, false);
        break;
    case LV_EVENT_PRESSING: {
        if (lv_obj_has_state(R.ui.playback_progress, LV_STATE_CHECKED)) {
            lv_point_t point;
            lv_point_t vec;
            lv_indev_t* indev = lv_indev_active();
            lv_indev_get_point(indev, &point);
            lv_indev_get_vect(indev, &vec);

            if (point.y <= 0) {
                return;
            }

            int32_t screen_height = lv_obj_get_height(lv_screen_active());
            int32_t screen_width = lv_obj_get_width(lv_screen_active());
            int32_t level_height = screen_height / 10;
            int32_t level = point.y / level_height + 1;

            uint64_t total_time = C.current_album ? C.current_album->total_time : 0;

            C.current_time += vec.x * level * (int32_t)total_time / screen_width / 10;
            app_set_playback_time(C.current_time);
        }
        break;
    }
    default:
        break;
    }
}

static bool init_resource(void)
{
    // 简化版本：只初始化基本功能模块
    
    // 使用LVGL内置字体 - 仅使用配置中启用的字体
    R.fonts.size_16.normal = &lv_font_montserrat_16;
    R.fonts.size_22.bold = &lv_font_montserrat_22;
    R.fonts.size_24.normal = &lv_font_montserrat_24;
    R.fonts.size_28.normal = &lv_font_montserrat_32;  // 使用32号字体替代28号
    R.fonts.size_60.bold = &lv_font_montserrat_32;    // 使用32号字体替代60号

    // 字体检查 - 内置字体总是可用的
    if (R.fonts.size_16.normal == NULL ||
        R.fonts.size_22.bold == NULL   ||
        R.fonts.size_24.normal == NULL ||
        R.fonts.size_28.normal == NULL ||
        R.fonts.size_60.bold == NULL ) {
        return false;
    }

    // Modern UI Styles
    lv_style_init(&R.styles.button_default);
    lv_style_init(&R.styles.button_pressed);
    lv_style_init(&R.styles.circular_cover);
    lv_style_init(&R.styles.vinyl_ring);
    lv_style_init(&R.styles.vinyl_center);
    lv_style_init(&R.styles.gradient_progress);
    lv_style_init(&R.styles.frosted_glass);
    lv_style_init(&R.styles.modern_card);

    // Button styles with modern hover effects - 优化点击体验
    lv_style_set_opa(&R.styles.button_default, LV_OPA_COVER);
    lv_style_set_opa(&R.styles.button_pressed, LV_OPA_70);  // 点击时降低透明度
    // 移除缩放效果，避免按钮变大影响用户体验
    lv_style_set_shadow_width(&R.styles.button_default, 10);
    lv_style_set_shadow_color(&R.styles.button_default, MODERN_PRIMARY_COLOR);

    // Circular cover style - 增强唱片效果
    lv_style_set_radius(&R.styles.circular_cover, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&R.styles.circular_cover, 4);  // 适中边框宽度
    lv_style_set_border_color(&R.styles.circular_cover, lv_color_hex(0x2D2D2D));  // 深灰色边框
    lv_style_set_border_opa(&R.styles.circular_cover, LV_OPA_COVER);
    lv_style_set_shadow_width(&R.styles.circular_cover, 20);
    lv_style_set_shadow_color(&R.styles.circular_cover, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&R.styles.circular_cover, LV_OPA_50);
    lv_style_set_shadow_spread(&R.styles.circular_cover, 3);

    // Vinyl ring style - 唱片外环样式
    lv_style_set_radius(&R.styles.vinyl_ring, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&R.styles.vinyl_ring, 8);  // 厚边框模拟唱片边缘
    lv_style_set_border_color(&R.styles.vinyl_ring, lv_color_hex(0x1A1A1A));  // 深黑色
    lv_style_set_border_opa(&R.styles.vinyl_ring, LV_OPA_COVER);
    lv_style_set_bg_color(&R.styles.vinyl_ring, lv_color_hex(0x0F0F0F));  // 非常深的背景
    lv_style_set_bg_opa(&R.styles.vinyl_ring, LV_OPA_30);  // 半透明
    lv_style_set_shadow_width(&R.styles.vinyl_ring, 30);
    lv_style_set_shadow_color(&R.styles.vinyl_ring, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&R.styles.vinyl_ring, LV_OPA_70);
    lv_style_set_shadow_spread(&R.styles.vinyl_ring, 8);

    // Vinyl center style - 唱片中心孔样式
    lv_style_set_radius(&R.styles.vinyl_center, LV_RADIUS_CIRCLE);
    lv_style_set_bg_color(&R.styles.vinyl_center, lv_color_hex(0x1A1A1A));  // 深黑色中心
    lv_style_set_bg_opa(&R.styles.vinyl_center, LV_OPA_COVER);
    lv_style_set_border_width(&R.styles.vinyl_center, 2);
    lv_style_set_border_color(&R.styles.vinyl_center, lv_color_hex(0x333333));
    lv_style_set_border_opa(&R.styles.vinyl_center, LV_OPA_COVER);

    // Gradient progress bar style
    lv_style_set_bg_color(&R.styles.gradient_progress, MODERN_PRIMARY_COLOR);
    lv_style_set_bg_grad_color(&R.styles.gradient_progress, MODERN_SECONDARY_COLOR);
    lv_style_set_bg_grad_dir(&R.styles.gradient_progress, LV_GRAD_DIR_HOR);
    lv_style_set_radius(&R.styles.gradient_progress, 10);

    // Frosted glass style (simplified without backdrop filter)
    lv_style_set_bg_color(&R.styles.frosted_glass, lv_color_hex(0x1E1E1E));
    lv_style_set_bg_opa(&R.styles.frosted_glass, LV_OPA_70);
    lv_style_set_radius(&R.styles.frosted_glass, 20);

    // Modern card style
    lv_style_set_bg_color(&R.styles.modern_card, MODERN_CARD_COLOR);
    lv_style_set_bg_opa(&R.styles.modern_card, LV_OPA_90);
    lv_style_set_radius(&R.styles.modern_card, 25);
    lv_style_set_shadow_width(&R.styles.modern_card, 15);
    lv_style_set_shadow_color(&R.styles.modern_card, lv_color_black());
    lv_style_set_shadow_opa(&R.styles.modern_card, LV_OPA_30);

    // transition animations
    lv_style_transition_dsc_init(&R.styles.transition_dsc, transition_props, &lv_anim_path_ease_in_out, 300, 0, NULL);
    lv_style_transition_dsc_init(&R.styles.button_transition_dsc, transition_props, &lv_anim_path_ease_in_out, 150, 0, NULL);
    lv_style_set_transition(&R.styles.button_default, &R.styles.button_transition_dsc);
    lv_style_set_transition(&R.styles.button_pressed, &R.styles.button_transition_dsc);

    // images
    R.images.playlist = ICONS_ROOT "/playlist.png";
    R.images.previous = ICONS_ROOT "/previous.png";
    R.images.play = ICONS_ROOT "/play.png";
    R.images.pause = ICONS_ROOT "/pause.png";
    R.images.next = ICONS_ROOT "/next.png";
    R.images.audio = ICONS_ROOT "/audio.png";
    R.images.mute = ICONS_ROOT "/mute.png";
    R.images.music = ICONS_ROOT "/music.png";
    R.images.nocover = ICONS_ROOT "/nocover.png";
    
    // 背景图片支持 - 使用background.png作为主界面背景
    R.images.background = ICONS_ROOT "/background.png";

    // albums
    reload_music_config();

    return true;
}

static void app_create_top_layer(void)
{
    lv_obj_t* top_layer = lv_layer_top();
    lv_obj_set_scroll_dir(top_layer, LV_DIR_NONE);
    lv_obj_set_style_bg_color(top_layer, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_COVER, LV_STATE_USER_1);
    lv_obj_set_style_bg_opa(top_layer, LV_OPA_0, LV_STATE_DEFAULT);
    lv_obj_set_style_transition(top_layer, &R.styles.transition_dsc, LV_STATE_DEFAULT);
    lv_obj_set_style_transition(top_layer, &R.styles.transition_dsc, LV_STATE_USER_1);
    
    // 注释：仅创建音量条，播放列表已移至新的playlist_manager系统

    // VOLUME BAR
    R.ui.volume_bar = lv_obj_create(top_layer);
    lv_obj_remove_style_all(R.ui.volume_bar);
    lv_obj_set_size(R.ui.volume_bar, 60, 180);
    lv_obj_set_style_bg_color(R.ui.volume_bar, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(R.ui.volume_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_opa(R.ui.volume_bar, LV_OPA_0, LV_STATE_DEFAULT);
    lv_obj_set_style_opa(R.ui.volume_bar, LV_OPA_COVER, LV_STATE_USER_1);
    lv_obj_set_style_border_width(R.ui.volume_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(R.ui.volume_bar, 16, LV_PART_MAIN);
    lv_obj_set_style_clip_corner(R.ui.volume_bar, true, LV_PART_MAIN);
    lv_obj_align(R.ui.volume_bar, LV_ALIGN_BOTTOM_RIGHT, -45, -95);
    lv_obj_set_style_transition(R.ui.volume_bar, &R.styles.transition_dsc, LV_STATE_DEFAULT);

    R.ui.volume_bar_indic = lv_obj_create(R.ui.volume_bar);
    lv_obj_remove_style_all(R.ui.volume_bar_indic);
    lv_obj_set_style_bg_color(R.ui.volume_bar_indic, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(R.ui.volume_bar_indic, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_size(R.ui.volume_bar_indic, LV_PCT(100), 40);
    lv_obj_align(R.ui.volume_bar_indic, LV_ALIGN_BOTTOM_MID, 0, 0);

    // 旧播放列表系统已移除 - 现在使用playlist_manager.c中的新系统
    // 设置播放列表相关UI指针为NULL，避免在其他地方误用
    R.ui.playlist_base = NULL;
    R.ui.playlist = NULL;

    lv_obj_add_flag(R.ui.volume_bar_indic, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(R.ui.volume_bar, app_volume_bar_event_handler, LV_EVENT_ALL, NULL);
}

static void app_create_error_page(void)
{
    lv_obj_t* root = lv_screen_active();
    lv_obj_t* label = lv_label_create(root);
    lv_obj_set_width(label, LV_PCT(80));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_label_set_text(label, "Vela的音乐播放器\n资源加载失败\n请检查设备和配置");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFF6B6B), LV_PART_MAIN);
    lv_obj_center(label);
}

static void app_create_main_page(void)
{
    lv_obj_t* root = lv_screen_active();

    // 🖼️ 背景图片设置 - 支持PNG背景图片
    if (access(R.images.background, F_OK) == 0) {
        // 如果背景图片存在，使用图片背景
        lv_obj_set_style_bg_img_src(root, R.images.background, LV_PART_MAIN);
        lv_obj_set_style_bg_image_opa(root, LV_OPA_30, LV_PART_MAIN);  // 半透明背景图
        lv_obj_set_style_bg_color(root, lv_color_hex(0x1F2937), LV_PART_MAIN);  // 保留渐变作为后备
        lv_obj_set_style_bg_grad_color(root, lv_color_hex(0x111827), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, LV_PART_MAIN);
        LV_LOG_USER("🖼️ 使用背景图片: %s", R.images.background);
    } else {
        // 如果背景图片不存在，使用渐变背景
        lv_obj_set_style_bg_color(root, lv_color_hex(0x1F2937), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(root, lv_color_hex(0x111827), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_dir(root, LV_GRAD_DIR_VER, LV_PART_MAIN);
        LV_LOG_WARN("⚠️ 背景图片不存在，使用默认渐变背景: %s", R.images.background);
    }
    lv_obj_set_style_border_width(root, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(root, 16, LV_PART_MAIN);

    // 🔝 顶部状态栏 - WiFi信号、电池、时间区域
    lv_obj_t* status_bar = lv_obj_create(root);
    lv_obj_remove_style_all(status_bar);
    lv_obj_set_size(status_bar, LV_PCT(100), 48);
    lv_obj_add_style(status_bar, &R.styles.frosted_glass, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(status_bar, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(status_bar, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 左侧车机品牌标识
    lv_obj_t* brand_label = lv_label_create(status_bar);
    lv_label_set_text(brand_label, "Vela Audio");
    lv_obj_set_style_text_font(brand_label, R.fonts.size_22.bold, LV_PART_MAIN);
    lv_obj_set_style_text_color(brand_label, lv_color_hex(0x3B82F6), LV_PART_MAIN); // 霓虹蓝

    // 右侧状态信息（信号、电池、时间）
    lv_obj_t* status_info = lv_obj_create(status_bar);
    lv_obj_remove_style_all(status_info);
    lv_obj_set_size(status_info, LV_PCT(60), LV_SIZE_CONTENT);  // 给状态信息更多空间
    lv_obj_set_flex_flow(status_info, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_info, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // WiFi信号
    lv_obj_t* wifi_label = lv_label_create(status_info);
    lv_label_set_text(wifi_label, "WiFi");
    lv_obj_set_style_text_font(wifi_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_margin_right(wifi_label, 16, LV_PART_MAIN);

    // 电池
    lv_obj_t* battery_label = lv_label_create(status_info);
    lv_label_set_text(battery_label, "85%");
    lv_obj_set_style_text_font(battery_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_margin_right(battery_label, 20, LV_PART_MAIN);

    // 时间和日期容器 - 垂直排列以节省水平空间
    lv_obj_t* time_container = lv_obj_create(status_info);
    lv_obj_remove_style_all(time_container);
    lv_obj_set_size(time_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(time_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(time_container, 0, LV_PART_MAIN);
    
    // 时间显示 - 较大字体
    lv_obj_t* time_label = lv_label_create(time_container);
    R.ui.time = time_label;
    lv_label_set_text(time_label, "12:34");  // 使用示例时间确保足够宽度
    lv_obj_set_style_text_font(time_label, R.fonts.size_24.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(time_label, 2, LV_PART_MAIN);

    // 星期显示 - 紧凑排列在时间下方
    lv_obj_t* date_label = lv_label_create(time_container);
    R.ui.date = date_label;
    lv_label_set_text(date_label, "Friday");    // 使用完整星期名确保足够宽度
    lv_obj_set_style_text_font(date_label, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xBBBBBB), LV_PART_MAIN);
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    // 主播放区域 - 现代卡片设计
    lv_obj_t* player_main = lv_obj_create(root);
    R.ui.player_group = player_main;
    lv_obj_remove_style_all(player_main);
    lv_obj_add_style(player_main, &R.styles.modern_card, LV_PART_MAIN);
    lv_obj_set_size(player_main, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(player_main, 32, LV_PART_MAIN);
    lv_obj_set_flex_flow(player_main, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(player_main, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_flex_grow(player_main, 1);

    // 3D效果专辑封面区域 + 高斯模糊背景
    lv_obj_t* cover_section = lv_obj_create(player_main);
    lv_obj_remove_style_all(cover_section);
    lv_obj_set_size(cover_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(cover_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cover_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(cover_section, 24, LV_PART_MAIN);

    // 专辑封面容器 - 支持3D旋转
    lv_obj_t* album_container = lv_obj_create(cover_section);
    R.ui.album_cover_container = album_container;
    lv_obj_remove_style_all(album_container);
    lv_obj_set_size(album_container, 320, 320);  // 增大容器以容纳多层效果
    lv_obj_set_style_transform_pivot_x(album_container, 160, 0);
    lv_obj_set_style_transform_pivot_y(album_container, 160, 0);

    // 唱片外环 - 最外层边框效果
    lv_obj_t* vinyl_ring = lv_obj_create(album_container);
    R.ui.vinyl_ring = vinyl_ring;
    lv_obj_remove_style_all(vinyl_ring);
    lv_obj_add_style(vinyl_ring, &R.styles.vinyl_ring, LV_PART_MAIN);
    lv_obj_set_size(vinyl_ring, 320, 320);
    lv_obj_center(vinyl_ring);

    // 🖼️ 专辑封面图片 - 完美圆形with动态边框 - 正确比例显示PNG图片
    lv_obj_t* album_cover = lv_image_create(album_container);
    R.ui.album_cover = album_cover;
    lv_obj_remove_style_all(album_cover);
    lv_obj_add_style(album_cover, &R.styles.circular_cover, LV_PART_MAIN);
    lv_obj_set_size(album_cover, 280, 280);
    lv_obj_center(album_cover);
    
    // 设置图片显示模式为保持宽高比，居中显示
    lv_image_set_scale(album_cover, 256);  // 保持适当缩放
    lv_image_set_inner_align(album_cover, LV_IMAGE_ALIGN_CENTER);
    lv_image_set_pivot(album_cover, 140, 140);  // 设置旋转中心点
    
    // 添加图片加载错误处理
    lv_image_set_src(album_cover, R.images.nocover);
    
    // 确保PNG图片按正确比例显示，不拉伸变形
    lv_obj_set_style_clip_corner(album_cover, true, LV_PART_MAIN);  // 启用圆形裁剪
    lv_obj_set_style_bg_img_recolor_opa(album_cover, LV_OPA_0, LV_PART_MAIN);  // 不重新着色

    // 移除唱片中心孔 - 用户要求不显示中间的黑点
    // lv_obj_t* vinyl_center = lv_obj_create(album_container);
    // R.ui.vinyl_center = vinyl_center;
    R.ui.vinyl_center = NULL;  // 设置为NULL避免其他代码引用

    // 歌曲信息区域 - 滚动字幕支持
    lv_obj_t* song_info = lv_obj_create(player_main);
    lv_obj_remove_style_all(song_info);
    lv_obj_set_size(song_info, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(song_info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(song_info, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(song_info, 20, LV_PART_MAIN);

    // 歌曲名称 - 大字体动态滚动字幕
    lv_obj_t* song_title = lv_label_create(song_info);
    R.ui.album_name = song_title;
    lv_label_set_text(song_title, "选择您的音乐");
    lv_label_set_long_mode(song_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(song_title, LV_PCT(90));
    lv_obj_set_style_text_align(song_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(song_title, R.fonts.size_28.normal, LV_PART_MAIN);  // 使用28号字体
    lv_obj_set_style_text_color(song_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(song_title, 12, LV_PART_MAIN);

    // 艺术家信息 - 增大字体
    lv_obj_t* artist_name = lv_label_create(song_info);
    R.ui.album_artist = artist_name;
    lv_label_set_text(artist_name, "Vela Music");
    lv_label_set_long_mode(artist_name, LV_LABEL_LONG_SCROLL);
    lv_obj_set_width(artist_name, LV_PCT(80));
    lv_obj_set_style_text_align(artist_name, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(artist_name, R.fonts.size_22.bold, LV_PART_MAIN);  // 从16增大到22
    lv_obj_set_style_text_color(artist_name, lv_color_hex(0xE5E7EB), LV_PART_MAIN);

    // 播放进度区域
    lv_obj_t* progress_section = lv_obj_create(player_main);
    R.ui.playback_group = progress_section;
    lv_obj_remove_style_all(progress_section);
    lv_obj_set_size(progress_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(progress_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(progress_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(progress_section, 24, LV_PART_MAIN);

    // 进度条和时间的横向布局容器
    lv_obj_t* progress_bar_container = lv_obj_create(progress_section);
    lv_obj_remove_style_all(progress_bar_container);
    lv_obj_set_size(progress_bar_container, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(progress_bar_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(progress_bar_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(progress_bar_container, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(progress_bar_container, 0, LV_PART_MAIN);

    // 渐变进度条 - 占据大部分宽度
    lv_obj_t* progress_bar = lv_bar_create(progress_bar_container);
    R.ui.playback_progress = progress_bar;
    lv_obj_remove_style_all(progress_bar);
    lv_obj_add_style(progress_bar, &R.styles.gradient_progress, LV_PART_INDICATOR);
    lv_obj_set_size(progress_bar, LV_PCT(65), 6);  // 调整宽度为65%，为时间显示留出更多空间
    lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(progress_bar, 3, LV_PART_MAIN);

    // 时间显示区域 - 右侧对齐，精简设计
    lv_obj_t* time_display = lv_spangroup_create(progress_bar_container);
    lv_span_t* current_time = lv_spangroup_new_span(time_display);
    lv_span_t* separator = lv_spangroup_new_span(time_display);
    lv_span_t* total_time = lv_spangroup_new_span(time_display);
    R.ui.playback_current_time = current_time;
    R.ui.playback_total_time = total_time;
    
    lv_span_set_text(current_time, "00:00");
    lv_span_set_text(separator, " / ");  // 使用"/"分隔符，更简洁
    lv_span_set_text(total_time, "00:00");
    lv_obj_set_style_text_font(time_display, R.fonts.size_16.normal, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_display, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_text_align(time_display, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);  // 右对齐
    lv_obj_set_style_margin_right(time_display, 8, LV_PART_MAIN);  // 右侧留白
    lv_style_set_text_color(&separator->style, lv_color_hex(0x9CA3AF));
    lv_style_set_text_color(&total_time->style, lv_color_hex(0x9CA3AF));

    // 多功能控制区域 - 水平分散排列，间距加大
    lv_obj_t* control_area = lv_obj_create(player_main);
    lv_obj_remove_style_all(control_area);
    lv_obj_set_size(control_area, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(control_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(control_area, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 均匀分散排列
    lv_obj_set_style_pad_all(control_area, 20, LV_PART_MAIN);  // 增加内边距从12到20
    lv_obj_set_style_pad_column(control_area, 16, LV_PART_MAIN);  // 设置按钮之间的列间距

    // 播放列表按钮 - 现代圆形设计
    lv_obj_t* playlist_btn = lv_button_create(control_area);
    lv_obj_t* playlist_icon = lv_image_create(playlist_btn);
    lv_obj_remove_style_all(playlist_btn);
    lv_obj_add_style(playlist_btn, &R.styles.button_default, LV_STATE_DEFAULT);
    lv_obj_add_style(playlist_btn, &R.styles.button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(playlist_btn, 64, 64);
    lv_obj_set_style_bg_color(playlist_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(playlist_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(playlist_btn, 32, LV_PART_MAIN);
    // 点击时的颜色变化
    lv_obj_set_style_bg_color(playlist_btn, lv_color_hex(0x4B5563), LV_STATE_PRESSED);
    lv_image_set_src(playlist_icon, R.images.playlist);
    lv_obj_set_size(playlist_icon, 28, 28);
    lv_obj_center(playlist_icon);
    // 移除右边距，使用flex自动分散排列

    // 上一首按钮
    lv_obj_t* prev_btn = lv_button_create(control_area);
    lv_obj_t* prev_icon = lv_image_create(prev_btn);
    lv_obj_remove_style_all(prev_btn);
    lv_obj_add_style(prev_btn, &R.styles.button_default, LV_STATE_DEFAULT);
    lv_obj_add_style(prev_btn, &R.styles.button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(prev_btn, 68, 68);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(prev_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(prev_btn, 34, LV_PART_MAIN);
    // 点击时的颜色变化
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x4B5563), LV_STATE_PRESSED);
    lv_image_set_src(prev_icon, R.images.previous);
    lv_obj_set_size(prev_icon, 32, 32);
    lv_obj_center(prev_icon);
    // 移除右边距，使用flex自动分散排列

    // 简化版本：去除快退10秒按钮，保持界面简洁

    // 主播放/暂停按钮 - 霓虹蓝发光效果
    lv_obj_t* play_btn = lv_button_create(control_area);
    lv_obj_t* play_icon = lv_image_create(play_btn);
    R.ui.play_btn = play_icon;
    lv_obj_remove_style_all(play_btn);
    lv_obj_add_style(play_btn, &R.styles.button_default, LV_STATE_DEFAULT);
    lv_obj_add_style(play_btn, &R.styles.button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(play_btn, 96, 96);
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(play_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(play_btn, 48, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(play_btn, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(play_btn, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(play_btn, LV_OPA_60, LV_PART_MAIN);
    // 点击时的特殊效果 - 更深的蓝色
    lv_obj_set_style_bg_color(play_btn, lv_color_hex(0x2563EB), LV_STATE_PRESSED);
    lv_image_set_src(play_icon, R.images.play);
    lv_obj_set_size(play_icon, 48, 48);
    lv_obj_center(play_icon);
    // 移除左右边距，使用flex自动分散排列

    // 简化版本：去除快进10秒按钮，保持界面简洁

    // 下一首按钮
    lv_obj_t* next_btn = lv_button_create(control_area);
    lv_obj_t* next_icon = lv_image_create(next_btn);
    lv_obj_remove_style_all(next_btn);
    lv_obj_add_style(next_btn, &R.styles.button_default, LV_STATE_DEFAULT);
    lv_obj_add_style(next_btn, &R.styles.button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(next_btn, 68, 68);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(next_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(next_btn, 34, LV_PART_MAIN);
    // 点击时的颜色变化
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x4B5563), LV_STATE_PRESSED);
    lv_image_set_src(next_icon, R.images.next);
    lv_obj_set_size(next_icon, 32, 32);
    lv_obj_center(next_icon);
    // 移除左边距，使用flex自动分散排列

    // 音量/音效按钮
    lv_obj_t* volume_btn = lv_button_create(control_area);
    lv_obj_t* volume_icon = lv_image_create(volume_btn);
    R.ui.audio = volume_icon;
    lv_obj_remove_style_all(volume_btn);
    lv_obj_add_style(volume_btn, &R.styles.button_default, LV_STATE_DEFAULT);
    lv_obj_add_style(volume_btn, &R.styles.button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(volume_btn, 64, 64);
    lv_obj_set_style_bg_color(volume_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(volume_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(volume_btn, 32, LV_PART_MAIN);
    // 点击时的颜色变化
    lv_obj_set_style_bg_color(volume_btn, lv_color_hex(0x4B5563), LV_STATE_PRESSED);
    lv_obj_set_size(volume_icon, 28, 28);
    lv_obj_center(volume_icon);
    // 移除左边距，使用flex自动分散排列

    // 创建顶层覆盖 (音量条、播放列表等)
    app_create_top_layer();

    // 事件绑定 - 支持手势、长按等高级交互
    lv_obj_add_event_cb(playlist_btn, app_playlist_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(volume_btn, app_audio_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(play_btn, app_play_status_event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(prev_btn, app_switch_album_event_handler, LV_EVENT_CLICKED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_PREV);
    lv_obj_add_event_cb(next_btn, app_switch_album_event_handler, LV_EVENT_CLICKED, (lv_uintptr_t*)SWITCH_ALBUM_MODE_NEXT);
    lv_obj_add_event_cb(progress_bar, app_playback_progress_bar_event_handler, LV_EVENT_ALL, NULL);

    // 启动时间更新定时器
    app_start_updating_date_time();
}

// 时间更新功能
static void app_start_updating_date_time(void)
{
    // 确保时间和日期UI组件已创建
    if (!R.ui.time || !R.ui.date) {
        LV_LOG_ERROR("Time/Date UI components not ready, cannot start timer");
        return;
    }

    LV_LOG_USER("Starting date/time update system...");
    
    // 立即更新一次时间
    app_refresh_date_time();
    
    // 创建定时器前，先检查是否已存在
    if (C.timers.refresh_date_time != NULL) {
        lv_timer_delete(C.timers.refresh_date_time);
        C.timers.refresh_date_time = NULL;
    }
    
    C.timers.refresh_date_time = lv_timer_create(app_refresh_date_time_timer_cb, 1000, NULL);
    if (C.timers.refresh_date_time == NULL) {
        LV_LOG_ERROR("Failed to create date/time update timer");
        return;
    }
    
    LV_LOG_USER("Date/Time update timer created successfully - updating every 1000ms");
}

static void read_configs(void)
{
    uint32_t file_size;
    lv_fs_file_t file;
    lv_fs_open(&file, RES_ROOT "/config.json", LV_FS_MODE_RD);

    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &file_size);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);

    char* buff = lv_malloc(file_size);
    lv_fs_read(&file, buff, file_size, NULL);

    const char* json_string = buff;

    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LV_LOG_ERROR("parse error: %p", error_ptr);
        }
        lv_free(buff);
        return;
    }

#if WIFI_ENABLED
    cJSON* wifi_object = cJSON_GetObjectItem(json, "wifi");

    const char* ssid = cJSON_GetStringValue(cJSON_GetObjectItem(wifi_object, "ssid"));
    const char* pswd = cJSON_GetStringValue(cJSON_GetObjectItem(wifi_object, "pswd"));
    const int version = cJSON_GetNumberValue(cJSON_GetObjectItem(wifi_object, "wpa_ver"));

    lv_strcpy(CF.wifi.ssid, ssid);
    lv_strcpy(CF.wifi.pswd, pswd);
    CF.wifi.ver_flag = version;
#endif

    cJSON_Delete(json);

    lv_free(buff);
}

static void reload_music_config(void)
{

    /* Clear previous music config */

    for (int i = 0; i < R.album_count; i++) {
        lv_free((void*)R.albums[i].name);
        lv_free((void*)R.albums[i].artist);
    }

    lv_free(R.albums);
    R.album_count = 0;

    /* Load music config */
    uint32_t file_size;
    lv_fs_file_t file;
    lv_fs_open(&file, MUSICS_ROOT "/manifest.json", LV_FS_MODE_RD);

    lv_fs_seek(&file, 0, LV_FS_SEEK_END);
    lv_fs_tell(&file, &file_size);
    lv_fs_seek(&file, 0, LV_FS_SEEK_SET);

    char* buff = lv_malloc(file_size);
    lv_fs_read(&file, buff, file_size, NULL);

    const char* json_string = buff;

    cJSON* json = cJSON_Parse(json_string);
    if (json == NULL) {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            LV_LOG_ERROR("parse error: %p", error_ptr);
        }
        lv_free(buff);
        return;
    }

    cJSON* musics_object = cJSON_GetObjectItem(json, "musics");

    if (musics_object == NULL) {
        lv_free(buff);
        return;
    }

    R.album_count = cJSON_GetArraySize(musics_object);
    R.albums = lv_malloc_zeroed(R.album_count * sizeof(album_info_t));

    for (int i = 0; i < R.album_count; i++) {
        cJSON* music_object = cJSON_GetArrayItem(musics_object, i);

        const char* path = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "path"));
        const char* name = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "name"));
        const char* artist = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "artist"));
        const char* cover = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "cover"));
        const double total_time_double = cJSON_GetNumberValue(cJSON_GetObjectItem(music_object, "total_time"));
        const char* color_str = cJSON_GetStringValue(cJSON_GetObjectItem(music_object, "color"));

        uint64_t total_time = (uint64_t)total_time_double;
        uint32_t color_int = strtoul(color_str + 1, NULL, 16);

        if (total_time == 0)
            total_time = 1;

        lv_color_t color = lv_color_hex(color_int);

        lv_snprintf(R.albums[i].path, sizeof(R.albums[i].path), "%s/%s", MUSICS_ROOT, path);
        lv_snprintf(R.albums[i].cover, sizeof(R.albums[i].cover), "%s/%s", MUSICS_ROOT, cover);
        R.albums[i].name = lv_strdup(name);
        R.albums[i].artist = lv_strdup(artist);
        R.albums[i].total_time = total_time;
        R.albums[i].color = color;

        LV_LOG_USER("Album %d: %s - %s | %s %s %lu", i, R.albums[i].name, R.albums[i].artist, R.albums[i].path, R.albums[i].cover, (unsigned long)total_time);
    }

    cJSON_Delete(json);

    lv_free(buff);
}

/**********************
 * MODERN UI ANIMATIONS
 **********************/

/**
 * @brief 封面旋转动画回调函数
 */
static void app_cover_rotation_anim_cb(void* obj, int32_t value)
{
    lv_obj_t* cover = (lv_obj_t*)obj;
    C.animations.rotation_angle = value;
    lv_obj_set_style_transform_rotation(cover, value, 0);
}

/**
 * @brief 开始封面旋转动画 - 优化平滑度和真实感
 */
static void app_start_cover_rotation_animation(void)
{
    if (C.animations.is_rotating) return;
    
    lv_anim_init(&C.animations.cover_rotation_anim);
    lv_anim_set_var(&C.animations.cover_rotation_anim, R.ui.album_cover_container);
    lv_anim_set_exec_cb(&C.animations.cover_rotation_anim, app_cover_rotation_anim_cb);
    lv_anim_set_values(&C.animations.cover_rotation_anim, C.animations.rotation_angle, C.animations.rotation_angle + 3600);
    lv_anim_set_duration(&C.animations.cover_rotation_anim, COVER_ROTATION_DURATION);
    lv_anim_set_repeat_count(&C.animations.cover_rotation_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&C.animations.cover_rotation_anim, lv_anim_path_linear);  // 线性动画确保匀速旋转
    lv_anim_start(&C.animations.cover_rotation_anim);
    
    C.animations.is_rotating = true;
    LV_LOG_USER("Album cover rotation started - simulating vinyl record");
}

/**
 * @brief 停止封面旋转动画 - 保持当前角度
 */
static void app_stop_cover_rotation_animation(void)
{
    if (!C.animations.is_rotating) return;
    
    lv_anim_delete(R.ui.album_cover_container, app_cover_rotation_anim_cb);
    C.animations.is_rotating = false;
    LV_LOG_USER("⏸️ Album cover rotation stopped - vinyl record paused");
}
