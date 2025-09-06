//
// Vela 音乐播放器 - 优化版播放列表管理器
// Created by Vela on 2025/8/25
// 实现全屏覆盖UI、大字体设计和专业交互体验
//

#include "music_player.h"
#include "font_config.h"
#include <stdio.h>

// 外部变量声明
extern struct resource_s R;
extern struct ctx_s C;

/*********************
 *      DEFINES
 *********************/
#define PLAYLIST_ITEM_HEIGHT 120       // 进一步增大列表项高度，提供更好的触控体验
#define PLAYLIST_ANIMATION_TIME 300
#define PLAYLIST_ITEM_PADDING 28       // 增大内边距，更舒适的视觉效果
#define PLAYLIST_TITLE_FONT_SIZE 32    // 标题字体大小
#define PLAYLIST_SONG_FONT_SIZE 24     // 歌曲名字体大小
#define PLAYLIST_INFO_FONT_SIZE 20     // 信息字体大小

// 响应式设计常量
#define MIN_TOUCH_TARGET_SIZE 44       // 最小触控目标尺寸（44px，符合人机交互标准）
#define RESPONSIVE_PADDING_RATIO 0.02  // 响应式内边距比例（屏幕宽度的2%）

/*********************
 *  STATIC VARIABLES
 *********************/
static lv_obj_t* playlist_container = NULL;
static lv_obj_t* playlist_scroll_area = NULL;
static lv_obj_t* search_input = NULL;
static lv_obj_t* sort_dropdown = NULL;
static char search_filter[64] = "";

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void playlist_item_click_cb(lv_event_t* e);
static void playlist_search_cb(lv_event_t* e);
static void playlist_sort_cb(lv_event_t* e);
static void create_playlist_item(album_info_t* album, int index);
static void playlist_item_hover_cb(lv_event_t* e);
static void playlist_close_cb(lv_event_t* e);
static void close_playlist_timer_cb(lv_timer_t* timer);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 创建全屏播放列表界面 - 优化版
 */
