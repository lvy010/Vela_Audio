# Vela Audio 音频项目

## 📁 项目结构

```
Vela_Audio/
├── music_player/          # 音乐播放器模块
│   ├── src/              # 源代码
│   ├── include/          # 头文件
│   ├── res/              # 资源文件
│   └── docs/             # 文档
├── .gitignore            # Git 忽略文件
└── README.md             # 项目说明
```

## 🎵 功能特性

### 音乐播放器 (music_player)
- 🎵 支持多种音频格式 (MP3, WAV, FLAC)
- 🎨 现代化用户界面设计
- 📱 响应式播放列表管理
- 🖼️ 背景图片自定义支持
- 🔊 音量控制和音效处理
- 📡 网络连接状态显示

## 🚀 快速开始

### 环境要求
- Vela 开发环境
- LVGL 图形库
- libmad (MP3解码支持)
- NuttX 操作系统

### 编译运行
```bash
# 进入项目目录
cd Vela_Audio/music_player

# 编译项目
make clean && make

# 运行音乐播放器
./music_player
```

### 资源配置
1. 将音频文件放入 `res/musics/` 目录
2. 将图标文件放入 `res/icons/` 目录
3. 配置 `res/musics/manifest.json` 文件

## 📋 开发指南

### 目录说明
- `src/` - 核心源代码
- `include/` - 公共头文件
- `res/` - 资源文件（图标、音频、配置）
- `docs/` - 项目文档和API说明

### 代码规范
- 使用 C99 标准
- 遵循 NuttX 编码规范
- 添加适当的注释和文档
- 进行单元测试

## 🛠️ 技术栈

- **操作系统**: NuttX
- **图形界面**: LVGL
- **音频解码**: libmad, libflac
- **构建系统**: Make
- **版本控制**: Git

## 📖 文档

详细文档请参考：
- [API 文档](docs/API.md)
- [开发指南](docs/DEVELOPMENT.md)
- [用户手册](docs/USER_MANUAL.md)

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目采用 Apache 2.0 许可证。

## 📞 联系方式

如有问题，请联系开发团队。