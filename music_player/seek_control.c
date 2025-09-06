//
// Vela 音乐播放器 - 精确进度控制模块
// Created by Vela on 2025/8/25
// 实现前进/后退10秒、精确跳转等进度控制功能
//

#include "music_player.h"
#include <sys/time.h>

// 外部变量声明
extern struct resource_s R;
extern struct ctx_s C;

/*********************
 *      DEFINES
 *********************/
#define SEEK_STEP_MS 10000          // 10秒跳转步长
#define SEEK_FEEDBACK_DURATION 1500 // 反馈显示时长
#define SEEK_ANIMATION_DURATION 200 // 跳转动画时长

/*********************
 *  STATIC VARIABLES
 *********************/
static lv_obj_t* seek_feedback_label = NULL;
static lv_timer_t* seek_feedback_timer = NULL;
static bool seek_operation_pending = false;

/*********************
 *  STATIC PROTOTYPES
 *********************/
static uint64_t get_current_time_ms(void);
static void show_seek_feedback(const char* text);
static void hide_seek_feedback_timer_cb(lv_timer_t* timer);
static void update_progress_bar_immediately(void);
static int64_t calculate_mp3_file_offset(audioctl_s* ctl, int64_t position_ms);
static int64_t calculate_wav_file_offset(audioctl_s* ctl, int64_t position_ms);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 快进10秒
 */
int audio_ctl_seek_forward_10s(audioctl_s* ctl)
{
    if (!ctl || ctl->state != AUDIO_CTL_STATE_START) {
        printf("⚠️ 音频控制器状态异常，无法快进\n");
        return -1;
    }
    
    // 获取当前播放位置
    int64_t current_pos = audio_ctl_get_current_position_ms(ctl);
    int64_t target_pos = current_pos + SEEK_STEP_MS;
    
    // 获取总时长
    int64_t total_duration = audio_ctl_get_total_duration_ms(ctl);
    
    // 边界检查
    if (target_pos >= total_duration) {
        target_pos = total_duration - 1000; // 留1秒缓冲
        printf("🎯 快进到文件末尾附近: %lld ms\n", target_pos);
    } else {
        printf("⏩ 快进10秒: %lld -> %lld ms\n", current_pos, target_pos);
    }
    
    // 显示反馈
    show_seek_feedback("⏩ +10s");
    
    return audio_ctl_seek_to_position(ctl, target_pos);
}

/**
 * @brief 快退10秒
 */
int audio_ctl_seek_backward_10s(audioctl_s* ctl)
{
    if (!ctl || ctl->state != AUDIO_CTL_STATE_START) {
        printf("⚠️ 音频控制器状态异常，无法快退\n");
        return -1;
    }
    
    // 获取当前播放位置
    int64_t current_pos = audio_ctl_get_current_position_ms(ctl);
    int64_t target_pos = current_pos - SEEK_STEP_MS;
    
    // 边界检查
    if (target_pos < 0) {
        target_pos = 0; // 回到开始
        printf("🎯 快退到文件开始: 0 ms\n");
    } else {
        printf("⏪ 快退10秒: %lld -> %lld ms\n", current_pos, target_pos);
    }
    
    // 显示反馈
    show_seek_feedback("⏪ -10s");
    
    return audio_ctl_seek_to_position(ctl, target_pos);
}

/**
 * @brief 精确跳转到指定位置
 */
int audio_ctl_seek_to_position(audioctl_s* ctl, int64_t position_ms)
{
    if (!ctl) {
        return -1;
    }
    
    printf("🎯 跳转到位置: %lld ms\n", position_ms);
    
    // 防止重复跳转操作
    if (seek_operation_pending) {
        printf("⚠️ 跳转操作正在进行中，忽略新请求\n");
        return -1;
    }
    
    seek_operation_pending = true;
    
    // 计算文件偏移量
    int64_t file_offset = 0;
    
    if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        file_offset = calculate_mp3_file_offset(ctl, position_ms);
        printf("🎵 MP3跳转计算: 位置%lld ms -> 文件偏移%lld\n", position_ms, file_offset);
    } else if (ctl->audio_format == AUDIO_FORMAT_WAV) {
        file_offset = calculate_wav_file_offset(ctl, position_ms);
        printf("🎵 WAV跳转计算: 位置%lld ms -> 文件偏移%lld\n", position_ms, file_offset);
    } else {
        printf("❌ 不支持的音频格式跳转\n");
        seek_operation_pending = false;
        return -1;
    }
    
    // 设置跳转参数
    ctl->seek = true;
    ctl->seek_position = file_offset;
    
    // 更新UI进度条
    update_progress_bar_immediately();
    
    seek_operation_pending = false;
    return 0;
}

