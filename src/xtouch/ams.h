#ifndef _XLCD_AMS
#define _XLCD_AMS

#ifdef __cplusplus
extern "C"
{
#endif

#include <Arduino.h>
#include "xtouch/bbl/bbl-errors.h"
#include <pgmspace.h>
    // Function to retrieve a value by key
    extern void xtouch_ams_parse_tray_now(const char *tray_now);
    extern void xtouch_ams_parse_status(int ams_status);

#ifdef __cplusplus
}
#endif

#endif