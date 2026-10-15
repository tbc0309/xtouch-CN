# 开发说明

简体中文 | [English](DEVELOPMENT.en.md) | [README](../README.md)

## 结构

- `src/ui/`：LVGL 页面、组件和交互；`ui_loaders.c` 负责切页和切换提示。
- `src/ui/i18n/`：简体、繁体、英文独立文案，`strings.h` 统一选择；业务代码使用 `XTOUCH_TEXT_*`。
- `src/xtouch/config.h`、`printers.h`：兼容单设备 `mqtt` 配置，解析最多四个 `printers` 条目。
- `src/xtouch/mqtt.h`：一次只连接一台，切换清理旧状态，连接失败采用退避重试。
- `src/xtouch/storage.h/.cpp`：内部 SPIFFS 原子保存、SD 镜像同步、日期/版本冲突处理和删除标记；其他模块不直接读写 SD 配置。
- `src/xtouch/settings.h`：设置持久化；语言值为 `zh`、`zh-TW`、`en`。
- `src/xtouch/firmware.h`、`net.h`、`ota-certs.h`：版本判断、HTTPS 下载、校验与安装。
- `resources/User_Setup.h`：2.8 英寸硬件与屏幕驱动；`resources/lv_conf.h`：全局字体和 LVGL 内存配置。

## 字体与事件生命周期

普通界面使用 W7 14px，大号数字使用 W7 28px，图标保持上游图标字体。字体按三种语言的静态文案生成灰阶子集；外部任务名和自定义设备名中的任意汉字不保证覆盖。

新增文案须补齐 `zh_cn.h`、`zh_tw.h`、`en.h` 和 `strings.h`，按 README 中的方法重新生成字库并运行 `python scripts/check_localization.py`。原始 TTF 不随代码发布。繁体文案为独立文件，构建不依赖转换软件。

切换设备与语言先排队，在主循环执行，避免删除正在处理事件的控件。LVGL 使用有限的内存池，切页先释放旧页再创建新页，中间不刷新空白页；不要同时缓存多个完整页面。

内部配置见 [存储说明](STORAGE.md)。`uploadfs` 或擦除 SPIFFS 分区会删除内部配置，不是普通固件升级步骤；调试前先确认 SD 备份已同步。

## 发布

1. 修改 `version.json`，正常构建并检查语言、字体和产物。
2. 应用文件为 `../xtouch-bin/ota/xtouch-2.8.bin`，完整镜像为 `../xtouch-bin/webusb/xtouch-2.8-full.bin`。
3. 将生成的 `../xtouch-bin/ota/ota.json` 复制为仓库根目录 `ota.json`；校验文件长度及 MD5 与最终应用一致。
4. 提交源码及清单，使用与 `version.json` 一致的版本标签创建 Release，上传两个 BIN 和 `ota.json`。
5. 验证较低版本设备通过项目地址完成下载、校验、安装和重启，再核对同版本不重复更新。

普通构建保持源码版本和仓库发布清单不变。正式发布使用完成验证的构建产物。证书链发生变更时应检查并更新 `ota-certs.h` 中的受信任根证书，不能关闭 TLS 校验来绕过问题。