/**
 * @brief 获取当前播放位置(毫秒)
 */
int64_t audio_ctl_get_current_position_ms(audioctl_s* ctl)
{
    if (!ctl) {
        return 0;
    }
    
    // 根据文件格式计算当前位置
    if (ctl->audio_format == AUDIO_FORMAT_WAV && ctl->wav.fmt.samplerate > 0) {
        // WAV格式可以精确计算
        int64_t bytes_per_ms = (ctl->wav.fmt.samplerate * ctl->wav.fmt.numchannels * 
                               ctl->wav.fmt.bitspersample / 8) / 1000;
        if (bytes_per_ms > 0) {
            return (ctl->file_position - ctl->wav.data_offset) / bytes_per_ms;
        }
    } else if (ctl->audio_format == AUDIO_FORMAT_MP3) {
        // MP3格式需要估算（基于文件位置比例）
        if (ctl->file_size > 0) {
            // 获取当前歌曲总时长
            album_info_t* current_album = &C.albums.data[C.albums.current];
            int64_t total_duration = current_album->total_time;
            
            // 按文件位置比例估算
            double progress = (double)ctl->file_position / ctl->file_size;
            return (int64_t)(progress * total_duration);
        }
    }
    
    return 0;
}

/**
 * @brief 获取总时长(毫秒)
 */
int64_t audio_ctl_get_total_duration_ms(audioctl_s* ctl)
{
    if (!ctl) {
        return 0;
    }
    
    // 从当前专辑信息获取总时长
    if (C.albums.current >= 0 && C.albums.current < C.albums.count) {
        return C.albums.data[C.albums.current].total_time;
    }
    
    return 0;
}

/**
 * @brief 创建前进/后退10秒按钮
 */
