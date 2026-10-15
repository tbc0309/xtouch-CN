#ifndef _XLCD_JSONCONFIG
#define _XLCD_JSONCONFIG

#include <ArduinoJson.h>
#include "filesystem.h"
#include "paths.h"
#include "printers.h"
#include "storage.h"

struct XTouchPrinterProfile {
    char name[64];
    char host[64];
    char accessCode[9];
    char serialNumber[32];
    char printerModel[32];
    bool configured;
};
static XTouchPrinterProfile xtouch_printers[XTOUCH_MAX_PRINTERS] = {};
static uint8_t xtouch_active_printer = 0;
static int xtouch_pending_printer = -1;
static bool xtouch_profiles_loaded = false;

bool xtouch_printer_configured(uint8_t slot) {
    return slot < XTOUCH_MAX_PRINTERS && xtouch_printers[slot].configured;
}
uint8_t xtouch_printer_active(void) { return xtouch_active_printer; }
const char *xtouch_printer_name(void) {
    return xtouch_printer_configured(xtouch_active_printer)
        ? xtouch_printers[xtouch_active_printer].name : "--";
}
bool xtouch_printer_switch_pending(void) { return xtouch_pending_printer >= 0; }
bool xtouch_printer_request(uint8_t slot) {
    if (!xtouch_printer_configured(slot) || slot == xtouch_active_printer) return false;
    xtouch_pending_printer = slot; // Process after LVGL returns, never delete a clicked widget here.
    return true;
}

static void xtouch_printer_apply(uint8_t slot) {
    const auto &p = xtouch_printers[slot];
    xtouch_active_printer = slot;
    strlcpy(xTouchConfig.xTouchAccessCode, p.accessCode, sizeof(xTouchConfig.xTouchAccessCode));
    strlcpy(xTouchConfig.xTouchSerialNumber, p.serialNumber, sizeof(xTouchConfig.xTouchSerialNumber));
    strlcpy(xTouchConfig.xTouchHost, p.host, sizeof(xTouchConfig.xTouchHost));
    strlcpy(xTouchConfig.xTouchPrinterModel, p.printerModel, sizeof(xTouchConfig.xTouchPrinterModel));
    strlcpy(xTouchConfig.xTouchPrinterName, p.name, sizeof(xTouchConfig.xTouchPrinterName));
}

static void xtouch_printer_parse(JsonVariantConst value, uint8_t slot) {
    if (!value.is<JsonObjectConst>()) return;
    auto &p = xtouch_printers[slot];
    const char *host = value["host"] | "";
    const char *code = value["accessCode"] | "";
    const char *serial = value["serialNumber"] | "";
    const char *model = value["printerModel"] | "";
    const char *name = value["name"] | "";
    // Reject truncated credentials rather than silently connect to the wrong target.
    if (!host[0] || strlen(host) >= sizeof(p.host) || strlen(code) != 8 ||
        !serial[0] || strlen(serial) >= sizeof(p.serialNumber) ||
        strlen(model) >= sizeof(p.printerModel) || strlen(name) >= sizeof(p.name)) return;
    strlcpy(p.host, host, sizeof(p.host));
    strlcpy(p.accessCode, code, sizeof(p.accessCode));
    strlcpy(p.serialNumber, serial, sizeof(p.serialNumber));
    strlcpy(p.printerModel, model, sizeof(p.printerModel));
    if (name[0]) strlcpy(p.name, name, sizeof(p.name));
    else snprintf(p.name, sizeof(p.name), "Printer %u", unsigned(slot + 1));
    p.configured = true;
}


DynamicJsonDocument xtouch_load_config()
{
    DynamicJsonDocument config = xtouch_storage_read(xtouch_paths_config, 4096);
    if (!config.isNull() && !xtouch_profiles_loaded) {
        if (config["printers"].is<JsonArray>()) {
            JsonArray list = config["printers"].as<JsonArray>();
            for (uint8_t i = 0; i < XTOUCH_MAX_PRINTERS && i < list.size(); i++)
                xtouch_printer_parse(list[i], i);
        } else if (!config.containsKey("printers")) {
            xtouch_printer_parse(config["mqtt"], 0);
        }
        uint8_t selected = config["activePrinter"] | 0;
        if (!xtouch_printer_configured(selected)) {
            for (selected = 0; selected < XTOUCH_MAX_PRINTERS; selected++)
                if (xtouch_printer_configured(selected)) break;
        }
        if (selected < XTOUCH_MAX_PRINTERS) xtouch_printer_apply(selected);
        xtouch_profiles_loaded = true;
    }

    return config;
}

#endif
