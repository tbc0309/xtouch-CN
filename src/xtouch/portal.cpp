#include "portal.h"
#include "storage.h"
#include "profile_schema.h"
#include "cloud.h"
#include "vault.h"
#include <Update.h>
#include "printers.h"
#include "ui/i18n/language.h"
#include <Arduino.h>
#include <WiFi.h>
#include <XTouchWebServer.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include <lvgl.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/pkcs5.h>
#include "portal_assets.h"

extern "C" { LV_FONT_DECLARE(ui_font_portal_title_22); void ui_confirmPanel_show(const char*,void(*)(void)); void loadScreen(int); extern lv_obj_t *introScreen; extern lv_obj_t *introScreenCaption; extern lv_obj_t *introScreenIcon; }
extern void xtouch_portal_release_transport();
namespace {
XTouchWebServer server(80);DNSServer dns;
String session,csrf,adminSalt,adminHash,apSsid;
bool mustChange=true,apMode=false,qrActive=false;
int showRequest=0;uint32_t rebootAt=0,sessionTouched=0,lockedUntil=0,cloudNext=0,codeNext=0,connectAt=0,scanAt=0;unsigned failures=0;
String scanCache="[]";bool scanning=false;
bool uploading=false,uploadOk=false,uploadDenied=false;unsigned uploadFiles=0;String uploadError,accountTarget;
struct AccountHealth{String id;int state=0;bool renewed=false;};
AccountHealth health[4];unsigned healthCount=0,healthIndex=0;bool background=false,renewing=false;uint32_t checkAt=20000,nextSweep=21600000;String manualAccount;
void inspectResult(JsonVariantConst result,const String &id){
 if(result["ok"].as<bool>()&&result["devices"].is<JsonArrayConst>()){
  String serials;for(JsonObjectConst p:result["devices"].as<JsonArrayConst>())serials+=String("|")+(p["serialNumber"]|"")+"|";
  xtouch_printer_set_cloud_health(id.c_str(),1,serials.c_str());
 }else if(!strcmp(result["error"]|"","authentication_rejected"))xtouch_printer_set_cloud_health(id.c_str(),-1,nullptr);
}
String binding(JsonVariantConst a){return String(a["id"]|"")+":"+(a["region"]|"cn")+":"+(a["account"]|"");}
String randomHex(unsigned bytes=16) {String out;out.reserve(bytes*2);for(unsigned i=0;i<bytes;++i){char b[3];snprintf(b,sizeof(b),"%02x",(uint8_t)esp_random());out+=b;}return out;}
String passwordHash(const String &password,const String &salt) {
 uint8_t result[32];mbedtls_md_context_t context;mbedtls_md_init(&context);
 if(mbedtls_md_setup(&context,mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),1))return "";
 int rc=mbedtls_pkcs5_pbkdf2_hmac(&context,(const unsigned char *)password.c_str(),password.length(),(const unsigned char *)salt.c_str(),salt.length(),10000,sizeof(result),result);
 mbedtls_md_free(&context);if(rc)return "";String out;out.reserve(64);for(auto value:result){char b[3];snprintf(b,sizeof(b),"%02x",value);out+=b;}return out;
}
bool equal(const String &a,const String &b){if(a.length()!=b.length())return false;uint8_t diff=0;for(size_t i=0;i<a.length();++i)diff|=a[i]^b[i];return !diff;}
bool saveAdmin(){StaticJsonDocument<256>doc;doc["salt"]=adminSalt;doc["hash"]=adminHash;doc["mustChange"]=mustChange;File f=SPIFFS.open("/xtouch/admin.json.tmp",FILE_WRITE);if(!f)return false;size_t n=serializeJson(doc,f);f.flush();f.close();if(n!=measureJson(doc))return false;SPIFFS.remove("/xtouch/admin.json.previous");if(SPIFFS.exists("/xtouch/admin.json"))SPIFFS.rename("/xtouch/admin.json","/xtouch/admin.json.previous");if(!SPIFFS.rename("/xtouch/admin.json.tmp","/xtouch/admin.json")){SPIFFS.rename("/xtouch/admin.json.previous","/xtouch/admin.json");return false;}return true;}
void loadAdmin(){StaticJsonDocument<256>doc;File f=SPIFFS.open("/xtouch/admin.json");if(f&&deserializeJson(doc,f)==DeserializationError::Ok){adminSalt=doc["salt"]|"";adminHash=doc["hash"]|"";mustChange=doc["mustChange"]|true;}f.close();if(adminSalt.length()!=32||adminHash.length()!=64){adminSalt=randomHex();adminHash=passwordHash("12345678",adminSalt);mustChange=true;saveAdmin();}}
void headers(){server.sendHeader("Content-Security-Policy","default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' data:; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'; form-action 'self'");server.sendHeader("Cache-Control","no-store");server.sendHeader("X-Content-Type-Options","nosniff");server.sendHeader("X-Frame-Options","DENY");server.sendHeader("Referrer-Policy","no-referrer");}
void json(int code,const String &body){headers();server.send(code,"application/json; charset=utf-8",body);}
void error(int code,const char *name){json(code,String("{\"error\":\"")+name+"\"}");}
bool origin(){String value=server.header("Origin");return value.isEmpty()||value=="http://"+server.hostHeader();}
bool hasSession(){
 String cookie=server.header("Cookie"),needle="xtouch_session="+session;bool present=false;
 int start=0;while(start<cookie.length()){int end=cookie.indexOf(';',start);if(end<0)end=cookie.length();String part=cookie.substring(start,end);part.trim();if(equal(part,needle))present=true;start=end+1;}
 return !session.isEmpty()&&present&&millis()-sessionTouched<=1800000;
}
bool authenticated(bool changeAllowed=false){
 if(!hasSession()){error(401,"login_required");return false;}
 if(!origin()){error(403,"origin");return false;}
 if(server.method()!=HTTP_GET&&!equal(server.header("X-XTouch-CSRF"),csrf)){error(403,"csrf");return false;}
 sessionTouched=millis();if(mustChange&&!changeAllowed){error(403,"password_change_required");return false;}return true;
}
bool body(JsonDocument &doc,size_t limit=32768){if(!origin()){error(403,"origin");return false;}if(!server.hasArg("plain")||server.arg("plain").length()>limit||deserializeJson(doc,server.arg("plain"))||doc.overflowed()){error(400,"invalid_json");return false;}return true;}
void login(){
 if((int32_t)(millis()-lockedUntil)<0){server.sendHeader("Retry-After","30");error(429,"login_throttled");return;}
 DynamicJsonDocument doc(512);if(!body(doc,256))return;
 String password=doc["password"]|"";if(password.length()>128||!equal(passwordHash(password,adminSalt),adminHash)){
  if(++failures>=5){lockedUntil=millis()+30000;failures=0;}error(401,"invalid_password");return;
 }
 failures=0;bool same=hasSession();
 if(!same){session=randomHex();csrf=randomHex();}sessionTouched=millis();server.sendHeader("Set-Cookie","xtouch_session="+session+"; Path=/; HttpOnly; SameSite=Strict");
 json(200,String("{\"csrf\":\"")+csrf+"\",\"mustChange\":"+(mustChange?"true":"false")+"}");
}
void changePassword(){if(!authenticated(true))return;DynamicJsonDocument doc(512);if(!body(doc,384))return;String password=doc["password"]|"";
 if(password.length()<8||password.length()>64||password=="12345678"){error(400,"password_length");return;}
 String oldHash=adminHash,oldSalt=adminSalt;bool oldChange=mustChange;adminSalt=randomHex();adminHash=passwordHash(password,adminSalt);mustChange=false;
 if(!saveAdmin()){adminHash=oldHash;adminSalt=oldSalt;mustChange=oldChange;error(507,"storage_failed");return;}session="";csrf="";server.sendHeader("Set-Cookie","xtouch_session=; Max-Age=0; Path=/; HttpOnly; SameSite=Strict");json(200,"{\"ok\":true,\"loginRequired\":true}");
}
void configGet(){if(!authenticated())return;if(xtouch_cloud_busy()){error(409,"cloud_busy");return;}xtouch_portal_release_transport();DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);DynamicJsonDocument doc(max(size_t(4096),saved.capacity()+4096));if(!doc.set(saved)||doc.overflowed()){error(503,"low_memory");return;}if(doc.isNull()){doc.to<JsonObject>();doc["ssid"]="";doc["activePrinter"]=0;doc.createNestedArray("printers");}
 doc["wifiPasswordSaved"]=strlen(doc["pwd"]|"")>0;doc["pwd"]="";
 unsigned slot=0;for(JsonObject p:doc["printers"].as<JsonArray>()){p["health"]=xtouch_printer_cloud_health(slot++);p["accessCodeSaved"]=strlen(p["accessCode"]|"")>0;p["accessCode"]="";}
 for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()){a["tokenSaved"]=strlen(a["token"]|"")>0;a["token"]="";String plain; a["passwordSaved"]=xtouch_vault_open(a["passwordEnc"]|"",binding(a),plain);a["passwordLocked"]=strlen(a["passwordEnc"]|"")&&!a["passwordSaved"].as<bool>();if(a["passwordLocked"].as<bool>())a["tokenSaved"]=false;a.remove("passwordEnc");for(unsigned i=0;i<healthCount;++i)if(health[i].id==String(a["id"]|""))a["health"]=health[i].state<0?"invalid":health[i].state>0?"valid":"unknown";}
 DynamicJsonDocument settings=xtouch_storage_read("/xtouch/settings.json",0);doc["settings"]=settings;String out;serializeJson(doc,out);json(200,out);
}
bool mergeSecrets(DynamicJsonDocument &doc){
 DynamicJsonDocument old=xtouch_storage_read("/xtouch.json",0);
 if(old.isNull()&&SPIFFS.exists("/xtouch.json"))return false;
 if(!strlen(doc["pwd"]|""))doc["pwd"]=old["pwd"].as<String>();
 for(JsonObject p:doc["printers"].as<JsonArray>()) {
  if(strlen(p["accessCode"]|""))continue;
  for(JsonObject oldP:old["printers"].as<JsonArray>())if(!strcmp(p["serialNumber"]|"",oldP["serialNumber"]|"")&&!strcmp(p["host"]|"",oldP["host"]|""))p["accessCode"]=oldP["accessCode"];
 }
 for(JsonObject a:doc["cloudAccounts"].as<JsonArray>())if(!a["clearToken"].as<bool>()&&!strlen(a["token"]|""))
  for(JsonObject oldA:old["cloudAccounts"].as<JsonArray>())if(binding(a)==binding(oldA))a["token"]=oldA["token"];
 for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()) {
  if(a["rememberPassword"].as<bool>()) {
   String password=a["password"]|"";
   if(password.length()){String encrypted=xtouch_vault_seal(password,binding(a));if(encrypted.isEmpty())return false;a["passwordEnc"]=encrypted;}
   else if(!strlen(a["passwordEnc"]|""))for(JsonObject oldA:old["cloudAccounts"].as<JsonArray>())
    if(binding(a)==binding(oldA))a["passwordEnc"]=oldA["passwordEnc"];
  } else a.remove("passwordEnc");
  if(strlen(a["passwordEnc"]|"")){String plain;if(!xtouch_vault_open(a["passwordEnc"],binding(a),plain)){a["token"]="";a["userId"]="";}}
  a.remove("password");a.remove("passwordSaved");a.remove("passwordLocked");
  for(const char *key:{"health","expanded","devices","needsCode","verified","lastError","tfaKey"})a.remove(key);
 }
 return !doc.overflowed();
}
void configSave(){if(!authenticated())return;if(xtouch_cloud_busy()){error(409,"cloud_busy");return;}xtouch_portal_release_transport();DynamicJsonDocument doc(min(size_t(49152),max(size_t(4096),server.arg("plain").length()*2+2048)));if(!body(doc))return;for(JsonVariantConst p:doc["printers"].as<JsonArrayConst>())if(!XTouchSchema::asciiName(p["name"])){error(400,"invalid_name");return;}for(JsonVariantConst a:doc["cloudAccounts"].as<JsonArrayConst>())if(!XTouchSchema::asciiName(a["name"])){error(400,"invalid_name");return;}if(!mergeSecrets(doc)||!XTouchSchema::root(doc)){error(400,"invalid_configuration");return;}
 DynamicJsonDocument settings(2048);settings.set(doc["settings"]);
 const char *lang=settings["language"]|"zh";
 if(settings.overflowed()||xtouch_language_index(lang)<0||
 (settings.containsKey("backlight")&&(!settings["backlight"].is<int>()||settings["backlight"].as<int>()<0||settings["backlight"].as<int>()>255))||
 (settings.containsKey("tftOff")&&(!settings["tftOff"].is<int>()||settings["tftOff"].as<int>()<0||settings["tftOff"].as<int>()>240))){error(400,"invalid_settings");return;}
 for(const char *key:{"ota","wop","tftInvert","auxFan","chamberFan","chamberTemp"})if(settings.containsKey(key)&&!settings[key].is<bool>()){error(400,"invalid_settings");return;}
 doc.remove("settings");doc.remove("wifiPasswordSaved");
 for(JsonObject p:doc["printers"].as<JsonArray>()){p.remove("accessCodeSaved");p.remove("health");}for(JsonObject a:doc["cloudAccounts"].as<JsonArray>()){a.remove("tokenSaved");a.remove("clearToken");a.remove("health");}
 if(!xtouch_storage_write("/xtouch.json",doc)){error(507,"storage_failed");return;}
 if(!settings.isNull()&&!xtouch_storage_write("/xtouch/settings.json",settings)){error(507,"settings_failed");return;}
 json(200,"{\"ok\":true,\"restarting\":true}");rebootAt=millis()+1800;
}
void scan(){if(!apMode&&!authenticated())return;
 if(!scanning&&(!scanAt||millis()-scanAt>30000)){WiFi.scanNetworks(true,true);scanning=true;}
 json(200,String("{\"scanning\":")+(scanning?"true":"false")+",\"networks\":"+scanCache+"}");
}
void wifiSave(){if(!apMode&&!authenticated())return;DynamicJsonDocument input(512);if(!body(input,384))return;
 if(!XTouchSchema::text(input["ssid"],33,true)||!XTouchSchema::text(input["pwd"],65)){error(400,"invalid_wifi");return;}
 DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);DynamicJsonDocument root(max(size_t(1024),saved.capacity()+512));root.set(saved);root["ssid"]=input["ssid"];root["pwd"]=input["pwd"]|"";if(!root["printers"].is<JsonArray>())root.createNestedArray("printers");root["activePrinter"]=root["activePrinter"]|0;
 if(!xtouch_storage_write("/xtouch.json",root)){error(507,"storage_failed");return;}
 json(200,"{\"ok\":true,\"restarting\":true}");rebootAt=millis()+1800;
}
void cloudStart(){if(!authenticated())return;if(background){error(409,"checking_accounts");return;}checkAt=millis()+60000;if(server.arg("plain").indexOf("\"devices\"")<0&&(int32_t)(millis()-cloudNext)<0){server.sendHeader("Retry-After",String((cloudNext-millis()+999)/1000));error(429,"wait_before_retry");return;}xtouch_portal_release_transport();
 DynamicJsonDocument request(6144);if(!body(request,5500))return;
 if(!strcmp(request["action"]|"","login")&&!strlen(request["password"]|"")){
  DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);
  for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(!strcmp(a["id"]|"",request["accountId"]|"")&&!strcmp(a["account"]|"",request["account"]|"")&&!strcmp(a["region"]|"cn",request["region"]|"cn")){
   String password;if(xtouch_vault_open(a["passwordEnc"]|"",binding(a),password))request["password"]=password;
  }
 }
 bool sendingCode=!strcmp(request["action"]|"","send_code");if(sendingCode&&(int32_t)(millis()-codeNext)<0){error(429,"wait_before_retry");return;}
 if(!strlen(request["token"]|"")&&request["accountId"].is<const char *>()) {DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(!strcmp(a["id"]|"",request["accountId"])&&!strcmp(a["region"]|"cn",request["region"]|"cn")){String plain;if(a["rememberPassword"].as<bool>()&&strlen(a["passwordEnc"]|"")&&!xtouch_vault_open(a["passwordEnc"],binding(a),plain)){error(409,"password_key_changed");return;}request["token"]=a["token"];}}
 if(!xtouch_cloud_job_start(request.as<JsonVariantConst>())){error(400,"invalid_or_busy");return;}
 manualAccount=request["accountId"].as<String>();cloudNext=millis()+5000;if(sendingCode)codeNext=millis()+60000;json(202,"{\"busy\":true}");
}
// Internal reset preserves the SD source. A fresh key makes old encrypted passwords unreadable.
void factoryReset(){
 if(xtouch_cloud_busy()||uploading)return;
 if(!xtouch_vault_rotate()){Serial.println("[RESET] key rotation failed");return;}
 const char *paths[]={"/xtouch.json","/xtouch/settings.json","/xtouch/touch.json","/xtouch/hardware.json","/xtouch/admin.json"};
 for(const char *path:paths)for(const char *suffix:{"",".tmp",".previous"})SPIFFS.remove(String(path)+suffix);
 rebootAt=millis()+500;
}
void firmwareChunk(){
 // WebServer also invokes this callback for raw POST bodies, without HTTPUpload.
 if(!server.header("Content-Type").startsWith("multipart/form-data")){uploadOk=false;uploadError="application_firmware_required";return;}
 HTTPUpload &upload=server.upload();
 if(upload.status==UPLOAD_FILE_START){
  if(++uploadFiles>1){uploadError="one_firmware_required";Update.abort();return;}uploadOk=false;uploadError="";
  if(!authenticated()){uploadDenied=true;uploadError="login_required";return;}if(xtouch_cloud_busy()||rebootAt){uploadError="not_ready";return;}
  xtouch_portal_release_transport();uploading=true;
 }else if(upload.status==UPLOAD_FILE_WRITE && uploading && uploadError.isEmpty()){
  if(!Update.isRunning()){if(upload.currentSize<24||upload.buf[0]!=0xE9||upload.buf[12]||upload.buf[13]){uploadError="application_firmware_required";return;}if(!Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH)){uploadError="update_begin_failed";return;}}
  if(upload.totalSize+upload.currentSize>0x1e0000||Update.write(upload.buf,upload.currentSize)!=upload.currentSize){uploadError="firmware_write_failed";Update.abort();}
 }else if(upload.status==UPLOAD_FILE_END){
  if(uploading&&uploadError.isEmpty())uploadOk=true;
  if(!uploadOk&&uploadError.isEmpty())uploadError="invalid_firmware";
  if(!uploadOk)Update.abort();uploading=false;
 }else if(upload.status==UPLOAD_FILE_ABORTED){Update.abort();uploading=false;uploadOk=false;uploadDenied=false;uploadFiles=0;uploadError="upload_aborted";}
}
void qr(bool wifi) {
 loadScreen(-1);qrActive=true;lv_obj_add_flag(introScreenIcon,LV_OBJ_FLAG_HIDDEN);
 // The startup screen uses flex layout; QR controls need explicit coordinates.
 lv_obj_set_layout(introScreen,0);lv_obj_set_style_pad_all(introScreen,0,0);
 lv_label_set_text(introScreenCaption,wifi?apSsid.c_str():WiFi.localIP().toString().c_str());
 lv_obj_set_width(introScreenCaption,304);lv_label_set_long_mode(introScreenCaption,LV_LABEL_LONG_DOT);
 lv_obj_set_style_text_color(introScreenCaption,lv_color_white(),0);lv_obj_set_style_text_opa(introScreenCaption,LV_OPA_COVER,0);
 lv_obj_align(introScreenCaption,LV_ALIGN_BOTTOM_MID,0,0);
 lv_obj_t *title=lv_label_create(introScreen);
 lv_obj_set_style_text_font(title,&ui_font_portal_title_22,0);
 lv_obj_set_style_text_color(title,lv_color_white(),0);lv_obj_set_style_text_opa(title,LV_OPA_COVER,0);
 lv_obj_set_width(title,304);lv_obj_set_height(title,ui_font_portal_title_22.line_height);lv_obj_set_style_text_align(title,LV_TEXT_ALIGN_CENTER,0);lv_label_set_long_mode(title,LV_LABEL_LONG_DOT);
 lv_label_set_text(title,wifi?xtouch_local("扫码连接 Wi-Fi","Scan to join Wi-Fi"):xtouch_local("扫码管理","Scan to manage"));
 lv_obj_align(title,LV_ALIGN_TOP_MID,0,6);
 lv_obj_t *code=lv_qrcode_create(introScreen,144,lv_color_hex(0x101820),lv_color_white());lv_obj_align(code,LV_ALIGN_TOP_MID,0,48);
 String value=wifi?"WIFI:T:WPA;S:"+apSsid+";P:12345678;;":"http://"+WiFi.localIP().toString()+"/"+(accountTarget.length()?"#account="+accountTarget:"");lv_qrcode_update(code,value.c_str(),value.length());
 if(wifi){
  lv_obj_t *hint=lv_label_create(introScreen);lv_label_set_text(hint,"192.168.4.1  /  12345678");
  lv_obj_set_style_text_color(hint,lv_color_white(),0);lv_obj_align(hint,LV_ALIGN_TOP_MID,0,195);
 }else{
  lv_obj_t *back=lv_btn_create(introScreen);lv_obj_set_size(back,160,24);lv_obj_align(back,LV_ALIGN_TOP_MID,0,195);
  lv_obj_t *label=lv_label_create(back);lv_label_set_text(label,xtouch_local("返回设置","Back to settings"));lv_obj_center(label);
  lv_obj_add_event_cb(back,[](lv_event_t*){showRequest=3;},LV_EVENT_CLICKED,nullptr);
 }
}

