//
// Vela 音乐播放器 - 新模块化主入口点
// Created by Vela Engineering Team on 2024/12/18
// 基于专业架构设计的全新入口点，实现清晰的模块初始化流程
//

// 系统头文件
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// LVGL和UV循环
#include <lvgl/lvgl.h>
#include <uv.h>

// 应用程序模块
#include "app_main.h"

/*********************
 *  STATIC PROTOTYPES
 *********************/
static void lv_nuttx_uv_loop(uv_loop_t* loop, lv_nuttx_result_t* result);
static int setup_lvgl_environment(lv_nuttx_result_t* result);
static void cleanup_lvgl_environment(lv_nuttx_result_t* result);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 应用程序主入口点
 */
int main(int argc, char* argv[])
{
    printf("🎵 Vela Music Player v%s 启动中...\n", app_main_get_version());
    
    // 检查依赖
    if (app_main_check_dependencies() != 0) {
        printf("❌ 依赖检查失败\n");
        return 1;
    }
    
    // 初始化LVGL环境
    lv_nuttx_result_t lvgl_result;
    if (setup_lvgl_environment(&lvgl_result) != 0) {
        printf("❌ LVGL环境初始化失败\n");
        return 1;
    }
    
    // 加载应用程序配置
    app_config_t config;
    if (app_main_load_default_config(&config) != 0) {
        printf("❌ 配置加载失败\n");
        cleanup_lvgl_environment(&lvgl_result);
        return 1;
    }
    
    // 如果存在配置文件，尝试加载
    if (access(config.config_file_path, F_OK) == 0) {
        app_main_load_config_from_file(&config, config.config_file_path);
        printf("📋 从文件加载配置：%s\n", config.config_file_path);
    }
    
    // 初始化应用程序
    app_instance_t* app = app_main_init(&config);
    if (!app) {
        printf("❌ 应用程序初始化失败\n");
        cleanup_lvgl_environment(&lvgl_result);
        return 1;
    }
    
    // 执行自检
    if (app_main_self_test(app) != 0) {
        printf("⚠️ 应用程序自检发现问题，但继续运行\n");
    }
    
    // 启动UV事件循环（在单独线程中）
    pthread_t uv_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    struct uv_thread_data {
        uv_loop_t loop;
        lv_nuttx_result_t* result;
    } uv_data = { .result = &lvgl_result };
    
    pthread_create(&uv_thread, &attr, (void*(*)(void*))lv_nuttx_uv_loop, &uv_data);
    pthread_attr_destroy(&attr);
    
    // 运行应用程序主循环
    int app_result = app_main_run(app);
    
    // 清理应用程序
    app_main_cleanup(app);
    
    // 清理LVGL环境
    cleanup_lvgl_environment(&lvgl_result);
    
    if (app_result == 0) {
        printf("✅ Vela Music Player 正常退出\n");
    } else {
        printf("❌ Vela Music Player 异常退出：%d\n", app_result);
    }
    
    return app_result;
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief LVGL UV事件循环
 */
static void lv_nuttx_uv_loop(uv_loop_t* loop, lv_nuttx_result_t* result)
{
    lv_nuttx_uv_t uv_info;
    void* data;

    uv_loop_init(loop);

    memset(&uv_info, 0, sizeof(uv_info));
    uv_info.loop = loop;
    uv_info.disp = result->disp;
    uv_info.indev = result->indev;
    
#ifdef CONFIG_UINPUT_TOUCH
    uv_info.uindev = result->utouch_indev;
#endif

    data = lv_nuttx_uv_init(&uv_info);
    uv_run(loop, UV_RUN_DEFAULT);
    lv_nuttx_uv_deinit(&data);
}

/**
 * @brief 设置LVGL环境
 */
static int setup_lvgl_environment(lv_nuttx_result_t* result)
{
    printf("🎨 初始化LVGL图形环境...\n");
    
    // 检查LVGL是否已初始化
    if (lv_is_initialized()) {
        printf("❌ LVGL已经初始化，退出\n");
        return -1;
    }
    
    // 初始化LVGL
    lv_init();
    
    // 初始化NuttX显示驱动
    lv_nuttx_dsc_t info;
    lv_nuttx_dsc_init(&info);
    lv_nuttx_init(&info, result);
    
    if (!result->disp) {
        printf("❌ LVGL显示驱动初始化失败\n");
        lv_deinit();
        return -1;
    }
    
    printf("✅ LVGL环境初始化成功\n");
    return 0;
}

/**
 * @brief 清理LVGL环境
 */
static void cleanup_lvgl_environment(lv_nuttx_result_t* result)
{
    printf("🧹 清理LVGL环境...\n");
    
    if (result) {
        lv_nuttx_deinit(result);
    }
    
    if (lv_is_initialized()) {
        lv_deinit();
    }
    
    printf("✅ LVGL环境清理完成\n");
}
