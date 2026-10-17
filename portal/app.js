const words = {
  importBackup: "Import backup",
  loginTitle: "Management sign-in",
  passwordLabel: "Password",
  firmware: "Firmware",
  installFirmware: "Upload & upgrade",
  appFirmware: "Application firmware only: xtouch-2.8.bin.",
  resetDevice: "Reset",
  resetHint:
    "Reset internal settings and management password. Keep SD backups; saved cloud passwords must be re-entered.",
  project: "xTouch-CN project",
  adminAccount: "Management account: administrator · password configured",
  currentPrinter: "Current device",
  connection: "Connection",
  freeMemory: "Free memory",
  running: "Uptime",
  configuredDevices: "Configured devices",
  wifiSetup: "Wi-Fi setup",
  ssid: "Wi-Fi name",
  scan: "Scan Wi-Fi",
  wifiPassword: "Wi-Fi password",
  connect: "Save and connect",
  wifiHint: "Restarts after saving.",
  login: "Sign in",
  changePassword: "Change password",
  newPassword: "New password (8–64 bytes)",
  confirmPassword: "Confirm password",
  savePassword: "Save password",
  devices: "Devices",
  settings: "Display & Wi-Fi",
  maintenance: "Maintenance",
  logout: "Sign out",
  addPrinter: "Add LAN device",
  accounts: "Bambu accounts",
  addAccount: "Add cloud account",
  screenLanguage: "Display language",
  brightness: "Brightness / 255",
  sleep: "Screen sleep / minutes (0 disables)",
  wifiKeep: "Wi-Fi password (blank to keep)",
  defaultDevice: "Startup device",
  wake: "Wake when printing starts",
  ota: "Check OTA on boot",
  backupHint:
    "Backups contain Wi-Fi credentials, access codes and cloud tokens. Keep them private.",
  backup: "Export backup",
  saveRestart: "Save & restart",
};
// localeWords is prepended by scripts/embed_portal.py from resources/portal-locales.json.
const languageCodes = ["zh", "zh-TW", "en", "fr", "de", "ru", "ja", "ko"];
function browserLanguage(value) {
  value = (value || "en").replaceAll("_", "-");
  if (/^zh/i.test(value)) return /(?:Hant|TW|HK|MO)/i.test(value) ? "zh-TW" : "zh";
  return languageCodes.find((code) => value.toLowerCase().split("-")[0] === code) || "en";
}
let lang = browserLanguage(navigator.languages?.[0] || navigator.language),
  csrf = "",
  cfg = null,
  selected = 0,
  info = {},
  poll = null,
  cloudBusy = false;
// Invalidate asynchronous management work when authentication changes.
let cleanState = "",
  sessionEpoch = 0;
let labTools = false;
function enableLabTools(e) {
  if (!cfg || $("app").hidden) return;
  if (e.ctrlKey && (e.key.toLowerCase() === "t" || (e.shiftKey && e.key.toLowerCase() === "y"))) {
    e.preventDefault();
    labTools = !labTools;
    accounts();
    message(labTools ? t("模拟器测试入口已开启", "Simulator tools enabled") : "");
  }
}
document.addEventListener("keydown", enableLabTools);
function collapseAccounts() {
  for (const a of cfg?.cloudAccounts || []) a.expanded = false;
}
function enterLogin() {
  sessionEpoch++;
  collapseAccounts();
  clearInterval(poll);
  cloudBusy = false;
  csrf = "";
  cfg = null;
  cleanState = "";
  $("accounts").replaceChildren();
  $("profiles").replaceChildren();
  $("editor").replaceChildren();
  $("logout").hidden = true;
  $("app").hidden = true;
  $("change").hidden = true;
  $("login").hidden = false;
  $("guestFeedback").append($("status"));
  message("");
  for (const b of document.querySelectorAll("#addAccount,#addSimulator,#save,#language"))
    b.disabled = false;
}
const $ = (id) => document.getElementById(id),
  t = (zh, en) => (lang === "zh" ? zh : localeWords[en]?.[languageCodes.indexOf(lang)] || en),
  defaults = new Map(
    [...document.querySelectorAll("[data-t]")].map((el) => [
      el.dataset.t,
      el.textContent,
    ]),
  );
