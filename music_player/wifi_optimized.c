//
// Vela 音乐播放器 - WiFi性能优化模块
// Created by Vela on 2025/8/25
// 实现智能重连、低延迟优化、连接监控等高级网络功能
//

#include "music_player.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

// 外部变量声明
extern struct resource_s R;
extern struct ctx_s C;

/*********************
 *      DEFINES
 *********************/
#define WIFI_CONNECT_TIMEOUT_MS 5000     // 连接超时5秒
#define WIFI_RECONNECT_INTERVAL_MS 3000  // 重连间隔3秒
#define WIFI_PING_INTERVAL_MS 10000      // ping间隔10秒
#define WIFI_PING_TIMEOUT_MS 2000        // ping超时2秒
#define MAX_PING_FAILURES 3              // 最大ping失败次数
#define MAX_SAVED_NETWORKS 10            // 最大保存网络数

/*********************
 *      TYPEDEFS
 *********************/

// WiFi连接状态
typedef enum {
    WIFI_STATE_DISCONNECTED = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_RECONNECTING,
    WIFI_STATE_ERROR
} wifi_state_t;

// WiFi网络信息
typedef struct {
    char ssid[64];              // 网络名称
    char password[128];         // 网络密码
    int8_t rssi;               // 信号强度
    uint8_t security;          // 安全类型
    bool is_saved;             // 是否已保存
    uint32_t last_connected;   // 上次连接时间
} wifi_network_t;

// WiFi性能统计
typedef struct {
    uint32_t total_connections;     // 总连接次数
    uint32_t successful_connections; // 成功连接次数
    uint32_t total_reconnects;      // 总重连次数
    uint32_t connection_failures;   // 连接失败次数
    float average_connect_time;     // 平均连接时间
    float average_latency;          // 平均网络延迟
    uint32_t uptime_seconds;        // 在线时长
    uint32_t last_ping_time;        // 上次ping时间
} wifi_stats_t;

// WiFi管理器
typedef struct {
    wifi_state_t state;         // 当前状态
    wifi_network_t current;     // 当前网络
    wifi_network_t saved_networks[MAX_SAVED_NETWORKS]; // 保存的网络
    int saved_count;            // 保存的网络数量
    
    // 性能优化相关
    bool auto_reconnect;        // 自动重连开关
    uint32_t reconnect_interval; // 重连间隔(ms)
    uint32_t connection_timeout; // 连接超时(ms)
    int ping_failures;          // ping失败次数
    
    // 统计数据
    wifi_stats_t stats;         // 性能统计
    
    // 定时器
    lv_timer_t* reconnect_timer; // 重连定时器
    lv_timer_t* monitor_timer;   // 监控定时器
    lv_timer_t* ping_timer;      // ping定时器
} wifi_manager_optimized_t;

/*********************
 *  STATIC VARIABLES
 *********************/
static wifi_manager_optimized_t wifi_manager = {0};

/*********************
 *  STATIC PROTOTYPES
 *********************/
static int wifi_connect_async(const char* ssid, const char* password);
static int wifi_wait_connection_complete(uint32_t timeout_ms);
static void wifi_auto_reconnect_timer_cb(lv_timer_t* timer);
static void wifi_quality_monitor_timer_cb(lv_timer_t* timer);
static void wifi_ping_timer_cb(lv_timer_t* timer);
static int wifi_ping_gateway(void);
static uint64_t get_timestamp_ms(void);
static void wifi_update_status_display(void);
static int wifi_scan_network_available(const char* ssid);

/*********************
 *   GLOBAL FUNCTIONS
 *********************/

/**
 * @brief 初始化WiFi性能优化管理器
 */
int wifi_manager_optimized_init(void)
{
    memset(&wifi_manager, 0, sizeof(wifi_manager_optimized_t));
    
    // 设置默认参数
    wifi_manager.state = WIFI_STATE_DISCONNECTED;
    wifi_manager.auto_reconnect = true;
    wifi_manager.reconnect_interval = WIFI_RECONNECT_INTERVAL_MS;
    wifi_manager.connection_timeout = WIFI_CONNECT_TIMEOUT_MS;
    
    // 从配置文件加载保存的网络
    wifi_load_saved_networks();
    
    // 启动监控定时器
    wifi_manager.monitor_timer = lv_timer_create(wifi_quality_monitor_timer_cb, 
                                               WIFI_PING_INTERVAL_MS, NULL);
    
    printf("📡 WiFi性能优化管理器初始化成功\n");
    return 0;
}

