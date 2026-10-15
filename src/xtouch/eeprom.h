#ifndef _XLCD_EEPROM
#define _XLCD_EEPROM
#include "storage.h"
#include <SPIFFS.h>
#define XTOUCH_EEPROM_POS_TFTFLIP 0

void xtouch_eeprom_setup()
{
    // Import the legacy byte once; do not format or truncate the old file.
    if (!xtouch_storage_has("/xtouch/hardware.json") && SPIFFS.exists("/eeprom.bin")) {
        File old = SPIFFS.open("/eeprom.bin", FILE_READ);
        int flip = old.read(); old.close();
        if (flip == 0 || flip == 1) {
            StaticJsonDocument<64> doc; doc["flip"] = flip;
            xtouch_storage_write("/xtouch/hardware.json", doc);
        }
    }
}
void xtouch_eeprom_write(int address, byte value)
{
    if (address != XTOUCH_EEPROM_POS_TFTFLIP) return;
    StaticJsonDocument<64> doc; doc["flip"] = value ? 1 : 0;
    xtouch_storage_write("/xtouch/hardware.json", doc);
}
uint8_t xtouch_eeprom_read(int address)
{
    if (address != XTOUCH_EEPROM_POS_TFTFLIP) return 0;
    auto doc = xtouch_storage_read("/xtouch/hardware.json", 128);
    return doc["flip"] | 0;
}
#endif