function language() {
  document.documentElement.lang = lang === "zh" ? "zh-CN" : lang;
  $("language").value = lang;
  for (const el of document.querySelectorAll("[data-t]"))
    el.textContent =
      t(defaults.get(el.dataset.t), words[el.dataset.t]);
  if (cfg) {
    profiles();
    accounts();
    updateInfo(info);
    message("");
  }
}
$("language").onchange = (e) => {
  lang = e.target.value;
  language();
};
language();
const errorText = {
  invalid_name: [
    "自定义名称仅限英文、数字和英文符号。",
    "Custom names allow ASCII letters, numbers and symbols only.",
  ],
  login_required: [
    "登录已失效，请重新登录。",
    "Session expired. Sign in again.",
  ],
  login_throttled: [
    "尝试过多，请稍后重试。",
    "Too many attempts. Retry shortly.",
  ],
  origin: [
    "访问来源不匹配，请通过设备地址重新打开。",
    "Origin mismatch. Reopen the device address.",
  ],
  csrf: ["页面会话已更新，请刷新后重试。", "Session changed. Reload to retry."],
  password_change_required: [
    "请先修改初始管理密码。",
    "Change the initial password first.",
  ],
  password_length: [
    "密码须为8–64字节，且不能使用初始密码。",
    "Use 8–64 bytes and change the default password.",
  ],
  invalid_json: ["请求格式错误。", "Invalid request format."],
  invalid_configuration: [
    "配置不完整或超出设备限制。",
    "Invalid or oversized configuration.",
  ],
  invalid_settings: ["请检查设置的范围。", "Check setting ranges."],
  invalid_wifi: [
    "请检查Wi-Fi名称和密码。",
    "Check the Wi-Fi name and password.",
  ],
  storage_failed: [
    "保存失败，请检查存储空间。",
    "Save failed. Check storage space.",
  ],
  settings_failed: [
    "设置保存失败，请重试。",
    "Settings could not be saved. Retry.",
  ],
  low_memory: ["可用内存不足，请稍后重试。", "Low memory. Retry shortly."],
  cloud_busy: [
    "云请求处理中，请稍候。",
    "Cloud request in progress. Please wait.",
  ],
  not_ready: ["设备忙，请稍后重试。", "Device busy. Retry shortly."],
  update_failed: ["固件升级失败。", "Firmware failed."],
  application_firmware_required: [
    "请选择ESP32应用固件。",
    "Choose ESP32 application firmware.",
  ],
  one_firmware_required: [
    "每次仅上传一个固件。",
    "Upload one firmware image at a time.",
  ],
  firmware_write_failed: [
    "固件写入失败或过大。",
    "Firmware write failed or image too large.",
  ],
  update_begin_failed: [
    "无法开始升级，请稍后重试。",
    "Unable to start update. Retry shortly.",
  ],
  invalid_firmware: ["固件校验失败。", "Firmware verification failed."],
  upload_aborted: ["上传已中断。", "Upload interrupted."],
  out_of_memory: ["内存不足，请稍后重试。", "Out of memory. Retry shortly."],
  confirmation_required: ["请确认重置操作。", "Confirm the reset operation."],
  password_key_changed: [
    "设备密钥已更换，请重新输入账号密码或导入 Token。",
    "Device key changed. Re-enter the password or import a token.",
  ],
  checking_accounts: [
    "正在检查云账号，请稍候重试。",
    "Checking cloud accounts. Retry shortly.",
  ],
  wait_before_retry: [
    "请稍候再试，当前请求尚在冷却。",
    "Please wait before retrying.",
  ],
  cloud_blocked: [
    "拓竹安全服务拦截了请求（Cloudflare）。建议使用官方登录取得 Token 后导入。",
    "Bambu security service blocked this request (Cloudflare). We recommend importing a token from official sign-in.",
  ],
  csrf_required: [
    "官方登录要求浏览器会话验证，请在官方网页登录后导入 Token。",
    "Browser session verification is required. Sign in officially and import a token.",
  ],
  cloud_forbidden: [
    "拓竹服务拒绝请求，不能据此判断密码错误。可尝试验证码或导入 Token。",
    "Bambu rejected this request; this does not establish an incorrect password. Try a verification code or import a token.",
  ],
  authentication_rejected: [
    "账号凭据或 Token 未被接受，请检查账号区域与凭据。",
    "Credentials or token rejected. Check the account region and credentials.",
  ],
  verification_required: [
    "请输入收到的验证码。",
    "Enter the verification code you received.",
  ],
  two_factor_required: [
    "请输入身份验证器的验证码。",
    "Enter the code from your authenticator.",
  ],
  clock_not_ready: [
    "设备时间尚未同步，请稍后重试。",
    "Device clock is not synchronized. Retry shortly.",
  ],
  cloud_network_error: [
    "连接拓竹服务失败，请检查网络与设备时间。",
    "Unable to connect. Check the network and device clock.",
  ],
  cloud_rate_limited: [
    "拓竹服务限制了请求频率，请稍后重试。",
    "Bambu rate limit reached. Retry later.",
  ],
  cloud_request_failed: [
    "云端请求失败，请查看 HTTP 状态。",
    "Cloud request failed. Check the HTTP status.",
  ],
  invalid_or_busy: [
    "请检查账号、密码、验证码或 Token 是否填写完整。",
    "Check the account, password, verification code or token.",
  ],
  invalid_password: ["管理密码错误。", "Incorrect management password."],
  response_too_large: [
    "服务端响应超出设备处理上限。",
    "Response exceeds the device limit.",
  ],
};
function failure(error) {
  return error.name === "TimeoutError" || error.name === "AbortError"
    ? t("请求超时，请重试。", "Request timed out. Retry.")
    : error instanceof TypeError
      ? t("网络连接中断，请重试。", "Network connection interrupted. Retry.")
      : error.message;
}
function explain(code) {
  return errorText[code]
    ? t(...errorText[code])
    : String(code);
}
function message(value, error = false) {
  $("status").textContent = value;
  $("status").title = value;
  $("status").classList.toggle("error", error);
}
function updateInfo(d) {
  info = d;
  if (typeof d.version === "string" && /^\d+\.\d+\.\d+$/.test(d.version))
    $("portalVersion").textContent = d.version;
  $("statPrinter").textContent = d.printer || "—";
  $("statConnection").textContent =
    `${d.mode || "LAN"} · ${d.online ? t("在线", "Online") : t("等待连接", "Waiting")}`;
  $("statHeap").textContent = `${Math.round(d.heap / 1024)} KB`;
  $("statUptime").textContent =
    `${Math.floor(d.uptime / 3600)}h ${Math.floor((d.uptime % 3600) / 60)}m`;
}
async function api(path, data) {
  const epoch = sessionEpoch;
  const response = await fetch(path, {
    method: data === undefined ? "GET" : "POST",
    credentials: "same-origin",
    headers:
      data === undefined
        ? {}
        : { "Content-Type": "application/json", "X-XTouch-CSRF": csrf },
    body: data === undefined ? undefined : JSON.stringify(data),
    signal: AbortSignal.timeout(20000),
  });
  let result;
  try {
    result = await response.json();
  } catch {
    throw Error(
      t(
        "设备返回异常，请刷新重试。",
        "Invalid device response. Reload to retry.",
      ),
    );
  }
  if (epoch != sessionEpoch)
    throw new DOMException("Obsolete session", "AbortError");
  if (!response.ok) {
    if (response.status === 401) enterLogin();
    throw Error(explain(result.error || response.status));
  }
  return result;
}
function showChange() {
  sessionEpoch++;
  cloudBusy = false;
  $("language").disabled = false;
  collapseAccounts();
  clearInterval(poll);
  message("");
  $("guestFeedback").append($("status"));
  $("app").hidden = true;
  $("login").hidden = true;
  $("change").hidden = false;
}
$("loginForm").onsubmit = async (e) => {
  e.preventDefault();
  try {
    let result = await api("/api/login", {
      password: $("loginPassword").value,
    });
    $("loginPassword").value = "";
    csrf = result.csrf;
    if (result.mustChange) showChange();
    else await load();
  } catch (error) {
    message(failure(error), true);
  }
};
$("passwordForm").onsubmit = async (e) => {
  e.preventDefault();
  if ($("newPassword").value !== $("confirmPassword").value) {
    message(t("两次密码不一致。", "Passwords do not match."));
    return;
  }
  try {
    await api("/api/password", { password: $("newPassword").value });
    $("newPassword").value = "";
    $("confirmPassword").value = "";
    enterLogin();
    message(t("密码已修改，请重新登录。", "Password changed. Sign in again."));
  } catch (error) {
    message(failure(error), true);
  }
};
$("passwordButton").onclick = showChange;
$("logout").onclick = async () => {
  try {
    await api("/api/logout", {});
  } finally {
    enterLogin();
  }
};
for (const button of document.querySelectorAll("[data-tab]"))
  button.onclick = () => {
    collapseAccounts();
    for (const tab of document.querySelectorAll(".tab"))
      tab.hidden = tab.id !== button.dataset.tab;
    for (const b of document.querySelectorAll("[data-tab]"))
      b.classList.toggle("active", b === button);
    if (cfg) accounts();
  };
