//
// Vela 音乐播放器 - 专业播放列表管理器头文件
// Created by Vela on 2025/8/20
// 播放列表管理器接口定义和函数声明
//

#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include "lvgl.h"
#include <stdbool.h>

/*********************
 *      TYPEDEFS
 *********************/

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 创建专业播放列表界面
 * @param parent 父容器
 */
void playlist_manager_create(lv_obj_t* parent);

/**
 * @brief 刷新播放列表显示
 */
void playlist_manager_refresh(void);

/**
 * @brief 关闭播放列表
 */
void playlist_manager_close(void);

/**
 * @brief 播放列表是否打开
 * @return true if open, false if closed
 */
bool playlist_manager_is_open(void);

#endif // PLAYLIST_MANAGER_H
