//
// Vela 音乐播放器 - 启动页面视图
// Created by Vela Engineering Team on 2024/12/18
// 专业启动页面视图，独立模块化设计
//

#ifndef SPLASH_VIEW_H
#define SPLASH_VIEW_H

#include <lvgl.h>
#include "../../core/state_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define SPLASH_DISPLAY_DURATION 2000  // 显示2秒
#define SPLASH_FADE_DURATION 500      // 淡出500ms

/*********************
 *      TYPEDEFS
 *********************/

// 启动页面数据结构
typedef struct {
    // UI组件
    lv_obj_t* container;
    lv_obj_t* logo_container;
    lv_obj_t* vinyl_container;
    lv_obj_t* brand_title;
    lv_obj_t* tagline;
    lv_obj_t* loading_indicator;
    
    // 动画对象
    lv_anim_t logo_fade_anim;
    lv_anim_t vinyl_rotation_anim;
    lv_anim_t fadeout_anim;
    
    // 定时器
    lv_timer_t* splash_timer;
    
    // 状态
    bool is_visible;
    bool animations_started;
    
} splash_view_data_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 创建启动页面视图
 * @param parent 父容器
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int splash_view_create(lv_obj_t* parent, void* user_data);

/**
 * @brief 显示启动页面视图
 * @param view_data 视图数据
 */
void splash_view_show(void* view_data);

/**
 * @brief 隐藏启动页面视图
 * @param view_data 视图数据
 */
void splash_view_hide(void* view_data);

/**
 * @brief 销毁启动页面视图
 * @param view_data 视图数据
 */
void splash_view_destroy(void* view_data);

/**
 * @brief 获取启动页面数据
 * @return 启动页面数据指针
 */
splash_view_data_t* splash_view_get_data(void);

/**
 * @brief 开始启动动画
 * @param view_data 视图数据
 * @return 0 成功, -1 失败
 */
int splash_view_start_animations(splash_view_data_t* view_data);

/**
 * @brief 停止启动动画
 * @param view_data 视图数据
 * @return 0 成功, -1 失败
 */
int splash_view_stop_animations(splash_view_data_t* view_data);

/**
 * @brief 设置启动完成回调
 * @param callback 完成回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int splash_view_set_completed_callback(lv_anim_completed_cb_t callback, void* user_data);

#ifdef __cplusplus
}
#endif

#endif // SPLASH_VIEW_H
