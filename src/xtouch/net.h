#ifndef _XLCD_NET
#define _XLCD_NET

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <MD5Builder.h>
#include <time.h>
#include "ota-certs.h"

// OTA runs before MQTT starts, so it does not compete for TLS buffers.
static bool xtouch_ota_sync_clock()
{
    if (time(NULL) > 1704067200) return true;
    configTime(0, 0, "pool.ntp.org", "time.cloudflare.com", "ntp.aliyun.com");
    uint32_t start = millis();
    while (time(NULL) < 1704067200 && millis() - start < 15000) delay(50);
    return time(NULL) > 1704067200;
}

bool downloadFileToSDCard(const char *url, const char *fileName,
                          void (*onProgress)(int) = NULL,
                          void (*onMD5Check)(int) = NULL,
                          const char *otaMD5 = NULL, size_t expectedSize = 0)
{
    if (!url || !String(url).startsWith("https://") || !xtouch_ota_sync_clock()) {
        ConsoleError.println("[OTA] HTTPS URL or synchronized clock unavailable");
        return false;
    }
    WiFiClientSecure client;
    client.setCACert(xtouch_ota_ca);
    client.setHandshakeTimeout(12);
    client.setTimeout(10000);
    HTTPClient http;
    http.setConnectTimeout(10000);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setRedirectLimit(5);
    http.useHTTP10(true); // Require a bounded, non-chunked download.
    if (!http.begin(client, url)) return false;
    int code = http.GET();
    int total = http.getSize();
    const size_t limit = otaMD5 ? 1966080 : 16384;
    if (code != HTTP_CODE_OK || total <= 0 || size_t(total) > limit ||
        (expectedSize && size_t(total) != expectedSize)) {
        ConsoleError.printf("[OTA] HTTP=%d length=%d expected=%u\n", code, total, unsigned(expectedSize));
        http.end();
        return false;
    }
    String partial = String(fileName) + ".part";
    SD.remove(partial.c_str());
    File file = SD.open(partial.c_str(), FILE_WRITE);
    if (!file) { http.end(); return false; }
    MD5Builder md5;
    md5.begin();
    uint8_t buffer[1024];
    size_t received = 0;
    int lastProgress = -1;
    uint32_t lastData = millis();
    while (received < size_t(total) && millis() - lastData < 10000) {
        int available = http.getStreamPtr()->available();
        if (available > 0) {
            size_t wanted = min(sizeof(buffer), size_t(total) - received);
            wanted = min(wanted, size_t(available));
            int count = http.getStreamPtr()->read(buffer, wanted);
            if (count <= 0 || file.write(buffer, count) != size_t(count)) break;
            md5.add(buffer, count);
            received += count;
            lastData = millis();
            int progress = received * 100 / total;
            if (onProgress && progress != lastProgress) onProgress(progress);
            lastProgress = progress;
        } else if (!http.connected()) break;
        delay(1);
    }
    file.close();
    http.end();
    bool success = received == size_t(total);
    if (otaMD5) {
        if (onMD5Check) onMD5Check(-1);
        md5.calculate();
        success = success && md5.toString().equalsIgnoreCase(otaMD5);
        if (onMD5Check) onMD5Check(success);
    }
    if (success) {
        SD.remove(fileName);
        success = SD.rename(partial.c_str(), fileName);
    }
    if (!success) SD.remove(partial.c_str());
    ConsoleInfo.printf("[OTA] download %s bytes=%u/%d\n", success ? "verified" : "failed", unsigned(received), total);
    return success;
}
#endif