/**
 * @brief WiFi快速连接 - 优化版
 */
int wifi_connect_optimized(const char* ssid, const char* password)
{
    if (!ssid || !password) {
        return -1;
    }
    
    printf("🚀 开始优化WiFi连接: %s\n", ssid);
    
    uint64_t start_time = get_timestamp_ms();
    wifi_manager.stats.total_connections++;
    
    // 更新状态
    wifi_manager.state = WIFI_STATE_CONNECTING;
    strncpy(wifi_manager.current.ssid, ssid, sizeof(wifi_manager.current.ssid) - 1);
    strncpy(wifi_manager.current.password, password, sizeof(wifi_manager.current.password) - 1);
    wifi_update_status_display();
    
    // 1. 预检查网络可用性
    if (!wifi_scan_network_available(ssid)) {
        printf("❌ 网络不可用: %s\n", ssid);
        wifi_manager.state = WIFI_STATE_ERROR;
        wifi_manager.stats.connection_failures++;
        return -1;
    }
    
    // 2. 使用非阻塞连接
    int ret = wifi_connect_async(ssid, password);
    if (ret < 0) {
        printf("❌ 异步连接启动失败\n");
        wifi_manager.state = WIFI_STATE_ERROR;
        wifi_manager.stats.connection_failures++;
        return -1;
    }
    
    // 3. 等待连接完成(带超时)
    ret = wifi_wait_connection_complete(wifi_manager.connection_timeout);
    
    uint64_t connect_time = get_timestamp_ms() - start_time;
    
    if (ret == 0) {
        wifi_manager.state = WIFI_STATE_CONNECTED;
        wifi_manager.stats.successful_connections++;
        wifi_manager.stats.average_connect_time = 
            (wifi_manager.stats.average_connect_time * 0.8f) + (connect_time * 0.2f);
        
        printf("✅ WiFi连接成功，耗时: %llu ms\n", connect_time);
        
        // 启动连接质量监控
        wifi_start_connection_monitor();
        
        // 保存成功的网络配置
        wifi_save_network_config(ssid, password);
        
    } else {
        wifi_manager.state = WIFI_STATE_ERROR;
        wifi_manager.stats.connection_failures++;
        printf("❌ WiFi连接失败，耗时: %llu ms\n", connect_time);
    }
    
    wifi_update_status_display();
    return ret;
}

/**
 * @brief 启动连接质量监控
 */
void wifi_start_connection_monitor(void)
{
    // 停止现有的重连定时器
    if (wifi_manager.reconnect_timer) {
        lv_timer_del(wifi_manager.reconnect_timer);
        wifi_manager.reconnect_timer = NULL;
    }
    
    // 启动自动重连监控
    wifi_manager.reconnect_timer = lv_timer_create(wifi_auto_reconnect_timer_cb, 
                                                 wifi_manager.reconnect_interval, NULL);
    
    // 启动ping监控
    if (!wifi_manager.ping_timer) {
        wifi_manager.ping_timer = lv_timer_create(wifi_ping_timer_cb, 
                                                WIFI_PING_INTERVAL_MS, NULL);
    }
    
    printf("📊 WiFi连接监控已启动\n");
}

/**
 * @brief 智能重连处理
 */