function field(parent, label, value, change, type = "text", max = 100) {
  const wrap = document.createElement("label"),
    caption = document.createElement("span"),
    input = document.createElement("input");
  caption.textContent = label;
  input.type = type;
  input.value = value ?? "";
  input.maxLength = max;
  input.oninput = () => change(input.value);
  wrap.append(caption, input);
  parent.append(wrap);
  return input;
}
function button(parent, label, click, danger = false) {
  let b = document.createElement("button");
  b.type = "button";
  b.textContent = label;
  b.onclick = click;
  if (danger) b.className = "danger";
  parent.append(b);
  return b;
}
function select(parent, label, options, value, change) {
  let wrap = document.createElement("label"),
    caption = document.createElement("span"),
    input = document.createElement("select");
  caption.textContent = label;
  for (const [key, name] of options) {
    let o = new Option(name, key);
    input.append(o);
  }
  input.value = value;
  input.onchange = () => change(input.value);
  wrap.append(caption, input);
  parent.append(wrap);
  return input;
}
function source(p) {
  const a = cfg.cloudAccounts.find((a) => a.id === p.accountId);
  if(a?.simulator) return {label:t("模拟器", "Simulator"),kind:"tokenSource"};
  return p.mode !== "cloud"
    ? { label: "LAN", kind: "lan" }
    : (a?.loginMethod || a?.authMode) === "password"
      ? { label: t("云账号", "Cloud account"), kind: "accountSource" }
      : { label: "Cloud Token", kind: "tokenSource" };
}
function openAccount(a) {
  document.querySelector('[data-tab="cloudAccounts"]').click();
  a.expanded = true;
  accounts();
  if ((a.token || a.tokenSaved) && !a.devices && !cloudBusy)
    cloud(a, { action: "devices" });
}
const modelNames = {
  "BL-P001": "X1C",
  "BL-P002": "X1",
  C11: "P1P",
  C12: "P1S",
  C13: "X1E",
  N1: "A1 mini",
  N2S: "A1",
  N6: "X2D",
  N7: "P2S",
  N9: "A2L",
  O1C: "H2C",
  O1C2: "H2C",
  O1D: "H2D",
  O1E: "H2D Pro",
  O1S: "H2S",
};
function model(p) {
  const raw =
    p.printerModel ||
    cfg.printers.find(
      (x) => x.serialNumber === p.serialNumber && x.printerModel,
    )?.printerModel ||
    "";
  return modelNames[raw] || raw || "—";
}
function icon(kind) {
  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  svg.setAttribute("viewBox", "0 0 24 24");
  svg.setAttribute("aria-hidden", "true");
  svg.classList.add("sourceIcon", kind);
  const paths = {
    lan: [
      "M3 8a15 15 0 0 1 18 0M6 12a10 10 0 0 1 12 0M9 16a5 5 0 0 1 6 0",
      "M12 20h.01",
    ],
    tokenSource: [
      "M7 17a5 5 0 1 1 0-10 6 6 0 0 1 11 1 4.5 4.5 0 0 1-1 9",
      "M13 13a2 2 0 1 0 0 4 2 2 0 0 0 0-4m2 2h6m-2 0v3",
    ],
    accountSource: [
      "M15.5 7a3.5 3.5 0 1 1-7 0 3.5 3.5 0 0 1 7 0M5 21v-3a7 7 0 0 1 14 0v3",
    ],
  };
  for (const d of paths[kind]) {
    const path = document.createElementNS(svg.namespaceURI, "path");
    path.setAttribute("d", d);
    path.setAttribute("fill", "none");
    path.setAttribute("stroke", "currentColor");
    path.setAttribute("stroke-width", "2");
    path.setAttribute("stroke-linecap", "round");
    path.setAttribute("stroke-linejoin", "round");
    svg.append(path);
  }
  return svg;
}
function deviceLabel(p) {
  return `[${model(p)}] ${p.name || p.serialNumber || t("新设备", "New device")}`;
}
function profiles() {
  const root = $("profiles");
  root.replaceChildren();
  cfg.printers.forEach((p, i) => {
    const row = document.createElement("div");
    row.className = "profile" + (i === selected ? " selected" : "");
    let pick = button(row, deviceLabel(p), () => {
      selected = i;
      profiles();
      const a = cfg.cloudAccounts.find((a) => a.id === p.accountId);
      if (p.mode === "cloud" && (a?.health === "invalid" || p.health < 0)) {
        openAccount(a);
        message(
          t("账号已失效，请重新登录。", "Account expired. Sign in again."),
          true,
        );
      }
    });
    pick.className = "pick " + source(p).kind;
    pick.prepend(icon(source(p).kind));
    pick.title = source(p).label;
    button(row, "↑", () => move(i, -1)).disabled = i === 0;
    button(row, "↓", () => move(i, 1)).disabled = i === cfg.printers.length - 1;
    button(
      row,
      "×",
      () => {
        const active = cfg.printers[cfg.activePrinter];
        cfg.printers.splice(i, 1);
        cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
        selected = Math.min(selected, Math.max(0, cfg.printers.length - 1));
        profiles();
      },
      true,
    );
    root.append(row);
  });
  $("defaultDevice").replaceChildren(
    ...cfg.printers.map(
      (p, i) => new Option(p.name || p.serialNumber || `${i + 1}`, i),
    ),
  );
  $("defaultDevice").value = cfg.activePrinter || 0;
  editor();
}
function move(i, delta) {
  const active = cfg.printers[cfg.activePrinter],
    current = cfg.printers[selected];
  [cfg.printers[i], cfg.printers[i + delta]] = [
    cfg.printers[i + delta],
    cfg.printers[i],
  ];
  cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
  selected = Math.max(0, cfg.printers.indexOf(current));
  profiles();
}
const newProfiles = new WeakSet();
const validName = (value) => /^[\x20-\x7E]*$/.test(value || "");
function nameField(parent, label, value, change) {
  const input = field(parent, label, value, change, "text", 63);
  input.pattern = "[ -~]*";
  input.title = t(
    "仅限英文字母、数字和英文符号",
    "English letters, numbers and ASCII symbols only",
  );
  input.addEventListener("input", () =>
    input.setCustomValidity(
      validName(input.value)
        ? ""
        : t(
            "仅限英文字母、数字和英文符号",
            "English letters, numbers and ASCII symbols only",
          ),
    ),
  );
  return input;
}
function lockEditor(input) {
  const wrap = input.parentElement;
  wrap.classList.add("editLocked");
  input.title = t(
    "双击修改；键盘按 F2 或 Enter",
    "Double-click to edit; F2 or Enter with keyboard",
  );
  let committed = input.value;
  const changed = input.onchange;
  input.onchange = () => {
    if (input.dataset.locked === "1") {
      input.value = committed;
      return;
    }
    if (changed) changed();
    committed = input.value;
  };
  const lock = () => {
    wrap.classList.add("editLocked");
    input.setAttribute("aria-readonly", "true");
    input.dataset.locked = "1";
    if (input.tagName === "INPUT") input.readOnly = true;
  };
  const unlock = () => {
    wrap.classList.remove("editLocked");
    input.setAttribute("aria-readonly", "false");
    input.dataset.locked = "0";
    if (input.tagName === "INPUT") input.readOnly = false;
    input.focus();
  };
  lock();
  input.addEventListener("dblclick", unlock);
  input.addEventListener("keydown", (e) => {
    if (input.dataset.locked === "1") {
      if (e.key === "F2" || e.key === "Enter") {
        e.preventDefault();
        unlock();
      } else if (input.tagName === "SELECT" && e.key !== "Tab")
        e.preventDefault();
    }
  });
  input.addEventListener("pointerdown", (e) => {
    if (input.tagName === "SELECT" && input.dataset.locked === "1")
      e.preventDefault();
  });
  input.addEventListener("blur", lock);
}
function editor() {
  const root = $("editor");
  root.replaceChildren();
  const p = cfg.printers[selected];
  if (!p) return;
  const grid = document.createElement("div");
  grid.className = "grid";
  root.append(grid);
  const title = document.createElement("h3");
  title.textContent = t("设备信息", "Device information");
  grid.append(title);
  nameField(grid, t("设备名称", "Device name"), p.name, (v) => (p.name = v));
  const hint = document.createElement("small");
  hint.className = "editHint";
  hint.textContent = t(
    "已有设备双击编辑，或聚焦后按 F2；名称仅限英文、数字和符号。",
    "Double-click existing fields or press F2. Names: ASCII letters, numbers and symbols.",
  );
  grid.insertBefore(hint, title.nextSibling);
  const show = (label, value) => {
    const row = document.createElement("div");
    row.className = "deviceDetail";
    const caption = document.createElement("small"),
      text = document.createElement("div");
    caption.textContent = label;
    text.textContent = value || "—";
    row.append(caption, text);
    grid.append(row);
  };
  if (p.mode === "cloud") {
    show(t("设备型号", "Device model"), model(p));
    show(t("序列号", "Serial number"), p.serialNumber);
    const account = cfg.cloudAccounts.find((a) => a.id === p.accountId);
    const row = document.createElement("div");
    row.className = "deviceDetail";
    const caption = document.createElement("small");
    caption.textContent = t("连接方式", "Connection type");
    const link = button(
      row,
      source(p).label +
        " (" +
        (account?.name || account?.account || account?.id || "—") +
        ")",
      () => {
        if (account) openAccount(account);
      },
    );
    link.className = "accountLink " + source(p).kind;
    row.prepend(caption);
    grid.append(row);
  } else {
    select(
      grid,
      t("设备型号", "Device model"),
      [["", t("请选择型号", "Select model")], ...Object.entries(modelNames)],
      p.printerModel || "",
      (v) => (p.printerModel = v),
    );
    field(
      grid,
      t("序列号", "Serial number"),
      p.serialNumber,
      (v) => (p.serialNumber = v),
      "text",
      31,
    );
    field(
      grid,
      t("IP / 主机名", "IP / hostname"),
      p.host,
      (v) => (p.host = v),
      "text",
      63,
    );
    const input = field(
      grid,
      t("访问码（留空保留）", "Access code (blank to keep)"),
      p.accessCode,
      (v) => (p.accessCode = v),
      "password",
      8,
    );
    input.placeholder = p.accessCodeSaved ? "••••••••" : "";
    show(t("连接方式", "Connection type"), t("局域网模式", "LAN mode"));
  }
  if (!newProfiles.has(p))
    for (const input of grid.querySelectorAll("input,select"))
      lockEditor(input);
}
$("addPrinter").onclick = () => {
  if (cfg.printers.length >= 100)
    return message(t("最多 100 台。", "Maximum 100 devices."));
  cfg.printers.push({
    mode: "lan",
    name: "",
    host: "",
    accessCode: "",
    serialNumber: "",
    printerModel: "",
  });
  selected = cfg.printers.length - 1;
  newProfiles.add(cfg.printers[selected]);
  profiles();
};
async function discoverLab(a){
  if(cloudBusy)return;
  cloudBusy=true;
  const epoch=sessionEpoch;
  try{
    const result=await api("/api/lab/discover",{host:a.mqttHost,port:Number(a.labHttpPort||8894)});
    if(epoch!==sessionEpoch||!cfg)return;
    a.devices=result.devices;a.token="xtouch-lab";a.userId="xtouch-lab";a.signedIn=true;a.verified=true;a.expanded=true;
    message(t("已获取虚拟设备，请勾选并保存。", "Select virtual devices, then save & restart."));
  }catch(e){if(epoch===sessionEpoch)message(failure(e),true);}
  finally{cloudBusy=false;if(epoch===sessionEpoch&&cfg)accounts();}
}
function labAccount(body,a){
  const note=document.createElement("p");note.className="subtle";note.textContent=t("仅连接局域网模拟器，使用固定测试凭据，不发送拓竹 Token 或密码。", "Local simulator only. Fixed test credentials; no Bambu tokens or passwords are sent.");body.append(note);
  const grid=document.createElement("div");grid.className="grid";body.append(grid);
  nameField(grid,t("备注", "Label"),a.name||a.id,v=>a.name=v);
  field(grid,t("模拟器 IP", "Simulator IP"),a.mqttHost,v=>a.mqttHost=v,"text",63);
  field(grid,"TLS MQTT port",a.mqttPort||8883,v=>a.mqttPort=Number(v),"number",5);
  field(grid,"Web port",a.labHttpPort||8894,v=>a.labHttpPort=Number(v),"number",5);
  button(body,t("获取虚拟设备", "Discover virtual devices"),()=>discoverLab(a));
  for(const p of a.devices||cfg.printers.filter(p=>p.accountId===a.id)){
    const row=document.createElement("label");row.className="checkbox deviceRow tokenSource";
    const check=document.createElement("input");check.type="checkbox";check.checked=cfg.printers.some(x=>x.accountId===a.id&&x.serialNumber===p.serialNumber);
    check.onchange=()=>{
      if(check.checked){if(cfg.printers.length>=100){check.checked=false;return message(t("最多 100 台设备。", "Maximum 100 devices."));}
        cfg.printers.push({name:p.name,serialNumber:p.serialNumber,printerModel:p.printerModel,mode:"cloud",accountId:a.id});
      }else{const active=cfg.printers[cfg.activePrinter||0];cfg.printers=cfg.printers.filter(x=>!(x.accountId===a.id&&x.serialNumber===p.serialNumber));cfg.activePrinter=Math.max(0,cfg.printers.indexOf(active));}
      profiles();editor();
    };
    row.append(check,icon("tokenSource"),document.createTextNode(deviceLabel(p)));body.append(row);
  }
}
function accounts() {
  const root = $("accounts");
  root.replaceChildren();
  $("addSimulator")?.remove();
  if(labTools) {
    const addSimulator = button($("accountActions"),t("添加 MQTT 模拟器", "Add MQTT simulator"),()=>{
    if(cfg.cloudAccounts.length>=4)return message(t("最多 4 个账号。", "Maximum 4 accounts."));
    let n=1;while(cfg.cloudAccounts.some(a=>a.id===`Lab-${n}`))n++;
    cfg.cloudAccounts.push({id:`Lab-${n}`,name:`Lab-${n}`,simulator:true,mqttHost:"",mqttPort:8883,labHttpPort:8894,region:"cn",token:"xtouch-lab",userId:"xtouch-lab",signedIn:false,expanded:true});
    accounts();
    });
    addSimulator.id = "addSimulator";
    addSimulator.disabled = cloudBusy;
  }
  cfg.cloudAccounts.forEach((a, i) => {
    const open = a.expanded === true,
      kind =
        (a.loginMethod || a.authMode) === "password"
          ? "accountSource"
          : "tokenSource";
    const card = document.createElement("article");
    card.className = "accountCard";
    root.append(card);
    const head = document.createElement("div");
    head.className = "accountHeader";
    card.append(head);
    const passwordState =
      a.simulator ? t("模拟器", "Simulator") : kind === "tokenSource"
        ? "Access Token"
        : a.passwordLocked
          ? t("重输密码", "Re-enter password")
          : a.passwordSaved
            ? t("密码已存", "Password saved")
            : t("密码未存", "Password not saved");
    const toggle = button(
      head,
      `${a.name || a.account || a.id}（${passwordState}）`,
      () => {
        a.expanded = !open;
        accounts();
        if (a.expanded && (a.token || a.tokenSaved) && !a.devices && !cloudBusy)
          cloud(a, { action: "devices" });
      },
    );
    const accountName = document.createElement("span");
    accountName.className = "accountName";
    accountName.textContent = toggle.textContent;
    toggle.title = toggle.textContent;
    toggle.replaceChildren(icon(kind), accountName);
    const chevron = document.createElement("span");
    chevron.className = "accountChevron";
    chevron.setAttribute("aria-hidden", "true");
    chevron.innerHTML = '<svg viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="currentColor" stroke-width="2.5"><path d="m6 9 6 6 6-6"/></svg>';
    toggle.append(chevron);
    toggle.className = "accountToggle " + kind;
    card.classList.toggle("isOpen", open);
    toggle.setAttribute("aria-expanded", String(open));
    const badge = document.createElement("span");
    badge.className =
      "badge" + (a.verified || a.health === "valid" ? " ok" : "");
    badge.textContent =
      a.health === "invalid"
        ? t("需登录", "Sign-in required")
        : a.verified || a.health === "valid"
          ? t("有效", "Valid")
          : a.token || a.tokenSaved
            ? t("待检查", "Unchecked")
            : t("未登录", "Signed out");
    head.append(badge);
    const tools = document.createElement("div");
    tools.className = "accountTools";
    head.append(tools);
    if (!a.simulator) button(
      tools,
      t("清除登录", "Clear login"),
      () => {
        a.token = "";
        a.tokenSaved = false;
        a.userId = "";
        a.clearToken = true;
        a.verified = false;
        a.health = "";
        a.passwordSaved = false;
        a.passwordEnc = "";
        a.rememberPassword = false;
        a.devices = null;
        a.expanded = true;
        accounts();
        message(t("请保存并重启。", "Save & restart to apply."));
      },
      true,
    );
    button(
      tools,
      t("删除账号", "Delete account"),
      () => {
        if (
          cfg.printers.some((p) => p.mode === "cloud" && p.accountId === a.id)
        )
          return message(
            t("请先移除关联设备。", "Remove linked devices first."),
            true,
          );
        cfg.cloudAccounts.splice(i, 1);
        accounts();
        editor();
      },
      true,
    );
    const body = document.createElement("div");
    body.className = "accountBody";
    body.id = "account-body-" + i;
    toggle.setAttribute("aria-controls", body.id);
    body.hidden = !open;
    card.append(body);
    if(a.simulator){labAccount(body,a);return;}
    const identity = document.createElement("div");
    identity.className = "grid";
    body.append(identity);
    nameField(
      identity,
      t("备注", "Label"),
      a.name || a.account || a.id,
      (v) => (a.name = v),
    );
    select(
      identity,
      t("区域", "Region"),
      [
        ["cn", t("中国区", "China")],
        ["global", t("国际区", "Global")],
      ],
      a.region || "cn",
      (v) => {
        a.region = v;
        a.signedIn = false;
        a.verified = false;
        a.tokenSaved = false;
        a.token = "";
        a.passwordSaved = false;
        a.devices = null;
      },
    );
    const modes = document.createElement("div");
    modes.className = "methodTabs";
    body.append(modes);
    for (const [mode, label] of [
      ["password", t("账号登录", "Account sign-in")],
      ["token", "Access Token"],
    ]) {
      const b = button(modes, label, () => {
        a.authMode = mode;
        if (mode === "token" && a.passwordLocked) {
          a.rememberPassword = false;
          a.passwordEnc = "";
          a.passwordLocked = false;
        }
        a.expanded = true;
        accounts();
        profiles();
      });
      b.classList.toggle("active", (a.authMode || "token") === mode);
    }
    const fields = document.createElement("div");
    fields.className = "grid";
    body.append(fields);
    if (a.authMode === "password") {
      field(
        fields,
        t("手机号／邮箱", "Phone / email"),
        a.account || "",
        (v) => {
          a.account = v;
          a.signedIn = false;
          a.passwordSaved = false;
          a.devices = null;
        },
        "text",
        100,
      );
      let password = "";
      const pw = field(
        fields,
        t("密码", "Password (password sign-in only)"),
        "",
        (v) => (password = v),
        "password",
        128,
      );
      pw.placeholder = a.passwordSaved
        ? t("已保存；留空使用", "Saved; leave blank to use")
        : "";
      pw.title = t("仅密码登录时需要", "Only for password sign-in");
      const keep = document.createElement("label");
      keep.className = "checkbox";
      keep.title = t("密码加密保存在本设备", "Password is encrypted on this device");
      const check = document.createElement("input");
      check.type = "checkbox";
      check.checked = !!a.rememberPassword;
      check.onchange = () => (a.rememberPassword = check.checked);
      keep.append(
        check,
        document.createTextNode(
          t("加密保存", "Remember encrypted password"),
        ),
      );
      const wrap = pw.parentElement,
        box = document.createElement("div");
      box.className = "passwordField";
      wrap.replaceWith(box);
      box.append(...wrap.childNodes);
      pw.setAttribute("aria-label", t("密码", "Password"));
      box.insertBefore(keep, pw);
      const actions = document.createElement("div");
      actions.className = "toolbar";
      body.append(actions);
      button(actions, t("密码登录", "Password sign-in"), async () => {
        if (!a.account || (!password && !a.passwordSaved))
          return message(
            t("请填写账号和密码。", "Enter account and password."),
            true,
          );
        await cloud(a, {
          action: "login",
          account: a.account,
          password,
          rememberPassword: !!a.rememberPassword,
        });
        password = "";
        pw.value = "";
      });
      button(actions, t("验证码登录", "Sign in with code"), async () => {
        if (!a.account)
          return message(
            t("请填写手机号或邮箱。", "Enter phone or email."),
            true,
          );
        a.needsCode = true;
        await cloud(a, { action: "send_code", account: a.account });
      });
      if (a.needsCode) {
        const verify = document.createElement("div");
        verify.className = "grid";
        body.append(verify);
        let code = "";
        field(
          verify,
          a.tfaKey
            ? t("身份验证器验证码", "Authenticator code")
            : t("验证码", "Verification code"),
          "",
          (v) => (code = v),
          "text",
          6,
        );
        button(verify, t("确认登录", "Confirm sign-in"), () =>
          cloud(a, {
            action: a.tfaKey ? "login_tfa" : "login_code",
            account: a.account,
            code,
            tfaKey: a.tfaKey || "",
          }),
        );
      }
    } else {
      const token = field(
        fields,
        "Access Token",
        a.token || "",
        (v) => {
          a.token = v;
          a.clearToken = false;
          a.verified = false;
        },
        "password",
        2048,
      );
      token.setAttribute("aria-label", "Access Token");
      const tokenGuide = document.createElement("a");
      tokenGuide.href = "https://github.com/tbc0309/xtouch-CN/blob/main/docs/ACCESS-TOKEN.md";
      tokenGuide.target = "_blank";
      tokenGuide.rel = "noopener noreferrer";
      tokenGuide.className = "tokenHelp";
      tokenGuide.textContent = t("获取 Token 教程", "Token guide");
      token.parentElement.querySelector("span").append(" (", tokenGuide, ")");
      token.placeholder = a.tokenSaved
        ? t("已保存；留空保留", "Saved; blank to keep")
        : "";
      button(fields, t("验证并获取设备", "Validate & get devices"), () =>
        cloud(a, { action: "validate" }),
      );
    }
    if (a.token || a.tokenSaved || a.devices) {
      const section = document.createElement("div");
      section.className = "linkedDevices";
      body.append(section);
      const heading = document.createElement("div");
      heading.className = "sectionHead";
      section.append(heading);
      const title = document.createElement("h3");
      title.textContent = t("关联设备", "Linked devices");
      heading.append(title);
      button(heading, t("刷新设备", "Refresh devices"), () =>
        cloud(a, { action: "devices" }),
      );
      for (const p of a.devices || []) {
        const label = document.createElement("label");
        label.className = "checkbox deviceRow " + kind;
        const check = document.createElement("input");
        check.type = "checkbox";
        check.checked = cfg.printers.some(
          (x) =>
            x.mode === "cloud" &&
            x.accountId === a.id &&
            x.serialNumber === p.serialNumber,
        );
        check.onchange = () => {
          if (check.checked) {
            if (cfg.printers.length >= 100) {
              check.checked = false;
              return message(t("最多 100 台。", "Maximum 100 devices."), true);
            }
            cfg.printers.push({
              name: validName(p.name) ? p.name : "",
              serialNumber: p.serialNumber,
              printerModel: p.printerModel || "",
              mode: "cloud",
              accountId: a.id,
            });
          } else {
            const active = cfg.printers[cfg.activePrinter];
            cfg.printers = cfg.printers.filter(
              (x) =>
                !(
                  x.mode === "cloud" &&
                  x.accountId === a.id &&
                  x.serialNumber === p.serialNumber
                ),
            );
            cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
          }
          profiles();
        };
        label.append(
          check,
          icon(kind),
          document.createTextNode(
            deviceLabel(p) +
              (p.online === false ? t(" · 离线", " · Offline") : ""),
          ),
        );
        section.append(label);
      }
    }
  });
  if (cloudBusy)
    for (const el of root.querySelectorAll("button,input,select"))
      el.disabled = true;
}
$("addAccount").onclick = () => {
  if (cfg.cloudAccounts.length >= 4)
    return message(t("最多 4 个账号。", "Maximum 4 accounts."));
  let n = cfg.cloudAccounts.length + 1;
  while (
    cfg.cloudAccounts.some(
      (a) => a.id === `Cloud-${n}` || a.name === `Cloud-${n}`,
    )
  )
    n++;
  cfg.cloudAccounts.push({
    id: `Cloud-${n}`,
    name: `Cloud-${n}`,
    region: "cn",
    token: "",
    userId: "",
  });
  accounts();
  editor();
};
// Cloud results belong to the session that started them, not the next login.
async function cloud(a, input) {
  if(a.simulator)return discoverLab(a);
  if (cloudBusy || !cfg || $("app").hidden) return;
  const epoch = sessionEpoch;
  const current = () => epoch === sessionEpoch && !!cfg && !$("app").hidden;
  cloudBusy = true;
  for (const b of document.querySelectorAll(
    "#accounts button,#accounts input,#accounts select,#addAccount,#addSimulator,#save,#language",
  ))
    b.disabled = true;
  try {
    message(
      t("正在联系拓竹服务，请等待结果…", "Contacting Bambu. Please wait…"),
    );
    await api("/api/cloud", {
      region: a.region || "cn",
      token: a.token || "",
      accountId: a.id,
      ...input,
    });
    const until = Date.now() + 45000;
    while (Date.now() < until) {
      await new Promise((r) => setTimeout(r, 750));
      if (!current()) return;
      const result = await api("/api/cloud");
      if (result.busy) continue;
      if (
        result.error === "verification_required" ||
        result.error === "two_factor_required"
      ) {
        a.needsCode = true;
        a.tfaKey = result.tfaKey || "";
        a.rememberPassword = false;
      }
      if (!result.ok)
        throw Error(
          explain(result.error || "cloud_request_failed") +
            (result.httpStatus ? ` (HTTP ${result.httpStatus})` : ""),
        );
      a.lastError = "";
      if (input.action === "send_code") {
        a.needsCode = true;
        a.expanded = !$("cloudAccounts").hidden;
        accounts();
        message(
          t(
            "验证码已发送，请填写收到的验证码。",
            "Code sent. Enter the verification code.",
          ),
        );
        return;
      }
      a.signedIn = true;
      if (result.token) {
        a.token = result.token;
        a.tokenSaved = false;
        a.clearToken = false;
      }
      if (result.userId) a.userId = result.userId;
      if (input.action !== "devices") {
        a.verified = true;
        a.loginMethod = input.action === "validate" ? "token" : "password";
      }
      if (a.token) {
        try {
          const encoded = a.token
            .split(".")[1]
            .replace(/-/g, "+")
            .replace(/_/g, "/");
          const payload = JSON.parse(
            atob(encoded.padEnd(Math.ceil(encoded.length / 4) * 4, "=")),
          );
          a.expiresAt = payload.exp || 0;
        } catch {
          a.expiresAt = 0;
        }
      }
      if (result.passwordEnc) {
        a.passwordEnc = result.passwordEnc;
        a.passwordSaved = true;
      }
      if (result.devices) {
        a.devices = result.devices;
        a.health = "valid";
        accounts();
        profiles();
        message(
          t(
            `已获取 ${result.devices.length} 台设备。`,
            `Found ${result.devices.length} devices.`,
          ),
        );
      } else {
        a.needsCode = false;
        delete a.tfaKey;
        accounts();
        message(t("登录成功，正在获取设备…", "Signed in. Fetching devices…"));
        cloudBusy = false;
        await cloud(a, { action: "devices" });
      }
      return;
    }
    throw Error(t("请求超时，请稍后重试。", "Request timed out. Retry later."));
  } catch (error) {
    if (!current()) return;
    a.lastError = failure(error);
    a.verified = false;
    a.expanded = !$("cloudAccounts").hidden && input.action !== "devices";
    if (
      /verification_required/.test(error.message) ||
      error.message.startsWith(explain("verification_required"))
    )
      a.needsCode = true;
    accounts();
    message(failure(error), true);
  } finally {
    if (current()) {
      cloudBusy = false;
      updatePending();
      for (const b of document.querySelectorAll(
        "#accounts button,#accounts input,#accounts select,#addAccount,#addSimulator,#save,#language",
      ))
        b.disabled = false;
    }
  }
}
async function load() {
  cfg = await api("/api/config");
  const active = (cfg.printers || [])[cfg.activePrinter || 0];
  cfg.printers = (cfg.printers || []).filter(Boolean);
  cfg.activePrinter = Math.max(0, cfg.printers.indexOf(active));
  cfg.cloudAccounts = cfg.cloudAccounts || [];
  for (const a of cfg.cloudAccounts) {
    a.expanded = false;
    a.loginMethod = a.loginMethod || "token";
    a.signedIn = !!(
      a.signedIn ||
      a.tokenSaved ||
      a.passwordSaved ||
      a.passwordLocked ||
      a.userId
    );
  }
  cfg.settings = cfg.settings || {};
  $("login").hidden = true;
  $("change").hidden = true;
  $("app").hidden = false;
  $("logout").hidden = false;
  $("saveFeedback").append($("status"));
  applyFields();
  selected = 0;
  profiles();
  accounts();
  info = await api("/api/info");
  updateInfo(info);
  message("");
  cleanState = configState();
  updatePending();
  if (location.hash.startsWith("#account=")) {
    const a = cfg.cloudAccounts.find(
      (a) => a.id === decodeURIComponent(location.hash.slice(9)),
    );
    if (a) openAccount(a);
  }
  clearInterval(poll);
  poll = setInterval(async () => {
    try {
      const session = await api("/api/session");
      csrf = session.csrf;
      let d = await api("/api/info");
      updateInfo(d);
    } catch {}
  }, 5000);
}
$("save").onclick = async () => {
  const invalid = [...cfg.printers, ...cfg.cloudAccounts].some(
    (x) => !validName(x.name),
  );
  if (invalid)
    return message(
      t(
        "自定义名称仅限英文字母、数字和英文符号。",
        "Custom names allow ASCII letters, numbers and symbols only.",
      ),
      true,
    );
  for (const input of document.querySelectorAll("#app input"))
    if (!input.reportValidity()) return;
  const b = $("save");
  b.disabled = true;
  try {
    cfg.ssid = $("ssid").value;
    cfg.pwd = $("wifiPassword").value;
    cfg.activePrinter = Number($("defaultDevice").value) || 0;
    Object.assign(cfg.settings, {
      language: $("screenLanguage").value,
      backlight: Number($("brightness").value),
      tftOff: Number($("sleep").value),
      wop: $("wake").checked,
      ota: $("ota").checked,
    });
    const payload = JSON.parse(JSON.stringify(cfg));
    payload.cloudAccounts = payload.cloudAccounts.filter((a) => a.signedIn);
    const active = payload.printers[payload.activePrinter];
    payload.printers = payload.printers.filter(
      (p) =>
        p.mode !== "cloud" ||
        payload.cloudAccounts.some((a) => a.id === p.accountId),
    );
    payload.activePrinter = Math.max(0, payload.printers.indexOf(active));
    for (const p of payload.printers) delete p.health;
    for (const a of payload.cloudAccounts) {
      delete a.verified;
      delete a.lastError;
      delete a.devices;
      delete a.expanded;
      delete a.needsCode;
      delete a.health;
      delete a.tfaKey;
    }
    await api("/api/config", payload);
    clearInterval(poll);
    message(
      t(
        "已保存，设备正在重启；请稍后刷新。",
        "Saved. Restarting; reload shortly.",
      ),
    );
  } catch (error) {
    message(failure(error), true);
    b.disabled = false;
  }
};
let scanTimer;
async function scan() {
  clearTimeout(scanTimer);
  $("scanSetup").disabled = true;
  try {
    const data = await api("/api/wifi/scan");
    $("setupNetworks").replaceChildren(
      ...(data.networks || []).map(
        (n) => new Option(`${n.ssid} (${n.rssi} dBm)`, n.ssid),
      ),
    );
    if (data.scanning) scanTimer = setTimeout(scan, 1500);
    else $("scanSetup").disabled = false;
  } catch (error) {
    $("scanSetup").disabled = false;
    message(failure(error), true);
  }
}
$("scanSetup").onclick = scan;
$("saveWifi").onclick = async () => {
  try {
    await api("/api/wifi", {
      ssid: $("setupSsid").value,
      pwd: $("setupPassword").value,
    });
    message(t("配网已保存，设备正在重启。", "Wi-Fi saved. Restarting."));
  } catch (error) {
    message(failure(error), true);
  }
};
(async () => {
  try {
    info = await api("/api/info");
    if (info.ap) {
      $("setup").hidden = false;
      $("login").hidden = true;
      scan();
      return;
    }
    try {
      let session = await api("/api/session");
      csrf = session.csrf;
      if (session.mustChange) showChange();
      else await load();
    } catch {}
  } catch (error) {
    message(failure(error), true);
  }
})();

