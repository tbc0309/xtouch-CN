# MQTT 支持范围与操作

简体中文 | [English](MQTT-CAPABILITIES.en.md) | [README](../README.md)

数据和控制指令取决于打印机型号及固件。能显示机型图片，不代表该机型的全部控制功能已适配。

## 当前支持

| 数据 | 处理方式 |
| --- | --- |
| 打印状态与进度 | 显示设备上报状态和进度 |
| 喷嘴、热床 | 当前温度、目标温度及设温操作 |
| 风扇 | 传统风扇显示与控制，速度档不作为 RPM |
| AMS | 材料、实际颜色、有效剩余百分比、湿度与温度 |
| 外置耗材 | 独立选择与颜色显示 |
| 仓温 | 兼容新旧温度遥测，不开放主动仓温加热 |
| 机型 | 按设备报告识别型号并显示图片 |

湿度等级不作为百分比，缺失或无效读数不显示为零。识别与显示细节见 [机型显示](PRINTER-MODELS.md)。

## 尚未完整适配

多喷头路由、主动仓温加热、AMS 干燥及新热床/喷嘴打包字段，仍需独立适配和实机验证。耗材克数应使用实际 `remain_g`，不能按固定 1kg 推算。传统单喷头控制指令不能直接套用到所有新机型。

## 协议参考

BambuStudio 解析器：[AMS](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceCore/DevFilaSystem.cpp)、[喷头](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceCore/DevExtruderSystem.cpp)、[热床](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceCore/DevBed.cpp)、[风扇](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceCore/DevFan.cpp)、[仓温](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceCore/DevChamber.cpp)。

## 耗材操作

- “外置”固定在右侧；左侧 AMS 栏同时显示最多 4 个单元，更多时左右拖动。点击只选择，拖动不切换耗材。标准四槽 AMS 的显示缓存最多容纳 8 个单元；AMS HT、混合 AMS Lite、双喷头等不同映射不能当作普通四槽单元直接套用。运动控制仍限已核对的传统单喷头机型与单元 0–3。
- 先选择 AMS 槽位，或右上角的“外置”。蓝框表示界面选择，绿框仍表示打印机实际装入的槽位；选择本身不会换料。再次点击同一槽位取消选择。切换打印机时清除选择。
- 未选择时进料、退料和重读禁用。选中后按钮显示来源，例如“A1 进料”。进料会调用打印机的换料流程，可能先退出当前耗材、加热并移动打印头。
- 退料只允许操作当前实际装入的来源，不能通过选择其他槽位退出不相关的耗材。
- 重读调用 `ams_get_rfid`，仅用于 AMS 标签识别；需先退出已装入的耗材，非 RFID 耗材不会变成自动识别。外置没有该操作。
- 进料温度从槽位上报的 `nozzle_temp_min` / `nozzle_temp_max` 计算；没有有效范围时使用已设定的喷嘴目标温度，仍没有温度则提示“先设温度”，不猜测材料。移除耗材或 AMS 会清除对应温度与耗材缓存。
- 外置进料使用独立的外置来源 `254`；没有耗材预设温度时需先设喷嘴温度，再按打印机提示手动送入。它不等同于 AMS 自动送入。外置退料仍只处理实际装入的外置耗材。
- 左列上下箭头仍是喷嘴挤出机的回抽/挤出 10 mm，与界面选中的 AMS 槽位无关；中间是喷嘴设温入口。
- 进料、退料和重读均先确认；确认后重新检查设备、槽位、在线、打印与 AMS 状态。打印、暂停、准备、AMS 忙碌或读取标签期间禁用。未知机型和未适配的多喷头控制不直接套用传统单喷头路由。

编码依据 [Bambu Studio DeviceManager.cpp](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceManager.cpp) 的 `command_ams_change_filament` 与 `command_ams_refresh_rfid2`。不同打印机固件的执行结果仍需对应实机验证。返回 `84033543` 表示固件拒绝未签名控制，可在打印机屏幕核对 LAN Only 与 Developer Mode（会影响云端连接）。固件按本机请求编号匹配回复，显示拒绝原因，不将其他客户端回复或单纯收到命令视为执行成功。

## 耗材操作确认

| 进料 | 退料 | 重读 RFID |
| --- | --- | --- |
| ![filament-load-confirm](images/filament-load-confirm.png) | ![filament-unload-confirm](images/filament-unload-confirm.png) | ![filament-rfid-confirm](images/filament-rfid-confirm.png) |
