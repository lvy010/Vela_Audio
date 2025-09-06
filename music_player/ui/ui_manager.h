//
// Vela 音乐播放器 - UI管理器
// Created by Vela Engineering Team on 2024/12/18
// 统一UI管理，实现视图切换和主题控制
//

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>
#include "../core/state_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define MAX_VIEWS 16
#define MAX_THEMES 8

/*********************
 *      TYPEDEFS
 *********************/

// 视图类型枚举
typedef enum {
    VIEW_TYPE_SPLASH = 0,
    VIEW_TYPE_MAIN,
    VIEW_TYPE_PLAYLIST,
    VIEW_TYPE_SETTINGS,
    VIEW_TYPE_EQUALIZER,
    VIEW_TYPE_ABOUT
} view_type_t;

// 主题类型枚举
typedef enum {
    THEME_DARK = 0,
    THEME_LIGHT,
    THEME_AUTO
} theme_type_t;

// 视图接口
typedef struct view_interface {
    view_type_t type;
    const char* name;
    
    // 视图生命周期函数
    int (*create)(lv_obj_t* parent, void* user_data);
    void (*show)(void* view_data);
    void (*hide)(void* view_data);
    void (*update)(void* view_data, const app_state_t* state);
    void (*destroy)(void* view_data);
    
    // 视图数据
    void* view_data;
    lv_obj_t* container;
    bool is_visible;
    bool is_created;
} view_interface_t;

// 主题配置
typedef struct {
    theme_type_t type;
    const char* name;
    
    // 颜色定义
    lv_color_t primary_color;
    lv_color_t secondary_color;
    lv_color_t background_color;
    lv_color_t surface_color;
    lv_color_t text_primary_color;
    lv_color_t text_secondary_color;
    lv_color_t accent_color;
    lv_color_t error_color;
    
    // 字体定义
    const lv_font_t* font_small;
    const lv_font_t* font_normal;
    const lv_font_t* font_large;
    const lv_font_t* font_title;
    
    // 样式定义
    lv_style_t button_style;
    lv_style_t card_style;
    lv_style_t progress_style;
} theme_config_t;

// UI管理器实例
typedef struct {
    // 视图管理
    view_interface_t views[MAX_VIEWS];
    int view_count;
    view_type_t current_view;
    view_type_t previous_view;
    
    // 主题管理
    theme_config_t themes[MAX_THEMES];
    int theme_count;
    theme_type_t current_theme;
    
    // 根容器
    lv_obj_t* root_container;
    lv_obj_t* overlay_container;
    
    // 事件监听器
    int event_listener_ids[8];
    int listener_count;
    
    // 状态
    bool initialized;
    bool animations_enabled;
    uint32_t animation_duration_ms;
    
    // 线程同步
    pthread_mutex_t ui_mutex;
    
} ui_manager_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 初始化UI管理器
 * @return UI管理器实例指针，NULL表示失败
 */
ui_manager_t* ui_manager_init(void);

/**
 * @brief 清理UI管理器
 * @param manager UI管理器实例
 */
void ui_manager_cleanup(ui_manager_t* manager);

/**
 * @brief 注册视图
 * @param manager UI管理器实例
 * @param view_interface 视图接口
 * @return 0 成功, -1 失败
 */
int ui_manager_register_view(ui_manager_t* manager, const view_interface_t* view_interface);

/**
 * @brief 切换到指定视图
 * @param manager UI管理器实例
 * @param view_type 视图类型
 * @param user_data 传递给视图的数据
 * @return 0 成功, -1 失败
 */
int ui_manager_switch_to_view(ui_manager_t* manager, view_type_t view_type, void* user_data);

/**
 * @brief 显示覆盖层视图
 * @param manager UI管理器实例
 * @param view_type 视图类型
 * @param user_data 传递给视图的数据
 * @return 0 成功, -1 失败
 */
int ui_manager_show_overlay(ui_manager_t* manager, view_type_t view_type, void* user_data);

/**
 * @brief 隐藏覆盖层视图
 * @param manager UI管理器实例
 * @param view_type 视图类型
 * @return 0 成功, -1 失败
 */
