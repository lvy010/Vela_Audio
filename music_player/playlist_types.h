//
// Vela 音乐播放器 - 播放列表类型定义
// Created by Vela on 2025/8/25
// 定义多播放列表系统的数据结构和常量
//

#ifndef PLAYLIST_TYPES_H
#define PLAYLIST_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/
#define MAX_PLAYLISTS 10           // 最大播放列表数量
#define MAX_SONGS_PER_PLAYLIST 500 // 每个列表最大歌曲数
#define PLAYLIST_NAME_MAX_LEN 64   // 列表名称最大长度
#define PLAYLIST_DESC_MAX_LEN 128  // 列表描述最大长度

/*********************
 *      TYPEDEFS
 *********************/

// 播放模式枚举
typedef enum {
    PLAY_MODE_SEQUENTIAL = 0,   // 顺序播放
    PLAY_MODE_SHUFFLE,          // 随机播放
    PLAY_MODE_REPEAT_ONE,       // 单曲循环
    PLAY_MODE_REPEAT_ALL        // 列表循环
} play_mode_t;

// 播放列表类型
typedef enum {
    PLAYLIST_TYPE_DEFAULT = 0,  // 默认列表
    PLAYLIST_TYPE_FAVORITE,     // 喜欢列表
    PLAYLIST_TYPE_SEARCH,       // 搜索结果
    PLAYLIST_TYPE_RECENT,       // 最近播放
    PLAYLIST_TYPE_CUSTOM        // 自定义列表
} playlist_type_t;

// 歌曲信息结构（扩展版）
typedef struct {
    char path[256];             // 文件路径
    char name[128];             // 歌曲名称
    char artist[128];           // 艺术家
    char album[128];            // 专辑名称
    char cover[256];            // 封面图片路径
    uint32_t total_time;        // 总时长(ms)
    char color[16];             // 主题色
    uint32_t play_count;        // 播放次数
    uint32_t last_played;       // 上次播放时间
    bool is_favorite;           // 是否收藏
    uint32_t file_size;         // 文件大小
    int sample_rate;            // 采样率
    int bitrate;               // 比特率
} song_info_t;

// 播放列表结构
typedef struct {
    char name[PLAYLIST_NAME_MAX_LEN];           // 列表名称
    char description[PLAYLIST_DESC_MAX_LEN];    // 列表描述
    playlist_type_t type;                       // 列表类型
    song_info_t* songs;                         // 歌曲数组
    int song_count;                             // 歌曲数量
    int max_capacity;                           // 最大容量
    int current_song_index;                     // 当前播放索引
    bool is_active;                             // 是否为活动列表
    uint32_t created_time;                      // 创建时间
    uint32_t modified_time;                     // 修改时间
    char icon[32];                              // 列表图标
    char color[16];                             // 主题色
} playlist_t;

// 播放列表管理器
typedef struct {
    playlist_t* playlists[MAX_PLAYLISTS];       // 播放列表数组
    int playlist_count;                         // 列表数量
    int current_playlist_id;                    // 当前活动列表ID
    int favorite_playlist_id;                   // 喜欢列表ID
    int search_playlist_id;                     // 搜索列表ID
    
    // 播放控制
    play_mode_t play_mode;                      // 播放模式
    bool shuffle_enabled;                       // 随机播放
    bool repeat_enabled;                        // 循环播放
    int* shuffle_order;                         // 随机播放顺序
    
    // 拖拽状态
    bool is_dragging;                           // 是否正在拖拽
    int drag_source_index;                      // 拖拽源索引
    int drag_target_index;                      // 拖拽目标索引
    lv_obj_t* drag_placeholder;                 // 拖拽占位符
    
    // 统计信息
    uint32_t total_play_time;                   // 总播放时间
    uint32_t total_songs_played;                // 总播放歌曲数
} playlist_manager_t;

/*********************
 *  GLOBAL VARIABLES
 *********************/
extern playlist_manager_t g_playlist_manager;

/*********************
 * FUNCTION PROTOTYPES
 *********************/

// 播放列表基础操作
playlist_t* playlist_create(const char* name, playlist_type_t type);
int playlist_add_song(playlist_t* playlist, song_info_t* song);
int playlist_remove_song(playlist_t* playlist, int index);
int playlist_move_song(playlist_t* playlist, int from_index, int to_index);
void playlist_destroy(playlist_t* playlist);
int playlist_find_song(playlist_t* playlist, const char* name);

// 多列表管理
int playlist_manager_init(void);
int playlist_manager_add_list(playlist_t* playlist);
int playlist_manager_remove_list(int playlist_id);
int playlist_manager_switch_list(int playlist_id);
playlist_t* playlist_manager_get_current(void);
playlist_t* playlist_manager_get_by_id(int playlist_id);

// 喜欢列表专用
int favorite_list_init(void);
int favorite_list_add_song(song_info_t* song);
int favorite_list_remove_song(int song_index);
bool favorite_list_contains_song(const char* song_path);
playlist_t* favorite_list_get(void);

// 播放模式控制
void playlist_set_play_mode(play_mode_t mode);
play_mode_t playlist_get_play_mode(void);
int playlist_get_next_song_index(void);
int playlist_get_previous_song_index(void);
void playlist_shuffle_generate_order(void);

// 拖拽功能
int playlist_start_drag(int source_index);
int playlist_update_drag_target(int target_index);
int playlist_finish_drag(void);
void playlist_cancel_drag(void);

// 数据持久化
int playlist_save_to_file(playlist_t* playlist, const char* filepath);
playlist_t* playlist_load_from_file(const char* filepath);
int playlist_manager_save_state(void);
int playlist_manager_load_state(void);

#endif // PLAYLIST_TYPES_H

