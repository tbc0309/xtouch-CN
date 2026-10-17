# 云端控制与命令授权

账号或 Access Token 可用于云端 MQTT 登录和设备状态订阅。能够订阅状态不等于打印机接受所有控制命令；固件没有按 Cloud 类型禁用 RFID 重读。

P1S 实机验证中，空闲、未装载状态下的 `ams_get_rfid` 请求收到 `err_code: 84033543`。

[BambuStudio 的实现](https://github.com/bambulab/BambuStudio/blob/master/src/slic3r/GUI/DeviceManager.cpp) 使用同样的 `ams_get_rfid`、`ams_id`、`slot_id`，再经网络代理的 `send_message` 发往云端。[网络插件签名研究](https://github.com/ClusterM/open-bamboo-networking/blob/master/research/10.04-mqtt-signing.md) 把此类错误对应到缺少或无效的命令签名。官方客户端的成功不能证明裸 Token MQTT 命令也获得相同授权。

当前版本未实现官方授权的签名通道，也不携带第三方应用的共享私钥。不能通过更换账号登录方式、取消按钮禁用或更改 JSON 字段解决这一实测拒绝。支持的局域网开发者连接可独立验证；云端仍需另行验证可合法使用的签名或官方连接接口，未验证前不承诺远程控制可用。

## 各类命令与实测边界

| 类别 | 当前实现与限制 |
| --- | --- |
| 状态、温度、AMS、打印进度 | LAN 和 Cloud 使用同一套状态解析；连接、状态订阅及 `get_version` 已验证 |
| 灯光 `system/ledctrl` | 当前 P1S 云端返回成功；成功回复不保证灯光状态已变化 |
| 温度、风扇、速度、轴移动 | 使用 `print/gcode_line` 等控制；只读 `M105` 云端实测同样返回 `84033543`，因此不能承诺这些控制可用 |
| 暂停、恢复、停止 | 独立的 `print` 命令，权限随机型与固件变化；实际打印过程中的操作尚未完成实机验证 |
| AMS 重读、进退料、烘干 | 分别使用 RFID、送料、烘干命令；重读已实测拒绝，其它命令的结果需单独验证；当前 P1S 没有新 AMS 烘干硬件 |
| 多喷头与喷嘴仓 | 使用独立控制命令；需要对应机型、硬件和命令授权，模拟回复只验证协议和 UI，不证明云端授权 |

授权错误不是 Token 过期，也不是设备离线。所有已发送的 `print` 和 `system` 控制回复都核对请求序号、命令和当前设备；短时间连续控制保留最多八个待应答项，切换设备与断线时清空。AMS 与喷头命令继续沿用各自的应答跟踪。

对于已确认的 `84033543`，屏幕先显示原始错误码，下方显示“部分功能仅支持局域网模式，且开启开发者。”；**需要改用 LAN 设备配置连接，单纯切换账号密码／Token 登录无效**。其它错误保留原始错误码，不一概提示重新登录或开启开发者模式。

## 参考项目的处理

- [ha-bambulab](https://github.com/greghesp/ha-bambulab/blob/main/custom_components/bambu_lab/pybambu/models.py) 根据 `mqtt_signature_required` 限制部分功能，普通云端 MQTT 发布不是签名授权通道。
- [BambuHelper](https://github.com/Keralots/BambuHelper)、[mqtt-bambu](https://github.com/mqtt-home/mqtt-bambu) 和 [Python Cloud API](https://github.com/coelacant1/Bambu-Lab-Cloud-API)／[中国区版本](https://github.com/E-R-Butch/bambu-lab-cloud-api-cn) 的常规命令同样通过 MQTT 发布；代理／兼容层把 API 接到 MQTT，不会自动取得打印机要求的命令签名。旧固件或不同机型可用不能证明当前机型也可用。
- BambuStudio 经官方网络插件处理授权。外部主机运行官方插件／连接软件是另一套部署方式，不能直接把桌面二进制放进 ESP32 固件。
- 网络插件研究描述证书及签名机制，但没有可直接放入本固件的通用授权凭据。本项目不内置提取出的共享应用私钥。当前版本不支持该签名授权通道。

屏幕错误码也可在[在线查询页](https://tbc0309.github.io/xtouch-CN/errors.html)检索。实际能力以打印机确认和状态变化为准。
