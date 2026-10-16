#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void xtouch_portal_show_management(void);
void xtouch_portal_show_account(const char *account);
void xtouch_portal_request_wifi(void);
void xtouch_portal_request_reset(void);
bool xtouch_portal_screen_active(void);
#ifdef __cplusplus
}
void xtouch_portal_begin();
void xtouch_portal_loop();
bool xtouch_portal_ap();
bool xtouch_portal_maintenance();
void xtouch_portal_cloud_rejected();
#endif
