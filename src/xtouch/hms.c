#include "hms.h"
#include <string.h>
/* Current telemetry, not a FIFO of superseded notifications. */
static uint32_t print_error,revision;
static uint64_t health[HMS_MAX_CODES];
static unsigned health_count;
static bool pending,truncated;
void hms_reset(void){print_error=health_count=0;pending=truncated=false;++revision;}
bool hms_set_print_error(uint32_t code){
 if(code==print_error)return false;print_error=code;++revision;
 if(code)pending=true;if(!code&&!health_count)pending=false;return true;
}
bool hms_set_health(const uint64_t *codes,unsigned count,bool overflow){
 uint64_t next[HMS_MAX_CODES];unsigned used=0;if(count&&!codes)return false;
 for(unsigned i=0;i<count;++i){uint64_t code=codes[i];if(!code||code==UINT64_MAX)continue;
  unsigned at=0;while(at<used&&next[at]<code)++at;if(at<used&&next[at]==code)continue;
  if(used==HMS_MAX_CODES){overflow=true;if(at<used){memmove(next+at+1,next+at,(used-at-1)*sizeof(*next));next[at]=code;}continue;}
  memmove(next+at+1,next+at,(used-at)*sizeof(*next));next[at]=code;++used;
 }
 if(used==health_count&&overflow==truncated&&!memcmp(next,health,used*sizeof(*next)))return false;
 for(unsigned i=0;i<used;++i){bool found=false;for(unsigned j=0;j<health_count;++j)if(next[i]==health[j]){found=true;break;}if(!found)pending=true;}
 if(overflow&&!truncated)pending=true;
 memcpy(health,next,used*sizeof(*next));health_count=used;truncated=overflow;++revision;
 if(!print_error&&!used)pending=false;return true;
}
uint32_t hms_print_error(void){return print_error;}
unsigned hms_health_count(void){return health_count;}
uint64_t hms_health_at(unsigned i){return i<health_count?health[i]:0;}
bool hms_health_truncated(void){return truncated;}
bool hms_has_pending(void){return pending;}
void hms_dismiss(void){pending=false;}
uint32_t hms_revision(void){return revision;}
#ifdef XTOUCH_ERROR_SELFTEST
bool hms_selftest(void){
 hms_reset();uint64_t a[]={0x0500050000010007ULL,0x0300010000010001ULL};
 if(!hms_set_print_error(0x07018004)||!hms_set_health(a,2,false)||!hms_has_pending())return false;
 hms_dismiss();uint64_t reverse[]={a[1],a[0],a[1]};
 if(hms_set_health(reverse,3,false)||hms_set_print_error(0x07018004)||hms_has_pending())return false;
 if(!hms_set_health(a,1,false)||hms_has_pending()||hms_health_count()!=1)return false;
 if(!hms_set_print_error(0)||hms_has_pending())return false;
 uint64_t zeroPrefix[]={1};
 if(!hms_set_health(zeroPrefix,1,false)||hms_health_at(0)!=1||!hms_has_pending())return false;
 if(!hms_set_health(NULL,0,false)||hms_health_count()||hms_has_pending())return false;
 hms_reset();return true;
}
#endif
