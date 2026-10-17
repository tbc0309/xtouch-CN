#pragma once
#include <Arduino.h>
#include <sdkconfig.h>
#include "firmware_image.h"
inline bool xtouch_firmware_header_valid(const uint8_t *header,size_t size){return xtouch_application_image(header,size,CONFIG_IDF_FIRMWARE_CHIP_ID);}

#ifdef XTOUCH_WAVESHARE
#define XTOUCH_FIRMWARE_MAX_BYTES (6U*1024U*1024U)
#ifdef XTOUCH_BOARD_S3
#define XTOUCH_FIRMWARE_FILENAME "/xtouch-waveshare-s3-v2.bin"
#else
#define XTOUCH_FIRMWARE_FILENAME "/xtouch-waveshare-p4-43.bin"
#endif
#else
#define XTOUCH_FIRMWARE_MAX_BYTES 1966080U
#define XTOUCH_FIRMWARE_FILENAME "/xtouch-2.8.bin"
#endif
