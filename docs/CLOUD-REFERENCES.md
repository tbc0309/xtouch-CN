# 云端连接参考与实现边界

本文列出云端连接实现的参考来源与兼容边界。

| 项目（核对版本） | 参考实现 | 在 xTouch 中采用的部分 |
| --- | --- | --- |
| [Bambu-Lab-Cloud-API](https://github.com/coelacant1/Bambu-Lab-Cloud-API/tree/29458fd) | Python，账号/密码、验证码、Token 和 MQTT；并非全部读取/控制功能都有同等实测覆盖 | 登录请求字段与 MQTT 账号结构 |
| [bambu-lab-cloud-api-cn](https://github.com/E-R-Butch/bambu-lab-cloud-api-cn/tree/1a28624) | 增加中国区 API/MQTT 域名；auth.py 仍主要实现邮箱验证码，不能因此认定短信流程完整 | 区域隔离；不照搬邮箱限制 |
| [mqtt-bambu](https://github.com/mqtt-home/mqtt-bambu/tree/0ed09e4) | Go 后端、Web UI、共享账号会话、设备发现、JWT 到期判断；需要服务器环境 | 账号与设备分开管理、共享 Token、状态区分 |
| [ha-bambulab](https://github.com/greghesp/ha-bambulab/tree/0e027ff) | 密码登录、手机号短信/邮箱验证码分流、Cloudflare/CSRF 错误分类、MQTT 与大量状态解析 | 短信/邮箱流程与错误分类；不将其 Python 网络依赖移植到 ESP32 |
| [OpenBambuAPI](https://github.com/Doridian/OpenBambuAPI/tree/cc383a2) | 协议文档，描述密码与验证码互斥、Token、MQTT 主题和消息字段 | 接口与解析交叉核对；它不是可直接移植的完整 SDK |
| [BambuHelper](https://github.com/Keralots/BambuHelper) | ESP32 登录任务、Token 管理、区域配置及设备绑定；UI 邮箱字段与后端通用 account 字段需要区分 | 面向小内存设备的账号管理方式；可选设备密钥加密密码 |

## 登录与状态

- 密码登录发送 `account`、`password`、`apiError`，手机号不使用邮箱类型输入框。
- 验证码登录发送 `account`、`code`，不同时发送密码。
- 中国手机号验证码按 ha-bambulab 使用 `https://bambulab.cn/api/v1/user-service/user/sendsmscode`；邮箱使用区域 API 的 `/v1/user-service/user/sendemail/code`。发送验证码由用户主动点击，60 秒限制独立于其它账号操作。
- `verifyCode` / `tfaKey` 决定验证码输入方式；TOTP 使用网站 CSRF 会话。验证码与 TOTP 尚待真实账号验证。
- 403 区分 Cloudflare 拦截、缺少 Cookie 和其它拒绝；401 表示凭据不被接受，429 表示请求频率限制。没有响应正文证据时，不能把 403 直接认定为密码错误或 Cloudflare。
- Token 校验和设备列表都需要真实请求成功。UI 仅显示“已配置 Token”时不代表已验证。
- 清除本地登录移除本地配置中的 Token，保存重启后生效；不等于撤销拓竹服务器 Token，也不删除历史备份。

## MQTT 与设备

按区域选择 `cn.mqtt.bambulab.com:8883` 或 `us.mqtt.bambulab.com:8883`。用户名 `u_<uid>`、密码 Access Token，主题 `device/<serial>/report` 和 `device/<serial>/request`。固件仅加载当前账号 Token。账号认证拒绝后延长重试，避免连续请求。云端命令会显示请求反馈；服务端或打印机拒绝不等于成功执行。

## 验证范围

| 功能 | 范围 |
| --- | --- |
| Token 验证、账号设备发现与 Cloud MQTT | 中国区 P1S 实机验证 |
| 配置保存、无卡运行与 SD 同步 | CYD 实机验证 |
| 密码登录 | 支持请求流程；可能被 Cloudflare 拦截 |
| 验证码、TOTP、全球区账号 | 接口已实现，真实账号兼容性仍需验证 |
| AMS、双喷头与新机型控制 | 解析和命令流程经过模拟验证，具体机械动作需对应实机验证 |

云端连接成功不代表控制命令均获授权，详见 [云端控制](CLOUD-CONTROL.md)。后台、凭据与网络边界见 [安全检查](SECURITY-REVIEW.md)。
