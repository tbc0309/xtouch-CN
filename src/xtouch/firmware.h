#ifndef _XLCD_FIRMWARE
#define _XLCD_FIRMWARE

#include <Arduino.h>
#include <Update.h>
#include <SD.h>
#include <stdio.h>
// #include <semver.h>

struct SemVer
{
    int major;
    int minor;
    int patch;
};

bool parseSemVer(const char *version, SemVer &semver)
{
    if (!version) return false;
    char suffix;
    int parsed = sscanf(version, "%d.%d.%d%c", &semver.major, &semver.minor, &semver.patch, &suffix);
    return parsed == 3 && semver.major >= 0 && semver.minor >= 0 && semver.patch >= 0;
}

int compareSemVer(const SemVer &v1, const SemVer &v2)
{
    if (v1.major != v2.major)
    {
        return v1.major - v2.major;
    }
    if (v1.minor != v2.minor)
    {
        return v1.minor - v2.minor;
    }
    if (v1.patch != v2.patch)
    {
        return v1.patch - v2.patch;
    }
    return 0; // Both versions are equal
}

bool isSemVerGreaterThan(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) > 0;
}

bool isSemVerLessThan(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) < 0;
}

bool isSemVerEqualTo(const SemVer &v1, const SemVer &v2)
{
    return compareSemVer(v1, v2) == 0;
}

static bool xtouch_firmware_manifest_valid(JsonDocument &doc)
{
    const char *version = doc["version"] | "";
    const char *url = doc["url"] | "";
    const char *md5 = doc["md5"] | "";
    SemVer parsed;
    if (!parseSemVer(version, parsed) || strlen(md5) != 32 ||
        !String(url).startsWith("https://github.com/tbc0309/xtouch-CN/releases/download/") ||
        !String(url).endsWith("/xtouch-2.8.bin") ||
        !doc["size"].is<unsigned>() || doc["size"].as<unsigned>() == 0 ||
        doc["size"].as<unsigned>() > 1966080) return false;
    for (unsigned i = 0; i < 32; i++) if (!isxdigit(md5[i])) return false;
    return true;
}

void xtouch_firmware_onProgress(size_t current, size_t total)
{
    if (!total) return;
    lv_label_set_text_fmt(introScreenCaption, XTOUCH_TEXT_UPDATING_D, int(current * 100 / total));
    lv_timer_handler();
}

void xtouch_firmware_checkOnlineFirmwareUpdate(void)
{
    if (!xTouchConfig.xTouchOTAEnabled) return;
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_CHECKING_FOR_OTA_UPDATE);
    lv_refr_now(NULL);
    ConsoleInfo.printf("[OTA] checking current=%s\n", XTOUCH_FIRMWARE_VERSION);
    if (!downloadFileToSDCard(xtouch_paths_firmware_ota_file, xtouch_paths_firmware_ota_json)) return;
    DynamicJsonDocument doc = xtouch_filesystem_readJson(SD, xtouch_paths_firmware_ota_json);
    SemVer current, remote;
    if (!xtouch_firmware_manifest_valid(doc) || !parseSemVer(XTOUCH_FIRMWARE_VERSION, current) ||
        !parseSemVer(doc["version"], remote)) {
        ConsoleError.println("[OTA] invalid manifest");
        SD.remove(xtouch_paths_firmware_ota_json);
        return;
    }
    if (!isSemVerGreaterThan(remote, current)) {
        ConsoleInfo.println("[OTA] already current");
        SD.remove(xtouch_paths_firmware_ota_json);
        return;
    }
    // Two bounded attempts; a failed update must never prevent normal startup.
    for (unsigned attempt = 0; attempt < 2; attempt++) {
        bool success = downloadFileToSDCard(doc["url"], xtouch_paths_firmware_ota_fw,
            [](int progress) {
                lv_label_set_text_fmt(introScreenCaption, XTOUCH_TEXT_DOWNLOADING_UPDATE_D, progress);
                lv_timer_handler();
            }, NULL, doc["md5"], doc["size"]);
        if (success) {
            ConsoleInfo.println("[OTA] staged, rebooting to install");
            delay(200);
            ESP.restart();
        }
    }
    SD.remove(xtouch_paths_firmware_ota_json);
    lv_label_set_text(introScreenCaption, XTOUCH_TEXT_FAILED_TO_DOWNLOAD_UPDATE);
    lv_refr_now(NULL);
}

void xtouch_firmware_checkFirmwareUpdate(void)
{
    if (!SD.exists(xtouch_paths_firmware_ota_fw)) return;
    DynamicJsonDocument doc = xtouch_filesystem_readJson(SD, xtouch_paths_firmware_ota_json);
    bool verified = xtouch_firmware_manifest_valid(doc);
    File firmware = SD.open(xtouch_paths_firmware_ota_fw, FILE_READ);
    // Plain firmware.bin on SD remains supported; an OTA manifest, when present,
    // must be valid and match the complete image before touching flash.
    bool manifestPresent = SD.exists(xtouch_paths_firmware_ota_json);
    bool valid = firmware && firmware.size() > 0 && firmware.size() <= 1966080 && firmware.peek() == 0xE9;
    if (manifestPresent) valid = valid && verified && firmware.size() == doc["size"].as<unsigned>();
    if (valid && manifestPresent) {
        MD5Builder md5;
        md5.begin();
        md5.addStream(firmware, firmware.size());
        md5.calculate();
        valid = md5.toString().equalsIgnoreCase(doc["md5"].as<const char *>());
        firmware.seek(0);
    }
    bool installed = false;
    if (valid) {
        Update.onProgress(xtouch_firmware_onProgress);
        if (Update.begin(firmware.size(), U_FLASH)) {
            if (manifestPresent) Update.setMD5(doc["md5"]);
            size_t written = Update.writeStream(firmware);
            installed = written == firmware.size() && Update.end();
            if (!installed) Update.abort();
        }
    }
    firmware.close();
    SD.remove(xtouch_paths_firmware_ota_json);
    SD.remove(xtouch_paths_firmware_ota_fw);
    ConsoleInfo.printf("[OTA] install %s\n", installed ? "SUCCESS" : "REJECTED");
    lv_label_set_text(introScreenCaption, installed ? XTOUCH_TEXT_UPDATE_FINISHED : XTOUCH_TEXT_UPDATE_ERROR);
    lv_refr_now(NULL);
    if (installed) { delay(300); ESP.restart(); }
}
#endif
