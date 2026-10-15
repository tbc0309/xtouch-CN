#ifndef _XLCD_NET
#define _XLCD_NET

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <MD5Builder.h>
#include <time.h>
#include <Update.h>
#include "ota-certs.h"

// OTA runs before MQTT starts, so it does not compete for TLS buffers.
static void xtouch_start_clock_sync()
{
    static bool started = false;
    if (started) return;
    started = true;
    configTime(0, 0, "ntp.aliyun.com", "time.cloudflare.com", "pool.ntp.org");
}

static bool xtouch_ota_sync_clock()
{
    if (time(NULL) > 1704067200) return true;
    xtouch_start_clock_sync();
    uint32_t start = millis();
    while (time(NULL) < 1704067200 && millis() - start < 15000) delay(50);
    return time(NULL) > 1704067200;
}

static bool xtouch_ota_open(HTTPClient &http, WiFiClientSecure &client,
                             const char *url, size_t limit, size_t expectedSize, int &total)
{
    if (!url || !String(url).startsWith("https://") || !xtouch_ota_sync_clock()) {
        ConsoleError.println("[OTA] HTTPS URL or synchronized clock unavailable");
        return false;
    }
    client.setCACert(xtouch_ota_ca);
    client.setHandshakeTimeout(12);
    client.setTimeout(10000);
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setRedirectLimit(5);
    http.useHTTP10(true);
    if (!http.begin(client, url)) return false;
    int code = http.GET(); total = http.getSize();
    if (code != HTTP_CODE_OK || total <= 0 || size_t(total) > limit ||
        (expectedSize && size_t(total) != expectedSize)) {
        ConsoleError.printf("[OTA] HTTP=%d length=%d expected=%u\n", code, total, unsigned(expectedSize));
        http.end(); return false;
    }
    return true;
}

template<class Sink>
static bool xtouch_ota_transfer(HTTPClient &http, int total, const char *expectedMD5,
                                Sink sink, void (*onProgress)(int))
{
    MD5Builder md5; md5.begin();
    uint8_t buffer[1024]; size_t received = 0;
    int lastProgress = -1; uint32_t lastData = millis();
    while (received < size_t(total) && millis() - lastData < 10000) {
        int available = http.getStreamPtr()->available();
        if (available > 0) {
            size_t wanted = min(sizeof(buffer), size_t(total) - received);
            wanted = min(wanted, size_t(available));
            int count = http.getStreamPtr()->read(buffer, wanted);
            if (count <= 0 || sink(buffer, count) != size_t(count)) break;
            md5.add(buffer, count); received += count; lastData = millis();
            int progress = received * 100 / total;
            if (onProgress && progress != lastProgress) onProgress(progress);
            lastProgress = progress;
        } else if (!http.connected()) break;
        delay(1);
    }
    md5.calculate();
    bool success = received == size_t(total) && (!expectedMD5 || md5.toString().equalsIgnoreCase(expectedMD5));
    ConsoleInfo.printf("[OTA] download %s bytes=%u/%d\n", success ? "verified" : "failed", unsigned(received), total);
    return success;
}

bool downloadFileToFS(fs::FS &fs, const char *url, const char *fileName,
                       void (*onProgress)(int) = NULL,
                       void (*onMD5Check)(int) = NULL,
                       const char *otaMD5 = NULL, size_t expectedSize = 0)
{
    WiFiClientSecure client; HTTPClient http; int total;
    if (!xtouch_ota_open(http, client, url, otaMD5 ? 1966080 : 16384, expectedSize, total)) return false;
    String partial = String(fileName) + ".part";
    if (fs.exists(partial)) fs.remove(partial);
    File file = fs.open(partial, FILE_WRITE);
    if (!file) { http.end(); return false; }
    if (onMD5Check) onMD5Check(-1);
    bool success = xtouch_ota_transfer(http, total, otaMD5,
        [&file](uint8_t *data, size_t size) { return file.write(data, size); }, onProgress);
    file.flush(); file.close(); http.end();
    if (onMD5Check) onMD5Check(success);
    if (success) {
        if (fs.exists(fileName)) fs.remove(fileName);
        success = fs.rename(partial, fileName);
    }
    if (!success && fs.exists(partial)) fs.remove(partial);
    return success;
}

bool downloadFirmwareToFlash(const char *url, const char *md5, size_t size,
                               void (*onProgress)(int))
{
    WiFiClientSecure client; HTTPClient http; int total;
    if (!xtouch_ota_open(http, client, url, 1966080, size, total)) return false;
    Update.onProgress(nullptr);
    if (!Update.begin(total, U_FLASH)) { http.end(); return false; }
    Update.setMD5(md5);
    bool success = xtouch_ota_transfer(http, total, md5,
        [](uint8_t *data, size_t count) { return Update.write(data, count); }, onProgress);
    http.end();
    if (success) success = Update.end();
    if (!success) Update.abort();
    ConsoleInfo.printf("[OTA] direct install %s\n", success ? "SUCCESS" : "REJECTED");
    return success;
}
#endif
