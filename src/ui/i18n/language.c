#include "language.h"

static uint8_t currentLanguage;
static int pendingLanguage = -1;
uint8_t xtouch_language_get(void) { return currentLanguage; }
void xtouch_language_set(uint8_t language) { currentLanguage = language < 6 ? language : 0; }
void xtouch_language_request(uint8_t language)
{
    if (language < 6 && language != currentLanguage) pendingLanguage = language;
}
/* Rebuild only after the dropdown event has returned. */
bool xtouch_language_apply_request(void)
{
    if (pendingLanguage < 0) return false;
    xtouch_language_set(pendingLanguage);
    pendingLanguage = -1;
    return true;
}
