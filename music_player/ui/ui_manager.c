//
// Vela 音乐播放器 - UI管理器实现（简化版）
// Created by Vela Engineering Team on 2024/12/18
// 提供基础UI管理功能，支持视图切换和主题管理
//

#include "ui_manager.h"
#include "../core/event_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void ui_event_listener(const event_t* event, void* user_data);
static int find_view_by_type(ui_manager_t* manager, view_type_t type);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 初始化UI管理器
 */
ui_manager_t* ui_manager_init(void)
{
    printf("🎨 初始化UI管理器...\n");
    
    ui_manager_t* manager = (ui_manager_t*)malloc(sizeof(ui_manager_t));
    if (!manager) {
        printf("❌ UI管理器内存分配失败\n");
        return NULL;
    }
    
    memset(manager, 0, sizeof(ui_manager_t));
    
    // 初始化状态
    manager->current_view = VIEW_TYPE_SPLASH;
    manager->current_theme = THEME_DARK;
    manager->animations_enabled = true;
    manager->animation_duration_ms = 300;
    
    // 初始化互斥锁
    if (pthread_mutex_init(&manager->ui_mutex, NULL) != 0) {
        printf("❌ UI管理器互斥锁初始化失败\n");
        free(manager);
        return NULL;
    }
    
    // 创建根容器
    manager->root_container = lv_screen_active();
    
    // 创建覆盖层容器
    manager->overlay_container = lv_layer_top();
    
    // 注册事件监听器
    int listener_id = event_bus_register_listener(EVENT_UI_VIEW_CHANGED, ui_event_listener, manager);
    if (listener_id >= 0) {
        manager->event_listener_ids[manager->listener_count++] = listener_id;
    }
    
    manager->initialized = true;
    
    printf("✅ UI管理器初始化成功\n");
    return manager;
}

/**
 * @brief 清理UI管理器
 */
void ui_manager_cleanup(ui_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    printf("🧹 清理UI管理器...\n");
    
    // 注销事件监听器
    for (int i = 0; i < manager->listener_count; i++) {
        event_bus_unregister_listener(manager->event_listener_ids[i]);
    }
    
    // 清理所有视图
    for (int i = 0; i < manager->view_count; i++) {
        if (manager->views[i].is_created && manager->views[i].destroy) {
            manager->views[i].destroy(manager->views[i].view_data);
        }
    }
    
    // 清理互斥锁
    pthread_mutex_destroy(&manager->ui_mutex);
    
    // 释放实例
    free(manager);
    
    printf("✅ UI管理器清理完成\n");
}

/**
 * @brief 注册视图
 */
int ui_manager_register_view(ui_manager_t* manager, const view_interface_t* view_interface)
{
    if (!manager || !view_interface || manager->view_count >= MAX_VIEWS) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->ui_mutex);
    
    // 检查是否已注册相同类型的视图
    int existing_index = find_view_by_type(manager, view_interface->type);
    if (existing_index >= 0) {
        printf("⚠️ 视图类型 %d 已存在，替换为新视图\n", view_interface->type);
        manager->views[existing_index] = *view_interface;
        pthread_mutex_unlock(&manager->ui_mutex);
        return existing_index;
    }
    
    // 注册新视图
    manager->views[manager->view_count] = *view_interface;
    int index = manager->view_count++;
    
    pthread_mutex_unlock(&manager->ui_mutex);
    
    printf("📝 注册UI视图：%s (类型: %d)\n", view_interface->name, view_interface->type);
    return index;
}

/**
 * @brief 切换到指定视图
 */
