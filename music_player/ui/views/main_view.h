//
// Vela 音乐播放器 - 主界面视图
// Created by Vela Engineering Team on 2024/12/18
// 专业主界面视图实现，解耦UI逻辑和业务逻辑
//

#ifndef MAIN_VIEW_H
#define MAIN_VIEW_H

#include <lvgl.h>
#include "../../core/state_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      DEFINES
 *********************/
#define MAIN_VIEW_ANIMATION_DURATION 300

/*********************
 *      TYPEDEFS
 *********************/

// 主视图数据结构
typedef struct {
    // UI组件
    lv_obj_t* container;
    lv_obj_t* status_bar;
    lv_obj_t* player_card;
    lv_obj_t* control_panel;
    
    // 状态栏组件
    lv_obj_t* time_label;
    lv_obj_t* date_label;
    lv_obj_t* wifi_label;
    lv_obj_t* battery_label;
    
    // 播放器组件
    lv_obj_t* album_cover_container;
    lv_obj_t* album_cover;
    lv_obj_t* track_name_label;
    lv_obj_t* artist_name_label;
    
    // 进度组件
    lv_obj_t* progress_bar;
    lv_obj_t* current_time_label;
    lv_obj_t* total_time_label;
    
    // 控制按钮
    lv_obj_t* playlist_btn;
    lv_obj_t* prev_btn;
    lv_obj_t* play_pause_btn;
    lv_obj_t* next_btn;
    lv_obj_t* volume_btn;
    
    // 音量控制
    lv_obj_t* volume_bar;
    lv_obj_t* volume_indicator;
    
    // 动画对象
    lv_anim_t cover_rotation_anim;
    lv_timer_t* progress_update_timer;
    lv_timer_t* time_update_timer;
    
    // 状态
    bool is_visible;
    bool cover_rotating;
    int16_t rotation_angle;
    
} main_view_data_t;

/*********************
 * GLOBAL PROTOTYPES
 *********************/

/**
 * @brief 创建主界面视图
 * @param parent 父容器
 * @param user_data 用户数据
 * @return 0 成功, -1 失败
 */
int main_view_create(lv_obj_t* parent, void* user_data);

/**
 * @brief 显示主界面视图
 * @param view_data 视图数据
 */
void main_view_show(void* view_data);

/**
 * @brief 隐藏主界面视图
 * @param view_data 视图数据
 */
void main_view_hide(void* view_data);

/**
 * @brief 更新主界面视图
 * @param view_data 视图数据
 * @param state 应用状态
 */
void main_view_update(void* view_data, const app_state_t* state);

/**
 * @brief 销毁主界面视图
 * @param view_data 视图数据
 */
void main_view_destroy(void* view_data);

/**
 * @brief 获取主视图数据
 * @return 主视图数据指针
 */
main_view_data_t* main_view_get_data(void);

/**
 * @brief 开始封面旋转动画
 * @param view_data 视图数据
 * @return 0 成功, -1 失败
 */
int main_view_start_cover_rotation(main_view_data_t* view_data);

/**
 * @brief 停止封面旋转动画
 * @param view_data 视图数据
 * @return 0 成功, -1 失败
 */
int main_view_stop_cover_rotation(main_view_data_t* view_data);

/**
 * @brief 更新播放进度显示
 * @param view_data 视图数据
 * @param current_ms 当前位置（毫秒）
 * @param total_ms 总时长（毫秒）
 * @return 0 成功, -1 失败
 */
int main_view_update_progress(main_view_data_t* view_data, uint32_t current_ms, uint32_t total_ms);

/**
 * @brief 更新音轨信息显示
 * @param view_data 视图数据
 * @param track 音轨信息
 * @return 0 成功, -1 失败
 */
int main_view_update_track_info(main_view_data_t* view_data, const track_info_t* track);

/**
 * @brief 更新音量显示
 * @param view_data 视图数据
 * @param volume 音量值 (0-100)
 * @param visible 音量条是否可见
 * @return 0 成功, -1 失败
 */
int main_view_update_volume(main_view_data_t* view_data, uint16_t volume, bool visible);

/**
 * @brief 更新播放按钮状态
 * @param view_data 视图数据
 * @param is_playing 是否正在播放
 * @return 0 成功, -1 失败
 */
int main_view_update_play_button(main_view_data_t* view_data, bool is_playing);

/**
 * @brief 更新状态栏信息
 * @param view_data 视图数据
 * @param wifi_connected Wi-Fi连接状态
 * @param wifi_ssid Wi-Fi网络名称
 * @return 0 成功, -1 失败
 */
int main_view_update_status_bar(main_view_data_t* view_data, bool wifi_connected, const char* wifi_ssid);

#ifdef __cplusplus
}
#endif

#endif // MAIN_VIEW_H
