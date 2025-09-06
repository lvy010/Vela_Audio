//
// Vela 音乐播放器 - 字体配置文件
// Created by Vela on 2025/1/27
// 支持UTF-8编码和中文显示的字体配置 - 使用MiSans字体
//

#ifndef FONT_CONFIG_H
#define FONT_CONFIG_H

#include "lvgl.h"

/*********************
 *      字体配置
 *********************/

// 字体编码支持
#define FONT_UTF8_SUPPORT 1
#define FONT_CHINESE_SUPPORT 1

// MiSans字体路径
#define MISANS_NORMAL_PATH "/data/res/fonts/MiSans-Normal.ttf"
#define MISANS_SEMIBOLD_PATH "/data/res/fonts/MiSans-Semibold.ttf"

// 使用系统默认字体（支持基本拉丁字符）+ 运行时字体渲染
#if LV_FONT_MONTSERRAT_16
    #define FONT_DEFAULT_16 &lv_font_montserrat_16
#else
    #define FONT_DEFAULT_16 &lv_font_default
#endif

#if LV_FONT_MONTSERRAT_20
    #define FONT_DEFAULT_20 &lv_font_montserrat_20
#else
    #define FONT_DEFAULT_20 FONT_DEFAULT_16
#endif

#if LV_FONT_MONTSERRAT_24
    #define FONT_DEFAULT_24 &lv_font_montserrat_24
#else
    #define FONT_DEFAULT_24 FONT_DEFAULT_20
#endif

#if LV_FONT_MONTSERRAT_28
    #define FONT_DEFAULT_28 &lv_font_montserrat_28
#else
    #define FONT_DEFAULT_28 FONT_DEFAULT_24
#endif

#if LV_FONT_MONTSERRAT_32
    #define FONT_DEFAULT_32 &lv_font_montserrat_32
#else
    #define FONT_DEFAULT_32 FONT_DEFAULT_28
#endif

// 中文字体支持 - 如果支持FreeType
#if LV_USE_FREETYPE
    extern lv_font_t* misans_font_16;
    extern lv_font_t* misans_font_20;
    extern lv_font_t* misans_font_24;
    extern lv_font_t* misans_font_28;
    extern lv_font_t* misans_font_32;
    
    #define CHINESE_FONT_16 misans_font_16
    #define CHINESE_FONT_20 misans_font_20
    #define CHINESE_FONT_24 misans_font_24
    #define CHINESE_FONT_28 misans_font_28
    #define CHINESE_FONT_32 misans_font_32
#else
    #define CHINESE_FONT_16 FONT_DEFAULT_16
    #define CHINESE_FONT_20 FONT_DEFAULT_20
    #define CHINESE_FONT_24 FONT_DEFAULT_24
    #define CHINESE_FONT_28 FONT_DEFAULT_28
    #define CHINESE_FONT_32 FONT_DEFAULT_32
#endif

/*********************
 *      字体函数
 *********************/

/**
 * @brief 获取适合的字体（基于大小）
 */
const lv_font_t* get_font_by_size(int size);

/**
 * @brief 检查文本是否包含中文字符
 */
bool text_contains_chinese(const char* text);

/**
 * @brief 设置标签的UTF-8文本
 */
void set_label_utf8_text(lv_obj_t* label, const char* text, const lv_font_t* font);

/**
 * @brief 初始化字体系统
 */
int font_system_init(void);

#endif // FONT_CONFIG_H
