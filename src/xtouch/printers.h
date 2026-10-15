#pragma once

#include <stdbool.h>
#include <stdint.h>

#define XTOUCH_MAX_PRINTERS 100

#ifdef __cplusplus
extern "C" {
#endif
bool xtouch_printer_configured(uint8_t slot);
uint8_t xtouch_printer_active(void);
const char *xtouch_printer_name(void);
const char *xtouch_printer_name_at(uint8_t slot);
const char *xtouch_printer_model_at(uint8_t slot);
bool xtouch_printer_request(uint8_t slot);
bool xtouch_printer_switch_pending(void);
bool xtouch_printer_online(void);
#ifdef __cplusplus
}
#endif
