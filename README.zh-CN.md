# CrossMux（Waveshare 版）

[English](./README.md) | **简体中文**

**CrossMux** 是面向 ESP32 墨水屏设备的 [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader) 社区 fork，本仓库仅维护 **Waveshare ESP32-S3 ePaper 3.97**（800×480、16 MiB 闪存、8 MiB PSRAM）。以阅读为核心，同时提供轻量应用、阅读分析、待机表盘和按需联网服务。

[固件发布](https://github.com/0x1abin/crossmux/releases) · [用户指南](./USER_GUIDE.md) · [参与贡献](./docs/contributing/README.md)

![CrossMux 墨水屏设备](./docs/images/cover.jpg)

## 核心功能

- **阅读与书库**：EPUB、TXT、XTC/XTCH 和图片，章节导航、书签、词典、自定义字体、阅读背景，以及 KOReader 进度同步。
- **无线功能**：浏览器传书与设置、Calibre 无线连接、OPDS 下载、WebDAV 和设备 OTA 更新。
- **Apps 应用中心**：文件传输、OPDS 浏览、阅读统计、待机表盘，以及（中国区）微信读书。[应用说明](./src/activities/apps/README.md)。
- **微信读书**：扫码登录、浏览书架、下载 EPUB 离线阅读和同步进度，在 China 内容区显示。[微信读书说明](./src/activities/apps/weread/README.md)。
- **阅读分析与待机**：阅读统计、热力图、档案与成就，以及时钟和老黄历表盘。[阅读分析说明](./src/activities/apps/reading-stats/README.md)。
- **语言与开发**：统一固件包含 33 种 UI 语言，并提供桌面模拟器辅助开发。

> **微信读书安全提示**：非公开 Web 协议可能变化。真机传输经过加密，但客户端不验证服务器证书与主机名，请仅在可信网络中使用。原生模拟器通过主机信任库验证证书，详见[传输说明](./docs/engineering/chinese-build.md#weread-transport)。

## 设备与发布

| 项目 | 说明 |
|---|---|
| 硬件 | [Waveshare ESP32-S3 ePaper 3.97](https://www.waveshare.com/) |
| 芯片 | ESP32-S3（16 MiB 闪存、8 MiB OPI PSRAM） |
| 面板 | 800×480 SSD1677 |
| 渠道 | Nightly（见 [Releases](https://github.com/0x1abin/crossmux/releases)） |

构建环境与版本见 [platformio.ini](./platformio.ini)；打包与 OTA 见 [firmware-release.md](./docs/engineering/firmware-release.md)；硬件说明与验收见 [waveshare-epaper-397.md](./docs/engineering/waveshare-epaper-397.md)。

## 安装固件

1. 打开 [CrossMux Releases](https://github.com/0x1abin/crossmux/releases)，选择 **Waveshare ePaper 3.97** 的 Nightly 安装包，按该版本说明刷写。
2. 更换固件前请备份 SD 卡。首次安装需完整安装包；仅含应用的 `firmware.bin` 不能单独完成首次刷机。
3. 刷写与恢复命令（偏移、USB 端口）以 [waveshare-epaper-397.md](./docs/engineering/waveshare-epaper-397.md) 为准，勿套用 ESP32-C3 / Xteink 文档。

已安装设备可在发布渠道提供匹配包时通过 Wi-Fi OTA 更新。

## 中文字体与内容区

每次构建产出一份统一语言固件。简体中文选择 China 内容区（用于微信读书等区域应用与元数据）；OTA、字体下载等网络服务统一使用 `crossmux.com` 与 global OTA 变体。其它 UI 语言使用 Global 内容区。

UI 内置精简的 8/10/12pt 简体中文回退字体。内置阅读字体选项共用 12pt 离线回退；完整字族、其它字号、粗斜体和更广 Unicode 覆盖请使用 SD 卡 `.cpfont` 字体。内嵌字库是子集，生僻字或繁体字可能需要相应 SD 字体。

可从 **设置 > 阅读器 > 管理字体** 下载字体，或将转换好的字体复制到 SD 卡。安装与转换见 [SD 卡字体](./docs/sd-card-fonts.md)，工具链见[中文支持](./docs/engineering/chinese-build.md)。

## 开发快速开始

安装 PlatformIO Core（`pio`）和 Python 3；仓库已固定 pioarduino 平台。完整代码检查还需要 clang-format 21+、CMake 和 Ninja。环境设置见[开发入门](./docs/contributing/getting-started.md)。

```bash
git clone --recursive https://github.com/0x1abin/crossmux.git
cd crossmux

# 如果尚未初始化子模块：
git submodule update --init --recursive

# Waveshare ESP32-S3 ePaper 3.97（默认环境）
pio run

# 构建并烧录到已连接的设备
pio run -t upload
```

应用固件位于 `.pio/build/waveshare_epaper_397/firmware.bin`。详见[构建文档](./docs/engineering/build-system.md)与 [Waveshare 设备指南](./docs/engineering/waveshare-epaper-397.md)。

### 桌面模拟器

安装 SDL2 与 curl（Linux 还需 OpenSSL 开发头文件），将 EPUB 放入 `fs_/books/`，然后运行：

```bash
pio run -e simulator -t run_simulator
```

[Waveshare 配置模拟器 fork](https://github.com/dezzw/crosspoint-simulator) 的版本固定在 `platformio.ini` 中（面键输入在该仓库实现，而非 CrossMux overlay）。它用于预览 UI 和按键输入，不能验证墨水屏波形、耗电或 Waveshare PMIC 时序。

### 检查与调试

```bash
./bin/ci-check       # 完整代码检查，不改写源文件
pio device monitor  # 已连接设备的串口日志
```

详见[测试与调试](./docs/contributing/testing-debugging.md)。纯文档修改检查链接、命令和空白即可，无需构建固件。

## 文档与贡献

- [用户指南](./USER_GUIDE.md) · [Web 传书](./docs/webserver.md) · [Web API](./docs/webserver-endpoints.md)
- [项目范围](./SCOPE.md) · [社区治理](./GOVERNANCE.md) · [贡献指南](./docs/contributing/README.md)
- [Agent 指南](./AGENTS.md) · [工程文档](./docs/engineering/index.md) · [按键与 UI](./docs/contributing/touch-and-ui.md)
- [缓存管理](./docs/engineering/cache-management.md) · [文件格式](./docs/file-formats.md)

请在 [CrossMux Issues](https://github.com/0x1abin/crossmux/issues) 反馈问题。贡献 PR 以 **`0x1abin/crossmux:main`** 为目标。代码中的 CrossPoint 类名与 SD 卡 `/.crosspoint` 目录为兼容保留；该目录保存设置与阅读进度，勿为清理单书缓存而删除整个目录。

## 致谢

感谢 [CrossPoint Reader](https://github.com/crosspoint-reader/crosspoint-reader)、[Inx](https://github.com/obijuankenobiii/inx)、[cpr-vcodex](https://github.com/franssjz/cpr-vcodex) 及其贡献者，以及 [diy-esp32-epub-reader](https://github.com/atomic14/diy-esp32-epub-reader) 的启发。

CrossMux 与 Waveshare 及任何设备厂商均无隶属关系。许可证见 [LICENSE](./LICENSE)。
