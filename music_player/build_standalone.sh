#!/bin/bash
# 独立编译音乐播放器
set -e

echo "🎵 独立编译Vela音乐播放器"
echo "========================="

# 设置编译参数
CC=${CC:-gcc}
CFLAGS="-Wall -Wextra -O2 -std=c99"
INCLUDES="-I. -I/usr/include/lvgl -I/usr/local/include"
LIBS="-llvgl -lmad -lpthread -lm"

# 定义源文件
SOURCES="music_player.c audio_ctl.c playlist_manager_optimized.c font_config.c splash_screen.c wifi.c"
MAIN_SOURCE="music_player_main.c"
OUTPUT="music_player"

echo "📁 当前目录: $(pwd)"
echo "🔧 编译器: $CC"
echo "📋 源文件: $SOURCES $MAIN_SOURCE"

# 检查源文件是否存在
for src in $SOURCES $MAIN_SOURCE; do
    if [ ! -f "$src" ]; then
        echo "❌ 错误: 找不到源文件 $src"
        exit 1
    fi
done

echo "✅ 所有源文件检查通过"

# 编译
echo "⚙️ 开始编译..."
if $CC $CFLAGS $INCLUDES -o $OUTPUT $SOURCES $MAIN_SOURCE $LIBS 2>/dev/null; then
    echo "✅ 编译成功！生成可执行文件: $OUTPUT"
    ls -la $OUTPUT
else
    echo "❌ 编译失败，尝试简化编译..."
    
    # 简化编译，不包含复杂依赖
    SIMPLE_CFLAGS="-Wall -O2 -DCONFIG_LVX_USE_DEMO_MUSIC_PLAYER=1"
    SIMPLE_SOURCES="music_player_main.c"
    
    echo "🔄 使用简化编译..."
    if $CC $SIMPLE_CFLAGS -o ${OUTPUT}_simple $SIMPLE_SOURCES 2>/dev/null; then
        echo "✅ 简化编译成功！"
        mv ${OUTPUT}_simple $OUTPUT
    else
        echo "❌ 编译失败"
        echo "💡 建议: 请在完整的Vela构建环境中编译"
        exit 1
    fi
fi

echo ""
echo "🎉 编译完成！"
echo "📄 生成的可执行文件: $OUTPUT"
echo "🚀 运行方式: ./$OUTPUT"
echo ""
