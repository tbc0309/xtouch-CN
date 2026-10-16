# 在线刷写与旧版配置工具

简体中文 | [English](WEB.en.md) | [README](../README.md)

入口：**[在线刷写](https://tbc0309.github.io/xtouch-CN/)**。中文浏览器默认中文，其它语言默认英文，右上角可手动切换。

## 在线安装

支持硬件为 [ESP32-2432S028R](HARDWARE.md)。

1. 电脑 Chrome / Edge 打开页面，数据线连接主板，关闭占用串口的软件。
2. 加载刷写工具，选择对应串口，在 ESP Web Tools 窗口确认安装。
3. 保留配置时不要勾选 **Erase device**；整片擦除会清空内部配置和校准。
4. 首次开机完成触摸校准，扫码连接设备热点，在 `192.168.4.1` 连接家庭 Wi-Fi。
5. 从屏幕“网页管理”扫码进入后台，修改初始管理密码，再添加局域网或云端打印机。

正常使用不需要 SD 卡。工具的串口授权、进度和安装确认窗口目前为英文。手机与内嵌浏览器不支持串口刷写。原版 SD 升级、CN OTA 和恢复步骤见 [更新与维护](UPDATING.md)。

## 旧版配置生成（可选）

首页底部的 **[旧版配置生成](https://tbc0309.github.io/xtouch-CN/legacy.html)**（仅适用于 v0.9.215） 用于生成 SD 导入文件，日常设置使用设备网页后台。

填写 Wi-Fi、打印机地址、访问码、序列号和型号，检查后下载 `xtouch.json` 到 SD 根目录。支持最多 100 台及原版单设备 `mqtt` 配置导入。此工具只处理 Wi-Fi、打印机和启动参数，不是完整云账号备份编辑器；完整备份请使用设备后台。

填写内容仅在当前网页内存处理，不上传、不自动保存。预览隐藏凭据，下载文件包含实际值，请妥善保管。

## 网站维护

- 页面：`web/`，静态 HTML/CSS/JavaScript，无服务端数据库或统计脚本。
- 参数校验：`web/config.js`；首页：`web/app.js`；旧版工具：`web/legacy.html`、`web/legacy.js`。
- 串口工具：ESP Web Tools 10.1.0，构建时打包到本站 `vendor/`，不在用户填表时加载外部 CDN 脚本。
- 部署：`.github/workflows/pages.yml`，网页更新或发布 Release 后部署 GitHub Pages；不重新编译固件。
- `scripts/build_web.py` 校验应用长度、MD5、完整镜像内的应用和配置区边界，再生成网页清单。固件来自同版本 Release。

本地预览（需先安装 Node.js 和 Python）：

```sh
cd web
npm ci
npm run bundle
cd ..
python scripts/build_web.py --firmware-dir release-images --output _site
python -m http.server 8895 --directory _site
```

`release-images` 中需准备与仓库 `ota.json` 对应的 `xtouch-2.8.bin` 和 `xtouch-2.8-full.bin`。打开 `http://localhost:8895`。ESP Web Tools 使用 Apache-2.0 许可，依赖许可见包内说明及生成脚本中的版权注释。
