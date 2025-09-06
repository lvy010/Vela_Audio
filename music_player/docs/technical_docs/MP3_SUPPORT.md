# MP3 支持测试指南

## 概述

本文档说明如何在 OpenVela 音乐播放器中测试 MP3 格式支持功能。

## 已实现的功能

### ✅ 核心功能
- [x] MP3 文件格式自动检测
- [x] libmad 库集成
- [x] MP3 到 PCM 实时解码
- [x] 多种采样率支持（8kHz - 48kHz）
- [x] 立体声和单声道支持
- [x] 可配置的解码缓冲区大小
- [x] 内存管理和资源清理

### ✅ 系统集成
- [x] Kconfig 配置选项
- [x] Makefile 构建支持
- [x] 音频控制器集成
- [x] 播放列表支持

## 配置步骤

### 1. 启用 MP3 支持

编辑配置文件或使用 menuconfig：

```bash
# 方法1：直接编辑 defconfig
echo "CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT=y" >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
echo "CONFIG_LVX_MUSIC_PLAYER_MP3_BUFFER_SIZE=8192" >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig
echo "CONFIG_LIB_MAD=y" >> vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap/defconfig

# 方法2：使用 menuconfig
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap menuconfig
# 导航到: Application Configuration -> Packages -> Demos -> Music Player
# 启用 "Enable MP3 audio format support"
```

### 2. 编译项目

```bash
# 清理并重新编译
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap distclean -j8
./build.sh vendor/openvela/boards/vela/configs/goldfish-armeabi-v7a-ap -j8
```

## 测试步骤

### 1. 准备测试文件

```bash
# 转换 WAV 到 MP3 进行测试
ffmpeg -i apps/packages/demos/music_player/res/musics/UnamedRhythm.wav \
       -acodec mp3 -ab 128k apps/packages/demos/music_player/res/musics/test.mp3

# 或使用在线 MP3 文件（确保版权合规）
wget -O apps/packages/demos/music_player/res/musics/test.mp3 \
     "https://example.com/public-domain-audio.mp3"
```

### 2. 更新播放列表

编辑 `apps/packages/demos/music_player/res/musics/manifest.json`：

```json
{
  "musics": [
    {
      "path": "UnamedRhythm.wav",
      "name": "UnamedRhythm (WAV)",
      "artist": "Benign X",
      "cover": "UnamedRhythm.png",
      "total_time": 12000,
      "color": "#114514"
    },
    {
      "path": "test.mp3",
      "name": "Test MP3 Song",
      "artist": "Test Artist",
      "cover": "nocover.png",
      "total_time": 30000,
      "color": "#FF5722"
    }
  ]
}
```

### 3. 部署和测试

```bash
# 启动模拟器
./emulator.sh vela &

# 等待启动完成
sleep 15

# 连接 ADB
adb connect 127.0.0.1:5555

# 推送更新的资源
adb -s emulator-5554 push apps/packages/demos/music_player/res /data/

# 启动音乐播放器
adb -s emulator-5554 shell "music_player &"
```

### 4. 功能验证

在音乐播放器界面中验证以下功能：

1. **文件识别**：MP3 文件显示在播放列表中
2. **格式检测**：应用能正确识别 `.mp3` 扩展名
3. **播放控制**：可以播放、暂停、停止 MP3 文件
4. **音质测试**：比较 MP3 和 WAV 的音质差异
5. **切换测试**：在 WAV 和 MP3 文件间切换播放

## 调试信息

### 编译时检查

验证 MP3 支持是否正确编译：

```bash
# 检查配置
grep -r "CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT" .config

# 检查编译产物中是否包含 libmad
nm nuttx/vela_ap.elf | grep mad

# 检查编译日志
grep -i "mad\|mp3" build.log
```

### 运行时调试

添加调试输出来跟踪 MP3 解码过程：

```c
// 在 audio_ctl.c 中添加调试信息
#define MP3_DEBUG 1

#if MP3_DEBUG
#define MP3_LOG(fmt, ...) printf("[MP3] " fmt "\\n", ##__VA_ARGS__)
#else
#define MP3_LOG(fmt, ...)
#endif

// 在关键函数中添加日志
MP3_LOG("Detected audio format: %s", 
        (format == AUDIO_FORMAT_MP3) ? "MP3" : "WAV");
MP3_LOG("MP3 decoder initialized, buffer size: %zu", decoder->buffer_size);
MP3_LOG("Decoded %zu bytes of PCM data", output_size);
```

## 性能测试

### 内存使用测试

```bash
# 监控内存使用
adb shell "ps | grep music_player"
adb shell "cat /proc/$(adb shell pidof music_player)/status | grep VmRSS"
```

### CPU 使用测试

```bash
# 监控 CPU 使用
adb shell "top -p $(adb shell pidof music_player)" | head -10
```

## 故障排除

### 常见问题

1. **编译错误：找不到 mad.h**
   ```bash
   # 确保 libmad 开发包已安装
   CONFIG_LIB_MAD=y
   ```

2. **运行时错误：MP3 解码失败**
   ```bash
   # 检查 MP3 文件格式
   file /data/res/musics/*.mp3
   
   # 检查文件权限
   adb shell "ls -la /data/res/musics/"
   ```

3. **音频播放无声音**
   ```bash
   # 检查音频设备
   adb shell "ls -la /dev/audio/"
   
   # 检查音量设置
   adb shell "cat /proc/asound/cards"
   ```

4. **内存泄漏**
   ```bash
   # 监控内存变化
   adb shell "while true; do grep VmRSS /proc/$(pidof music_player)/status; sleep 5; done"
   ```

### 日志分析

查看系统日志中的错误信息：

```bash
# 查看音乐播放器日志
adb shell "dmesg | grep -i 'music\|audio\|mad'"

# 查看内核音频日志
adb shell "dmesg | grep -i 'audio\|pcm'"
```

## 基准测试

### 解码性能测试

创建不同格式的测试文件进行性能对比：

```bash
# 生成测试文件
ffmpeg -f lavfi -i "sine=frequency=440:duration=60" -acodec pcm_s16le test_60s.wav
ffmpeg -f lavfi -i "sine=frequency=440:duration=60" -acodec mp3 -ab 128k test_60s_128k.mp3
ffmpeg -f lavfi -i "sine=frequency=440:duration=60" -acodec mp3 -ab 320k test_60s_320k.mp3

# 测试播放并记录 CPU/内存使用
```

### 延迟测试

测量音频播放的启动延迟：

```bash
# 记录从点击播放到声音输出的时间
time adb shell "music_player play_file /data/res/musics/test.mp3"
```

## 验收标准

MP3 支持功能通过测试的标准：

- ✅ 能够正确识别和加载 MP3 文件
- ✅ 音频播放质量无明显失真
- ✅ 内存使用稳定，无泄漏
- ✅ CPU 使用在合理范围内
- ✅ 支持常见的 MP3 格式（128k-320k bitrate）
- ✅ 播放控制功能正常（播放/暂停/停止）
- ✅ 能够在 WAV 和 MP3 间正常切换

## 结论

通过本测试指南，您可以全面验证 OpenVela 音乐播放器的 MP3 支持功能。如果遇到问题，请参考故障排除部分或查看详细的调试日志。

---
*测试完成日期：2024-08-20*
*测试环境：OpenVela + QEMU 模拟器*
