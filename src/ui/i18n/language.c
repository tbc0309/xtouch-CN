#include "language.h"
#include <string.h>
#include "extra_locales.h"

static uint8_t currentLanguage;
static int pendingLanguage = -1;
uint8_t xtouch_language_get(void) { return currentLanguage; }
const char *xtouch_language_code(uint8_t language) {
    static const char *const codes[]={"zh","zh-TW","en","fr","de","ru","ja","ko"};
    return codes[language<XTOUCH_LANGUAGE_COUNT?language:0];
}
int xtouch_language_index(const char *code) {
    if(code)for(unsigned i=0;i<XTOUCH_LANGUAGE_COUNT;i++)if(!strcmp(code,xtouch_language_code(i)))return i;
    return -1;
}
const char *xtouch_local(const char *zh,const char *en) {
    if(!currentLanguage)return zh;
    unsigned lo=0,hi=sizeof(extraLocales)/sizeof(extraLocales[0]);
    while(lo<hi){unsigned mid=(lo+hi)/2;int c=strcmp(en,extraLocales[mid][2]);
        if(!c)return extraLocales[mid][currentLanguage];if(c<0)hi=mid;else lo=mid+1;}
    return en;
}
void xtouch_language_set(uint8_t language) { currentLanguage = language < XTOUCH_LANGUAGE_COUNT ? language : 0; }
void xtouch_language_request(uint8_t language)
{
    if (language < XTOUCH_LANGUAGE_COUNT && language != currentLanguage) pendingLanguage = language;
}
/* Rebuild only after the dropdown event has returned. */
bool xtouch_language_apply_request(void)
{
    if (pendingLanguage < 0) return false;
    xtouch_language_set(pendingLanguage);
    pendingLanguage = -1;
    return true;
}
