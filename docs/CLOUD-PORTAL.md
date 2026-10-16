# 网页管理与云账号

xTouch-CN 1.0.0。适用于 2.8 寸 ESP32 CYD，无 PSRAM，保留原有 SD/OTA 分区。

## 初始化与设备管理

1. 无 Wi-Fi 配置时，扫描屏幕热点二维码，打开 `http://192.168.4.1/` 配网。
2. 连接家庭 Wi-Fi 后访问设备地址，或在屏幕设置里打开“网页管理”扫码。点击二维码下方的“返回设置”返回。
3. 初始管理密码为 `12345678`，首次登录必须修改。管理账号固定为管理员；新登录替换旧会话。
4. “添加局域网设备”用于填写 LAN 地址、访问码和序列号。云端设备从对应账号的关联列表勾选加入。
5. 完成修改后点击标签栏右侧“保存并重启”。编辑和排序不会切换屏幕当前设备。

屏幕与网页支持简体中文、繁体中文、英语、法语、德语、俄语、日语和韩语。网页默认跟随浏览器语言。

## 拓竹账号

每个账号独立选择区域与登录方式：

- **账号登录**：手机号或邮箱加密码；也可直接选择短信／邮箱验证码登录，无需密码。
- **Access Token**：导入 Token，验证用户 ID 后自动读取关联设备。

密码登录返回 `verifyCode` 时自动显示验证码输入框；返回 `tfaKey` 时显示身份验证器验证码输入框。验证码请求设有冷却，防止重复发送。验证码与 TOTP 的真实账号流程仍需要分别验证。

登录成功后折叠账号信息，显示状态。关联设备列表独立显示，可勾选加入或取消。已保存且有效的账号在打开账号管理时自动获取设备；临时网络失败不会直接判为账号失效。清除本地登录不等于撤销拓竹服务器上的 Token。

开机后依次检查账号与设备归属，之后每六小时复查；MQTT 认证拒绝也会触发复查。仅在明确认证失败、且已加密保存密码时尝试重新登录。需要验证码、Cloudflare 拦截或重新登录失败时，停止自动续期并提示人工登录。不能保证 Token 永不过期。

LAN、Token、账号登录分别使用青蓝、紫色、金色标识。失效设备在屏幕上被选择时显示对应账号的管理二维码，浏览器打开后定位该账号。

## 密码与备份

- 管理密码使用随机盐和 PBKDF2-SHA256 校验值，不可还原为明文。
- 云密码默认不保存。勾选后使用 AES-256-GCM，随机 nonce，并绑定设备、账号标识、区域和账号名称。
- 随机密钥保存在设备 NVS，不导出到 SD 或配置备份；配置接口不返回密码或密文。
- 普通重启、SD 应用升级和 OTA 保留密钥。清空 Flash 后的全新安装、设备重置生成新密钥。保留 NVS 的串口写入不会自动换密钥。
- 重置保留 SD 配置；再次导入后，旧云密码无法解密，需重新输入。SD 中的 Token、Wi-Fi 密码和 LAN 访问码仍属于敏感配置，应妥善保管。

设备未启用硬件 Flash 加密，NVS 密钥不构成防物理拆机读取的保护。网页管理使用本地 HTTP，只应在可信网络中操作。

## 维护与重置

维护页支持下载配置、修改管理密码、上传应用固件和重置。上传使用 `xtouch-2.8.bin`，不使用合并镜像；检查 ESP32 镜像头、分区大小和 Update 校验，成功后重启。

设置页版本号：连续三击检查 OTA；连续八击显示重置确认。确认重置后清除内部配置与管理密码、更新密码密钥，再从 SD 导入配置。无卡时重新初始化。开机 OTA 由设置开关控制，更新清单检查最长 15 秒。

## Cloudflare 与云端控制

固件验证云端 HTTPS/MQTT 证书，并为网站 POST 获取 CSRF Cookie。Cloudflare 拒绝、验证码、401 和限流分别处理，不将所有 403 当成密码错误。

[BambuHelper 的电脑端实现](https://github.com/Keralots/BambuHelper/blob/main/tools/companion/bambu_core.py) 使用共享浏览器 TLS 会话、Cookie 和 CSRF；其 `curl_cffi` 依赖不能直接放进这台 ESP32。设备直接登录被拦截时，可从官方网页取得 Token 后导入。不要关闭证书验证。

Cloud MQTT 状态订阅和灯光控制已经实测通过。耗材重读使用同一 MQTT 请求协议，并显示发送、接受、拒绝与超时；本次 P1S 的云端重读返回 `84033543`，未被接受。Token 有效不代表所有控制命令获准；需使用支持的 LAN 控制或官方应用。

## English

xTouch-CN supports eight interface languages, LAN and Cloud profiles, QR Wi-Fi setup, and one authenticated browser session. Add LAN profiles manually; select Cloud devices from each account. Save edits with **Save & restart**.

Accounts support password, passwordless SMS/email code, authenticator challenge and Token import. Verification fields follow the server response. Live code/TOTP flows still require verification. Login fetches linked devices automatically. Saved cloud passwords are optional and use device-bound AES-256-GCM with a random NVS key. OTA retains that key; reset and erased-flash installation replace it. SD imports after reset require re-entering encrypted passwords. Backups still contain sensitive Wi-Fi/access-code/Token data.

Accounts are checked at boot and every six hours. Rejected authentication may attempt renewal once with a remembered password; verification or Cloudflare blocks require user action. Device management shows distinct LAN/Token/account identifiers. Invalid profiles open the corresponding account management QR code.

Three version taps check OTA; eight taps open reset confirmation. Maintenance supports application firmware upload and reset. Cloud telemetry works, but the tested P1S rejected cloud RFID reread with error `84033543`; cloud control support must not be assumed.

See [cloud reference and hardware results](CLOUD-REFERENCES.md).
