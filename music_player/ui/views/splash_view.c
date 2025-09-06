//
// Vela 音乐播放器 - 启动页面视图实现
// Created by Vela Engineering Team on 2024/12/18
// 模块化启动页面视图实现
//

#include "splash_view.h"
#include "../../core/event_bus.h"
#include <stdio.h>
#include <stdlib.h>

/*********************
 *  STATIC VARIABLES
 *********************/
static splash_view_data_t* g_splash_data = NULL;

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void splash_timer_callback(lv_timer_t* timer);
static void splash_fadeout_completed(lv_anim_t* anim);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 创建启动页面视图
 */
int splash_view_create(lv_obj_t* parent, void* user_data)
{
    printf("🌟 创建启动页面视图...\n");
    
    // 分配视图数据
    g_splash_data = (splash_view_data_t*)malloc(sizeof(splash_view_data_t));
    if (!g_splash_data) {
        printf("❌ 启动页面数据分配失败\n");
        return -1;
    }
    
    memset(g_splash_data, 0, sizeof(splash_view_data_t));
    
    // 创建主容器
    g_splash_data->container = lv_obj_create(parent);
    lv_obj_remove_style_all(g_splash_data->container);
    lv_obj_set_size(g_splash_data->container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(g_splash_data->container, lv_color_hex(0x0A0A0A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(g_splash_data->container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(g_splash_data->container);
    lv_obj_set_flex_flow(g_splash_data->container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_splash_data->container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Logo容器
    g_splash_data->logo_container = lv_obj_create(g_splash_data->container);
    lv_obj_remove_style_all(g_splash_data->logo_container);
    lv_obj_set_size(g_splash_data->logo_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(g_splash_data->logo_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(g_splash_data->logo_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // 品牌标题
    g_splash_data->brand_title = lv_label_create(g_splash_data->logo_container);
    lv_label_set_text(g_splash_data->brand_title, "Vela Audio");
    lv_obj_set_style_text_font(g_splash_data->brand_title, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_splash_data->brand_title, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(g_splash_data->brand_title, 8, LV_PART_MAIN);
    
    // 标语
    g_splash_data->tagline = lv_label_create(g_splash_data->logo_container);
    lv_label_set_text(g_splash_data->tagline, "Music Connects Souls");
    lv_obj_set_style_text_font(g_splash_data->tagline, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(g_splash_data->tagline, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    
    printf("✅ 启动页面视图创建成功\n");
    return 0;
}

/**
 * @brief 显示启动页面视图
 */
void splash_view_show(void* view_data)
{
    if (!g_splash_data) {
        return;
    }
    
    printf("👁️ 显示启动页面\n");
    
    g_splash_data->is_visible = true;
    
    // 启动动画
    splash_view_start_animations(g_splash_data);
    
    // 创建定时器，2秒后自动跳转
    g_splash_data->splash_timer = lv_timer_create(splash_timer_callback, SPLASH_DISPLAY_DURATION, NULL);
    lv_timer_set_repeat_count(g_splash_data->splash_timer, 1);
}

/**
 * @brief 隐藏启动页面视图
 */
void splash_view_hide(void* view_data)
{
    if (!g_splash_data) {
        return;
    }
    
    printf("🙈 隐藏启动页面\n");
    
    g_splash_data->is_visible = false;
    
    // 停止动画
    splash_view_stop_animations(g_splash_data);
    
    // 隐藏容器
    if (g_splash_data->container) {
        lv_obj_add_flag(g_splash_data->container, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
 * @brief 销毁启动页面视图
 */
void splash_view_destroy(void* view_data)
{
    if (!g_splash_data) {
        return;
    }
    
    printf("🗑️ 销毁启动页面视图\n");
    
    // 停止定时器
    if (g_splash_data->splash_timer) {
        lv_timer_delete(g_splash_data->splash_timer);
    }
    
    // 停止动画
    splash_view_stop_animations(g_splash_data);
    
    // 删除容器
    if (g_splash_data->container) {
        lv_obj_delete(g_splash_data->container);
    }
    
    // 释放数据
    free(g_splash_data);
    g_splash_data = NULL;
    
    printf("✅ 启动页面视图已销毁\n");
}

/**
 * @brief 开始启动动画
 */
int splash_view_start_animations(splash_view_data_t* view_data)
{
    if (!view_data || !view_data->logo_container) {
        return -1;
    }
    
    printf("✨ 启动页面动画开始\n");
    
    // Logo淡入动画
    lv_anim_init(&view_data->logo_fade_anim);
    lv_anim_set_var(&view_data->logo_fade_anim, view_data->logo_container);
    lv_anim_set_exec_cb(&view_data->logo_fade_anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
    lv_anim_set_values(&view_data->logo_fade_anim, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&view_data->logo_fade_anim, 800);
    lv_anim_set_path_cb(&view_data->logo_fade_anim, lv_anim_path_ease_out);
    lv_anim_start(&view_data->logo_fade_anim);
    
    view_data->animations_started = true;
    return 0;
}

/**
 * @brief 停止启动动画
 */
int splash_view_stop_animations(splash_view_data_t* view_data)
{
    if (!view_data) {
        return -1;
    }
    
    if (view_data->animations_started) {
        lv_anim_delete(view_data->logo_container, NULL);
        view_data->animations_started = false;
        printf("⏹️ 启动页面动画已停止\n");
    }
    
    return 0;
}

/**
 * @brief 获取启动页面数据
 */
splash_view_data_t* splash_view_get_data(void)
{
    return g_splash_data;
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 启动页面定时器回调
 */
static void splash_timer_callback(lv_timer_t* timer)
{
    printf("⏰ 启动页面定时器触发，准备跳转到主界面\n");
    
    if (g_splash_data) {
        // 开始淡出动画
        lv_anim_init(&g_splash_data->fadeout_anim);
        lv_anim_set_var(&g_splash_data->fadeout_anim, g_splash_data->container);
        lv_anim_set_exec_cb(&g_splash_data->fadeout_anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
        lv_anim_set_values(&g_splash_data->fadeout_anim, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&g_splash_data->fadeout_anim, SPLASH_FADE_DURATION);
        lv_anim_set_completed_cb(&g_splash_data->fadeout_anim, splash_fadeout_completed);
        lv_anim_start(&g_splash_data->fadeout_anim);
    }
    
    // 删除定时器
    lv_timer_delete(timer);
    if (g_splash_data) {
        g_splash_data->splash_timer = NULL;
    }
}

/**
 * @brief 启动页面淡出完成回调
 */
static void splash_fadeout_completed(lv_anim_t* anim)
{
    printf("🎬 启动页面淡出完成，跳转到主界面\n");
    
    // 发布视图切换事件
    ui_event_data_t event_data = {
        .view_id = VIEW_TYPE_MAIN,
        .visible = true
    };
    
    event_bus_publish(EVENT_UI_VIEW_CHANGED, &event_data, sizeof(event_data));
    
    // 隐藏启动页面
    splash_view_hide(NULL);
}
