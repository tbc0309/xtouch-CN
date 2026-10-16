# Access Token 获取与导入

Access Token 用于连接拓竹云账号，与打印机的局域网访问码不同。请在电脑浏览器操作。

## 获取 Token

| 官网 | 地址 |
| --- | --- |
| 中国区官网（中文） | [bambulab.cn](https://bambulab.cn/) |
| 国际官网（英文） | [bambulab.com](https://bambulab.com/) |

1. 根据账号所属区域打开上方对应官网，完成登录。
2. 按 **F12** 打开开发者工具，选择 **Application（应用）**。找不到时点击工具栏的 `»`。
3. 左侧展开 **Storage → Cookies**，选择当前拓竹网站域名。
4. 搜索名为 **token** 的 Cookie，复制它的 **Value（值）**，不要复制整行或 Cookie 名称。

如果没有找到，确认官网登录成功后刷新页面，再检查相应域名。不要使用第三方网页收集 Token，也不需要在控制台粘贴脚本。

## 导入 xTouch-CN

1. 打开设备网页管理，进入 **拓竹账号 → 添加云账号**。
2. 选择与官网账号一致的区域，切换到 **Access Token**。
3. 粘贴 Token，点击 **验证 Token**。验证成功后会显示关联设备。
4. 勾选要添加的设备，点击右上角 **保存并重启**。

已存在的账号可以展开原卡片更新 Token，无需重复添加。Token 过期或失效时，重新登录官网并重复以上步骤。

Token 相当于账号凭据，不要发到聊天、截图或公开仓库。配置备份也包含 Token，应妥善保管。Token 登录成功不代表所有控制命令均被打印机允许，详见 [云端控制说明](CLOUD-CONTROL.md)。

## English

| Official website | Address |
| --- | --- |
| China (Chinese) | [bambulab.cn](https://bambulab.cn/) |
| Global (English) | [bambulab.com](https://bambulab.com/) |

1. Open the official website matching your account region and sign in.
2. Open **F12 → Application → Storage → Cookies**, select the Bambu website domain, and copy the **Value** of the cookie named **token**.
3. In xTouch-CN, open **Bambu accounts → Add cloud account → Access Token**. Select the matching region, paste the Token and verify it.
4. Select the linked printers, then choose **Save & restart**. For renewal, update the existing account card.

Keep Tokens and configuration backups private. If the cookie is absent, confirm you are signed in and refresh the website. No console script is required.

Browser-cookie retrieval follows the [BambuHelper Token guide](https://bambuhelper.stolaris.dev/cloud-token.html). Website layouts and login policies may change.