$("resetDevice").onclick = async () => {
  if (
    !confirm(
      t(
        "重置内部配置和管理密码？SD 备份将保留；云账号密码需重新输入。",
        "Reset internal settings and admin password? SD backups remain; cloud passwords must be re-entered.",
      ),
    )
  )
    return;
  try {
    await api("/api/reset", { confirm: "RESET" });
    clearInterval(poll);
    message(t("正在重置并重启。", "Resetting and restarting."));
  } catch (e) {
    message(failure(e), true);
  }
};
$("uploadFirmware").onclick = () => {
  const file = $("firmwareFile").files[0];
  if (!file)
    return message(t("请选择应用固件。", "Choose application firmware."), true);
  if (!confirm(t("上传此固件并重启？", "Install this firmware and restart?")))
    return;
  const form = new FormData();
  form.append("firmware", file);
  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/api/firmware");
  xhr.setRequestHeader("X-XTouch-CSRF", csrf);
  xhr.timeout = 180000;
  xhr.upload.onprogress = (e) => {
    if (e.lengthComputable) {
      $("uploadProgress").hidden = false;
      $("uploadProgress").value = (e.loaded / e.total) * 100;
    }
  };
  xhr.onload = () => {
    let result;
    try {
      result = JSON.parse(xhr.responseText);
    } catch {}
    message(
      xhr.status === 200
        ? t("升级成功，正在重启。", "Updated. Restarting.")
        : explain(result?.error || "update_failed"),
      xhr.status !== 200,
    );
    $("uploadFirmware").disabled = false;
  };
  xhr.onerror = xhr.ontimeout = () => {
    message(t("上传中断，请重试。", "Upload interrupted. Retry."), true);
    $("uploadFirmware").disabled = false;
  };
  $("uploadFirmware").disabled = true;
  clearInterval(poll);
  xhr.send(form);
};

