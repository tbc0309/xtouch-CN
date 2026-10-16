#include <driver/i2s.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include "xtouch/debug.h"
#include "xtouch/paths.h"
#include "xtouch/storage.h"
#include "xtouch/eeprom.h"
#include "xtouch/types.h"
#include "xtouch/bblp.h"
#include "xtouch/globals.h"
#include "xtouch/filesystem.h"
#include "ui/ui.h"
#include "xtouch/sdcard.h"
#include "xtouch/hms.h"

#if defined(__XTOUCH_SCREEN_28__)
#include "devices/2.8/screen.h"
#endif

#include "xtouch/settings.h"
#include "xtouch/net.h"
#include "xtouch/firmware.h"
#include "xtouch/ota_request.h"
#include "xtouch/mqtt.h"
#include "xtouch/sensors/chamber.h"
#include "xtouch/events.h"
#include "xtouch/connection.h"
#include "xtouch/coldboot.h"
#include "xtouch/portal.h"

void xtouch_portal_release_transport() {
  xtouch_pubSubClient.disconnect();xtouch_wiFiClientSecure.stop();
  xtouch_mqtt_hasStatus=false;xtouch_mqtt_nextAttempt=millis()+3000;
}

void xtouch_intro_show(void)
{
  ui_introScreen_screen_init();
  lv_disp_load_scr(introScreen);
  lv_timer_handler();
}

void setup()
{

#if XTOUCH_USE_SERIAL == true || XTOUCH_DEBUG_ERROR == true || XTOUCH_DEBUG_DEBUG == true || XTOUCH_DEBUG_INFO == true
  Serial.begin(115200);
#endif

#ifdef XTOUCH_ERROR_SELFTEST
  Serial.printf("[ERROR-CODES] formatting, actions and faults: %s\n",(xtouch_errors_selftest()&&hms_selftest())?"PASS":"FAIL");
#endif
  bool storageReady = xtouch_storage_begin();
  if (storageReady) xtouch_eeprom_setup();
  xtouch_globals_init();
  xtouch_screen_setup();
  xtouch_intro_show();
  while (!storageReady) {
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_STORAGE_ERROR);
    lv_timer_handler();
    delay(100);
  }
  xtouch_sdcard_setup();
  xtouch_storage_sync();
  // A recovery card containing only firmware.bin must not be blocked by
  // missing Wi-Fi/printer provisioning on an uninitialized device.
  if (!xtouch_storage_has(xtouch_paths_config)) xtouch_firmware_checkFirmwareUpdate();
  xtouch_coldboot_check();

  xtouch_settings_loadSettings();

  xtouch_firmware_checkFirmwareUpdate();

  xtouch_touch_setup();

  { DynamicJsonDocument bootConfig=xtouch_load_config(); }
  xtouch_portal_begin();
  while (WiFi.status() != WL_CONNECTED) {
    lv_timer_handler(); xtouch_portal_loop(); xtouch_sdcard_poll(false); delay(5);
  }

  // Non-blocking time synchronization also dates changes when OTA is disabled.
  xtouch_start_clock_sync();

  xtouch_firmware_checkOnlineFirmwareUpdate();

  xtouch_screen_setupScreenTimer();
  xtouch_setupGlobalEvents();

  xtouch_mqtt_setup();
  xtouch_chamber_timer_init();
}

void loop()
{
  lv_timer_handler();
  lv_task_handler();
  if (xtouch_language_apply_request()) {
    xtouch_settings_save();
    ui_confirmPanel_hide();
    lv_obj_del(ui_confirmComponent);
    lv_obj_del(ui_hmsComponent);
    initTopLayer();
    loadScreen(4);
  }
  if(xtouch_ota_take_request()) {
    // Check after returning from the touch event, with printer TLS buffers freed.
    xtouch_pubSubClient.disconnect();xtouch_wiFiClientSecure.stop();xtouch_mqtt_hasStatus=false;
    loadScreen(-1);xtouch_firmware_checkOnlineFirmwareUpdate(true);
    uint32_t shown=millis();while(millis()-shown<1800){lv_timer_handler();delay(5);}
    loadScreen(4);xtouch_mqtt_nextAttempt=millis();
  }
  xtouch_portal_loop();
  xtouch_device_filament_poll();
  if (xtouch_portal_maintenance()) {
    if (xtouch_pubSubClient.connected()) xtouch_pubSubClient.disconnect();
    xtouch_wiFiClientSecure.stop();
    if (xtouch_cloud_job_pending()) xtouch_cloud_job_result();
  } else xtouch_mqtt_loop();
  static bool clockReady = false;
  if (!clockReady && time(NULL) > 1577836800) {
    xtouch_storage_clock_ready();
    xtouch_storage_sync(true);
    clockReady = true;
  }
  xtouch_sdcard_poll();
  if (xtouch_storage_restart_required() || xtouch_sdcard_firmware_waiting) {
    // Imported configuration is applied as one consistent boot snapshot.
    delay(200);
    ESP.restart();
  }
}