int ui_manager_hide_overlay(ui_manager_t* manager, view_type_t view_type);

/**
 * @brief 返回上一个视图
 * @param manager UI管理器实例
 * @return 0 成功, -1 失败
 */
int ui_manager_go_back(ui_manager_t* manager);

/**
 * @brief 注册主题
 * @param manager UI管理器实例
 * @param theme 主题配置
 * @return 0 成功, -1 失败
 */
int ui_manager_register_theme(ui_manager_t* manager, const theme_config_t* theme);

/**
 * @brief 切换主题
 * @param manager UI管理器实例
 * @param theme_type 主题类型
 * @return 0 成功, -1 失败
 */
int ui_manager_switch_theme(ui_manager_t* manager, theme_type_t theme_type);

/**
 * @brief 获取当前主题
 * @param manager UI管理器实例
 * @return 当前主题配置指针
 */
const theme_config_t* ui_manager_get_current_theme(ui_manager_t* manager);

/**
 * @brief 更新所有视图
 * @param manager UI管理器实例
 * @param state 应用状态
 * @return 0 成功, -1 失败
 */
int ui_manager_update_views(ui_manager_t* manager, const app_state_t* state);

/**
 * @brief 启用/禁用动画
 * @param manager UI管理器实例
 * @param enabled 是否启用
 * @param duration_ms 动画时长
 * @return 0 成功, -1 失败
 */
int ui_manager_set_animations(ui_manager_t* manager, bool enabled, uint32_t duration_ms);

/**
 * @brief 获取当前视图类型
 * @param manager UI管理器实例
 * @return 当前视图类型
 */
view_type_t ui_manager_get_current_view(ui_manager_t* manager);

/**
 * @brief 检查视图是否可见
 * @param manager UI管理器实例
 * @param view_type 视图类型
 * @return true 可见, false 不可见
 */
bool ui_manager_is_view_visible(ui_manager_t* manager, view_type_t view_type);

/**
 * @brief 处理UI事件
 * @param manager UI管理器实例
 * @note 应在主循环中定期调用
 */
void ui_manager_process_events(ui_manager_t* manager);

/**
 * @brief 创建通用样式
 * @param theme 主题配置
 * @return 0 成功, -1 失败
 */
int ui_manager_create_common_styles(const theme_config_t* theme);

/**
 * @brief 应用主题到对象
 * @param obj LVGL对象
 * @param theme 主题配置
 * @param style_type 样式类型
 * @return 0 成功, -1 失败
 */
int ui_manager_apply_theme_to_object(lv_obj_t* obj, const theme_config_t* theme, const char* style_type);

/*********************
 * UTILITY FUNCTIONS
 *********************/

/**
 * @brief 创建带动画的视图切换
 * @param from_view 源视图容器
 * @param to_view 目标视图容器
 * @param duration_ms 动画时长
 * @param completed_cb 完成回调
 * @return 0 成功, -1 失败
 */
int ui_manager_animate_view_transition(lv_obj_t* from_view, lv_obj_t* to_view, 
                                      uint32_t duration_ms, lv_anim_completed_cb_t completed_cb);

/**
 * @brief 创建加载指示器
 * @param parent 父容器
 * @param text 加载文本
 * @return 加载指示器对象
 */
lv_obj_t* ui_manager_create_loading_indicator(lv_obj_t* parent, const char* text);

/**
 * @brief 显示错误消息
 * @param manager UI管理器实例
 * @param title 错误标题
 * @param message 错误消息
 * @param duration_ms 显示时长
 * @return 0 成功, -1 失败
 */
int ui_manager_show_error_message(ui_manager_t* manager, const char* title, 
                                 const char* message, uint32_t duration_ms);

/**
 * @brief 显示成功消息
 * @param manager UI管理器实例
 * @param message 消息内容
 * @param duration_ms 显示时长
 * @return 0 成功, -1 失败
 */
int ui_manager_show_success_message(ui_manager_t* manager, const char* message, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H
