const words = {
  errorLookup: ["故障码查询", "Error lookup"],
  projectLink: ["项目地址", "Project"],
  eyebrow: ["在线安装 · 扫码配网", "INSTALL · CONNECT · GET STARTED"],
  title: ["在线安装 xTouch-CN", "Install xTouch-CN online"],
  intro: [
    "支持 2.8 寸 CYD 电阻触摸主板，刷写后扫码配网，在网页管理打印机。",
    "For the 2.8-inch resistive-touch CYD board. Install, scan to join Wi-Fi, and manage printers in your browser.",
  ],
  step1: ["生成连接配置", "Create configuration"],
  step2: ["刷入本项目固件", "Install the firmware"],
  step3: ["插卡开机，完成初始化", "Insert SD and initialize"],
  configTab: ["配置生成", "Configuration"],
  flashTab: ["在线刷写", "Web installer"],
  configTitle: ["准备你的 xtouch.json", "Prepare your xtouch.json"],
  privacy: [
    "填写内容仅在当前页面处理，不上传、不自动保存。",
    "Entries stay in this page: no uploads or automatic storage.",
  ],
  import: ["导入已有配置", "Import configuration"],
  wifi: ["Wi-Fi 连接", "Wi-Fi connection"],
  ssid: ["Wi-Fi 名称（SSID）", "Wi-Fi name (SSID)"],
  wifiPassword: ["Wi-Fi 密码", "Wi-Fi password"],
  openWifi: [
    "开放网络可留空；不会改变前后空格。",
    "Leave empty for an open network. Spaces are preserved.",
  ],
  printers: ["打印机", "Printers"],
  add: ["＋ 添加打印机", "＋ Add printer"],
  advanced: ["高级参数", "Advanced options"],
  timeout: ["Wi-Fi 连接等待 / 毫秒", "Wi-Fi connection wait / ms"],
  coldboot: ["冷启动等待 / 毫秒", "Cold-start delay / ms"],
  advancedHint: [
    "通常保持默认 5000 毫秒即可，范围 0–30000。",
    "Normally keep the 5000 ms defaults. Range: 0–30000.",
  ],
  ready: ["保存到 SD 卡根目录", "Save to the SD card root"],
  downloadHint: [
    "文件名为 xtouch.json。首次导入后可脱卡运行。",
    "File name: xtouch.json. SD is optional after the first import.",
  ],
  preview: ["检查配置", "Check configuration"],
  download: ["下载 xtouch.json", "Download xtouch.json"],
  jsonPreview: ["配置预览", "Configuration preview"],
  masked: [
    "预览隐藏密码和访问码；下载文件包含实际填写值，请妥善保管。",
    "Preview masks passwords and access codes. The downloaded file contains actual values: keep it private.",
  ],
  flashTitle: ["安装 xtouch-CN", "Install xtouch-CN"],
  flashIntro: [
    "使用数据线连接 CYD 主板，在浏览器中完成刷写。",
    "Connect the CYD board with a data cable and install in your browser.",
  ],
  usbTag: ["USB 在线安装", "USB WEB INSTALL"],
  usbTitle: ["连接并刷写", "Connect and install"],
  usb1: [
    "电脑使用 Chrome 或 Edge，关闭占用串口的软件。",
    "Use desktop Chrome or Edge. Close software using the serial port.",
  ],
  usb2: [
    "连接支持的 CYD 主板，加载刷写工具，再选择它的串口。",
    "Connect the supported CYD board, load the installer, then select its serial port.",
  ],
  usb3: [
    "按提示安装，开机完成触摸校准，再扫码配网。",
    "Follow the installation dialog, calibrate the screen, then scan the Wi-Fi QR code.",
  ],
  erase: [
    "保留设置时不要勾选 Erase device（擦除设备）；勾选会清空内部配置和校准。刷写期间保持供电。",
    "To keep settings, leave “Erase device” unchecked. Erasing removes internal configuration and calibration. Keep power connected.",
  ],
  loadInstaller: ["加载刷写工具", "Load installer"],
  installerEnglish: [
    "刷写工具的连接、进度和确认窗口由 ESP Web Tools 提供，目前为英文。",
    "Connection, progress and confirmation dialogs are supplied by ESP Web Tools in English.",
  ],
  sdTag: ["已有原版 / 离线升级", "ORIGINAL FIRMWARE / SD UPDATE"],
  sdTitle: ["也可以使用 SD 卡", "SD installation is also available"],
  sdSupport: [
    "支持原版 xTouch 直接通过 SD 卡升级为 xtouch-CN。",
    "Original xTouch firmware can upgrade directly to xtouch-CN using SD.",
  ],
  sd1: [
    "下载应用固件 xtouch-2.8.bin，改名为 firmware.bin。",
    "Download the application xtouch-2.8.bin and rename it firmware.bin.",
  ],
  sd2: [
    "放到 SD 卡根目录，保留连接配置和校准，移除旧 xtouch/ota.json。",
    "Place it in the SD root. Keep configuration and calibration; remove old xtouch/ota.json.",
  ],
  sd3: [
    "插卡开机，等待更新和自动重启。",
    "Insert SD and power on. Wait for installation and automatic restart.",
  ],
  downloadApp: ["下载 SD 升级固件", "Download SD update"],
  sdNote: [
    "不要把 full 完整镜像改名用于 SD 升级。",
    "Do not use the full USB image for SD updates.",
  ],
  otaHint: [
    "已安装 CN：可在设置开启 OTA，或连续点击版本号三次检查更新。",
    "Already on CN? Enable OTA in Settings or tap the version three times to check for updates.",
  ],
  boardHint: [
    "仅支持 ESP32-2432S028R：ILI9341 显示 + XPT2046 电阻触摸，4MB Flash。",
    "For ESP32-2432S028R: ILI9341 display, XPT2046 resistive touch and 4MB Flash.",
  ],
  guide: ["使用文档 ↗", "Documentation ↗"],
  blog: ["项目介绍 ↗", "Project introduction ↗"],
  unofficial: [
    "个人开源改版，与拓竹官方及硬件商家无关联。不提供技术支持，遇到问题请查阅文档或询问 AI。",
    "An independent open-source modification, not affiliated with Bambu Lab or hardware vendors. No technical support is provided; consult the documentation or AI.",
  ],
  name: ["设备名称（可选）", "Device name (optional)"],
  nameHint: [
    "建议英文或数字，任意汉字不保证覆盖。",
    "English/numbers recommended; arbitrary Chinese glyphs are not guaranteed.",
  ],
  host: ["打印机 IP / 主机名", "Printer IP / hostname"],
  accessCode: ["局域网访问码", "LAN access code"],
  serialNumber: ["打印机序列号", "Printer serial number"],
  printerModel: ["机型 / 协议代码", "Model / protocol code"],
  unknown: ["其他 / 自动识别", "Other / automatic"],
  default: ["开机默认", "Startup default"],
  up: ["上移", "Move up"],
  down: ["下移", "Move down"],
  remove: ["删除", "Remove"],
  printer: ["打印机", "Printer"],
  count: ["台，最多 100 台", " / 100 printers"],
  importConfirm: [
    "导入会替换当前表单内容，是否继续？",
    "Import replaces the current form. Continue?",
  ],
  imported: [
    "配置已导入，请核对后下载。仅保留 Wi-Fi、打印机及启动参数，空槽位已移除。",
    "Imported. Review before downloading. Wi-Fi, printer and startup fields are retained; empty slots are removed.",
  ],
  importInvalid: [
    "配置文件不是有效的 xtouch JSON。",
    "Not a valid xtouch configuration file.",
  ],
  fileSize: [
    "配置文件过大，最大 33792 字节。",
    "Configuration file exceeds 33792 bytes.",
  ],
  valid: ["配置检查通过。", "Configuration check passed."],
  downloaded: [
    "已生成 xtouch.json，请保存到 SD 卡根目录。",
    "xtouch.json generated. Save it to the SD card root.",
  ],
  ssidInvalid: [
    "Wi-Fi 名称不能为空，最多 32 字节。",
    "Wi-Fi name is required (maximum 32 bytes).",
  ],
  passwordInvalid: [
    "Wi-Fi 密码最多 64 字节。",
    "Wi-Fi password must not exceed 64 bytes.",
  ],
  delayInvalid: [
    "等待时间必须为 0–30000 的整数。",
    "Delay must be an integer from 0 to 30000.",
  ],
  countInvalid: ["请配置 1–100 台打印机。", "Configure 1–100 printers."],
  activeInvalid: [
    "请选择有效的开机默认设备。",
    "Choose a valid startup printer.",
  ],
  printerInvalid: ["打印机条目无效。", "Invalid printer entry."],
  nameInvalid: [
    "设备名称最多 63 字节。",
    "Device name must not exceed 63 bytes.",
  ],
  hostInvalid: [
    "请输入有效 IP 或主机名（最多 63 字节，不含 http://、端口或路径）。",
    "Enter an IP or hostname, up to 63 bytes, without http://, port or path.",
  ],
  codeInvalid: [
    "访问码必须为 8 个非空白 ASCII 字符。",
    "Access code must be 8 non-whitespace ASCII characters.",
  ],
  serialInvalid: [
    "序列号需为 1–31 位英文字母或数字。",
    "Serial number must contain 1–31 letters or digits.",
  ],
  modelInvalid: [
    "机型代码最多 31 字节。",
    "Model code must not exceed 31 bytes.",
  ],
  sizeInvalid: [
    "配置超过固件的 32KB 限制，请缩短名称或减少设备。",
    "Configuration exceeds the firmware’s 32KB limit. Shorten names or remove printers.",
  ],
  unavailable: [
    "刷写资源暂不可用，请刷新重试或前往 Releases。",
    "Installer assets are unavailable. Reload or visit Releases.",
  ],
  unsupported: [
    "请在电脑 Chrome / Edge 中通过 HTTPS 打开此页面。",
    "Open this page over HTTPS in desktop Chrome or Edge.",
  ],
  loading: ["正在加载刷写工具…", "Loading installer…"],
  connect: ["选择串口并连接", "Select port and connect"],
  loadFailed: [
    "工具加载失败，请检查网络后重试，或使用 SD 卡升级。",
    "Installer could not load. Check the network and retry, or use SD installation.",
  ],
  version: ["固件", "Firmware"],
};
Object.assign(words, {
 legacyConfig: ["旧版配置生成（仅适用于 v0.9.215）", "Legacy configuration generator (v0.9.215 only)"],
 setupTitle: ["扫码配网与初始化", "Wi-Fi and setup"],
 setup1: ["首次开机按提示完成触摸校准。", "Calibrate the touchscreen on first boot."],
 setup2: ["扫描屏幕二维码连接 XTOUCH-CN 热点；密码 12345678。", "Scan the screen QR code to join the XTOUCH-CN hotspot. Password: 12345678."],
 setup3: ["打开 192.168.4.1，连接家庭 2.4GHz Wi-Fi。", "Open 192.168.4.1 and join your 2.4GHz Wi-Fi."],
 setup4: ["在设置打开网页管理扫码进入后台；初始密码 12345678，首次登录修改后添加局域网或云设备。", "Open Web management in Settings and scan its QR code. Sign in with 12345678, change it, then add LAN or cloud printers."]
});
function browserLanguage(value=navigator.languages?.[0] || navigator.language || "en") {return /^zh(?:[-_]|$)/i.test(value)?"zh":"en";}
let lang=browserLanguage(), metadata=null, installerReady=false;
const $=id=>document.getElementById(id), t=key=>(words[key]||[key,key])[lang==="en"?1:0];
function text(tag,value,className){const el=document.createElement(tag);el.textContent=value;if(className)el.className=className;return el;}
function applyLanguage(){
 $("language").value=lang;document.documentElement.lang=lang==="zh"?"zh-CN":"en";
 document.title=lang==="zh"?"xTouch-CN · 在线刷写":"xTouch-CN · Web installer";
 document.querySelectorAll("[data-i18n]").forEach(el=>el.textContent=t(el.dataset.i18n));
 $("guide").href=`https://github.com/tbc0309/xtouch-CN/blob/main/README${lang==="en"?".en":""}.md`;
 if(metadata)$("version").textContent=`${t("version")} v${metadata.version}`;
 if(installerReady && $("connect-installer"))$("connect-installer").textContent=t("connect");
}
$("language").addEventListener("change",()=>{lang=$("language").value;applyLanguage();});
async function initFirmware() {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 10000);
  try {
    const response = await fetch("firmware.json", {
      cache: "no-cache",
      signal: controller.signal,
    });
    if (!response.ok) throw Error();
    const data = await response.json();
    if (
      !/^\d+\.\d+\.\d+$/.test(data.version) ||
      !/^firmware\/xtouch-2\.8-full\.bin$/.test(data.fullPath) ||
      !/^https:\/\/github\.com\/tbc0309\/xtouch-CN\/releases\/download\/[\d.]+\/xtouch-2\.8\.bin$/.test(
        data.appUrl,
      )
    )
      throw Error();
    metadata = data;
    $("load-installer").disabled = false;
    $("version").textContent = `${t("version")} v${data.version}`;

  } catch {
    $("version").textContent = "Releases";
    $("load-installer").disabled = true;
    $("installer-status").textContent = t("unavailable");
  } finally {
    clearTimeout(timer);
  }
}
$("load-installer").addEventListener("click", async () => {
  if (!window.isSecureContext || !("serial" in navigator)) {
    $("installer-status").textContent = t("unsupported");
    return;
  }
  if (!metadata) return;
  $("load-installer").disabled = true;
  $("installer-status").textContent = t("loading");
  let timer;
  try {
    await Promise.race([
      import("./vendor/install-button.js"),
      new Promise((_, reject) => {
        timer = setTimeout(() => reject(Error("installer timeout")), 20000);
      }),
    ]);
    await customElements.whenDefined("esp-web-install-button");
    const installer = document.createElement("esp-web-install-button");
    installer.manifest = "manifest.json";
    const connect = text("button", t("connect"), "button primary");
    connect.id = "connect-installer";
    connect.slot = "activate";
    installer.append(connect);
    $("installer").replaceChildren(installer);
    $("installer-status").textContent = "";
    installerReady = true;
  } catch {
    $("load-installer").disabled = false;
    $("installer-status").textContent = t("loadFailed");
  } finally {
    clearTimeout(timer);
  }
});
applyLanguage();
initFirmware();
