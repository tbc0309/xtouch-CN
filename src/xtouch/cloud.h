#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// One active account and one bounded API job. Remembered passwords use the device vault.
bool xtouch_cloud_select(const char *accountId);
const char *xtouch_cloud_broker();
const char *xtouch_cloud_user();
const char *xtouch_cloud_token();
const char *xtouch_cloud_error();
const char *xtouch_cloud_trust();
bool xtouch_cloud_ready();
bool xtouch_cloud_job_start(JsonVariantConst request);
bool xtouch_cloud_busy();
bool xtouch_cloud_job_pending();
String xtouch_cloud_job_result();
void xtouch_cloud_job_clear();