void create_seek_control_buttons(lv_obj_t* parent)
{
    // 在现有的控制按钮组中添加前进/后退按钮
    
    // 🔍 查找控制按钮容器
    lv_obj_t* controls_container = R.ui.controls_section;
    if (!controls_container) {
        printf("❌ 找不到控制按钮容器\n");
        return;
    }
    
    // 获取现有按钮以调整布局
    lv_obj_t* prev_btn = R.ui.prev_btn;
    lv_obj_t* play_btn = R.ui.play_btn;
    lv_obj_t* next_btn = R.ui.next_btn;
    
    // 🎯 创建快退10秒按钮
    lv_obj_t* backward_10s_btn = lv_button_create(controls_container);
    lv_obj_remove_style_all(backward_10s_btn);
    lv_obj_set_size(backward_10s_btn, 56, 56);  // 比主按钮稍小
    lv_obj_set_style_bg_color(backward_10s_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(backward_10s_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(backward_10s_btn, 28, LV_PART_MAIN);
    lv_obj_set_style_border_width(backward_10s_btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(backward_10s_btn, lv_color_hex(0x6B7280), LV_PART_MAIN);
    lv_obj_add_event_cb(backward_10s_btn, seek_backward_10s_btn_event_cb, LV_EVENT_ALL, NULL);
    
    // 在上一首按钮后插入
    if (prev_btn) {
        lv_obj_move_to_index(backward_10s_btn, lv_obj_get_index(prev_btn) + 1);
    }
    
    lv_obj_t* backward_icon = lv_label_create(backward_10s_btn);
    lv_label_set_text(backward_icon, "-10s");
    lv_obj_set_style_text_font(backward_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(backward_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(backward_icon);
    
    // 🎯 创建快进10秒按钮
    lv_obj_t* forward_10s_btn = lv_button_create(controls_container);
    lv_obj_remove_style_all(forward_10s_btn);
    lv_obj_set_size(forward_10s_btn, 56, 56);  // 比主按钮稍小
    lv_obj_set_style_bg_color(forward_10s_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(forward_10s_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(forward_10s_btn, 28, LV_PART_MAIN);
    lv_obj_set_style_border_width(forward_10s_btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(forward_10s_btn, lv_color_hex(0x6B7280), LV_PART_MAIN);
    lv_obj_add_event_cb(forward_10s_btn, seek_forward_10s_btn_event_cb, LV_EVENT_ALL, NULL);
    
    // 在播放按钮后插入
    if (play_btn) {
        lv_obj_move_to_index(forward_10s_btn, lv_obj_get_index(play_btn) + 1);
    }
    
    lv_obj_t* forward_icon = lv_label_create(forward_10s_btn);
    lv_label_set_text(forward_icon, "+10s");
    lv_obj_set_style_text_font(forward_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(forward_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(forward_icon);
    
    // 保存按钮引用
    R.ui.backward_10s_btn = backward_10s_btn;
    R.ui.forward_10s_btn = forward_10s_btn;
    
    printf("✅ 前进/后退10秒按钮已创建\n");
}

/**
 * @brief 快进按钮事件处理
 */
void seek_forward_10s_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);
    
    switch (code) {
        case LV_EVENT_CLICKED:
            // 执行快进
            if (C.audio_ctl && audio_ctl_seek_forward_10s(C.audio_ctl) == 0) {
                printf("⏩ 快进10秒操作完成\n");
            }
            break;
            
        case LV_EVENT_PRESSED:
            // 按下时的视觉反馈
            lv_obj_set_style_transform_scale(btn, 90, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x3B82F6), LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x60A5FA), LV_PART_MAIN);
            break;
            
        case LV_EVENT_RELEASED:
            // 释放时恢复
            lv_obj_set_style_transform_scale(btn, 100, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x374151), LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x6B7280), LV_PART_MAIN);
            break;
    }
}

/**
 * @brief 快退按钮事件处理
 */
void seek_backward_10s_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);
    
    switch (code) {
        case LV_EVENT_CLICKED:
            // 执行快退
            if (C.audio_ctl && audio_ctl_seek_backward_10s(C.audio_ctl) == 0) {
                printf("⏪ 快退10秒操作完成\n");
            }
            break;
            
        case LV_EVENT_PRESSED:
            // 按下时的视觉反馈
            lv_obj_set_style_transform_scale(btn, 90, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x3B82F6), LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x60A5FA), LV_PART_MAIN);
            break;
            
        case LV_EVENT_RELEASED:
            // 释放时恢复
            lv_obj_set_style_transform_scale(btn, 100, LV_PART_MAIN);
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x374151), LV_PART_MAIN);
            lv_obj_set_style_border_color(btn, lv_color_hex(0x6B7280), LV_PART_MAIN);
            break;
    }
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 获取当前时间(毫秒)
 */
static uint64_t get_current_time_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * @brief 显示跳转反馈
 */
