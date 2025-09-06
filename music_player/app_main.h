//
// Vela 音乐播放器 - 应用程序主入口头文件
// Created by Vela Engineering Team on 2024/12/18
// 专业模块化架构的应用程序入口点定义
//

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdint.h>
#include <stdbool.h>

// 核心模块
#include "core/event_bus.h"
#include "core/state_manager.h"
#include "core/audio_engine.h"

// 服务层
#include "services/playback_controller.h"
#include "services/media_library.h"

// UI层
#include "ui/ui_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define APP_VERSION_MAJOR 2
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0
#define APP_NAME "Vela Music Player"
#define APP_VENDOR "Vela Audio Systems"

/*********************
 *      TYPEDEFS
 *********************/

// 应用程序状态
typedef enum {
    APP_STATE_INITIALIZING = 0,
    APP_STATE_LOADING_RESOURCES,
    APP_STATE_READY,
    APP_STATE_RUNNING,
    APP_STATE_SHUTTING_DOWN,
    APP_STATE_ERROR
} app_main_state_t;

// 应用程序配置
typedef struct {
    // 资源路径
    char resource_root[256];
    char config_file_path[256];
    char state_file_path[256];
    
    // 音频配置
    audio_engine_config_t audio_config;
    
    // UI配置
    bool enable_animations;
    uint32_t animation_duration_ms;
    theme_type_t default_theme;
    
    // 系统配置
    bool auto_save_state;
    uint32_t state_save_interval_ms;
    bool enable_wifi;
    bool enable_logging;
    
} app_config_t;

// 应用程序实例
typedef struct {
    app_main_state_t state;
    app_config_t config;
    
    // 核心模块实例
    audio_engine_t* audio_engine;
    ui_manager_t* ui_manager;
    playback_controller_t* playback_controller;
    media_library_t* media_library;
    
    // 事件监听器
    int event_listener_ids[16];
    int listener_count;
    
    // 主循环控制
    bool should_exit;
    pthread_t main_thread;
    
    // 统计信息
    uint32_t start_time;
    uint32_t total_runtime_ms;
    
} app_instance_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 初始化应用程序
 * @param config 应用程序配置
 * @return 应用程序实例指针，NULL表示失败
 */
app_instance_t* app_main_init(const app_config_t* config);

/**
 * @brief 启动应用程序主循环
 * @param app 应用程序实例
 * @return 0 成功, -1 失败
 */
int app_main_run(app_instance_t* app);

/**
 * @brief 停止应用程序
 * @param app 应用程序实例
 * @return 0 成功, -1 失败
 */
int app_main_stop(app_instance_t* app);

/**
 * @brief 清理应用程序
 * @param app 应用程序实例
 */
void app_main_cleanup(app_instance_t* app);

/**
 * @brief 获取应用程序版本字符串
 * @return 版本字符串
 */
const char* app_main_get_version(void);

/**
 * @brief 获取应用程序信息
 * @param name 应用名称（输出）
 * @param vendor 供应商名称（输出）
 * @param version 版本号（输出）
 * @return 0 成功, -1 失败
 */
int app_main_get_info(const char** name, const char** vendor, const char** version);

/**
 * @brief 加载默认配置
 * @param config 配置结构（输出）
 * @return 0 成功, -1 失败
 */
int app_main_load_default_config(app_config_t* config);

/**
 * @brief 从文件加载配置
 * @param config 配置结构（输出）
 * @param config_file 配置文件路径
 * @return 0 成功, -1 失败
 */
int app_main_load_config_from_file(app_config_t* config, const char* config_file);

/**
 * @brief 保存配置到文件
 * @param config 配置结构
 * @param config_file 配置文件路径
 * @return 0 成功, -1 失败
 */
int app_main_save_config_to_file(const app_config_t* config, const char* config_file);

/**
 * @brief 获取应用程序状态
 * @param app 应用程序实例
 * @return 应用程序状态
 */
app_main_state_t app_main_get_state(app_instance_t* app);

/**
 * @brief 处理系统信号
 * @param app 应用程序实例
 * @param signal 信号类型
 * @return 0 成功, -1 失败
 */
int app_main_handle_signal(app_instance_t* app, int signal);

/*********************
 * LIFECYCLE CALLBACKS
 *********************/

// 应用程序生命周期回调类型
typedef void (*app_lifecycle_cb_t)(app_instance_t* app, app_main_state_t old_state, 
                                  app_main_state_t new_state, void* user_data);

/**
 * @brief 设置生命周期回调
 * @param app 应用程序实例
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int app_main_set_lifecycle_callback(app_instance_t* app, app_lifecycle_cb_t callback, void* user_data);

/*********************
 * UTILITY FUNCTIONS
 *********************/

/**
 * @brief 检查应用程序依赖
 * @return 0 所有依赖满足, -1 缺少依赖
 */
int app_main_check_dependencies(void);

/**
 * @brief 初始化日志系统
 * @param log_level 日志级别
 * @param log_file 日志文件路径（NULL使用默认）
 * @return 0 成功, -1 失败
 */
int app_main_init_logging(int log_level, const char* log_file);

/**
 * @brief 获取应用程序运行时间
 * @param app 应用程序实例
 * @return 运行时间（毫秒）
 */
uint32_t app_main_get_runtime(app_instance_t* app);

/**
 * @brief 执行应用程序自检
 * @param app 应用程序实例
 * @return 0 成功, -1 失败
 */
int app_main_self_test(app_instance_t* app);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H
