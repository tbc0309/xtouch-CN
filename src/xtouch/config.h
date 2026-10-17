#ifndef _XLCD_JSONCONFIG
#define _XLCD_JSONCONFIG

#include <ArduinoJson.h>
#include "filesystem.h"
#include "paths.h"
#include "printers.h"
#include "storage.h"
#include "profile_schema.h"
#include "cloud.h"
#include "portal.h"

struct XTouchPrinterProfile {
    char name[64];
    char host[64];
    char accessCode[9];
    char serialNumber[32];
    char printerModel[32];
    char accountId[32];
    int8_t health;
    uint8_t source;
    bool cloud;
    bool configured;
};
static XTouchPrinterProfile *xtouch_printers;
static uint8_t xtouch_profile_capacity;
static uint8_t xtouch_active_printer = 0;
static int xtouch_pending_printer = -1;
static bool xtouch_profiles_loaded = false;

bool xtouch_printer_camera_credentials(char *host,size_t hostSize,char *code,size_t codeSize){
 if(!host||!code||hostSize<2||codeSize<9||!xtouch_printers||!xtouch_profile_capacity)return false;
 host[0]=code[0]=0;const auto &active=xtouch_printers[xtouch_active_printer];
 for(unsigned pass=0;pass<=xtouch_profile_capacity;++pass){unsigned slot=pass?pass-1:xtouch_active_printer;const auto &candidate=xtouch_printers[slot];
  if(!candidate.configured||strcmp(candidate.serialNumber,active.serialNumber)||strlen(candidate.accessCode)!=8)continue;
  unsigned a,b,c,d;char tail;if(sscanf(candidate.host,"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)!=4||a>255||b>255||c>255||d>255)continue;
  if(!(a==10||(a==172&&b>=16&&b<=31)||(a==192&&b==168)))continue;
  strlcpy(host,candidate.host,hostSize);strlcpy(code,candidate.accessCode,codeSize);return true;
 }
 return false;
}

bool xtouch_printer_cloud(void) { return xtouch_printers && xtouch_profile_capacity && xtouch_printers[xtouch_active_printer].cloud; }

uint8_t xtouch_printer_source(uint8_t slot) { return xtouch_printer_configured(slot)?xtouch_printers[slot].source:0; }
uint32_t xtouch_printer_source_color(uint8_t slot) { static const uint32_t colors[]={0x59D5E8,0xD4A5FF,0xFFCB70}; return colors[xtouch_printer_source(slot)]; }

const char *xtouch_printer_account(void){return xtouch_printer_cloud()?xtouch_printers[xtouch_active_printer].accountId:"";}
int xtouch_printer_cloud_health(uint8_t slot){return xtouch_printer_configured(slot)?xtouch_printers[slot].health:0;}
void xtouch_printer_set_cloud_health(const char *account,int state,const char *serials){
 for(unsigned i=0;i<xtouch_profile_capacity;++i){auto &p=xtouch_printers[i];if(!p.cloud||strcmp(p.accountId,account))continue;
  String needle=String("|")+p.serialNumber+"|";p.health=state==1&&serials&&!strstr(serials,needle.c_str())?-1:state;
 }
}
bool xtouch_printer_configured(uint8_t slot) {
    return xtouch_printers && slot < xtouch_profile_capacity && xtouch_printers[slot].configured;
}
uint8_t xtouch_printer_active(void) { return xtouch_active_printer; }
const char *xtouch_printer_name(void) {
    return xtouch_printer_configured(xtouch_active_printer)
        ? xtouch_printers[xtouch_active_printer].name : "--";
}
const char *xtouch_printer_name_at(uint8_t slot) {
    return xtouch_printer_configured(slot) ? xtouch_printers[slot].name : "--";
}
const char *xtouch_printer_model_at(uint8_t slot) {
    if (!xtouch_printer_configured(slot)) return "";
    if (slot==xtouch_active_printer && bambuStatus.printer_type[0]) return bambuStatus.printer_type;
    return xtouch_printers[slot].printerModel;
}
bool xtouch_printer_switch_pending(void) { return xtouch_pending_printer >= 0; }
bool xtouch_printer_request(uint8_t slot) {
    if (!xtouch_printer_configured(slot)) return false;
    if(xtouch_printers[slot].cloud&&xtouch_printers[slot].health<0){xtouch_portal_show_account(xtouch_printers[slot].accountId);return false;}
    if(slot==xtouch_active_printer)return false;
    xtouch_pending_printer = slot; // Process after LVGL returns, never delete a clicked widget here.
    return true;
}

static void xtouch_printer_apply(uint8_t slot) {
    const auto &p = xtouch_printers[slot];
    xtouch_active_printer = slot;
    xtouch_cloud_select(p.cloud ? p.accountId : "");
    strlcpy(xTouchConfig.xTouchAccessCode, p.accessCode, sizeof(xTouchConfig.xTouchAccessCode));
    strlcpy(xTouchConfig.xTouchSerialNumber, p.serialNumber, sizeof(xTouchConfig.xTouchSerialNumber));
    strlcpy(xTouchConfig.xTouchHost, p.host, sizeof(xTouchConfig.xTouchHost));
    strlcpy(xTouchConfig.xTouchPrinterModel, p.printerModel, sizeof(xTouchConfig.xTouchPrinterModel));
    strlcpy(xTouchConfig.xTouchPrinterName, p.name, sizeof(xTouchConfig.xTouchPrinterName));
}

static void xtouch_printer_parse(JsonVariantConst value, uint8_t slot) {
    if (slot>=xtouch_profile_capacity || !value.is<JsonObjectConst>()) return;
    auto &p = xtouch_printers[slot];
    const char *host = value["host"] | "";
    const char *code = value["accessCode"] | "";
    const char *serial = value["serialNumber"] | "";
    const char *model = value["printerModel"] | "";
    const char *name = value["name"] | "";
    if (!XTouchSchema::printer(value)) return;
    p.cloud = !strcmp(value["mode"] | "lan", "cloud");
    strlcpy(p.accountId, value["accountId"] | "", sizeof(p.accountId));
    strlcpy(p.host, host, sizeof(p.host));
    strlcpy(p.accessCode, code, sizeof(p.accessCode));
    strlcpy(p.serialNumber, serial, sizeof(p.serialNumber));
    strlcpy(p.printerModel, model, sizeof(p.printerModel));
    if (name[0]&&XTouchSchema::asciiName(value["name"])) strlcpy(p.name, name, sizeof(p.name));
    else snprintf(p.name, sizeof(p.name), "Printer %u", unsigned(slot + 1));
    p.configured = true;
}


DynamicJsonDocument xtouch_load_config()
{
    DynamicJsonDocument config = xtouch_storage_read(xtouch_paths_config, 0);
    if (!config.isNull() && !xtouch_profiles_loaded) {
        if(!xtouch_printers){
            xtouch_profile_capacity=config["printers"].is<JsonArray>()?min((size_t)XTOUCH_MAX_PRINTERS,config["printers"].size()):1;
            xtouch_printers=(XTouchPrinterProfile*)calloc(max(1,(int)xtouch_profile_capacity),sizeof(XTouchPrinterProfile));
        }
        if(!xtouch_printers){ConsoleError.println("[PRINTER] profile allocation failed");return config;}
        if (config["printers"].is<JsonArray>()) {
            JsonArray list = config["printers"].as<JsonArray>();
            for (uint8_t i = 0; i < XTOUCH_MAX_PRINTERS && i < list.size(); i++)
                xtouch_printer_parse(list[i], i);
            for(uint8_t i=0;i<xtouch_profile_capacity;++i) {
                auto &p=xtouch_printers[i]; p.source=p.cloud?1:0;
                if(p.cloud)for(JsonObject a:config["cloudAccounts"].as<JsonArray>())
                    if(!strcmp(a["id"]|"",p.accountId)&&!strcmp(a["loginMethod"]| (a["authMode"]|"token"),"password"))p.source=2;
            }
        } else if (!config.containsKey("printers")) {
            xtouch_printer_parse(config["mqtt"], 0);
        }
        unsigned requested=config["activePrinter"] | 0U;
        uint8_t selected=requested<XTOUCH_MAX_PRINTERS?requested:XTOUCH_MAX_PRINTERS;
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
