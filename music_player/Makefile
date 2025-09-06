#
# Copyright (C) 2022 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

include $(APPDIR)/Make.defs

ifeq ($(CONFIG_LVX_USE_DEMO_MUSIC_PLAYER), y)
PROGNAME = music_player
PRIORITY = 100
STACKSIZE = 40960
MODULE = $(CONFIG_LVX_USE_DEMO_MUSIC_PLAYER)

# 原有源文件（保持兼容性） + 字体配置支持
CSRCS = music_player.c audio_ctl.c wifi.c splash_screen.c playlist_manager_optimized.c font_config.c

# 新架构模块（可选启用）
ifeq ($(CONFIG_LVX_MUSIC_PLAYER_NEW_ARCHITECTURE), y)
CSRCS += core/event_bus.c core/state_manager.c core/audio_engine.c
CSRCS += services/playback_controller.c services/media_library.c
CSRCS += ui/ui_manager.c
CSRCS += adapters/audio_adapter.c
CSRCS += legacy/music_player_compat.c
CSRCS += app_main.c
MAINSRC = vela_music_main.c
else
MAINSRC = music_player_main.c
endif

# Add MP3 support libraries if enabled
ifeq ($(CONFIG_LVX_MUSIC_PLAYER_MP3_SUPPORT), y)
LDLIBS += -lmad
endif
endif

-include $(APPDIR)/Application.mk
