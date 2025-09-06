//
// Vela 音乐播放器 - 专业模块化应用程序入口
// Created by Vela Engineering Team on 2024/12/18
// 实现清晰的模块初始化、依赖注入和生命周期管理
//

#include "app_main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

// LVGL
#include <lvgl.h>

// 视图实现
#include "ui/views/splash_view.h"
#include "ui/views/main_view.h"
#include "ui/views/playlist_view.h"

/*********************
 *  STATIC VARIABLES
 *********************/
static app_instance_t* g_app_instance = NULL;

/*********************
 *  STATIC PROTOTYPES
 *********************/
static int initialize_core_modules(app_instance_t* app);
static int initialize_service_modules(app_instance_t* app);
static int initialize_ui_modules(app_instance_t* app);
static int register_event_listeners(app_instance_t* app);
static void* main_loop_thread(void* arg);
static void app_event_handler(const event_t* event, void* user_data);
static void signal_handler(int signal);
static int load_media_library(app_instance_t* app);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 初始化应用程序
 */
app_instance_t* app_main_init(const app_config_t* config)
{
    printf("🚀 Vela Music Player v%d.%d.%d 正在启动...\n", 
           APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    
    // 分配应用程序实例
    app_instance_t* app = (app_instance_t*)malloc(sizeof(app_instance_t));
    if (!app) {
        printf("❌ 应用程序内存分配失败\n");
        return NULL;
    }
    
    memset(app, 0, sizeof(app_instance_t));
    app->state = APP_STATE_INITIALIZING;
    app->start_time = (uint32_t)time(NULL);
    
    // 复制配置
    if (config) {
        memcpy(&app->config, config, sizeof(app_config_t));
    } else {
        app_main_load_default_config(&app->config);
    }
    
    // 设置信号处理器
    g_app_instance = app;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 初始化核心模块
    if (initialize_core_modules(app) != 0) {
        printf("❌ 核心模块初始化失败\n");
        app_main_cleanup(app);
        return NULL;
    }
    
    // 初始化服务模块
    if (initialize_service_modules(app) != 0) {
        printf("❌ 服务模块初始化失败\n");
        app_main_cleanup(app);
        return NULL;
    }
    
    // 初始化UI模块
    if (initialize_ui_modules(app) != 0) {
        printf("❌ UI模块初始化失败\n");
        app_main_cleanup(app);
        return NULL;
    }
    
    // 注册事件监听器
    if (register_event_listeners(app) != 0) {
        printf("❌ 事件监听器注册失败\n");
        app_main_cleanup(app);
        return NULL;
    }
    
    // 加载媒体库
    if (load_media_library(app) != 0) {
        printf("⚠️ 媒体库加载失败，使用空库\n");
    }
    
    app->state = APP_STATE_READY;
    printf("✅ Vela Music Player 初始化完成\n");
    
    return app;
}

/**
 * @brief 启动应用程序主循环
 */
int app_main_run(app_instance_t* app)
{
    if (!app || app->state != APP_STATE_READY) {
        return -1;
    }
    
    app->state = APP_STATE_RUNNING;
    printf("🎵 Vela Music Player 开始运行...\n");
    
    // 显示启动页面
    ui_manager_switch_to_view(app->ui_manager, VIEW_TYPE_SPLASH, NULL);
    
    // 创建主循环线程
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    
    int result = pthread_create(&app->main_thread, &attr, main_loop_thread, app);
    pthread_attr_destroy(&attr);
    
    if (result != 0) {
        printf("❌ 主循环线程创建失败\n");
        return -1;
    }
    
    pthread_setname_np(app->main_thread, "vela_music_main");
    
    // 等待线程完成
    pthread_join(app->main_thread, NULL);
    
    printf("🛑 Vela Music Player 已停止\n");
    return 0;
}

/**
 * @brief 停止应用程序
 */
int app_main_stop(app_instance_t* app)
{
    if (!app) {
        return -1;
    }
    
    printf("🛑 正在停止 Vela Music Player...\n");
    
    app->state = APP_STATE_SHUTTING_DOWN;
    app->should_exit = true;
    
    // 停止播放
    if (app->playback_controller) {
        playback_controller_stop(app->playback_controller);
    }
    
    // 保存状态
    if (app->config.auto_save_state) {
        state_manager_save_to_file(app->config.state_file_path);
    }
    
    return 0;
}

/**
 * @brief 清理应用程序
 */
void app_main_cleanup(app_instance_t* app)
{
    if (!app) {
        return;
    }
    
    printf("🧹 正在清理 Vela Music Player...\n");
    
    // 注销事件监听器
    for (int i = 0; i < app->listener_count; i++) {
        event_bus_unregister_listener(app->event_listener_ids[i]);
    }
    
    // 清理模块（按依赖顺序逆序清理）
    if (app->ui_manager) {
        ui_manager_cleanup(app->ui_manager);
    }
    
    if (app->playback_controller) {
        playback_controller_cleanup(app->playback_controller);
    }
    
    if (app->media_library) {
        media_library_cleanup(app->media_library);
    }
    
    if (app->audio_engine) {
        audio_engine_cleanup(app->audio_engine);
    }
    
    // 清理核心系统
    state_manager_cleanup();
    event_bus_cleanup();
    
    // 释放应用程序实例
    free(app);
    g_app_instance = NULL;
    
    printf("✅ Vela Music Player 清理完成\n");
}

/**
 * @brief 获取应用程序版本字符串
 */
const char* app_main_get_version(void)
{
    static char version_str[32];
    snprintf(version_str, sizeof(version_str), "%d.%d.%d", 
             APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    return version_str;
}

/**
 * @brief 加载默认配置
 */
int app_main_load_default_config(app_config_t* config)
{
    if (!config) {
        return -1;
    }
    
    memset(config, 0, sizeof(app_config_t));
    
    // 设置默认路径
    strcpy(config->resource_root, "/data/res");
    strcpy(config->config_file_path, "/data/config.json");
    strcpy(config->state_file_path, "/data/app_state.bin");
    
    // 音频配置
    config->audio_config.sample_rate = 44100;
    config->audio_config.channels = 2;
    config->audio_config.bits_per_sample = 16;
    config->audio_config.buffer_size = AUDIO_BUFFER_SIZE;
    config->audio_config.auto_crossfade = false;
    config->audio_config.crossfade_duration_ms = CROSSFADE_DURATION_MS;
    
    // UI配置
    config->enable_animations = true;
    config->animation_duration_ms = 300;
    config->default_theme = THEME_DARK;
    
    // 系统配置
    config->auto_save_state = true;
    config->state_save_interval_ms = 30000; // 30秒保存一次
    config->enable_wifi = true;
    config->enable_logging = true;
    
    printf("📋 默认配置已加载\n");
    return 0;
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 初始化核心模块
 */
static int initialize_core_modules(app_instance_t* app)
{
    printf("🔧 初始化核心模块...\n");
    
    // 初始化事件总线
    if (event_bus_init() != 0) {
        printf("❌ 事件总线初始化失败\n");
        return -1;
    }
    
    // 初始化状态管理器
    if (state_manager_init() != 0) {
        printf("❌ 状态管理器初始化失败\n");
        return -1;
    }
    
    // 初始化音频引擎
    app->audio_engine = audio_engine_init(&app->config.audio_config);
    if (!app->audio_engine) {
        printf("❌ 音频引擎初始化失败\n");
        return -1;
    }
    
    printf("✅ 核心模块初始化完成\n");
    return 0;
}

/**
 * @brief 初始化服务模块
 */
static int initialize_service_modules(app_instance_t* app)
{
    printf("🔧 初始化服务模块...\n");
    
    // 初始化媒体库
    app->media_library = media_library_init();
    if (!app->media_library) {
        printf("❌ 媒体库初始化失败\n");
        return -1;
    }
    
    // 初始化播放控制器
    app->playback_controller = playback_controller_init(app->audio_engine);
    if (!app->playback_controller) {
        printf("❌ 播放控制器初始化失败\n");
        return -1;
    }
    
    printf("✅ 服务模块初始化完成\n");
    return 0;
}

/**
 * @brief 初始化UI模块
 */
static int initialize_ui_modules(app_instance_t* app)
{
    printf("🔧 初始化UI模块...\n");
    
    // 初始化UI管理器
    app->ui_manager = ui_manager_init();
    if (!app->ui_manager) {
        printf("❌ UI管理器初始化失败\n");
        return -1;
    }
    
    // 注册视图
    view_interface_t splash_view = {
        .type = VIEW_TYPE_SPLASH,
        .name = "启动页面",
        .create = splash_view_create,
        .show = splash_view_show,
        .hide = splash_view_hide,
        .destroy = splash_view_destroy
    };
    ui_manager_register_view(app->ui_manager, &splash_view);
    
    view_interface_t main_view = {
        .type = VIEW_TYPE_MAIN,
        .name = "主界面",
        .create = main_view_create,
        .show = main_view_show,
        .hide = main_view_hide,
        .update = main_view_update,
        .destroy = main_view_destroy
    };
    ui_manager_register_view(app->ui_manager, &main_view);
    
    view_interface_t playlist_view = {
        .type = VIEW_TYPE_PLAYLIST,
        .name = "播放列表",
        .create = playlist_view_create,
        .show = playlist_view_show,
        .hide = playlist_view_hide,
        .update = playlist_view_update,
        .destroy = playlist_view_destroy
    };
    ui_manager_register_view(app->ui_manager, &playlist_view);
    
    printf("✅ UI模块初始化完成\n");
    return 0;
}

/**
 * @brief 注册事件监听器
 */
static int register_event_listeners(app_instance_t* app)
{
    printf("📡 注册事件监听器...\n");
    
    // 注册主应用事件监听器
    int listener_id = event_bus_register_listener(EVENT_AUDIO_PLAY_START, app_event_handler, app);
    if (listener_id >= 0) {
        app->event_listener_ids[app->listener_count++] = listener_id;
    }
    
    listener_id = event_bus_register_listener(EVENT_AUDIO_PLAY_PAUSE, app_event_handler, app);
    if (listener_id >= 0) {
        app->event_listener_ids[app->listener_count++] = listener_id;
    }
    
    listener_id = event_bus_register_listener(EVENT_AUDIO_PLAY_STOP, app_event_handler, app);
    if (listener_id >= 0) {
        app->event_listener_ids[app->listener_count++] = listener_id;
    }
    
    listener_id = event_bus_register_listener(EVENT_PLAYLIST_TRACK_SELECTED, app_event_handler, app);
    if (listener_id >= 0) {
        app->event_listener_ids[app->listener_count++] = listener_id;
    }
    
    printf("✅ 注册了 %d 个事件监听器\n", app->listener_count);
    return 0;
}

/**
 * @brief 主循环线程
 */
static void* main_loop_thread(void* arg)
{
    app_instance_t* app = (app_instance_t*)arg;
    
    printf("🔄 主循环线程启动\n");
    
    while (!app->should_exit) {
        // 处理事件总线
        event_bus_process_events();
        
        // 处理UI事件
        ui_manager_process_events(app->ui_manager);
        
        // 更新UI状态
        const app_state_t* state = state_manager_get_state();
        ui_manager_update_views(app->ui_manager, state);
        
        // 处理LVGL任务
        lv_timer_handler();
        
        // 短暂休眠避免CPU占用过高
        usleep(10000); // 10ms
    }
    
    printf("🔄 主循环线程结束\n");
    return NULL;
}

/**
 * @brief 应用程序事件处理器
 */
static void app_event_handler(const event_t* event, void* user_data)
{
    app_instance_t* app = (app_instance_t*)user_data;
    if (!app || !event) {
        return;
    }
    
    switch (event->type) {
        case EVENT_AUDIO_PLAY_START:
            printf("🎵 播放开始事件\n");
            break;
            
        case EVENT_AUDIO_PLAY_PAUSE:
            printf("⏸️ 播放暂停事件\n");
            break;
            
        case EVENT_AUDIO_PLAY_STOP:
            printf("⏹️ 播放停止事件\n");
            break;
            
        case EVENT_PLAYLIST_TRACK_SELECTED: {
            const playlist_event_data_t* data = (const playlist_event_data_t*)event->data;
            printf("🎵 选择音轨：%s (索引: %u)\n", data->track_name, data->track_index);
            break;
        }
        
        case EVENT_SYSTEM_LOW_MEMORY:
            printf("⚠️ 系统内存不足警告\n");
            // 可以在这里实现内存清理逻辑
            break;
            
        default:
            break;
    }
}

/**
 * @brief 信号处理器
 */
static void signal_handler(int signal)
{
    printf("📡 收到信号 %d，准备退出...\n", signal);
    
    if (g_app_instance) {
        app_main_stop(g_app_instance);
    }
}

/**
 * @brief 加载媒体库
 */
static int load_media_library(app_instance_t* app)
{
    printf("📚 加载媒体库...\n");
    
    // 添加扫描路径
    char music_path[512];
    snprintf(music_path, sizeof(music_path), "%s/musics", app->config.resource_root);
    
    if (media_library_add_scan_path(app->media_library, music_path) != 0) {
        printf("❌ 无法添加音乐扫描路径：%s\n", music_path);
        return -1;
    }
    
    // 开始扫描
    if (media_library_start_scan(app->media_library, false) != 0) {
        printf("❌ 媒体库扫描启动失败\n");
        return -1;
    }
    
    printf("✅ 媒体库扫描已启动\n");
    return 0;
}

/**
 * @brief 获取应用程序信息
 */
int app_main_get_info(const char** name, const char** vendor, const char** version)
{
    if (name) *name = APP_NAME;
    if (vendor) *vendor = APP_VENDOR;
    if (version) *version = app_main_get_version();
    return 0;
}

/**
 * @brief 检查应用程序依赖
 */
int app_main_check_dependencies(void)
{
    printf("🔍 检查应用程序依赖...\n");
    
    // 检查LVGL
    if (!lv_is_initialized()) {
        printf("❌ LVGL未初始化\n");
        return -1;
    }
    
    // 检查必要的系统调用
    if (access("/dev/audio", F_OK) != 0) {
        printf("⚠️ 音频设备不可用\n");
    }
    
    printf("✅ 依赖检查完成\n");
    return 0;
}

/**
 * @brief 获取应用程序运行时间
 */
uint32_t app_main_get_runtime(app_instance_t* app)
{
    if (!app) {
        return 0;
    }
    
    uint32_t current_time = (uint32_t)time(NULL);
    return (current_time - app->start_time) * 1000; // 转换为毫秒
}

/**
 * @brief 执行应用程序自检
 */
int app_main_self_test(app_instance_t* app)
{
    if (!app) {
        return -1;
    }
    
    printf("🔧 执行应用程序自检...\n");
    
    // 检查模块状态
    if (!app->audio_engine || !app->ui_manager || 
        !app->playback_controller || !app->media_library) {
        printf("❌ 关键模块未初始化\n");
        return -1;
    }
    
    // 检查状态管理器
    const app_state_t* state = state_manager_get_state();
    if (!state->initialized) {
        printf("❌ 状态管理器未正确初始化\n");
        return -1;
    }
    
    // 检查音频引擎
    audio_engine_state_t audio_state = audio_engine_get_state(app->audio_engine);
    if (audio_state == AUDIO_ENGINE_STATE_ERROR) {
        printf("❌ 音频引擎处于错误状态\n");
        return -1;
    }
    
    printf("✅ 应用程序自检通过\n");
    return 0;
}

/**
 * @brief 获取应用程序状态
 */
app_main_state_t app_main_get_state(app_instance_t* app)
{
    return app ? app->state : APP_STATE_ERROR;
}