int ui_manager_switch_to_view(ui_manager_t* manager, view_type_t view_type, void* user_data)
{
    if (!manager) {
        return -1;
    }
    
    printf("🔄 切换视图：%d\n", view_type);
    
    pthread_mutex_lock(&manager->ui_mutex);
    
    int view_index = find_view_by_type(manager, view_type);
    if (view_index < 0) {
        printf("❌ 未找到视图类型：%d\n", view_type);
        pthread_mutex_unlock(&manager->ui_mutex);
        return -1;
    }
    
    view_interface_t* view = &manager->views[view_index];
    
    // 隐藏当前视图
    if (manager->current_view != view_type) {
        int current_index = find_view_by_type(manager, manager->current_view);
        if (current_index >= 0 && manager->views[current_index].is_visible) {
            if (manager->views[current_index].hide) {
                manager->views[current_index].hide(manager->views[current_index].view_data);
            }
            manager->views[current_index].is_visible = false;
        }
    }
    
    // 创建视图（如果未创建）
    if (!view->is_created && view->create) {
        if (view->create(manager->root_container, user_data) == 0) {
            view->is_created = true;
        } else {
            printf("❌ 视图创建失败：%d\n", view_type);
            pthread_mutex_unlock(&manager->ui_mutex);
            return -1;
        }
    }
    
    // 显示视图
    if (view->show) {
        view->show(view->view_data);
    }
    view->is_visible = true;
    
    // 更新当前视图
    manager->previous_view = manager->current_view;
    manager->current_view = view_type;
    
    pthread_mutex_unlock(&manager->ui_mutex);
    
    printf("✅ 视图切换成功：%d\n", view_type);
    return 0;
}

/**
 * @brief 更新所有视图
 */
int ui_manager_update_views(ui_manager_t* manager, const app_state_t* state)
{
    if (!manager || !state) {
        return -1;
    }
    
    pthread_mutex_lock(&manager->ui_mutex);
    
    // 更新所有可见视图
    for (int i = 0; i < manager->view_count; i++) {
        if (manager->views[i].is_visible && manager->views[i].update) {
            manager->views[i].update(manager->views[i].view_data, state);
        }
    }
    
    pthread_mutex_unlock(&manager->ui_mutex);
    
    return 0;
}

/**
 * @brief 处理UI事件
 */
void ui_manager_process_events(ui_manager_t* manager)
{
    if (!manager) {
        return;
    }
    
    // 处理LVGL定时器任务
    lv_timer_handler();
}

/**
 * @brief 获取当前视图类型
 */
view_type_t ui_manager_get_current_view(ui_manager_t* manager)
{
    if (!manager) {
        return VIEW_TYPE_SPLASH;
    }
    
    return manager->current_view;
}

/**
 * @brief 显示覆盖层视图
 */
int ui_manager_show_overlay(ui_manager_t* manager, view_type_t view_type, void* user_data)
{
    if (!manager) {
        return -1;
    }
    
    printf("📋 显示覆盖层视图：%d\n", view_type);
    
    // 简化实现：直接切换视图
    return ui_manager_switch_to_view(manager, view_type, user_data);
}

/**
 * @brief 隐藏覆盖层视图
 */
int ui_manager_hide_overlay(ui_manager_t* manager, view_type_t view_type)
{
    if (!manager) {
        return -1;
    }
    
    printf("📋 隐藏覆盖层视图：%d\n", view_type);
    
    // 简化实现：返回到主视图
    return ui_manager_switch_to_view(manager, VIEW_TYPE_MAIN, NULL);
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief UI事件监听器
 */
static void ui_event_listener(const event_t* event, void* user_data)
{
    ui_manager_t* manager = (ui_manager_t*)user_data;
    if (!manager || !event) {
        return;
    }
    
    switch (event->type) {
        case EVENT_UI_VIEW_CHANGED:
            printf("🔄 UI视图变化事件\n");
            break;
        default:
            break;
    }
}

/**
 * @brief 根据类型查找视图
 */
static int find_view_by_type(ui_manager_t* manager, view_type_t type)
{
    for (int i = 0; i < manager->view_count; i++) {
        if (manager->views[i].type == type) {
            return i;
        }
    }
    return -1;
}
