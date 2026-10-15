# 配置生成与在线刷写

简体中文 | [English](WEB.en.md) | [README](../README.md)

入口：**[配置生成与在线刷写](https://tbc0309.github.io/xtouch-CN/)**。根据浏览器语言显示中文或英文，右上角可手动切换。

## 生成配置

1. 填 Wi-Fi 名称和密码；开放网络的密码可留空。
2. 填打印机地址、局域网访问码、序列号，并选择机型。可添加最多 100 台设备，调整顺序、删除条目、选择开机默认设备。
3. 点击“检查配置”校验。预览隐藏密码和访问码，下载文件保留实际值。
4. 下载 `xtouch.json` 放到 SD 卡根目录，插卡开机导入。

支持导入原版单设备 `mqtt` 和本项目多设备 `printers` 配置。导入替换当前表单，仅保留 Wi-Fi、打印机和启动等待参数，移除空槽位，重新计算开机默认索引；不编辑触摸校准或其他设置文件。导出前请核对。

填写内容仅保留在当前页面内存，不发送到服务器，不自动保存到浏览器。刷新或关闭页面会丢失尚未下载的内容。配置文件包含密码与设备访问码，请自行保管。

## USB 在线安装

支持硬件为 [ESP32-2432S028R](HARDWARE.md)，不能只按屏幕尺寸选择固件。

1. 在电脑 Chrome / Edge 中通过 HTTPS 打开页面，用数据线连接主板，关闭占用串口的软件。
2. 选择“在线刷写”，加载刷写工具，再点击连接并选择串口。
3. 在 ESP Web Tools 窗口确认安装。**需要保留配置时不要勾选 Erase device；勾选会清空内部设置和校准。**
4. 完成后准备 SD 配置并启动，首次使用按屏幕提示校准。

网页中英文可切换；ESP Web Tools 的串口连接、安装进度和确认窗口目前为英文。若浏览器不支持 Web Serial、无法授权串口或网络受限，可使用 [SD 卡升级或 在线刷写](UPDATING.md)。网页不通过串口写入 Wi-Fi 或打印机配置，仍使用 SD 卡导入。

在线安装使用同站点的已发布完整镜像，从 `0x0` 写入。页面不会自动执行擦除或安装，必须由用户选择串口并确认。手机和内嵌浏览器不作为支持环境。

## 网站维护

- 页面：`web/`，静态 HTML/CSS/JavaScript，无服务端数据库或统计脚本。
- 参数校验：`web/config.js`；中英文文案：`web/app.js`。
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