function authLayout() {
  const app = $("app"),
    head = document.querySelector("header"),
    card = ["login", "change", "setup"].map($).find((x) => !x.hidden);
  const auth = app.hidden && !!card;
  document.body.classList.toggle("authMode", auth);
  if (auth) {
    if (head.parentElement !== card) card.prepend(head);
  } else if (head.parentElement !== document.body) document.body.prepend(head);
}
new MutationObserver(authLayout).observe(document.querySelector("main"), {
  subtree: true,
  attributes: true,
  attributeFilter: ["hidden"],
});
authLayout();

function applyFields() {
  $("ssid").value = cfg.ssid || "";
  $("wifiPassword").value = "";
  $("screenLanguage").value = languageCodes.includes(cfg.settings.language) ? cfg.settings.language : "zh";
  $("brightness").value = cfg.settings.backlight ?? 128;
  $("sleep").value = cfg.settings.tftOff ?? 15;
  $("wake").checked = cfg.settings.wop ?? true;
  $("ota").checked = cfg.settings.ota ?? false;
}
function configState() {
  return JSON.stringify(
    [
      cfg,
      ...[
        "ssid",
        "wifiPassword",
        "screenLanguage",
        "brightness",
        "sleep",
        "defaultDevice",
      ].map((id) => $(id).value),
      $("wake").checked,
      $("ota").checked,
    ],
    (key, value) =>
      [
        "health",
        "devices",
        "expanded",
        "verified",
        "lastError",
        "needsCode",
        "tfaKey",
        "expiresAt",
        "accessCodeSaved",
        "tokenSaved",
        "passwordSaved",
        "passwordLocked",
      ].includes(key)
        ? undefined
        : value,
  );
}
function updatePending() {
  const dirty = !!cfg && !!cleanState && configState() !== cleanState;
  $("save").classList.toggle("pending", dirty);
}
for (const event of ["input", "change", "click"])
  document.addEventListener(event, () => queueMicrotask(updatePending));
