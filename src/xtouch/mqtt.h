#ifndef _XLCD_MQTT
#define _XLCD_MQTT
#include "ui/printer_models.h"
#include "ui/printer_picker.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "ui/ui_msgs.h"
#include "types.h"
#include "autogrowstream.h"
#include "bbl-certs.h"
// #include "xtouch/ams-status.hpp"

WiFiClientSecure xtouch_wiFiClientSecure;
PubSubClient xtouch_pubSubClient(xtouch_wiFiClientSecure);

String xtouch_mqtt_request_topic;
String xtouch_mqtt_report_topic;

#include "ams.h"
#include "device.h"
#include "config.h"

#define XTOUCH_MQTT_SERVER_TIMEOUT 20
#define XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT 15
#define XTOUCH_MQTT_SERVER_JSON_PARSE_SIZE 8192

/* ---------------------------------------------- */
uint32_t xtouch_mqtt_lastPushStatus = 0;
static uint32_t xtouch_mqtt_nextAttempt = 0;
static uint32_t xtouch_mqtt_retryMs = 5000;
static bool xtouch_mqtt_hasStatus = false;

bool xtouch_printer_online(void) {
    return xtouch_pubSubClient.connected() && xtouch_mqtt_hasStatus;
}

XtouchAutoGrowBufferStream stream;

void xtouch_mqtt_sendMsg(XTOUCH_MESSAGE message, unsigned long long data = 0)
{
    XTOUCH_MESSAGE_DATA eventData;
    eventData.data = data;
    lv_msg_send(message, &eventData);
}

void xtouch_mqtt_topic_setup()
{
    String xtouch_device_topic = String("device/") + xTouchConfig.xTouchSerialNumber;
    xtouch_mqtt_request_topic = xtouch_device_topic + String("/request");
    xtouch_mqtt_report_topic = xtouch_device_topic + String("/report");
}

String xtouch_mqtt_parse_printer_type(String type_str)
{
    return String(ui_printer_model_code(type_str.c_str()));
}

// Printer payloads are external input; never let names or IDs overrun state.
template <size_t N>
static void xtouch_mqtt_copy(char (&target)[N], const char *source) {
    strlcpy(target, source ? source : "", N);
}

void xtouch_mqtt_update_slice_info(const char *project_id, const char *profile_id, const char *subtask_id, int plate_idx)
{
    xtouch_mqtt_copy(bambuStatus.project_id_, project_id);
    xtouch_mqtt_copy(bambuStatus.profile_id_, profile_id);
    xtouch_mqtt_copy(bambuStatus.subtask_id_, subtask_id);
}

