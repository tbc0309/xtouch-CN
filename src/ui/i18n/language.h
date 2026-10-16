#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
uint8_t xtouch_language_get(void);
enum { XTOUCH_LANGUAGE_COUNT = 8 };
const char *xtouch_language_code(uint8_t language);
int xtouch_language_index(const char *code);
const char *xtouch_local(const char *zh, const char *en);
void xtouch_language_set(uint8_t language);
void xtouch_language_request(uint8_t language);
bool xtouch_language_apply_request(void);
#ifdef __cplusplus
}
#endif
