# 开发说明

简体中文 | [English](DEVELOPMENT.en.md) | [README](../README.md)

## 构建产物

环境为 `esp32dev`，应用输出 `.pio/build/esp32dev/firmware.bin`；打包文件位于相邻的 `xtouch-bin/ota/` 和 `xtouch-bin/webusb/`。普通构建不会改变 `version.json` 或仓库根目录的 `ota.json`。

## 结构

- `src/ui/`：LVGL 页面、组件和交互；`ui_loaders.c` 负责切页和切换提示。
- `src/ui/i18n/`：八种语言独立文案，`strings.c` 统一查询；业务代码使用 `XTOUCH_TEXT_*`。
- `src/xtouch/config.h`、`printers.h`：兼容单设备 `mqtt` 配置，解析最多 100 个 `printers` 条目。
- `src/xtouch/mqtt.h`：一次只连接一台，切换清理旧状态，连接失败采用退避重试。
- `src/xtouch/storage.h/.cpp`：内部 SPIFFS 原子保存、SD 镜像同步、日期/版本冲突处理和删除标记；其他模块不直接读写 SD 配置。
- `src/xtouch/settings.h`：设置持久化；语言值为 `zh`、`zh-TW`、`en`、`fr`、`de`、`ru`、`ja`、`ko`。
- `src/xtouch/firmware.h`、`net.h`、`ota-certs.h`：版本判断、HTTPS 下载、校验与安装。
- `resources/User_Setup.h`：2.8 英寸硬件与屏幕驱动；`resources/lv_conf.h`：全局字体和 LVGL 内存配置。

机型图片、AMS 分组和仓温遥测见 [机型与 MQTT 能力](PRINTER-MODELS.md)。

## 字体与事件生命周期

字体按界面静态文案生成子集，外部任务名中的任意字符不保证覆盖。

新增文案须补齐八语文案文件以及 `strings.h/.c`，按下方方法重新生成字库并运行 `python scripts/check_localization.py`。原始 TTF 不随代码发布。

切换设备与语言先排队，在主循环执行，避免删除正在处理事件的控件。LVGL 使用有限的内存池，切页先释放旧页再创建新页，中间不刷新空白页；不要同时缓存多个完整页面。

内部配置见 [存储说明](STORAGE.md)。`uploadfs` 或擦除 SPIFFS 分区会删除内部配置，不是普通固件升级步骤；调试前先确认 SD 备份已同步。

## 字库生成

设置页可选择八种语言，立即切换并保存到内部 `/xtouch/settings.json`，有 SD 卡时同步备份，重启后保留。独立语言文件位于 `src/ui/i18n/`，由 `strings.c` 统一查询。

- 八种语言文案与选择入口：`src/ui/i18n/`；新增文案须补齐八种语言。
- 全局字体：`resources/lv_conf.h` 中的 `LV_FONT_DEFAULT` 和 `XTOUCH_FONT_NUMERIC`。
- 字库生成：`scripts/localize_zh.py`，只生成字体，不改写应用源码。
- 覆盖检查：`python scripts/check_localization.py`。

更换文案或字体时，安装 Node.js 并指定有权使用的输入字体：

```powershell
$env:XTOUCH_W7_FONT='C:\fonts\primary.ttf'
$env:XTOUCH_LATIN_FONT='C:\fonts\latin.ttf'
$env:XTOUCH_KOREAN_FONT='C:\fonts\korean.ttf'
python -m pip install fonttools
python scripts/generate_locales.py
python scripts/localize_zh.py
python scripts/check_localization.py
python -m platformio run
```

字体相关权利归原权利人所有，使用和再分发需自行确认对应授权。故障文案保存在项目文档中，固件只显示故障码；见 [故障码说明](ERROR-CODES.md)。

## 发布

1. 修改 `version.json`，正常构建并检查语言、字体和产物。
2. 应用文件为 `../xtouch-bin/ota/xtouch-2.8.bin`，完整镜像为 `../xtouch-bin/webusb/xtouch-2.8-full.bin`。
3. 将生成的 `../xtouch-bin/ota/ota.json` 复制为仓库根目录 `ota.json`；校验文件长度及 MD5 与最终应用一致。
4. 提交源码及清单，使用与 `version.json` 一致的版本标签创建 Release，上传两个 BIN 和 `ota.json`。
5. 验证较低版本设备通过项目地址完成下载、校验、安装和重启，再核对同版本不重复更新。

普通构建保持源码版本和仓库发布清单不变。正式发布使用完成验证的构建产物。证书链发生变更时应检查并更新 `ota-certs.h` 中的受信任根证书，不能关闭 TLS 校验来绕过问题。

管理网页编译时复用 web 目录锁定的 esbuild，压缩后使用 Zopfli 生成标准 gzip。源码保持可读；首次编译前安装依赖：

```sh
npm ci --prefix web
python -m pip install -r scripts/requirements.txt
```

## 故障码与固件体积

完整文案保存在 `docs/data/error-codes.json`，不编入固件。`scripts/generate_error_docs.py` 生成对照文档和少量重试／完成规则；`src/xtouch/errors.c` 使用 32 字节静态缓冲区格式化错误码，不分配堆内存。返回指针在下次查询前有效，LVGL 显示时复制文本。新增非零故障码无需更新白名单也能显示。

设置 `XTOUCH_ERROR_SELFTEST=1` 可编译实机格式化、输入校验及操作规则自检，正常构建不含自检代码。OTA 应用分区仍为 0x1e0000，打包脚本拒绝超限固件。

## 语言与字库

语言代码固定为 `zh`, `zh-TW`, `en`, `fr`, `de`, `ru`, `ja`, `ko`。基础界面字典在 `src/ui/i18n/`，新增控制和管理网页字典分别在 `resources/ui-locales.json` 与 `resources/portal-locales.json`；每行固定八语顺序。`scripts/generate_locales.py` 生成只读固件表，网页表由 `embed_portal.py` 合入 gzip 包。改动文案后重新生成字库并执行覆盖检查。

资源生成所需的主字体、补充拉丁字形和韩文字形分别由 `XTOUCH_W7_FONT`、`XTOUCH_LATIN_FONT`、`XTOUCH_KOREAN_FONT` 指定。相关授权文件保存在 [licenses](licenses/)。