void xtouch_mqtt_processPushStatus(JsonDocument &incomingJson)
{
    xtouch_mqtt_lastPushStatus = millis();
    xtouch_mqtt_hasStatus = true;
    ConsoleDebug.println(F("[XTouch][MQTT] ProcessPushStatus"));

    if (incomingJson != NULL && incomingJson.containsKey("print"))
    {
        // #pragma region printing

        if (incomingJson["print"].containsKey("print_type"))
        {
            xtouch_mqtt_copy(bambuStatus.print_type, incomingJson["print"]["print_type"]);
        }
        if (incomingJson["print"].containsKey("home_flag"))
        {
            bambuStatus.home_flag = incomingJson["print"]["home_flag"].as<int>();
        }

        if (incomingJson["print"].containsKey("hw_switch_state"))
        {
            bambuStatus.hw_switch_state = incomingJson["print"]["hw_switch_state"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_remaining_time"))
        {
            if (incomingJson["print"]["mc_remaining_time"].is<String>())
            {
                String timeStr = incomingJson["print"]["mc_remaining_time"].as<String>();

                bambuStatus.mc_left_time = atoi(timeStr.c_str()) * 60;
            }
            else if (incomingJson["print"]["mc_remaining_time"].is<int>())
            {
                bambuStatus.mc_left_time = incomingJson["print"]["mc_remaining_time"].as<int>() * 60;
            }
        }

        if (incomingJson["print"].containsKey("mc_percent"))
        {
            if (incomingJson["print"]["mc_percent"].is<String>())
                bambuStatus.mc_print_percent = atoi(incomingJson["print"]["mc_percent"].as<String>().c_str());
            else if (incomingJson["print"]["mc_percent"].is<int>())
                bambuStatus.mc_print_percent = incomingJson["print"]["mc_percent"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_sub_stage"))
        {
            bambuStatus.mc_print_sub_stage = incomingJson["print"]["mc_print_sub_stage"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_stage"))
        {
            if (incomingJson["print"]["mc_print_stage"].is<String>())
                bambuStatus.mc_print_stage = atoi(incomingJson["print"]["mc_print_stage"].as<String>().c_str());
            if (incomingJson["print"]["mc_print_stage"].is<int>())
                bambuStatus.mc_print_stage = incomingJson["print"]["mc_print_stage"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_error_code"))
        {
            if (incomingJson["print"]["mc_print_error_code"].is<int>())
                bambuStatus.mc_print_error_code = incomingJson["print"]["mc_print_error_code"].as<int>();
        }

        if (incomingJson["print"].containsKey("mc_print_line_number"))
        {
            if (incomingJson["print"]["mc_print_line_number"].is<String>())
            {
                String mc_print_line_number_str = incomingJson["print"]["mc_print_line_number"].as<String>();
                if (mc_print_line_number_str != "")
                {
                    bambuStatus.mc_print_line_number = atoi(mc_print_line_number_str.c_str());
                }
            }
        }

        if (incomingJson["print"].containsKey("print_error"))
        {
            if (incomingJson["print"]["print_error"].is<int>())
            {
                char prefix_str[9];
                bambuStatus.print_error = incomingJson["print"]["print_error"].as<int>();
                sprintf(prefix_str, "%08X", bambuStatus.print_error);

                if (xtouch_errors_isKeyPresent(prefix_str, device_error_keys, device_error_length))
                {
                    hms_enqueue(incomingJson["print"]["print_error"].as<unsigned long long>());
                    xtouch_mqtt_sendMsg(XTOUCH_ON_ERROR, 0);
                }
            }
        }

        // #pragma endregion

        // #pragma region online
        // #pragma endregion

        // #pragma region print_task
        if (incomingJson["print"].containsKey("printer_type"))
        {
            String code = xtouch_mqtt_parse_printer_type(incomingJson["print"]["printer_type"].as<String>());
            if (code.length() && code != bambuStatus.printer_type) {
                xtouch_mqtt_copy(bambuStatus.printer_type, code.c_str());
                xtouch_mqtt_sendMsg(XTOUCH_ON_MODEL);
            }
        }

        if (incomingJson["print"].containsKey("subtask_name"))
        {
            xtouch_mqtt_copy(bambuStatus.subtask_name, incomingJson["print"]["subtask_name"]);
        }

        if (incomingJson["print"].containsKey("layer_num"))
        {
            bambuStatus.current_layer = incomingJson["print"]["layer_num"].as<int>();
        }

        if (incomingJson["print"].containsKey("total_layer_num"))
        {
            bambuStatus.total_layers = incomingJson["print"]["total_layer_num"].as<int>();
        }

        if (incomingJson["print"].containsKey("gcode_state"))
        {
            xtouch_device_set_print_state(incomingJson["print"]["gcode_state"].as<String>());
        }

        if (incomingJson["print"].containsKey("queue_number"))
        {
            bambuStatus.queue_number = incomingJson["print"]["queue_number"].as<int>();
        }

        if (incomingJson["print"].containsKey("task_id"))
        {
            xtouch_mqtt_copy(bambuStatus.task_id, incomingJson["print"]["task_id"]);
        }

        if (incomingJson["print"].containsKey("gcode_file"))
        {
            xtouch_mqtt_copy(bambuStatus.gcode_file, incomingJson["print"]["gcode_file"]);
        }

        if (incomingJson["print"].containsKey("gcode_file_prepare_percent"))
        {
            String percent_str = incomingJson["print"]["gcode_file_prepare_percent"].as<String>();
            if (percent_str != "")
            {
                bambuStatus.gcode_file_prepare_percent = atoi(percent_str.c_str());
            }
        }

        if (incomingJson["print"].containsKey("project_id") && incomingJson["print"].containsKey("profile_id") && incomingJson["print"].containsKey("subtask_id"))
        {
            String obj_subtask_id_string = incomingJson["print"]["subtask_id"].as<String>();
            xtouch_mqtt_copy(bambuStatus.obj_subtask_id, obj_subtask_id_string.c_str());

            int plate_index = -1;
            /* parse local plate_index from task */
            if (obj_subtask_id_string == "0" && incomingJson["print"]["profile_id"].as<String>() != "0")
            {
                if (incomingJson["print"].containsKey("gcode_file"))
                {
                    String gcode_file_string = incomingJson["print"]["gcode_file"].as<String>();
                    xtouch_mqtt_copy(bambuStatus.gcode_file, incomingJson["print"]["gcode_file"]);

                    int idx_start = gcode_file_string.lastIndexOf("_") + 1;
                    int idx_end = gcode_file_string.lastIndexOf(".");
                    if (idx_start > 0 && idx_end > idx_start)
                    {
                        plate_index = atoi(gcode_file_string.substring(idx_start, idx_end - idx_start).c_str());
                        bambuStatus.plate_index = plate_index;
                    }
                }
            }
            xtouch_mqtt_update_slice_info(incomingJson["print"]["project_id"], incomingJson["print"]["profile_id"], incomingJson["print"]["subtask_id"], plate_index);

            xtouch_mqtt_copy(bambuStatus.task_id, incomingJson["print"]["subtask_id"]);
        }
        // #pragma region print_task

        // #pragma region status

        if (incomingJson["print"].containsKey("bed_temper"))
        {
            bambuStatus.bed_temper = incomingJson["print"]["bed_temper"].as<double>();
            xtouch_mqtt_sendMsg(XTOUCH_ON_BED_TEMP, bambuStatus.bed_temper);
        }

        if (incomingJson["print"].containsKey("bed_target_temper"))
        {
            bambuStatus.bed_target_temper = incomingJson["print"]["bed_target_temper"].as<double>();
            xtouch_mqtt_sendMsg(XTOUCH_ON_BED_TARGET_TEMP, bambuStatus.bed_target_temper);
        }

        if (incomingJson["print"].containsKey("frame_temper"))
        {
            bambuStatus.frame_temp = incomingJson["print"]["frame_temper"].as<double>();
        }

        if (incomingJson["print"].containsKey("nozzle_temper"))
        {
            bambuStatus.nozzle_temper = incomingJson["print"]["nozzle_temper"].as<double>();
            xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TEMP, bambuStatus.nozzle_temper);
        }

        if (incomingJson["print"].containsKey("nozzle_target_temper"))
        {
            bambuStatus.nozzle_target_temper = incomingJson["print"]["nozzle_target_temper"].as<double>();
            xtouch_mqtt_sendMsg(XTOUCH_ON_NOZZLE_TARGET_TEMP, bambuStatus.nozzle_target_temper);
        }

        if (incomingJson["print"].containsKey("chamber_temper"))
        {
            bambuStatus.chamber_temper = incomingJson["print"]["chamber_temper"].as<double>();
            xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP, bambuStatus.chamber_temper);
        }

        // BambuStudio DevChamber: legacy target and newer packed CTC telemetry.
        if (incomingJson["print"]["ctt"].is<int>())
            bambuStatus.chamber_target_temper = incomingJson["print"]["ctt"].as<int>();
        JsonVariant packed = incomingJson["print"]["device"]["ctc"]["info"]["temp"];
        if (packed.is<uint32_t>() || packed.is<int32_t>()) {
            uint32_t value = packed.as<uint32_t>();
            unsigned actual = value & 0xFFFF, target = value >> 16;
            // 0xFFFF denotes unavailable. Do not overwrite a valid temperature.
            if (actual != 0xFFFF && actual <= 200) {
                bambuStatus.chamber_temper = actual;
                xtouch_mqtt_sendMsg(XTOUCH_ON_CHAMBER_TEMP, actual);
            }
            if (target != 0xFFFF && target <= 200) bambuStatus.chamber_target_temper = target;
        }

        // link_th
        // link_ams
        if (incomingJson["print"].containsKey("wifi_signal"))
        {
            String wifi_signal = incomingJson["print"]["wifi_signal"].as<String>();
            wifi_signal.replace("dBm", "");
            bambuStatus.wifi_signal = abs(wifi_signal.toInt());
            xtouch_mqtt_sendMsg(XTOUCH_ON_WIFI_SIGNAL, bambuStatus.wifi_signal);
        }

        if (incomingJson["print"].containsKey("fan_gear"))
        {
            uint32_t fan_gear = incomingJson["print"]["fan_gear"].as<uint32_t>();
            bambuStatus.cooling_fan_speed = (int)((fan_gear & 0x000000FF) >> 0);
            bambuStatus.big_fan1_speed = (int)((fan_gear & 0x0000FF00) >> 8);
            bambuStatus.big_fan2_speed = (int)((fan_gear & 0x00FF0000) >> 16);
            xtouch_mqtt_sendMsg(XTOUCH_ON_PART_FAN_SPEED, bambuStatus.cooling_fan_speed);
            xtouch_mqtt_sendMsg(XTOUCH_ON_PART_AUX_SPEED, bambuStatus.big_fan1_speed);
            xtouch_mqtt_sendMsg(XTOUCH_ON_PART_CHAMBER_SPEED, bambuStatus.big_fan2_speed);
        }
        else
        {
            if (incomingJson["print"].containsKey("cooling_fan_speed"))
            {
                int speed = incomingJson["print"]["cooling_fan_speed"].as<int>();
                bambuStatus.cooling_fan_speed = round(floor(speed / float(1.5)) * float(25.5));
                xtouch_mqtt_sendMsg(XTOUCH_ON_PART_FAN_SPEED, bambuStatus.cooling_fan_speed);
            }

            if (incomingJson["print"].containsKey("big_fan1_speed"))
            {
                int speed = incomingJson["print"]["big_fan1_speed"].as<int>();
                bambuStatus.big_fan1_speed = round(floor(speed / float(1.5)) * float(25.5));
                xtouch_mqtt_sendMsg(XTOUCH_ON_PART_AUX_SPEED, bambuStatus.big_fan1_speed);
            }

            if (incomingJson["print"].containsKey("big_fan2_speed"))
            {
                int speed = incomingJson["print"]["big_fan2_speed"].as<int>();
                bambuStatus.big_fan2_speed = round(floor(speed / float(1.5)) * float(25.5));
                xtouch_mqtt_sendMsg(XTOUCH_ON_PART_CHAMBER_SPEED, bambuStatus.big_fan2_speed);
            }
        }

        // heatbreak_fan_speed

        if (incomingJson["print"].containsKey("spd_lvl"))
        {
            bambuStatus.printing_speed_lvl = incomingJson["print"]["spd_lvl"].as<int>();
        }

        if (incomingJson["print"].containsKey("spd_mag"))
        {
            bambuStatus.printing_speed_mag = incomingJson["print"]["spd_mag"].as<int>();
        }

        // stg
        // stg_cur
        // filam_bak
        // mess_production_state
        // lifecycle

        if (incomingJson["print"].containsKey("lights_report"))
        {

            if (incomingJson["print"]["lights_report"][0].containsKey("mode"))
            {
                XTOUCH_MESSAGE_DATA eventData;
                if (incomingJson["print"]["lights_report"][0]["mode"] == "on")
                {
                    bambuStatus.chamberLed = true;
                    eventData.data = 1;
                }
                else
                {
                    bambuStatus.chamberLed = false;
                    eventData.data = 0;
                }
                lv_msg_send(XTOUCH_ON_LIGHT_REPORT, &eventData);
            }
        }

        // sdcard

        // #pragma endregion

        if (incomingJson["print"].containsKey("nozzle_diameter"))
        {
            if (incomingJson["print"]["nozzle_diameter"].is<float>())
            {
                bambuStatus.nozzle_diameter = incomingJson["print"]["nozzle_diameter"].as<float>();
            }
            else if (incomingJson["print"]["nozzle_diameter"].is<String>())
            {
                bambuStatus.nozzle_diameter = incomingJson["print"]["nozzle_diameter"].as<String>().toFloat();
            }
        }

        // #pragma region upgrade
        // #pragma endregion

        // #pragma region  camera
        if (incomingJson["print"].containsKey("ipcam"))
        {

            if (incomingJson["print"]["ipcam"].containsKey("ipcam_record"))
            {
                bambuStatus.camera_recording_when_printing = incomingJson["ipcam"]["ipcam_record"].as<String>() == "enable";
            }
            if (incomingJson["print"]["ipcam"].containsKey("timelapse"))
            {
                bambuStatus.camera_timelapse = incomingJson["print"]["ipcam"]["timelapse"].as<String>() == "enable";
            }
            if (incomingJson["print"]["ipcam"].containsKey("ipcam_dev"))
            {
                bambuStatus.has_ipcam = incomingJson["print"]["ipcam"]["ipcam_dev"].as<String>() == "1";
            }
            xtouch_mqtt_sendMsg(XTOUCH_ON_IPCAM);
        }

        // xcam

        // #pragma endregion

        // #pragma region hms
        if (incomingJson["print"].containsKey("hms"))
        {
            if (incomingJson["print"]["hms"].is<JsonArray>())
            {

                for (JsonVariant value : incomingJson["print"]["hms"].as<JsonArray>())
                {
                    JsonObject element = value.as<JsonObject>();
                    unsigned attr = element["attr"].as<unsigned>();
                    unsigned code = element["code"].as<unsigned>();
                    int module_id;
                    unsigned module_num;
                    unsigned part_id;
                    unsigned reserved;
                    int msg_level;
                    int msg_code;
                    unsigned int model_id_int = (attr >> 24) & 0xFF;
                    if (model_id_int < MODULE_MAX)
                        module_id = model_id_int;
                    else
                        module_id = MODULE_UKNOWN;
                    module_num = (attr >> 16) & 0xFF;
                    part_id = (attr >> 8) & 0xFF;
                    reserved = (attr >> 0) & 0xFF;
                    unsigned msg_level_int = code >> 16;
                    if (msg_level_int < HMS_MSG_LEVEL_MAX)
                        msg_level = msg_level_int;
                    else
                        msg_level = HMS_UNKNOWN;
                    msg_code = code & 0xFFFF;

                    char buffer[17];
                    sprintf(buffer, "%02X%02X%02X00000%01X%04X",
                            module_id,
                            module_num,
                            part_id,
                            msg_level,
                            msg_code);

                    char *endPtr;

                    unsigned long long intValue = strtoull(buffer, &endPtr, 16);

                    if (xtouch_errors_isKeyPresent(buffer, hms_error_values, hms_error_length))
                    {
                        hms_enqueue(intValue);
                        xtouch_mqtt_sendMsg(XTOUCH_ON_ERROR, 0);
                    }
                }
            }
        }

        // #pragma endregion

        // AMS reports are deltas under print.ams, not at the document root.
        JsonObjectConst ams = incomingJson["print"]["ams"].as<JsonObjectConst>();
        if (!ams.isNull()) {
            auto integer = [](JsonVariantConst v) -> long {
                return v.is<const char *>() ? strtol(v.as<const char *>(), nullptr, 10) : v.as<long>();
            };
            auto number = [](JsonVariantConst v, double &value) -> bool {
                if (v.is<const char *>()) {
                    const char *text = v.as<const char *>(); char *end;
                    if (!text || !*text) return false;
                    value = strtod(text, &end);
                    return *end == 0 && isfinite(value);
                }
                if (!v.is<double>() && !v.is<long>()) return false;
                value = v.as<double>(); return isfinite(value);
            };
            // Presence masks are hexadecimal strings; tray numbers are decimal.
            auto bits = [](JsonVariantConst v) -> long {
                return v.is<const char *>() ? strtoul(v.as<const char *>(), nullptr, 16) : v.as<long>();
            };
            if (ams.containsKey("ams_exist_bits")) bambuStatus.ams_exist_bits = bits(ams["ams_exist_bits"]);
            if (ams.containsKey("tray_exist_bits")) {
                bambuStatus.tray_exist_bits = bits(ams["tray_exist_bits"]);
                for (unsigned i = 0; i < XTOUCH_MAX_AMS_SLOTS; ++i) if (!(bambuStatus.tray_exist_bits & (1UL << i))) {
                    bambuStatus.ams_tray_color_valid[i] = false;
                    bambuStatus.ams_tray_material[i][0] = 0;
                    bambuStatus.ams_nozzle_min[i] = bambuStatus.ams_nozzle_max[i] = 0;
                    bambuStatus.ams_remain_valid &= ~(1U << i);
                }
            }
            if (ams.containsKey("tray_now")) {
                bambuStatus.m_tray_now = integer(ams["tray_now"]);
                char value[12]; snprintf(value, sizeof(value), "%d", bambuStatus.m_tray_now);
                xtouch_ams_parse_tray_now(value);
            }
            if (ams.containsKey("tray_tar")) bambuStatus.m_tray_tar = integer(ams["tray_tar"]);
            if (ams.containsKey("version")) bambuStatus.ams_version = integer(ams["version"]);
            if (ams.containsKey("tray_is_bbl_bits")) bambuStatus.tray_is_bbl_bits = bits(ams["tray_is_bbl_bits"]);
            if (ams.containsKey("tray_read_done_bits")) bambuStatus.tray_read_done_bits = bits(ams["tray_read_done_bits"]);
            if (ams.containsKey("tray_reading_bits")) bambuStatus.tray_reading_bits = bits(ams["tray_reading_bits"]);
            JsonArrayConst units = ams["ams"].as<JsonArrayConst>();
            for (JsonObjectConst unit : units) {
                if (!unit.containsKey("id")) continue;
                int id = integer(unit["id"]);
                if (id < 0 || id >= XTOUCH_MAX_AMS_UNITS) continue;
                if (!ams.containsKey("ams_exist_bits")) bambuStatus.ams_exist_bits |= 1UL << id;
                if (unit.containsKey("humidity")) bambuStatus.ams_humidity = integer(unit["humidity"]);
                double value;
                if (unit.containsKey("humidity_raw")) {
                    if (number(unit["humidity_raw"], value) && value >= 0 && value <= 100) {
                        bambuStatus.ams_humidity_percent[id] = value;
                        bambuStatus.ams_humidity_valid |= 1U << id;
                    } else bambuStatus.ams_humidity_valid &= ~(1U << id);
                }
                if (unit.containsKey("temp")) {
                    if (number(unit["temp"], value) && value >= -20 && value <= 120) {
                        bambuStatus.ams_temperature_deci[id] = lround(value*10);
                        bambuStatus.ams_temperature_valid |= 1U << id;
                    } else bambuStatus.ams_temperature_valid &= ~(1U << id);
                }
                for (JsonObjectConst tray : unit["tray"].as<JsonArrayConst>()) {
                    if (!tray.containsKey("id")) continue;
                    int slot = integer(tray["id"]);
                    if (slot < 0 || slot >= 4) continue;
                    unsigned index = id * 4 + slot;
                    if (tray.containsKey("remain")) {
                        if (number(tray["remain"], value) && value >= 0 && value <= 100) {
                            bambuStatus.ams_remain_percent[index] = value;
                            bambuStatus.ams_remain_valid |= 1U << index;
                        } else bambuStatus.ams_remain_valid &= ~(1U << index);
                    }
                    if (tray.containsKey("nozzle_temp_min")) bambuStatus.ams_nozzle_min[index] =
                        number(tray["nozzle_temp_min"], value) && value>=150 && value<=350 ? value : 0;
                    if (tray.containsKey("nozzle_temp_max")) bambuStatus.ams_nozzle_max[index] =
                        number(tray["nozzle_temp_max"], value) && value>=150 && value<=350 ? value : 0;
                    if (tray.containsKey("tray_color")) {
                        const char *hex = tray["tray_color"] | "";
                        bool valid = strlen(hex) == 8;
                        for (unsigned c = 0; valid && c < 8; ++c) valid = isxdigit((unsigned char)hex[c]);
                        bambuStatus.ams_tray_color_valid[index] = valid;
                        if (valid) bambuStatus.ams_tray_color[index] = strtoul(hex, nullptr, 16) >> 8;
                    }
                    if (tray.containsKey("tray_type"))
                        strlcpy(bambuStatus.ams_tray_material[index], tray["tray_type"] | "", sizeof(bambuStatus.ams_tray_material[index]));
                }
            }
            if (ams.containsKey("ams") && units.size() == 0 && !ams.containsKey("ams_exist_bits"))
                bambuStatus.ams_exist_bits = 0;
            if (ams.containsKey("ams_exist_bits") || ams.containsKey("ams"))
                bambuStatus.ams = bambuStatus.ams_exist_bits != 0;
            if (!bambuStatus.ams) {
                memset(bambuStatus.ams_tray_color_valid, 0, sizeof(bambuStatus.ams_tray_color_valid));
                memset(bambuStatus.ams_tray_material, 0, sizeof(bambuStatus.ams_tray_material));
            }
            // Removing a unit invalidates its optional readings and all four slots.
            bambuStatus.ams_humidity_valid &= bambuStatus.ams_exist_bits;
            bambuStatus.ams_temperature_valid &= bambuStatus.ams_exist_bits;
            for (unsigned id=0; id<XTOUCH_MAX_AMS_UNITS; ++id) {
                if (bambuStatus.ams_exist_bits & (1U<<id)) continue;
                bambuStatus.ams_remain_valid &= ~(0xFU << (id*4));
                bambuStatus.tray_exist_bits &= ~(0xFUL << (id*4));
                for (unsigned slot=id*4; slot<id*4+4; ++slot) {
                    bambuStatus.ams_nozzle_min[slot]=bambuStatus.ams_nozzle_max[slot]=0;
                    bambuStatus.ams_tray_color_valid[slot]=false;
                    bambuStatus.ams_tray_material[slot][0]=0;
                }
            }
            bambuStatus.is_ams_need_update = true;
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS, bambuStatus.ams);
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS, 0);
        }
        if (incomingJson["print"].containsKey("ams_status")) {
            xtouch_ams_parse_status(incomingJson["print"]["ams_status"].as<int>());
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS, 0);
        }

        // vt_tray
        if (incomingJson["print"].containsKey("vt_tray"))
        {
            bambuStatus.ams_support_virtual_tray = true;
            JsonObjectConst external=incomingJson["print"]["vt_tray"].as<JsonObjectConst>();
            if(external.containsKey("tray_type")) bambuStatus.external_present=strlen(external["tray_type"] | "")>0;
            if(external.containsKey("tray_color")) {
                const char *hex=external["tray_color"] | "";bool valid=strlen(hex)==8;
                for(unsigned i=0;valid && i<8;++i) valid=isxdigit(hex[i]);
                bambuStatus.external_color=valid ? strtoul(hex,nullptr,16)>>8 : 0x777777;
            }
            auto temperature=[](JsonVariantConst v)->uint16_t {
                long value=0;
                if (v.is<const char *>()) {
                    const char *text=v.as<const char *>(); char *end;
                    value=strtol(text,&end,10); if(end==text || *end) return 0;
                } else if(v.is<int>()) value=v.as<int>();
                return value>=150 && value<=350 ? value : 0;
            };
            if(external.containsKey("nozzle_temp_min")) bambuStatus.external_nozzle_min=temperature(external["nozzle_temp_min"]);
            if(external.containsKey("nozzle_temp_max")) bambuStatus.external_nozzle_max=temperature(external["nozzle_temp_max"]);
            xtouch_mqtt_sendMsg(XTOUCH_ON_AMS_BITS,0);
        }
        else
        {
            // An omitted delta is not a capability removal.
        }
        // #pragma endregion

        if (incomingJson["print"].containsKey("gcode_state") ||
            incomingJson["print"].containsKey("layer_num") ||
            incomingJson["print"].containsKey("total_layer_num") ||
            incomingJson["print"].containsKey("mc_remaining_time") ||
            incomingJson["print"].containsKey("mc_percent") ||
            incomingJson["print"].containsKey("spd_lvl") ||
            incomingJson["print"].containsKey("spd_mag"))
        {

            xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
        }
    }
}

void xtouch_mqtt_parseMessage(char *topic, byte *payload, unsigned int length, byte type = 0)
{

    ConsoleDebug.println(F("[XTouch][MQTT] ParseMessage"));
    DynamicJsonDocument incomingJson(XTOUCH_MQTT_SERVER_JSON_PARSE_SIZE);

    DynamicJsonDocument amsFilter(384);
    amsFilter["print"]["*"] = true;
    amsFilter["camera"]["*"] = true;
    amsFilter["info"]["module"][0]["name"] = true;
    amsFilter["info"]["module"][0]["product_name"] = true;
    amsFilter["print"]["ams"] = type == 0;

    auto deserializeError = deserializeJson(incomingJson, payload, length, DeserializationOption::Filter(amsFilter));

    // xtouch_debug_json(incomingJson);
    if (!deserializeError)
    {
        // get_version is already requested on connect. Only OTA identifies the
        // printer; an AMS/accessory product name cannot replace its model.
        for (JsonObject module : incomingJson["info"]["module"].as<JsonArray>()) {
            const char *name = module["name"] | "";
            if (strcmp(name, "ota")) continue;
            const char *code = ui_printer_model_code(module["product_name"] | "");
            if (code[0] && strcmp(code, bambuStatus.printer_type)) {
                xtouch_mqtt_copy(bambuStatus.printer_type, code);
                xtouch_mqtt_sendMsg(XTOUCH_ON_MODEL);
            }
        }


        if ((millis() - xtouch_mqtt_lastPushStatus) > (XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT * 1000))
        {
            Serial.println(F("[XTouch][MQTT] Force Reconnect after no Push Status for 30s"));
            xtouch_pubSubClient.disconnect();
        }

        if (incomingJson.containsKey("print") && incomingJson["print"].containsKey("command"))
        {

            String command = incomingJson["print"]["command"].as<String>();

            if (command == "push_status")
            {
                xtouch_mqtt_processPushStatus(incomingJson);
            }
            else if (command=="ams_change_filament" || command=="ams_get_rfid") {
                xtouch_device_filament_reply(incomingJson["print"].as<JsonObjectConst>());
            }
            else if (command == "gcode_line")
            {
                ConsoleDebug.println(F("[XTouch][MQTT] gcode_line ack"));
                ConsoleDebug.write(payload, length);
                ConsoleDebug.println();
            }

            // project_file
            // ams_filament_setting
            // xcam_control_set
            // print_option
            // extrusion_cali | flowrate_cali
            // extrusion_cali_set
            // extrusion_cali_sel
            // extrusion_cali_get
            // extrusion_cali_get_result
            // flowrate_get_result
        }

        // info

        if (incomingJson.containsKey("camera"))
        {
            if (incomingJson["camera"].containsKey("command"))
            {
                if (incomingJson["camera"]["command"].as<String>() == "ipcam_timelapse")
                {
                    bambuStatus.camera_timelapse = incomingJson["camera"]["control"].as<String>() == "enable";
                }
                else if (incomingJson["camera"]["command"].as<String>() == "ipcam_record_set")
                {
                    bambuStatus.camera_recording_when_printing = incomingJson["camera"]["control"].as<String>() == "enable";
                }
                xtouch_mqtt_sendMsg(XTOUCH_ON_IPCAM);
            }
        }

        // upgrade
        // event info
    }
    else
    {
        ConsoleError.println(F("[XTouch][MQTT] ParseMessage deserializeJson failed"));
    }

    // if (firstParseMessage)
    // {
    //     firstParseMessage = false;
    //     xtouch_device_command_getPaCalibration();
    // }
}

void xtouch_pubSubClient_streamCallback(char *topic, byte *payload, unsigned int length)
{
    // Discard late packets from the previous device after a switch.
    if (strcmp(topic, xtouch_mqtt_report_topic.c_str()) != 0 || xtouch_printer_switch_pending()) {
        stream.flush();
        return;
    }
    xtouch_mqtt_parseMessage(topic, (byte *)stream.get_buffer(), stream.current_length(), 0);

    if (stream.includes("\"ams\""))
    {
        xtouch_mqtt_parseMessage(topic, (byte *)stream.get_buffer(), stream.current_length(), 1);
    }

    stream.flush();
}

const char *xtouch_mqtt_generateRandomKey(int keyLength)
{
    char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static char key[17];

    for (int i = 0; i < keyLength; i++)
    {
        int randomIndex = random(sizeof(charset) - 1);
        key[i] = charset[randomIndex];
    }

    key[keyLength] = '\0';

    return key;
}

// One bounded connection attempt; the main loop services touch between retries.
void xtouch_mqtt_connect()
{
    if (!xtouch_printer_configured(xtouch_active_printer) ||
        (int32_t)(millis() - xtouch_mqtt_nextAttempt) < 0) return;
    String clientId = "XTOUCH-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool transportReady = xtouch_wiFiClientSecure.connect(xTouchConfig.xTouchHost, 8883, 2000) == 1;
    if (transportReady) xtouch_wiFiClientSecure.setTimeout(2000);
    bool connected = transportReady &&
        xtouch_pubSubClient.connect(clientId.c_str(), "bblp", xTouchConfig.xTouchAccessCode);
    if (connected) {
        if (!xtouch_pubSubClient.subscribe(xtouch_mqtt_report_topic.c_str())) {
            xtouch_pubSubClient.disconnect();
        } else {
            xtouch_mqtt_retryMs = 5000;
            xtouch_mqtt_lastPushStatus = millis();
            xtouch_device_pushall();
            xtouch_device_get_version();
            ConsoleInfo.printf("[XTouch][PRINTER] slot=%u connected\n", xtouch_active_printer + 1);
        }
    }
    if (!xtouch_pubSubClient.connected()) {
        xtouch_wiFiClientSecure.stop();
        ConsoleError.printf("[XTouch][PRINTER] slot=%u retry state=%d\n",
                            xtouch_active_printer + 1, xtouch_pubSubClient.state());
        xtouch_mqtt_nextAttempt = millis() + xtouch_mqtt_retryMs;
        xtouch_mqtt_retryMs = min(xtouch_mqtt_retryMs * 2, uint32_t(30000));
    }
    xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
}

static void xtouch_mqtt_switch_printer()
{
    if (xtouch_pending_printer < 0) return;
    uint8_t slot = xtouch_pending_printer;
    xtouch_pubSubClient.disconnect();
    xtouch_wiFiClientSecure.stop();
    stream.flush();
    xtouch_printer_apply(slot);
    xtouch_pending_printer = -1;
    ui_printer_picker_close();
    memset(&bambuStatus, 0, sizeof(bambuStatus));
    bambuStatus.m_tray_now = 255;
    bambuStatus.m_tray_tar = 255;
    memset(&controlMode, 0, sizeof(controlMode));
    controlMode.inc = 1;
    lastPrintState = "IDLE";
    frontIndex = rearIndex = queueSize = 0;
    ui_hmsPanel_hide();
    ui_confirmPanel_hide();
    xtouch_mqtt_hasStatus = false;
    xtouch_mqtt_retryMs = 5000;
    // Keep network handshakes from blocking the two-second switch notice.
    xtouch_mqtt_nextAttempt = millis() + 2100;
    xtouch_mqtt_topic_setup();
    xtouch_pubSubClient.setServer(xTouchConfig.xTouchHost, 8883);
    loadScreen(0);
    showPrinterNotice();
    lv_refr_now(NULL);
    ConsoleInfo.printf("[XTouch][PRINTER] selected slot=%u\n", slot + 1);
}

void xtouch_mqtt_setup()
{
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_CONNECTING_PRINTER);
    lv_timer_handler();
    lv_task_handler();
    delay(32);

    xtouch_mqtt_topic_setup();

    xtouch_wiFiClientSecure.flush();
    xtouch_wiFiClientSecure.stop();

    xtouch_wiFiClientSecure.setInsecure();
    xtouch_wiFiClientSecure.setHandshakeTimeout(3);

    xtouch_pubSubClient.setServer(xTouchConfig.xTouchHost, 8883);
    xtouch_pubSubClient.setBufferSize(2048); // 2KB for mqtt message JWT output
    xtouch_pubSubClient.setStream(stream);
    xtouch_pubSubClient.setCallback(xtouch_pubSubClient_streamCallback);
    xtouch_pubSubClient.setKeepAlive(10);
    xtouch_pubSubClient.setSocketTimeout(2);

    /* home */
    lv_msg_subscribe(XTOUCH_COMMAND_LIGHT_TOGGLE, (lv_msg_subscribe_cb_t)xtouch_device_onLightToggleCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_STOP, (lv_msg_subscribe_cb_t)xtouch_device_onStopCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PAUSE, (lv_msg_subscribe_cb_t)xtouch_device_onPauseCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_RESUME, (lv_msg_subscribe_cb_t)xtouch_device_onResumeCommand, NULL);

    /* control */
    lv_msg_subscribe(XTOUCH_COMMAND_HOME, (lv_msg_subscribe_cb_t)xtouch_device_onHomeCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_LEFT, (lv_msg_subscribe_cb_t)xtouch_device_onLeftCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_RIGHT, (lv_msg_subscribe_cb_t)xtouch_device_onRightCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_UP, (lv_msg_subscribe_cb_t)xtouch_device_onUpCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_DOWN, (lv_msg_subscribe_cb_t)xtouch_device_onDownCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_BED_TARGET_TEMP, (lv_msg_subscribe_cb_t)xtouch_device_onBedTargetTempCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_NOZZLE_TARGET_TEMP, (lv_msg_subscribe_cb_t)xtouch_device_onNozzleTargetCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PART_FAN_SPEED, (lv_msg_subscribe_cb_t)xtouch_device_onPartSpeedCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_AUX_FAN_SPEED, (lv_msg_subscribe_cb_t)xtouch_device_onAuxSpeedCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_CHAMBER_FAN_SPEED, (lv_msg_subscribe_cb_t)xtouch_device_onChamberSpeedCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_PRINT_SPEED, (lv_msg_subscribe_cb_t)xtouch_device_onPrintSpeedCommand, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_FILAMENT_ACTION, xtouch_device_onFilamentAction, NULL);

    lv_msg_subscribe(XTOUCH_COMMAND_AMS_CONTROL, (lv_msg_subscribe_cb_t)xtouch_device_command_ams_control, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_CLEAN_PRINT_ERROR, (lv_msg_subscribe_cb_t)xtouch_device_command_clean_print_error, NULL);

    /* filament */
    lv_msg_subscribe(XTOUCH_COMMAND_EXTRUDE_UP, (lv_msg_subscribe_cb_t)xtouch_device_onNozzleUp, NULL);
    lv_msg_subscribe(XTOUCH_COMMAND_EXTRUDE_DOWN, (lv_msg_subscribe_cb_t)xtouch_device_onNozzleDown, NULL);

    loadScreen(0);
    xtouch_screen_startScreenTimer();
    xtouch_mqtt_nextAttempt = millis() + 150;
}

void xtouch_mqtt_loop()
{
    xtouch_mqtt_switch_printer();
    if (xtouch_pubSubClient.connected()) {
        xtouch_pubSubClient.loop();
        if ((uint32_t)(millis() - xtouch_mqtt_lastPushStatus) >
            XTOUCH_MQTT_SERVER_PUSH_STATUS_TIMEOUT * 1000U) {
            xtouch_pubSubClient.disconnect();
            xtouch_mqtt_hasStatus = false;
            xtouch_mqtt_nextAttempt = millis() + 5000;
            xtouch_mqtt_sendMsg(XTOUCH_ON_PRINT_STATUS);
        }
    } else {
        xtouch_mqtt_connect();
    }
    delay(1);
}

#endif