static void show_seek_feedback(const char* text)
{
    // 如果已有反馈标签，先清理
    if (seek_feedback_label) {
        lv_obj_del(seek_feedback_label);
        seek_feedback_label = NULL;
    }
    
    if (seek_feedback_timer) {
        lv_timer_del(seek_feedback_timer);
        seek_feedback_timer = NULL;
    }
    
    // 创建反馈标签 - 在进度条上方显示
    if (R.ui.progress_section) {
        seek_feedback_label = lv_label_create(R.ui.progress_section);
        lv_label_set_text(seek_feedback_label, text);
        lv_obj_set_style_text_font(seek_feedback_label, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(seek_feedback_label, lv_color_hex(0x3B82F6), LV_PART_MAIN);
        lv_obj_set_style_bg_color(seek_feedback_label, lv_color_hex(0x1F2937), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(seek_feedback_label, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_radius(seek_feedback_label, 12, LV_PART_MAIN);
        lv_obj_set_style_pad_all(seek_feedback_label, 12, LV_PART_MAIN);
        lv_obj_align(seek_feedback_label, LV_ALIGN_TOP_MID, 0, -40);
        
        // 入场动画
        lv_anim_t anim_in;
        lv_anim_init(&anim_in);
        lv_anim_set_var(&anim_in, seek_feedback_label);
        lv_anim_set_values(&anim_in, LV_OPA_TRANSP, LV_OPA_80);
        lv_anim_set_duration(&anim_in, SEEK_ANIMATION_DURATION);
        lv_anim_set_exec_cb(&anim_in, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
        lv_anim_start(&anim_in);
        
        // 设置自动隐藏定时器
        seek_feedback_timer = lv_timer_create(hide_seek_feedback_timer_cb, SEEK_FEEDBACK_DURATION, NULL);
        lv_timer_set_repeat_count(seek_feedback_timer, 1);
    }
}

/**
 * @brief 隐藏跳转反馈定时器回调
 */
static void hide_seek_feedback_timer_cb(lv_timer_t* timer)
{
    if (seek_feedback_label) {
        // 退场动画
        lv_anim_t anim_out;
        lv_anim_init(&anim_out);
        lv_anim_set_var(&anim_out, seek_feedback_label);
        lv_anim_set_values(&anim_out, LV_OPA_80, LV_OPA_TRANSP);
        lv_anim_set_duration(&anim_out, SEEK_ANIMATION_DURATION);
        lv_anim_set_exec_cb(&anim_out, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
        lv_anim_set_deleted_cb(&anim_out, (lv_anim_deleted_cb_t)lv_obj_del);
        lv_anim_start(&anim_out);
        
        seek_feedback_label = NULL;
    }
    
    seek_feedback_timer = NULL;
}

/**
 * @brief 立即更新进度条显示
 */
static void update_progress_bar_immediately(void)
{
    if (!C.audio_ctl || !R.ui.progress_bar) {
        return;
    }
    
    int64_t current_pos = audio_ctl_get_current_position_ms(C.audio_ctl);
    int64_t total_duration = audio_ctl_get_total_duration_ms(C.audio_ctl);
    
    if (total_duration > 0) {
        int32_t progress_value = (int32_t)((current_pos * 100) / total_duration);
        lv_bar_set_value(R.ui.progress_bar, progress_value, LV_ANIM_ON);
        
        // 更新时间显示
        if (R.ui.current_time_label) {
            uint32_t current_seconds = current_pos / 1000;
            uint32_t current_minutes = current_seconds / 60;
            current_seconds %= 60;
            
            char time_text[16];
            snprintf(time_text, sizeof(time_text), "%02u:%02u", current_minutes, current_seconds);
            lv_label_set_text(R.ui.current_time_label, time_text);
        }
    }
}

/**
 * @brief 计算MP3文件跳转偏移量
 */
static int64_t calculate_mp3_file_offset(audioctl_s* ctl, int64_t position_ms)
{
    if (!ctl || ctl->file_size == 0) {
        return 0;
    }
    
    // 获取总时长
    int64_t total_duration = audio_ctl_get_total_duration_ms(ctl);
    if (total_duration == 0) {
        return 0;
    }
    
    // 按时间比例计算文件偏移量
    double time_ratio = (double)position_ms / total_duration;
    int64_t file_offset = (int64_t)(time_ratio * ctl->file_size);
    
    // 确保偏移量在有效范围内
    if (file_offset < 0) file_offset = 0;
    if (file_offset >= ctl->file_size) file_offset = ctl->file_size - 1;
    
    return file_offset;
}

/**
 * @brief 计算WAV文件跳转偏移量
 */
static int64_t calculate_wav_file_offset(audioctl_s* ctl, int64_t position_ms)
{
    if (!ctl || ctl->audio_format != AUDIO_FORMAT_WAV) {
        return 0;
    }
    
    // WAV格式可以精确计算
    int64_t bytes_per_ms = (ctl->wav.fmt.samplerate * ctl->wav.fmt.numchannels * 
                           ctl->wav.fmt.bitspersample / 8) / 1000;
    
    if (bytes_per_ms > 0) {
        int64_t data_offset = position_ms * bytes_per_ms;
        return ctl->wav.data_offset + data_offset;
    }
    
    return 0;
}

