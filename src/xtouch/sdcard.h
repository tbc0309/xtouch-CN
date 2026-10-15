#ifndef _XLCD_SDCARD
#define _XLCD_SDCARD
#include <SD.h>
#include "storage.h"

static bool xtouch_sdcard_firmware_waiting = false;

static bool xtouch_sdcard_probe()
{
    // The board has no card-detect pin. One bounded CMD0 avoids the SD
    // library's repeated 100ms delays on every empty-slot poll.
    constexpr uint8_t chipSelect = 5;
    SPI.begin();
    pinMode(chipSelect, OUTPUT);
    digitalWrite(chipSelect, HIGH);
    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (unsigned i = 0; i < 10; i++) SPI.transfer(0xff);
    digitalWrite(chipSelect, LOW);
    const uint8_t command[] = {0x40, 0, 0, 0, 0, 0x95};
    for (auto byte : command) SPI.transfer(byte);
    uint8_t response = 0xff;
    for (unsigned i = 0; i < 16 && (response & 0x80); i++) response = SPI.transfer(0xff);
    digitalWrite(chipSelect, HIGH);
    SPI.transfer(0xff);
    SPI.endTransaction();
    return response == 1;
}

bool xtouch_sdcard_setup()
{
    if (!xtouch_sdcard_probe()) return false;
    bool available = SD.begin() && SD.cardType() != CARD_NONE;
    if (!available) SD.end();
    xtouch_storage_set_sd(available);
    if (available) {
        if (!SD.exists("/xtouch")) SD.mkdir("/xtouch");
        ConsoleInfo.printf("[SD] mounted %lluMB\n", SD.cardSize() / (1024 * 1024));
    }
    return available;
}

void xtouch_sdcard_poll(bool runtime = true)
{
    static uint32_t next = 0;
    if ((int32_t)(millis() - next) < 0) return;
    // Do not perform card insertion probes immediately after user interaction.
    if (runtime && lv_disp_get_inactive_time(NULL) < 750) return;
    next = millis() + 10000;
    if (xtouch_storage_sd_available()) {
        uint8_t sector[512];
        if (!SD.readRAW(sector, 0)) {
            xtouch_storage_set_sd(false); SD.end();
            ConsoleInfo.println("[SD] removed; internal configuration remains active");
            return;
        }
    } else if (!xtouch_sdcard_setup()) return;
    xtouch_storage_sync(runtime);
    if (runtime && SD.exists("/firmware.bin")) {
        File firmware = SD.open("/firmware.bin", FILE_READ);
        xtouch_sdcard_firmware_waiting = firmware && firmware.size() > 0 && firmware.size() <= 1966080 && firmware.peek() == 0xE9;
        firmware.close();
    }
}
#endif