void playlist_manager_create(lv_obj_t* parent)
{
    if (playlist_container) {
        return; // 已经创建
    }

    // 🎵 全屏覆盖容器 - 完全覆盖主界面，优化动画效果
    playlist_container = lv_obj_create(parent);
    lv_obj_remove_style_all(playlist_container);
    lv_obj_set_size(playlist_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(playlist_container, 0, 0);  // 确保完全覆盖
    lv_obj_set_style_bg_color(playlist_container, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(playlist_container, LV_OPA_80, LV_PART_MAIN);  // 增加透明度提升可读性
    lv_obj_set_style_radius(playlist_container, 0, LV_PART_MAIN);
    
    // 添加淡入动画
    lv_obj_set_style_opa(playlist_container, LV_OPA_0, LV_PART_MAIN);
    lv_anim_t fade_in;
    lv_anim_init(&fade_in);
    lv_anim_set_var(&fade_in, playlist_container);
    lv_anim_set_exec_cb(&fade_in, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&fade_in, LV_OPA_0, LV_OPA_COVER);
    lv_anim_set_duration(&fade_in, 250);  // 快速淡入
    lv_anim_set_path_cb(&fade_in, lv_anim_path_ease_out);
    lv_anim_start(&fade_in);
    
    // 添加背景点击关闭功能
    lv_obj_add_flag(playlist_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(playlist_container, playlist_close_cb, LV_EVENT_CLICKED, NULL);

    // 🎵 主内容区域 - 全屏设计，现代化卡片效果
    lv_obj_t* main_content = lv_obj_create(playlist_container);
    lv_obj_remove_style_all(main_content);
    lv_obj_set_size(main_content, LV_PCT(95), LV_PCT(95));  // 稍微缩小以显示背景
    lv_obj_center(main_content);  // 居中显示
    lv_obj_set_style_bg_color(main_content, lv_color_hex(0x111827), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_content, LV_OPA_90, LV_PART_MAIN);  // 高透明度
    lv_obj_set_style_radius(main_content, 20, LV_PART_MAIN);  // 圆角效果
    lv_obj_set_style_shadow_width(main_content, 30, LV_PART_MAIN);  // 阴影效果
    lv_obj_set_style_shadow_color(main_content, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(main_content, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_flex_flow(main_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(main_content, 32, LV_PART_MAIN);  // 适当内边距
    
    // 防止内容区域事件冒泡
    lv_obj_remove_flag(main_content, LV_OBJ_FLAG_EVENT_BUBBLE);

    // 🎵 顶部导航栏 - 更大更清晰
    lv_obj_t* header = lv_obj_create(main_content);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), 100);  // 更大的标题栏
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_margin_bottom(header, 32, LV_PART_MAIN);

    // 返回按钮 - 更大更明显，支持UTF-8
    lv_obj_t* back_btn = lv_button_create(header);
    lv_obj_remove_style_all(back_btn);
    lv_obj_set_size(back_btn, 80, 60);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(back_btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_btn, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(back_btn, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, playlist_close_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    // 使用UTF-8字体配置系统
    set_label_utf8_text(back_label, "返回", get_font_by_size(20));
    lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(back_label);

    // 标题文字 - 更大更突出，支持UTF-8中文
    lv_obj_t* title = lv_label_create(header);
    // 使用UTF-8字体配置系统
    set_label_utf8_text(title, "我的播放列表", get_font_by_size(32));
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);  // 纯白色

    // 设置按钮占位符（保持布局平衡）
    lv_obj_t* placeholder = lv_obj_create(header);
    lv_obj_remove_style_all(placeholder);
    lv_obj_set_size(placeholder, 80, 60);

    // 🔍 搜索输入框 - 更大字体和更好视觉效果
    search_input = lv_textarea_create(main_content);
    lv_obj_set_size(search_input, LV_PCT(100), 72);  // 增加高度
    lv_textarea_set_placeholder_text(search_input, "搜索歌曲或艺术家...");
    lv_textarea_set_one_line(search_input, true);
    // 使用系统可用的字体
#if LV_FONT_MONTSERRAT_24
    lv_obj_set_style_text_font(search_input, &lv_font_montserrat_24, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(search_input, &lv_font_montserrat_20, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(search_input, &lv_font_montserrat_16, LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(search_input, &lv_font_default, LV_PART_MAIN);
#endif
    lv_obj_set_style_bg_color(search_input, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(search_input, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(search_input, lv_color_hex(0x3B82F6), LV_PART_MAIN);  // 蓝色边框
    lv_obj_set_style_border_width(search_input, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(search_input, 20, LV_PART_MAIN);
    lv_obj_set_style_text_color(search_input, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(search_input, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_all(search_input, 20, LV_PART_MAIN);  // 增加内边距
    lv_obj_add_event_cb(search_input, playlist_search_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 📊 排序下拉框 - 更大字体和更好交互
    sort_dropdown = lv_dropdown_create(main_content);
    lv_obj_set_size(sort_dropdown, LV_PCT(100), 64);  // 增加高度
    lv_dropdown_set_options(sort_dropdown, "按名称排序\n按艺术家排序\n按时长排序");
    // 使用系统可用的字体
#if LV_FONT_MONTSERRAT_22
    lv_obj_set_style_text_font(sort_dropdown, &lv_font_montserrat_22, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(sort_dropdown, &lv_font_montserrat_20, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(sort_dropdown, &lv_font_montserrat_16, LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(sort_dropdown, &lv_font_default, LV_PART_MAIN);
#endif
    lv_obj_set_style_bg_color(sort_dropdown, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sort_dropdown, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(sort_dropdown, lv_color_hex(0x6B7280), LV_PART_MAIN);
    lv_obj_set_style_border_width(sort_dropdown, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(sort_dropdown, 16, LV_PART_MAIN);
    lv_obj_set_style_text_color(sort_dropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(sort_dropdown, 32, LV_PART_MAIN);
    lv_obj_set_style_pad_all(sort_dropdown, 20, LV_PART_MAIN);  // 增加内边距
    lv_obj_add_event_cb(sort_dropdown, playlist_sort_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 📋 歌曲列表滚动区域 - 占用剩余全部空间
    playlist_scroll_area = lv_obj_create(main_content);
    lv_obj_remove_style_all(playlist_scroll_area);
    lv_obj_set_size(playlist_scroll_area, LV_PCT(100), LV_PCT(100));  // 使用剩余空间
    lv_obj_set_flex_grow(playlist_scroll_area, 1);  // 自动扩展
    lv_obj_set_style_bg_opa(playlist_scroll_area, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(playlist_scroll_area, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(playlist_scroll_area, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(playlist_scroll_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(playlist_scroll_area, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(playlist_scroll_area, 16, LV_PART_MAIN);  // 列表项间距

    // 防止滚动区域事件冒泡
    lv_obj_remove_flag(playlist_scroll_area, LV_OBJ_FLAG_EVENT_BUBBLE);

    // 🎵 加载播放列表内容
    playlist_manager_refresh();
    
    // 🎬 入场动画效果
    lv_anim_t anim_in;
    lv_anim_init(&anim_in);
    lv_anim_set_var(&anim_in, playlist_container);
    lv_anim_set_values(&anim_in, LV_OPA_TRANSP, LV_OPA_70);
    lv_anim_set_duration(&anim_in, PLAYLIST_ANIMATION_TIME);
    lv_anim_set_exec_cb(&anim_in, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
    lv_anim_set_path_cb(&anim_in, lv_anim_path_ease_out);
    lv_anim_start(&anim_in);
}

/**
 * @brief 刷新播放列表内容
 */
void playlist_manager_refresh(void)
{
    if (!playlist_scroll_area) {
        return;
    }

    // 清空现有列表
    lv_obj_clean(playlist_scroll_area);

    // 重新创建列表项
    for (int i = 0; i < R.album_count; i++) {
        create_playlist_item(&R.albums[i], i);
    }
}

/**
 * @brief 创建优化的播放列表项 - 响应式大字体设计
 */
static void create_playlist_item(album_info_t* album, int index)
{
    // 🎵 列表项容器 - 响应式设计，适配不同屏幕尺寸
    lv_obj_t* item = lv_obj_create(playlist_scroll_area);
    lv_obj_remove_style_all(item);
    lv_obj_set_size(item, LV_PCT(100), PLAYLIST_ITEM_HEIGHT);
    lv_obj_set_style_bg_color(item, lv_color_hex(0x1F2937), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(item, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(item, 2, LV_PART_MAIN);  // 增加边框宽度
    lv_obj_set_style_border_color(item, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);  // 改为左对齐
    lv_obj_set_style_pad_all(item, PLAYLIST_ITEM_PADDING, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(item, 16, LV_PART_MAIN);  // 增加间距
    
    // 添加悬停和选中状态样式
    lv_obj_set_style_bg_color(item, lv_color_hex(0x2563EB), LV_STATE_PRESSED);  // 按下时蓝色
    lv_obj_set_style_border_color(item, lv_color_hex(0x3B82F6), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(item, 8, LV_STATE_PRESSED);
    lv_obj_set_style_shadow_color(item, lv_color_hex(0x3B82F6), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_opa(item, LV_OPA_50, LV_STATE_PRESSED);
    
    // 添加点击和悬停效果
    lv_obj_add_flag(item, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(item, playlist_item_click_cb, LV_EVENT_CLICKED, (void*)(uintptr_t)index);
    lv_obj_add_event_cb(item, playlist_item_hover_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(item, playlist_item_hover_cb, LV_EVENT_RELEASED, NULL);

    // 🎵 左侧播放图标区域
    lv_obj_t* play_icon_container = lv_obj_create(item);
    lv_obj_remove_style_all(play_icon_container);
    lv_obj_set_size(play_icon_container, 60, 60);
    lv_obj_set_style_bg_color(play_icon_container, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(play_icon_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(play_icon_container, 30, LV_PART_MAIN);
    
    lv_obj_t* play_icon = lv_label_create(play_icon_container);
    lv_label_set_text(play_icon, "▶");  // 使用符号代替文字
    
    // 使用系统可用的字体
#if LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(play_icon, &lv_font_montserrat_16, LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(play_icon, &lv_font_default, LV_PART_MAIN);
#endif
    lv_obj_set_style_text_color(play_icon, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(play_icon);

    // 🎵 中间歌曲信息区域
    lv_obj_t* info_container = lv_obj_create(item);
    lv_obj_remove_style_all(info_container);
    lv_obj_set_flex_grow(info_container, 1);  // 占用剩余空间
    lv_obj_set_height(info_container, LV_PCT(100));
    lv_obj_set_flex_flow(info_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_left(info_container, 20, LV_PART_MAIN);

    // 歌曲名称 - 大字体，支持UTF-8编码
    lv_obj_t* song_name = lv_label_create(info_container);
    
    // 使用UTF-8字体配置系统显示歌曲名称
    const char* display_name = (album->name && strlen(album->name) > 0) ? album->name : "未知歌曲";
    set_label_utf8_text(song_name, display_name, get_font_by_size(24));
    
    lv_obj_set_style_text_color(song_name, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_label_set_long_mode(song_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(song_name, LV_PCT(90));  // 留出更多空间避免文字被截断
    lv_obj_set_style_text_align(song_name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);  // 左对齐

    // 艺术家名称 - 中等字体，支持UTF-8编码
    lv_obj_t* artist_name = lv_label_create(info_container);
    
    // 使用UTF-8字体配置系统显示艺术家名称
    const char* display_artist = (album->artist && strlen(album->artist) > 0) ? album->artist : "未知艺术家";
    set_label_utf8_text(artist_name, display_artist, get_font_by_size(20));
    
    lv_obj_set_style_text_color(artist_name, lv_color_hex(0xD1D5DB), LV_PART_MAIN);  // 浅灰色
    lv_label_set_long_mode(artist_name, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(artist_name, LV_PCT(90));  // 留出更多空间
    lv_obj_set_style_text_align(artist_name, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);  // 左对齐

    // 🕒 右侧时长显示 - 大字体
    lv_obj_t* duration_label = lv_label_create(item);
    
    // 格式化时长显示
    uint32_t total_seconds = album->total_time / 1000;
    uint32_t minutes = total_seconds / 60;
    uint32_t seconds = total_seconds % 60;
    
    char duration_text[16];
    snprintf(duration_text, sizeof(duration_text), "%02lu:%02lu", 
             (unsigned long)minutes, (unsigned long)seconds);
    
    lv_label_set_text(duration_label, duration_text);
    
    // 使用系统可用的字体
#if LV_FONT_MONTSERRAT_24
    lv_obj_set_style_text_font(duration_label, &lv_font_montserrat_24, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_20
    lv_obj_set_style_text_font(duration_label, &lv_font_montserrat_20, LV_PART_MAIN);
#elif LV_FONT_MONTSERRAT_16
    lv_obj_set_style_text_font(duration_label, &lv_font_montserrat_16, LV_PART_MAIN);
#else
    lv_obj_set_style_text_font(duration_label, &lv_font_default, LV_PART_MAIN);
#endif
    lv_obj_set_style_text_color(duration_label, lv_color_hex(0x9CA3AF), LV_PART_MAIN);  // 中等灰色
}

/**
 * @brief 关闭播放列表
 */
void playlist_manager_close(void)
{
    if (!playlist_container) {
        return;
    }

    // 🎬 退场动画
    lv_anim_t anim_out;
    lv_anim_init(&anim_out);
    lv_anim_set_var(&anim_out, playlist_container);
    lv_anim_set_values(&anim_out, LV_OPA_70, LV_OPA_TRANSP);
    lv_anim_set_duration(&anim_out, PLAYLIST_ANIMATION_TIME);
    lv_anim_set_exec_cb(&anim_out, (lv_anim_exec_xcb_t)lv_obj_set_style_bg_opa);
    lv_anim_set_path_cb(&anim_out, lv_anim_path_ease_in);
    lv_anim_set_deleted_cb(&anim_out, (lv_anim_deleted_cb_t)lv_obj_del);
    lv_anim_start(&anim_out);

    playlist_container = NULL;
    playlist_scroll_area = NULL;
    search_input = NULL;
    sort_dropdown = NULL;
}

/**
 * @brief 检查播放列表是否打开
 */
bool playlist_manager_is_open(void)
{
    return playlist_container != NULL;
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 播放列表项点击回调 - 修复交互逻辑
 */
static void playlist_item_click_cb(lv_event_t* e)
{
    int index = (int)(uintptr_t)lv_event_get_user_data(e);
    
    // 🎵 切换到选中的歌曲
    if (index >= 0 && index < R.album_count) {
        printf("🎵 用户选择歌曲: %s (索引: %d)\n", 
               R.albums[index].name, index);
        
        // 安全地更新当前播放专辑
        C.current_album = &R.albums[index];
        
        // 调用切换函数，传入正确的索引参数
        extern void app_switch_to_album(int index);  // 外部声明
        app_switch_to_album(index);
        
        // 设置播放状态（如果需要自动播放）
        extern void app_set_play_status(play_status_t status);  // 外部声明
        app_set_play_status(PLAY_STATUS_PLAY);  // 设置为播放状态
        
        // 关闭播放列表（延迟关闭以确保切换完成）
        lv_timer_t* close_timer = lv_timer_create(close_playlist_timer_cb, 500, NULL);
        lv_timer_set_repeat_count(close_timer, 1);
        
        printf("🎵 播放列表切换完成: %s\n", R.albums[index].name);
    } else {
        printf("❌ 无效的歌曲索引: %d (总数: %d)\n", index, R.album_count);
    }
}

/**
 * @brief 搜索输入回调
 */
static void playlist_search_cb(lv_event_t* e)
{
    const char* text = lv_textarea_get_text(search_input);
    strncpy(search_filter, text, sizeof(search_filter) - 1);
    search_filter[sizeof(search_filter) - 1] = '\0';
    
    // 重新刷新列表（这里可以添加搜索过滤逻辑）
    playlist_manager_refresh();
}

/**
 * @brief 排序下拉框回调
 */
static void playlist_sort_cb(lv_event_t* e)
{
    uint16_t selected = lv_dropdown_get_selected(sort_dropdown);
    
    // 这里可以添加排序逻辑
    printf("📊 选择排序方式: %d\n", selected);
    
    // 重新刷新列表
    playlist_manager_refresh();
}

/**
 * @brief 列表项悬停效果
 */
static void playlist_item_hover_cb(lv_event_t* e)
{
    lv_obj_t* item = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_PRESSED) {
        // 按下时的视觉反馈
        lv_obj_set_style_bg_color(item, lv_color_hex(0x3B82F6), LV_PART_MAIN);
        lv_obj_set_style_transform_scale(item, 98, LV_PART_MAIN);  // 轻微缩放
    } else if (code == LV_EVENT_RELEASED) {
        // 释放时恢复
        lv_obj_set_style_bg_color(item, lv_color_hex(0x1F2937), LV_PART_MAIN);
        lv_obj_set_style_transform_scale(item, 100, LV_PART_MAIN);
    }
}

/**
 * @brief 关闭播放列表回调
 */
static void playlist_close_cb(lv_event_t* e)
{
    // 只有点击背景容器时才关闭
    if (lv_event_get_target(e) == playlist_container) {
        playlist_manager_close();
    }
}

/**
 * @brief 延迟关闭播放列表的定时器回调
 */
static void close_playlist_timer_cb(lv_timer_t* timer)
{
    playlist_manager_close();
    lv_timer_del(timer);
}