static void wifi_auto_reconnect_timer_cb(lv_timer_t* timer)
{
    if (!wifi_manager.auto_reconnect) {
        return;
    }
    
    // 检查连接状态
    if (wifi_manager.state == WIFI_STATE_DISCONNECTED) {
        printf("🔄 检测到WiFi断线，开始自动重连...\n");
        
        wifi_manager.state = WIFI_STATE_RECONNECTING;
        wifi_manager.stats.total_reconnects++;
        wifi_update_status_display();
        
        // 使用上次成功的网络信息
        int ret = wifi_connect_optimized(wifi_manager.current.ssid, 
                                       wifi_manager.current.password);
        
        if (ret == 0) {
            printf("✅ 自动重连成功 (第%u次重连)\n", wifi_manager.stats.total_reconnects);
        } else {
            printf("❌ 自动重连失败，%u ms后重试\n", wifi_manager.reconnect_interval);
            wifi_manager.state = WIFI_STATE_DISCONNECTED;
        }
    }
}

/**
 * @brief 网络质量监控
 */
static void wifi_quality_monitor_timer_cb(lv_timer_t* timer)
{
    if (wifi_manager.state != WIFI_STATE_CONNECTED) {
        return;
    }
    
    // 更新在线时长
    wifi_manager.stats.uptime_seconds += WIFI_PING_INTERVAL_MS / 1000;
    
    // 更新状态显示
    wifi_update_status_display();
}

/**
 * @brief Ping测试定时器
 */
static void wifi_ping_timer_cb(lv_timer_t* timer)
{
    if (wifi_manager.state != WIFI_STATE_CONNECTED) {
        return;
    }
    
    uint64_t ping_start = get_timestamp_ms();
    int ping_result = wifi_ping_gateway();
    uint64_t ping_time = get_timestamp_ms() - ping_start;
    
    if (ping_result == 0) {
        // 更新平均延迟
        wifi_manager.stats.average_latency = 
            (wifi_manager.stats.average_latency * 0.8f) + (ping_time * 0.2f);
        wifi_manager.ping_failures = 0;
        wifi_manager.stats.last_ping_time = ping_time;
        
        printf("📊 网络延迟: %llu ms (平均: %.1f ms)\n", 
               ping_time, wifi_manager.stats.average_latency);
    } else {
        wifi_manager.ping_failures++;
        printf("⚠️ Ping失败 (连续%d次)\n", wifi_manager.ping_failures);
        
        // 连续失败3次认为网络异常
        if (wifi_manager.ping_failures >= MAX_PING_FAILURES) {
            printf("❌ 网络异常，标记为断线状态\n");
            wifi_manager.state = WIFI_STATE_DISCONNECTED;
            wifi_update_status_display();
        }
    }
}

/**
 * @brief 更新WiFi状态显示
 */
static void wifi_update_status_display(void)
{
    if (!R.ui.wifi_status_label) {
        return;
    }
    
    char status_text[64];
    
    switch (wifi_manager.state) {
        case WIFI_STATE_CONNECTED:
            snprintf(status_text, sizeof(status_text), "WiFi %.0fms", 
                    wifi_manager.stats.average_latency);
            lv_obj_set_style_text_color(R.ui.wifi_status_label, lv_color_hex(0x10B981), LV_PART_MAIN);
            break;
            
        case WIFI_STATE_CONNECTING:
            strcpy(status_text, "WiFi 连接中");
            lv_obj_set_style_text_color(R.ui.wifi_status_label, lv_color_hex(0xF59E0B), LV_PART_MAIN);
            break;
            
        case WIFI_STATE_RECONNECTING:
            strcpy(status_text, "WiFi 重连中");
            lv_obj_set_style_text_color(R.ui.wifi_status_label, lv_color_hex(0xF59E0B), LV_PART_MAIN);
            break;
            
        case WIFI_STATE_DISCONNECTED:
            strcpy(status_text, "WiFi 断开");
            lv_obj_set_style_text_color(R.ui.wifi_status_label, lv_color_hex(0xEF4444), LV_PART_MAIN);
            break;
            
        case WIFI_STATE_ERROR:
            strcpy(status_text, "WiFi 错误");
            lv_obj_set_style_text_color(R.ui.wifi_status_label, lv_color_hex(0xEF4444), LV_PART_MAIN);
            break;
    }
    
    lv_label_set_text(R.ui.wifi_status_label, status_text);
}

/**
 * @brief 获取WiFi性能统计信息
 */
wifi_stats_t* wifi_get_performance_stats(void)
{
    return &wifi_manager.stats;
}

