// Pure configuration rules shared by the form and local checks. No persistence or network.
export const MODELS = [
  ["C12", "P1S"],
  ["C11", "P1P"],
  ["BL-P001", "X1 Carbon"],
  ["BL-P002", "X1"],
  ["C13", "X1E"],
  ["N1", "A1 mini"],
  ["N2S", "A1"],
  ["N6", "X2D"],
  ["N7", "P2S"],
  ["N9", "A2L"],
  ["O1C", "H2C"],
  ["O1C2", "H2C"],
  ["O1D", "H2D"],
  ["O1E", "H2D Pro"],
  ["O1S", "H2S"],
];
export const bytes = (value) => new TextEncoder().encode(value).length;
export const blankPrinter = () => ({
  name: "",
  host: "",
  accessCode: "",
  serialNumber: "",
  printerModel: "C12",
});
export function errorsFor(config) {
  const errors = [];
  const add = (field, reason) => errors.push({ field, reason });
  if (
    typeof config.ssid !== "string" ||
    !config.ssid ||
    bytes(config.ssid) > 32
  )
    add("ssid", "ssidInvalid");
  if (typeof config.pwd !== "string" || bytes(config.pwd) > 64)
    add("pwd", "passwordInvalid");
  for (const key of ["timeout", "coldboot"])
    if (
      !Number.isInteger(config[key]) ||
      config[key] < 0 ||
      config[key] > 30000
    )
      add(key, "delayInvalid");
  if (
    !Array.isArray(config.printers) ||
    config.printers.length < 1 ||
    config.printers.length > 100
  ) {
    add("printers", "countInvalid");
    return errors;
  }
  if (
    !Number.isInteger(config.activePrinter) ||
    config.activePrinter < 0 ||
    config.activePrinter >= config.printers.length
  )
    add("activePrinter", "activeInvalid");
  config.printers.forEach((p, i) => {
    if (!p || typeof p !== "object") {
      add(`p${i}-host`, "printerInvalid");
      return;
    }
    if (typeof p.name !== "string" || bytes(p.name) > 63)
      add(`p${i}-name`, "nameInvalid");
    if (
      typeof p.host !== "string" ||
      !p.host ||
      bytes(p.host) > 63 ||
      /[\s/:?#<>]/u.test(p.host)
    )
      add(`p${i}-host`, "hostInvalid");
    else if (
      /^\d+(\.\d+){3}$/.test(p.host) &&
      p.host.split(".").some((n) => Number(n) > 255)
    )
      add(`p${i}-host`, "hostInvalid");
    if (
      typeof p.accessCode !== "string" ||
      bytes(p.accessCode) !== 8 ||
      /[^\x21-\x7e]/.test(p.accessCode)
    )
      add(`p${i}-accessCode`, "codeInvalid");
    if (
      typeof p.serialNumber !== "string" ||
      !p.serialNumber ||
      bytes(p.serialNumber) > 31 ||
      !/^[A-Za-z0-9]+$/.test(p.serialNumber)
    )
      add(`p${i}-serialNumber`, "serialInvalid");
    if (typeof p.printerModel !== "string" || bytes(p.printerModel) > 31)
      add(`p${i}-printerModel`, "modelInvalid");
  });
  if (bytes(JSON.stringify(config)) > 32768) add("printers", "sizeInvalid");
  return errors;
}
export function importConfig(data) {
  if (!data || typeof data !== "object" || Array.isArray(data))
    throw Error("importInvalid");
  let list = Array.isArray(data.printers)
    ? data.printers
    : data.mqtt
      ? [data.mqtt]
      : null;
  if (!list || !list.length || list.length > 100) throw Error("countInvalid");
  const sourceActive = Number.isInteger(data.activePrinter)
    ? data.activePrinter
    : 0;
  const valid = list
    .map((p, index) => ({ p, index }))
    .filter(({ p }) => p !== null);
  if (
    !valid.length ||
    valid.some(({ p }) => !p || typeof p !== "object" || Array.isArray(p))
  )
    throw Error("importInvalid");
  const aliases = {
    "3DPrinter-X1": "BL-P002",
    "3DPrinter-X1-Carbon": "BL-P001",
  };
  return {
    ssid: typeof data.ssid === "string" ? data.ssid : "",
    pwd: typeof data.pwd === "string" ? data.pwd : "",
    timeout: data.timeout ?? 5000,
    coldboot: data.coldboot ?? 5000,
    activePrinter: Math.max(
      0,
      valid.findIndex(({ index }) => index === sourceActive),
    ),
    printers: valid.map(({ p }) => ({
      name: typeof p.name === "string" ? p.name : "",
      host: typeof p.host === "string" ? p.host : "",
      accessCode: typeof p.accessCode === "string" ? p.accessCode : "",
      serialNumber: typeof p.serialNumber === "string" ? p.serialNumber : "",
      printerModel:
        aliases[p.printerModel] ??
        (typeof p.printerModel === "string" ? p.printerModel : ""),
    })),
  };
}
export function redacted(config) {
  return {
    ...config,
    pwd: config.pwd ? "••••••••" : "",
    printers: config.printers.map((p) => ({ ...p, accessCode: "••••••••" })),
  };
}
