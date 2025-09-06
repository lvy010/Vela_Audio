//
// Vela 音乐播放器 - 字体配置实现
// Created by Vela on 2025/1/27
// 支持UTF-8编码和中文显示的字体配置实现
//

#include "font_config.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// FreeType字体实例
#if LV_USE_FREETYPE
lv_font_t* misans_font_16 = NULL;
lv_font_t* misans_font_20 = NULL;
lv_font_t* misans_font_24 = NULL;
lv_font_t* misans_font_28 = NULL;
lv_font_t* misans_font_32 = NULL;

// FreeType字体支持暂时禁用，使用LVGL内置字体
// static lv_ft_info_t misans_info_16;
// static lv_ft_info_t misans_info_20;
// static lv_ft_info_t misans_info_24;
// static lv_ft_info_t misans_info_28;
// static lv_ft_info_t misans_info_32;
#endif

// 简化版字体支持标志
static bool font_system_initialized = false;

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 获取适合的字体（基于大小）- 优先使用中文字体
 */
const lv_font_t* get_font_by_size(int size)
{
#if LV_USE_FREETYPE
    // 如果支持FreeType，优先使用MiSans中文字体
    if (size >= 32 && CHINESE_FONT_32) {
        return CHINESE_FONT_32;
    } else if (size >= 28 && CHINESE_FONT_28) {
        return CHINESE_FONT_28;
    } else if (size >= 24 && CHINESE_FONT_24) {
        return CHINESE_FONT_24;
    } else if (size >= 20 && CHINESE_FONT_20) {
        return CHINESE_FONT_20;
    } else if (CHINESE_FONT_16) {
        return CHINESE_FONT_16;
    }
#endif
    
    // 回退到系统默认字体
    if (size >= 32) {
        return FONT_DEFAULT_32;
    } else if (size >= 28) {
        return FONT_DEFAULT_28;
    } else if (size >= 24) {
        return FONT_DEFAULT_24;
    } else if (size >= 20) {
        return FONT_DEFAULT_20;
    } else {
        return FONT_DEFAULT_16;
    }
}

/**
 * @brief 检查文本是否包含中文字符
 */
bool text_contains_chinese(const char* text)
{
    if (!text) return false;
    
    const unsigned char* p = (const unsigned char*)text;
    while (*p) {
        // 检查UTF-8编码的中文字符范围
        if (*p >= 0xE4 && *p <= 0xE9) {
            // 可能是中文字符
            if (*(p+1) >= 0x80 && *(p+1) <= 0xBF && 
                *(p+2) >= 0x80 && *(p+2) <= 0xBF) {
                return true;
            }
        }
        p++;
    }
    return false;
}

/**
 * @brief 设置标签的UTF-8文本
 */
void set_label_utf8_text(lv_obj_t* label, const char* text, const lv_font_t* font)
{
    if (!label || !text) return;
    
    // 设置字体
    if (font) {
        lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    }
    
    // 设置文本 - LVGL会自动处理UTF-8编码
    lv_label_set_text(label, text);
    
    // 如果包含中文，可能需要特殊处理
    if (text_contains_chinese(text)) {
        // 对于中文文本，可能需要调整一些显示属性
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    }
}

/**
 * @brief 初始化字体系统
 */
int font_system_init(void)
{
    printf("🔤 字体系统初始化...\n");
    
    // 检查可用字体
    printf("📝 可用字体检查:\n");
    
#if LV_FONT_MONTSERRAT_16
    printf("  ✅ Montserrat 16px\n");
#endif
#if LV_FONT_MONTSERRAT_20
    printf("  ✅ Montserrat 20px\n");
#endif
#if LV_FONT_MONTSERRAT_24
    printf("  ✅ Montserrat 24px\n");
#endif
#if LV_FONT_MONTSERRAT_28
    printf("  ✅ Montserrat 28px\n");
#endif
#if LV_FONT_MONTSERRAT_32
    printf("  ✅ Montserrat 32px\n");
#endif

#if LV_USE_FREETYPE
    printf("🇨🇳 检查MiSans中文字体支持...\n");
    
    // 检查字体文件是否存在
    if (access(MISANS_NORMAL_PATH, F_OK) == 0) {
        printf("  ✅ 找到MiSans-Normal.ttf\n");
        // TODO: 在后续版本中启用FreeType字体加载
        printf("  📋 字体文件已就绪，暂时使用默认字体\n");
    } else {
        printf("  ⚠️ 未找到MiSans字体文件: %s\n", MISANS_NORMAL_PATH);
    }
#else
    printf("  📋 FreeType未启用，使用LVGL内置字体\n");
#endif

    // 测试UTF-8支持
    const char* test_text = "测试中文显示";
    if (text_contains_chinese(test_text)) {
        printf("  ✅ UTF-8中文字符检测正常\n");
    }
    
    font_system_initialized = true;
    printf("✅ 字体系统初始化完成\n");
    return 0;
}
