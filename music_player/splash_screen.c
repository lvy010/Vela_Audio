//
// Vela 音乐播放器启动页面
// Created by Vela on 2025/8/12
// 实现启动动画、品牌展示和过渡效果
//

#include "music_player.h"

/*********************
 *      DEFINES
 *********************/
#define SPLASH_DURATION 2000  // 启动页显示2秒

/*********************
 *  STATIC VARIABLES
 *********************/
static lv_obj_t* splash_screen;
static lv_timer_t* splash_timer;

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void splash_timer_cb(lv_timer_t* timer);
static void logo_fade_in_anim_cb(void* obj, int32_t value);
static void vinyl_rotation_anim_cb(void* obj, int32_t value);
static void splash_fadeout_complete_cb(lv_anim_t* anim);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 创建启动页面
 */
void splash_screen_create(void)
{
    // 创建启动页面容器
    splash_screen = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(splash_screen);
    lv_obj_set_size(splash_screen, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(splash_screen, lv_color_hex(0x0A0A0A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(splash_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(splash_screen);
    lv_obj_set_flex_flow(splash_screen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(splash_screen, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 品牌Logo区域
    lv_obj_t* logo_container = lv_obj_create(splash_screen);
    lv_obj_remove_style_all(logo_container);
    lv_obj_set_size(logo_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(logo_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(logo_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // 唱片旋转动画容器
    lv_obj_t* vinyl_container = lv_obj_create(logo_container);
    lv_obj_remove_style_all(vinyl_container);
    lv_obj_set_size(vinyl_container, 120, 120);
    lv_obj_set_style_radius(vinyl_container, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(vinyl_container, lv_color_hex(0x1A1A1A), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(vinyl_container, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(vinyl_container, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(vinyl_container, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(vinyl_container, 15, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(vinyl_container, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(vinyl_container, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(vinyl_container, 20, LV_PART_MAIN);
    
    // 设置旋转中心点
    lv_obj_set_style_transform_pivot_x(vinyl_container, 60, 0);
    lv_obj_set_style_transform_pivot_y(vinyl_container, 60, 0);

    // 唱片中心小圆点
    lv_obj_t* vinyl_center = lv_obj_create(vinyl_container);
    lv_obj_remove_style_all(vinyl_center);
    lv_obj_set_size(vinyl_center, 20, 20);
    lv_obj_set_style_radius(vinyl_center, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(vinyl_center, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(vinyl_center, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_center(vinyl_center);

    // 音乐图标
    lv_obj_t* music_icon = lv_label_create(vinyl_center);
    lv_label_set_text(music_icon, "V");  // 使用Vela的首字母
    lv_obj_set_style_text_font(music_icon, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(music_icon, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(music_icon);

    // 品牌标题
    lv_obj_t* brand_title = lv_label_create(logo_container);
    lv_label_set_text(brand_title, "Vela Audio");
    lv_obj_set_style_text_font(brand_title, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(brand_title, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(brand_title, 8, LV_PART_MAIN);

    // 标语
    lv_obj_t* tagline = lv_label_create(logo_container);
    lv_label_set_text(tagline, "Music Connects Souls");
    lv_obj_set_style_text_font(tagline, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(tagline, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(tagline, 40, LV_PART_MAIN);

    // 加载进度指示器（使用简单的旋转圆圈代替spinner）
    lv_obj_t* loading_circle = lv_obj_create(splash_screen);
    lv_obj_remove_style_all(loading_circle);
    lv_obj_set_size(loading_circle, 40, 40);
    lv_obj_set_style_radius(loading_circle, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(loading_circle, 3, LV_PART_MAIN);
    lv_obj_set_style_border_color(loading_circle, lv_color_hex(0x3B82F6), LV_PART_MAIN);
    lv_obj_set_style_border_opa(loading_circle, LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(loading_circle, LV_OPA_TRANSP, LV_PART_MAIN);

    // 启动Logo淡入动画
    lv_anim_t logo_anim;
    lv_anim_init(&logo_anim);
    lv_anim_set_var(&logo_anim, logo_container);
    lv_anim_set_exec_cb(&logo_anim, logo_fade_in_anim_cb);
    lv_anim_set_values(&logo_anim, 0, 255);  // 透明度从0到255
    lv_anim_set_duration(&logo_anim, 800);   // 800ms淡入
    lv_anim_set_path_cb(&logo_anim, lv_anim_path_ease_out);
    lv_anim_start(&logo_anim);

    // 唱片旋转动画
    lv_anim_t vinyl_anim;
    lv_anim_init(&vinyl_anim);
    lv_anim_set_var(&vinyl_anim, vinyl_container);
    lv_anim_set_exec_cb(&vinyl_anim, vinyl_rotation_anim_cb);
    lv_anim_set_values(&vinyl_anim, 0, 3600);  // 旋转一圈
    lv_anim_set_duration(&vinyl_anim, 3000);   // 3秒一圈
    lv_anim_set_repeat_count(&vinyl_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&vinyl_anim);

    // 启动页定时器 - 2秒后跳转主界面
    splash_timer = lv_timer_create(splash_timer_cb, SPLASH_DURATION, NULL);
    lv_timer_set_repeat_count(splash_timer, 1);
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief Logo淡入动画回调
 */
static void logo_fade_in_anim_cb(void* obj, int32_t value)
{
    lv_obj_t* logo = (lv_obj_t*)obj;
    lv_obj_set_style_opa(logo, value, LV_PART_MAIN);
}

/**
 * @brief 唱片旋转动画回调
 */
static void vinyl_rotation_anim_cb(void* obj, int32_t value)
{
    lv_obj_t* vinyl = (lv_obj_t*)obj;
    lv_obj_set_style_transform_rotation(vinyl, value, 0);
}

/**
 * @brief 启动页定时器回调 - 跳转到主界面
 */
static void splash_timer_cb(lv_timer_t* timer)
{
    LV_UNUSED(timer);
    
    // 淡出动画
    lv_anim_t fadeout_anim;
    lv_anim_init(&fadeout_anim);
    lv_anim_set_var(&fadeout_anim, splash_screen);
    lv_anim_set_exec_cb(&fadeout_anim, logo_fade_in_anim_cb);
    lv_anim_set_values(&fadeout_anim, 255, 0);  // 透明度从255到0
    lv_anim_set_duration(&fadeout_anim, 500);   // 500ms淡出
    lv_anim_set_path_cb(&fadeout_anim, lv_anim_path_ease_in);
    
    // 淡出完成后删除启动页并创建主界面
    lv_anim_set_completed_cb(&fadeout_anim, splash_fadeout_complete_cb);
    
    lv_anim_start(&fadeout_anim);
    
    // 清理定时器
    lv_timer_delete(splash_timer);
    splash_timer = NULL;
}

/**
 * @brief 启动页淡出完成回调 - 跳转到主界面
 */
static void splash_fadeout_complete_cb(lv_anim_t* anim)
{
    LV_UNUSED(anim);
    lv_obj_delete(splash_screen);
    app_create();  // 创建主应用界面
}
