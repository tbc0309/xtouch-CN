# 机型图片与 MQTT 能力

简体中文 | [English](PRINTER-MODELS.en.md) | [README](../README.md)

空闲首页按设备报告显示打印机图片。识别字段为 `info.module[name=ota].product_name` / `print.printer_type`。收到有效型号前及未知型号均保留 xTouch 标志，配置的 `printerModel` 不用于提前显示图片。设备名称仍用于切换提示和设备切换按钮。

## 图片资源

图片来自 BambuStudio，经过 112 × 112、128 色调色板及逐行 RLE 无损编码；重复图片共享像素数据，存于程序 Flash，不依赖 SD 卡。源图片、授权和再生成方式见 [资源说明](../resources/printer-images/NOTICE.md)。

图片映射包括 X1 / X1 Carbon / X1E、P1P / P1S、A1 / A1 mini，以及上游资源中的 P2S、X2D、A2L、H2C、H2D、H2D Pro、H2S。**存在图片只表示可显示该型号，不代表新机型全部控制功能已通过实机测试。**

## AMS

普通四槽 AMS 按编号显示为 AMS A、B 等，显示缓存最多支持 8 组。耗材页同时显示最多 4 组标签，更多时横向滑动；点击仅切换正在查看的耗材组，不向打印机发送换料命令。首页色块显示正在浏览的 AMS 实际耗材色，多组时才显示编号；点击按已连接的编号循环，离开首页再返回恢复第一组。有外置耗材配置时以 `|` 分隔显示其颜色，无配置时隐藏。未配置/无有效颜色时显示灰色；正在供料为绿边，RFID 读取为橙边。

AMS HT、混合 AMS 和新多喷头结构使用其他 ID/存在位规则，需要独立适配，不能将其 ID 直接乘以 4 或截断到普通 AMS。

## 仓温

兼容旧 `print.chamber_temper` / `print.ctt`，并解析新 `print.device.ctc.info.temp` 的低 16 位实温、高 16 位目标温度。缺失字段保留已有值，`0xFFFF` 不覆盖有效温度。

机型表单独记录仓温加热能力：P1S 不支持主动仓温加热；X1E、H2 系列等按上游机型定义区分。本次仅增加遥测解析与能力判断，未增加加热命令或宣称新机型已完成实机验证。新双喷头、干燥控制和联网授权限制仍需单独验证。

## 上游依据

- [机型定义](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/resources/printers/C12.json)
- [仓温解析](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceCore/DevChamber.cpp)
- [仓温控制协议](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceCore/DevChamberCtrl.cpp)
- [AMS 类型与编号](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceCore/DevFilaSystem.cpp)
- [版本响应解析](https://github.com/bambulab/BambuStudio/blob/79df7ae309be398ed30c9f397c13c9911c2edc57/src/slic3r/GUI/DeviceManager.cpp)

## 资源预算与显示

15 个型号共用 13 份像素数据，共 113,143 字节（约 110.5 KiB），比原 104px 索引版本的 153,920 字节减少约 40 KiB。图片先裁透明留白，再等比例放入统一 112px 方框；不拉伸机身。低分辨率源图放大不会生成新的细节。

RLE 只压缩索引，不再减少调色板颜色。RGB565 调色板、行偏移及压缩数据直接存于 Flash；绘制时仅输出当前扫描行，不创建全图解码缓冲，也不从 SD 卡或网络加载。生成器逐行校验往返结果，并限制去重资源总量不超过 128 KiB；后续型号优先复用相同图片，未知型号继续使用标志。

## 耗材遥测

单 AMS 标签同时显示 `humidity_raw` 湿度百分比及 `temp` 箱内温度，耗材卡片在 `remain` 有效时显示剩余百分比。`humidity` 是湿度等级，不当作百分比；`remain=-1` 或缺失能力不显示为 0%。多个 AMS 的窄标签保留名称，避免挤压。字段缺省保留增量缓存，明确无效或设备移除会清除对应有效位；设备切换清空旧快照。以上使用已有状态推送，不增加轮询。

更多候选字段与界面建议见 [MQTT 参数清单](MQTT-CAPABILITIES.md)。

## 设备切换与余量提醒

首页设备卡片保持原来的短宽度，长名称循环滚动。4 台以内短按按配置顺序切换，超过 4 台短按打开滚动列表；长按始终打开列表，点击条目后切换。列表只包含已配置设备，绿色圆点表示当前设备；点击关闭或外部区域退出。长按松手不会再触发短按切换。列表采用单个绘制层并异步释放，设备连接在主循环排队处理，不在触摸回调中销毁首页。

耗材编号 A1–A4 使用淡蓝色；有效余量 `≤10%` 红色、`11–20%` 黄色、`>20%` 正常白色。未知余量仍不显示百分比。材料名称颜色不变。