void startAP(){if(apMode)return;apMode=true;WiFi.mode(WIFI_AP_STA);apSsid="XTOUCH-CN-"+String((uint32_t)ESP.getEfuseMac(),HEX);WiFi.softAP(apSsid.c_str(),"12345678");dns.start(53,"*",WiFi.softAPIP());qr(true);}
}
extern "C" void xtouch_portal_show_management(){accountTarget="";showRequest=1;}
extern "C" void xtouch_portal_show_account(const char *id){accountTarget=id;showRequest=1;}
extern "C" void xtouch_portal_request_wifi(){showRequest=2;}
extern "C" void xtouch_portal_request_reset(){showRequest=4;}
extern "C" bool xtouch_portal_screen_active(){return qrActive;}
bool xtouch_portal_ap(){return apMode;}
bool xtouch_portal_maintenance(){return rebootAt||uploading||xtouch_cloud_busy();}
void xtouch_portal_cloud_rejected(){
 const char *id=xtouch_printer_account();for(unsigned i=0;i<healthCount;++i)if(health[i].id==id&&!health[i].renewed&&!background){healthIndex=i;checkAt=millis()+5000;break;}
}
void xtouch_portal_begin(){
 WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);
 Serial.printf("[VAULT] authenticated encryption tests: %s\n",xtouch_vault_selftest()?"PASS":"FAIL");loadAdmin();const char *keys[]={"Cookie","Origin","X-XTouch-CSRF","Content-Type"};server.collectHeaders(keys,4);
 server.on("/",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/html; charset=utf-8",(const char*)portal_index,sizeof(portal_index));});
 server.on("/portal.js",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/javascript; charset=utf-8",(const char*)portal_script,sizeof(portal_script));});
 server.on("/portal.css",HTTP_GET,[]{headers();server.sendHeader("Content-Encoding","gzip");server.send_P(200,"text/css; charset=utf-8",(const char*)portal_style,sizeof(portal_style));});
 server.on("/api/info",HTTP_GET,[]{DynamicJsonDocument d(1024);d["ap"]=apMode;if(hasSession()&&!mustChange){d["ip"]=WiFi.localIP().toString();d["ssid"]=WiFi.SSID();lv_mem_monitor_t graphics;lv_mem_monitor(&graphics);d["lvglFree"]=graphics.free_size;d["lvglLargest"]=graphics.free_biggest_size;d["heap"]=ESP.getFreeHeap();d["minHeap"]=ESP.getMinFreeHeap();d["mode"]=xtouch_printer_cloud()?"Cloud":"LAN";d["clockReady"]=time(nullptr)>1577836800;d["uptime"]=millis()/1000;d["online"]=xtouch_printer_online();d["printer"]=xtouch_printer_name();d["version"]=XTOUCH_FIRMWARE_VERSION;d["resetReason"]=(int)esp_reset_reason();}String out;serializeJson(d,out);json(200,out);});
 server.on("/api/login",HTTP_POST,login);
 server.on("/api/password",HTTP_POST,changePassword);
 server.on("/api/logout",HTTP_POST,[]{if(!authenticated(true))return;session="";csrf="";xtouch_cloud_job_clear();json(200,"{\"ok\":true}");});
 server.on("/api/session",HTTP_GET,[]{if(!authenticated(true))return;json(200,String("{\"csrf\":\"")+csrf+"\",\"mustChange\":"+(mustChange?"true":"false")+"}");});
 server.on("/api/reset",HTTP_POST,[]{if(!authenticated())return;StaticJsonDocument<64> d;if(!body(d,64))return;if(strcmp(d["confirm"]|"","RESET")){error(400,"confirmation_required");return;}if(xtouch_cloud_busy()||uploading){error(409,"busy");return;}json(200,"{\"ok\":true}");factoryReset();});
 server.on("/api/firmware",HTTP_POST,[]{if(uploadDenied){uploadDenied=false;uploadFiles=0;return;}if(!authenticated()){Update.abort();uploadFiles=0;return;}uploadFiles=0;if(uploadOk&&uploadError.isEmpty())uploadOk=Update.end(true);if(uploadOk&&uploadError.isEmpty()){json(200,"{\"ok\":true}");rebootAt=millis()+1800;}else {Update.abort();error(400,uploadError.isEmpty()?"update_failed":uploadError.c_str());}},firmwareChunk);
 server.on("/api/config",HTTP_GET,configGet);server.on("/api/config",HTTP_POST,configSave);
 server.on("/api/wifi/scan",HTTP_GET,scan);server.on("/api/wifi",HTTP_POST,wifiSave);
 server.on("/api/backup",HTTP_GET,[]{if(!authenticated())return;DynamicJsonDocument saved=xtouch_storage_read("/xtouch.json",0);DynamicJsonDocument doc(saved.capacity()+2048);doc.set(saved);doc["settings"]=xtouch_storage_read("/xtouch/settings.json",0);if(doc.overflowed()){error(503,"low_memory");return;}String out;serializeJson(doc,out);server.sendHeader("Content-Disposition","attachment; filename=xtouch.json");json(200,out);});
 server.on("/api/cloud",HTTP_POST,cloudStart);server.on("/api/cloud",HTTP_GET,[]{if(!authenticated())return;if(background){json(200,"{\"busy\":true}");return;}
 String value=xtouch_cloud_job_result();if(!xtouch_cloud_busy()){
  DynamicJsonDocument d(min((size_t)18432,max((size_t)2048,value.length()*2)));deserializeJson(d,value);inspectResult(d,manualAccount);
  if(!strcmp(d["error"]|"","cloud_blocked"))cloudNext=millis()+60000;
  for(unsigned i=0;i<healthCount;++i)if(health[i].id==manualAccount){if(d["ok"].as<bool>())health[i].state=1;else if(!strcmp(d["error"]|"","authentication_rejected"))health[i].state=-1;}
 }json(200,value);});
 server.onNotFound([]{if(apMode){server.sendHeader("Location","http://192.168.4.1/");server.send(302,"text/plain","");}else error(404,"not_found");});server.begin();
 DynamicJsonDocument config=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:config["cloudAccounts"].as<JsonArray>())if(healthCount<4)health[healthCount++].id=a["id"].as<String>();String ssid=config["ssid"]|"",password=config["pwd"]|"";
 if(ssid.isEmpty())startAP();else{WiFi.begin(ssid.c_str(),password.c_str());connectAt=millis();}
}
void xtouch_portal_loop(){
 if((int32_t)(millis()-nextSweep)>=0&&!background){healthIndex=0;nextSweep=millis()+21600000;for(unsigned i=0;i<healthCount;++i)health[i].renewed=false;}

 if(background && !xtouch_cloud_busy()){
  String result=xtouch_cloud_job_result();DynamicJsonDocument d(min((size_t)18432,max((size_t)2048,result.length()*2)));deserializeJson(d,result);
  auto &h=health[healthIndex];
  if(renewing&&d["ok"].as<bool>()&&strlen(d["token"]|"")){
   DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(h.id==String(a["id"]|"")){a["token"]=d["token"];a["userId"]=d["userId"];}
   if(xtouch_storage_write("/xtouch.json",root)){h.state=0;if(h.id==xtouch_printer_account())xtouch_cloud_select(h.id.c_str());}else h.state=-1;
   renewing=false;background=false;checkAt=millis()+1000;xtouch_cloud_job_clear();
  }else if(!renewing&&!strcmp(d["error"]|"","authentication_rejected")&&!h.renewed){
   h.renewed=true;DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);StaticJsonDocument<1536> req;String password;
   for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(h.id==String(a["id"]|"")&&a["rememberPassword"].as<bool>()&&xtouch_vault_open(a["passwordEnc"]|"",binding(a),password)){
    req["action"]="login";req["accountId"]=h.id;req["region"]=a["region"];req["account"]=a["account"];req["password"]=password;
   }
   xtouch_cloud_job_clear();if(!req.isNull()&&xtouch_cloud_job_start(req.as<JsonVariantConst>()))renewing=true;
   else {h.state=-1;inspectResult(d,h.id);background=false;++healthIndex;}
  }else{
   h.state=d["ok"].as<bool>()?1:!strcmp(d["error"]|"","authentication_rejected")||renewing?-1:0;
   if(renewing)xtouch_printer_set_cloud_health(h.id.c_str(),-1,nullptr);else inspectResult(d,h.id);
   renewing=false;background=false;++healthIndex;checkAt=millis()+2000;xtouch_cloud_job_clear();
  }
 }
 if(!background && healthIndex<healthCount && WiFi.status()==WL_CONNECTED && time(nullptr)>1577836800 && !xtouch_cloud_busy() && !uploading && !rebootAt && (int32_t)(millis()-checkAt)>=0){
  DynamicJsonDocument root=xtouch_storage_read("/xtouch.json",0);StaticJsonDocument<3072> req;
  for(JsonObject a:root["cloudAccounts"].as<JsonArray>())if(health[healthIndex].id==String(a["id"]|"")){
   req["action"]="devices";req["region"]=a["region"];req["token"]=a["token"];
   String password;bool remembered=a["rememberPassword"].as<bool>()&&strlen(a["passwordEnc"]|"");
   if(remembered&&!xtouch_vault_open(a["passwordEnc"],binding(a),password)){req.clear();health[healthIndex].state=-1;xtouch_printer_set_cloud_health(health[healthIndex].id.c_str(),-1,nullptr);}
   else if(!strlen(a["token"]|"")&&remembered){req["action"]="login";req["accountId"]=a["id"];req["account"]=a["account"];req["password"]=password;renewing=true;health[healthIndex].renewed=true;}
  }
  xtouch_portal_release_transport();background=xtouch_cloud_job_start(req.as<JsonVariantConst>());if(!background){renewing=false;++healthIndex;}
 }

 server.handleClient();if(apMode)dns.processNextRequest();
 if(scanning){int count=WiFi.scanComplete();if(count>=0){DynamicJsonDocument doc(8192);JsonArray list=doc.to<JsonArray>();for(int i=0;i<count&&list.size()<30;++i){if(WiFi.SSID(i).isEmpty())continue;JsonObject n=list.createNestedObject();n["ssid"]=WiFi.SSID(i);n["rssi"]=WiFi.RSSI(i);n["open"]=WiFi.encryptionType(i)==WIFI_AUTH_OPEN;}scanCache="";serializeJson(doc,scanCache);WiFi.scanDelete();scanning=false;scanAt=millis();}else if(count==WIFI_SCAN_FAILED){scanning=false;scanAt=millis();}}
 if(connectAt&&WiFi.status()!=WL_CONNECTED&&millis()-connectAt>20000){connectAt=0;startAP();}
 int request=showRequest;showRequest=0;if(request==1){if(WiFi.status()==WL_CONNECTED)qr(false);else startAP();}else if(request==2){startAP();qr(true);}else if(request==3){qrActive=false;loadScreen(4);}else if(request==4){ui_confirmPanel_show(xtouch_local("重置设备？保留SD备份", "Reset device? SD backup remains."),factoryReset);}
 if(rebootAt&&(int32_t)(millis()-rebootAt)>=0)ESP.restart();
}