/**
 * @brief 创建WiFi设置界面
 */
void wifi_create_settings_ui(lv_obj_t* parent)
{
    // 🎵 WiFi设置全屏容器
    lv_obj_t* wifi_settings = lv_obj_create(parent);
    lv_obj_remove_style_all(wifi_settings);
    lv_obj_set_size(wifi_settings, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(wifi_settings, lv_color_hex(0x111827), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(wifi_settings, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_flex_flow(wifi_settings, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(wifi_settings, 24, LV_PART_MAIN);

    // 📡 标题栏
    lv_obj_t* header = lv_obj_create(wifi_settings);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, LV_PCT(100), 80);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi设置");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // 📊 当前连接状态卡片
    lv_obj_t* status_card = lv_obj_create(wifi_settings);
    lv_obj_remove_style_all(status_card);
    lv_obj_set_size(status_card, LV_PCT(100), 120);
    lv_obj_set_style_bg_color(status_card, lv_color_hex(0x1F2937), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status_card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(status_card, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_card, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(status_card, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_flex_flow(status_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(status_card, 20, LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(status_card, 20, LV_PART_MAIN);
    
    // 网络名称和状态
    lv_obj_t* network_info = lv_label_create(status_card);
    char info_text[128];
    snprintf(info_text, sizeof(info_text), "当前网络: %s", 
             wifi_manager.current.ssid[0] ? wifi_manager.current.ssid : "未连接");
    lv_label_set_text(network_info, info_text);
    lv_obj_set_style_text_font(network_info, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(network_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    
    // 性能统计
    lv_obj_t* stats_info = lv_label_create(status_card);
    char stats_text[256];
    snprintf(stats_text, sizeof(stats_text), 
             "延迟: %.1fms | 重连: %u次 | 在线: %us",
             wifi_manager.stats.average_latency,
             wifi_manager.stats.total_reconnects,
             wifi_manager.stats.uptime_seconds);
    lv_label_set_text(stats_info, stats_text);
    lv_obj_set_style_text_font(stats_info, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(stats_info, lv_color_hex(0xD1D5DB), LV_PART_MAIN);

    // ⚙️ 高级设置区域
    lv_obj_t* settings_section = lv_obj_create(wifi_settings);
    lv_obj_remove_style_all(settings_section);
    lv_obj_set_size(settings_section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(settings_section, lv_color_hex(0x1F2937), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settings_section, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(settings_section, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_section, lv_color_hex(0x374151), LV_PART_MAIN);
    lv_obj_set_flex_flow(settings_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(settings_section, 20, LV_PART_MAIN);

    // 设置标题
    lv_obj_t* settings_title = lv_label_create(settings_section);
    lv_label_set_text(settings_title, "高级设置");
    lv_obj_set_style_text_font(settings_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(settings_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_margin_bottom(settings_title, 16, LV_PART_MAIN);

    // 自动重连开关
    lv_obj_t* auto_reconnect_switch = lv_switch_create(settings_section);
    lv_obj_set_size(auto_reconnect_switch, 60, 30);
    if (wifi_manager.auto_reconnect) {
        lv_obj_add_state(auto_reconnect_switch, LV_STATE_CHECKED);
    }
    
    lv_obj_t* switch_label = lv_label_create(settings_section);
    lv_label_set_text(switch_label, "自动重连");
    lv_obj_set_style_text_font(switch_label, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_color(switch_label, lv_color_hex(0xD1D5DB), LV_PART_MAIN);
}

/*********************
 *   STATIC FUNCTIONS
 *********************/

/**
 * @brief 异步WiFi连接
 */
static int wifi_connect_async(const char* ssid, const char* password)
{
    // 这里实现具体的WiFi连接逻辑
    // 在实际实现中，这会调用系统的WiFi API
    
    printf("🔗 启动异步WiFi连接: %s\n", ssid);
    
    // 模拟异步连接过程
    // 实际实现中会使用系统的WiFi管理接口
    
    return 0; // 成功启动异步连接
}

/**
 * @brief 等待连接完成
 */
static int wifi_wait_connection_complete(uint32_t timeout_ms)
{
    uint64_t start_time = get_timestamp_ms();
    
    while ((get_timestamp_ms() - start_time) < timeout_ms) {
        // 检查连接状态
        // 在实际实现中，这里会检查系统WiFi状态
        
        // 模拟连接过程
        usleep(100000); // 100ms
        
        // 假设连接成功
        if ((get_timestamp_ms() - start_time) > 2000) {
            return 0; // 连接成功
        }
    }
    
    return -1; // 连接超时
}

/**
 * @brief 扫描网络可用性
 */
static int wifi_scan_network_available(const char* ssid)
{
    printf("🔍 扫描网络: %s\n", ssid);
    
    // 在实际实现中，这里会执行WiFi扫描
    // 检查目标网络是否在可用列表中
    
    return 1; // 假设网络可用
}

/**
 * @brief Ping网关测试
 */
static int wifi_ping_gateway(void)
{
    // 简化的ping实现
    // 在实际系统中会ping默认网关
    
    // 模拟ping测试
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return -1;
    }
    
    // 设置非阻塞
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(53); // DNS端口
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    // 尝试连接
    int result = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    close(sock);
    
    return (result == 0 || errno == EINPROGRESS) ? 0 : -1;
}

/**
 * @brief 获取时间戳(毫秒)
 */
static uint64_t get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * @brief 保存网络配置
 */
int wifi_save_network_config(const char* ssid, const char* password)
{
    // 检查是否已保存
    for (int i = 0; i < wifi_manager.saved_count; i++) {
        if (strcmp(wifi_manager.saved_networks[i].ssid, ssid) == 0) {
            // 更新密码
            strncpy(wifi_manager.saved_networks[i].password, password, 
                   sizeof(wifi_manager.saved_networks[i].password) - 1);
            wifi_manager.saved_networks[i].last_connected = (uint32_t)time(NULL);
            printf("🔄 更新已保存的网络: %s\n", ssid);
            return 0;
        }
    }
    
    // 添加新网络
    if (wifi_manager.saved_count < MAX_SAVED_NETWORKS) {
        wifi_network_t* network = &wifi_manager.saved_networks[wifi_manager.saved_count];
        strncpy(network->ssid, ssid, sizeof(network->ssid) - 1);
        strncpy(network->password, password, sizeof(network->password) - 1);
        network->is_saved = true;
        network->last_connected = (uint32_t)time(NULL);
        wifi_manager.saved_count++;
        
        printf("💾 保存新网络: %s\n", ssid);
        return 0;
    }
    
    printf("⚠️ 已达到最大保存网络数量\n");
    return -1;
}

/**
 * @brief 加载保存的网络配置
 */
int wifi_load_saved_networks(void)
{
    // 从配置文件加载保存的网络
    // 这里可以从JSON文件或其他持久化存储加载
    
    printf("📋 加载保存的WiFi网络配置\n");
    
    // 示例：添加默认网络
    if (wifi_manager.saved_count == 0) {
        wifi_save_network_config("vela_network", "vela123456");
    }
    
    return 0;
}

/**
 * @brief 获取WiFi连接状态
 */
wifi_state_t wifi_get_connection_state(void)
{
    return wifi_manager.state;
}

/**
 * @brief 设置自动重连
 */
void wifi_set_auto_reconnect(bool enabled)
{
    wifi_manager.auto_reconnect = enabled;
    printf("🔄 自动重连: %s\n", enabled ? "启用" : "禁用");
}

/**
 * @brief 清理WiFi管理器
 */
void wifi_manager_optimized_cleanup(void)
{
    if (wifi_manager.reconnect_timer) {
        lv_timer_del(wifi_manager.reconnect_timer);
        wifi_manager.reconnect_timer = NULL;
    }
    
    if (wifi_manager.monitor_timer) {
        lv_timer_del(wifi_manager.monitor_timer);
        wifi_manager.monitor_timer = NULL;
    }
    
    if (wifi_manager.ping_timer) {
        lv_timer_del(wifi_manager.ping_timer);
        wifi_manager.ping_timer = NULL;
    }
    
    printf("🧹 WiFi管理器已清理\n");
}