$("importBackup").onclick = () => $("backupFile").click();
$("backupFile").onchange = async () => {
  const file = $("backupFile").files[0];
  if (!file) return;
  try {
    if (file.size > 32768)
      throw Error(t("备份文件过大。", "Backup file too large."));
    const next = JSON.parse(await file.text());
    if (
      !next ||
      typeof next.ssid !== "string" ||
      !Array.isArray(next.printers) ||
      next.printers.length > 100 ||
      next.printers.some(
        (p) => p && (!p.serialNumber || typeof p.serialNumber !== "string"),
      ) ||
      (next.cloudAccounts &&
        (!Array.isArray(next.cloudAccounts) || next.cloudAccounts.length > 4))
    )
      throw Error(t("配置备份格式错误。", "Invalid configuration backup."));
    if (
      !confirm(
        t(
          "载入此备份？检查后保存并重启生效。",
          "Load this backup? Review, then save and restart.",
        ),
      )
    )
      return;
    next.settings = next.settings || cfg.settings;
    next.cloudAccounts = next.cloudAccounts || [];
    for (const a of next.cloudAccounts) {
      a.expanded = false;
      a.signedIn = !!(a.signedIn || a.userId || a.passwordEnc);
    }
    cfg = next;
    cfg.printers = cfg.printers.filter(Boolean);
    cfg.activePrinter = Math.min(
      cfg.activePrinter || 0,
      Math.max(0, cfg.printers.length - 1),
    );
    applyFields();
    $("wifiPassword").value = cfg.pwd || "";
    selected = 0;
    profiles();
    accounts();
    updatePending();
    message(
      t(
        "已载入备份，请检查后保存并重启。",
        "Backup loaded. Review, then save and restart.",
      ),
    );
  } catch (e) {
    message(
      e instanceof SyntaxError
        ? t("配置备份格式错误。", "Invalid configuration backup.")
        : failure(e),
      true,
    );
  } finally {
    $("backupFile").value = "";
  }
};
