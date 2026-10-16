#include "vault.h"
#include <Preferences.h>
#include <esp_system.h>
#include <mbedtls/gcm.h>
#include <mbedtls/base64.h>
namespace {
uint8_t key[32];bool ready=false;
bool initialize(bool rotate){
 Preferences nvs;if(!nvs.begin("xtouch-vault",false))return false;
 bool valid=!rotate&&nvs.getBytesLength("key")==sizeof(key)&&nvs.getBytes("key",key,sizeof(key))==sizeof(key);
 if(!valid){esp_fill_random(key,sizeof(key));valid=nvs.putBytes("key",key,sizeof(key))==sizeof(key);}
 nvs.end();ready=valid;return valid;
}
String aad(const String &binding){return String((uint32_t)(ESP.getEfuseMac()>>32),HEX)+String((uint32_t)ESP.getEfuseMac(),HEX)+":"+binding;}
}
bool xtouch_vault_begin(){return ready||initialize(false);}
bool xtouch_vault_rotate(){return initialize(true);}
String xtouch_vault_seal(const String &plain,const String &binding){
 if(!xtouch_vault_begin()||plain.isEmpty()||plain.length()>128)return "";
 uint8_t bytes[156]={0};esp_fill_random(bytes,12);String context=aad(binding);
 mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);
 int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,256);
 if(!rc)rc=mbedtls_gcm_crypt_and_tag(&gcm,MBEDTLS_GCM_ENCRYPT,plain.length(),bytes,12,(const uint8_t*)context.c_str(),context.length(),(const uint8_t*)plain.c_str(),bytes+28,16,bytes+12);
 mbedtls_gcm_free(&gcm);if(rc)return "";
 unsigned char encoded[213];size_t n=0;rc=mbedtls_base64_encode(encoded,sizeof(encoded),&n,bytes,plain.length()+28);memset(bytes,0,sizeof(bytes));
 return rc?String():String("v1:")+String((char*)encoded,n);
}
bool xtouch_vault_open(const String &sealed,const String &binding,String &plain){
 plain="";if(!xtouch_vault_begin()||!sealed.startsWith("v1:")||sealed.length()>211)return false;
 uint8_t bytes[156],decoded[129]={0};size_t n=0;
 if(mbedtls_base64_decode(bytes,sizeof(bytes),&n,(const unsigned char*)sealed.c_str()+3,sealed.length()-3)||n<=28)return false;
 String context=aad(binding);mbedtls_gcm_context gcm;mbedtls_gcm_init(&gcm);
 int rc=mbedtls_gcm_setkey(&gcm,MBEDTLS_CIPHER_ID_AES,key,256);
 if(!rc)rc=mbedtls_gcm_auth_decrypt(&gcm,n-28,bytes,12,(const uint8_t*)context.c_str(),context.length(),bytes+12,16,bytes+28,decoded);
 mbedtls_gcm_free(&gcm);if(!rc)plain=String((char*)decoded,n-28);memset(decoded,0,sizeof(decoded));memset(bytes,0,sizeof(bytes));return !rc;
}

bool xtouch_vault_selftest(){
 if(!xtouch_vault_begin())return false;
 String input="vault-test-0123456789",plain;String encrypted=xtouch_vault_seal(input,"test");
 bool valid=xtouch_vault_open(encrypted,"test",plain)&&plain==input;
 valid=valid&&!xtouch_vault_open(encrypted,"other",plain);
 String corrupt=encrypted;corrupt.setCharAt(45,corrupt[45]=='A'?'B':'A');
 valid=valid&&!xtouch_vault_open(corrupt,"test",plain);
 uint8_t previous[32];memcpy(previous,key,32);esp_fill_random(key,32);
 valid=valid&&!xtouch_vault_open(encrypted,"test",plain);memcpy(key,previous,32);memset(previous,0,32);
 String maximum;for(unsigned i=0;i<128;++i)maximum+='x';
 valid=valid&&xtouch_vault_open(xtouch_vault_seal(maximum,"test"),"test",plain)&&plain==maximum;
 return valid&&xtouch_vault_seal(maximum+"x","test").isEmpty()&&xtouch_vault_open(encrypted,"test",plain);
}
