# xTouch-CN 1.0.1

本版本适用于 2.8 英寸 CYD（ESP32-2432S028R）主板。

- 优化 HMS 与打印错误提示：区分错误、警告、信息和操作确认；权限提示按设备会话去重，普通信息短暂显示，重要故障保留手动确认。
- 统一弹窗按钮颜色和作用：重试、完成、关闭分别区分，关闭仅影响本地提示，不向打印机发送清错指令。
- 核对并修正八语言界面文案，区分连接状态与方式、本地发送失败与设备拒绝；保留“部分功能仅支持局域网模式，且开启开发者。”。
- 改善首页机型与信号显示；小屏信号数字放大并叠加在 Wi-Fi 图标右上角。
- 设置页 GitHub 图标与完整单行网址紧邻显示，改善小屏可读性。

S3、P4 移植及摄像头功能暂不包含在此版本。

## 安装与升级

- 在线 USB 刷写：使用项目首页。
- 后台上传、OTA 或 SD 卡升级：`xtouch-2.8.bin`。
- 完整 USB 镜像：`xtouch-2.8-full.bin`，不能用于 OTA／SD 卡升级。
- `SHA256SUMS.txt` 提供附件校验值。

## English

CYD (ESP32-2432S028R) release. Improves HMS notification timing and deduplication, typed dialogs, action-button colors, model/signal display, project-link readability, and eight-language wording. Distinguishes local send failures from printer rejections. S3/P4 ports and camera playback are deferred.

Use `xtouch-2.8.bin` for web management uploads, OTA, or SD updates. Use `xtouch-2.8-full.bin` only for full USB installation. Checksums are provided in `SHA256SUMS.txt`.
