#pragma once
#include <ArduinoJson.h>
#include <ctype.h>

namespace XTouchSchema {
inline bool labHost(const char *host) {
    unsigned a,b,c,d; char extra;
    if(!host || sscanf(host,"%u.%u.%u.%u%c",&a,&b,&c,&d,&extra)!=4 || a>255 || b>255 || c>255 || d>255)return false;
    return a==10 || (a==172 && b>=16 && b<=31) || (a==192 && b==168);
}
inline bool text(JsonVariantConst value, size_t limit, bool required = false) {
    if (!value.is<const char *>())
        return !required && value.isNull();
    const char *s = value.as<const char *>();
    if ((required && !*s) || strlen(s) >= limit)
        return false;
    for (const uint8_t *p = (const uint8_t *)s; *p; ++p)
        if (*p < 32 || *p == 127)
            return false;
    return true;
}
inline bool asciiName(JsonVariantConst value) {
    if (!text(value, 64))
        return false;
    for (const unsigned char *p = (const unsigned char *)(value | ""); *p; p++)
        if (*p < 32 || *p > 126)
            return false;
    return true;
}
inline bool account(JsonVariantConst a) {
    if(a["simulator"].as<bool>())
        return text(a["id"],32,true) && asciiName(a["name"]) && labHost(a["mqttHost"]|"") &&
            a["mqttPort"].is<unsigned>() && a["mqttPort"].as<unsigned>()>0 && a["mqttPort"].as<unsigned>()<=65535 &&
            !strcmp(a["token"]|"","xtouch-lab") && !strcmp(a["userId"]|"","xtouch-lab") &&
            !a["rememberPassword"].as<bool>() && !strlen(a["passwordEnc"]|"") && !strlen(a["account"]|"");
    return a.is<JsonObjectConst>() && text(a["id"], 32, true) && text(a["token"], 2049) &&
           text(a["userId"], 48) && text(a["account"], 101) && text(a["name"], 64) &&
           text(a["passwordEnc"], 212) &&
           (!a.containsKey("rememberPassword") || a["rememberPassword"].is<bool>()) &&
           (!strcmp(a["loginMethod"] | "token", "token") ||
            !strcmp(a["loginMethod"] | "token", "password")) &&
           (!strcmp(a["authMode"] | "token", "token") ||
            !strcmp(a["authMode"] | "token", "password")) &&
           (!strcmp(a["region"] | "cn", "cn") || !strcmp(a["region"] | "cn", "global"));
}
inline bool printer(JsonVariantConst p) {
    if (!p.is<JsonObjectConst>() || !text(p["name"], 64) || !text(p["printerModel"], 32) ||
        !text(p["serialNumber"], 32, true))
        return false;
    const char *mode = p["mode"] | "lan";
    if (!strcmp(mode, "cloud"))
        return text(p["accountId"], 32, true);
    const char *host = p["host"] | "", *code = p["accessCode"] | "";
    if (strcmp(mode, "lan") || !text(p["host"], 64, true) || strlen(code) != 8)
        return false;
    for (const char *c = host; *c; ++c)
        if (!(isalnum((unsigned char)*c) || *c == '.' || *c == '-' || *c == '_'))
            return false;
    for (const char *c = code; *c; ++c)
        if ((uint8_t)*c < 33 || (uint8_t)*c > 126)
            return false;
    return true;
}
inline bool root(JsonDocument &doc) {
    if (!text(doc["ssid"], 33, true) || !text(doc["pwd"], 65) || !doc["printers"].is<JsonArray>())
        return false;
    if (doc["printers"].size() > 100 ||
        (doc.containsKey("cloudAccounts") && !doc["cloudAccounts"].is<JsonArray>()) ||
        doc["cloudAccounts"].size() > 4)
        return false;
    for (JsonVariantConst a : doc["cloudAccounts"].as<JsonArrayConst>()) {
        if (!account(a))
            return false;
        unsigned matches = 0;
        for (JsonVariantConst b : doc["cloudAccounts"].as<JsonArrayConst>())
            if (!strcmp(a["id"], b["id"] | ""))
                ++matches;
        if (matches != 1)
            return false;
    }
    for (JsonVariantConst p : doc["printers"].as<JsonArrayConst>()) {
        if (p.isNull())
            continue;
        if (!printer(p))
            return false;
        if (!strcmp(p["mode"] | "lan", "cloud")) {
            bool found = false;
            for (JsonVariantConst a : doc["cloudAccounts"].as<JsonArrayConst>())
                if (!strcmp(a["id"], p["accountId"])) {
                    if(a["simulator"].as<bool>() && strncmp(p["serialNumber"]|"","XTSIM",5))return false;
                    found = true;
                }
            if (!found)
                return false;
        }
    }
    unsigned active = doc["activePrinter"] | 0U;
    return doc["printers"].size() == 0 ? active == 0 : active < doc["printers"].size();
}
} // namespace XTouchSchema
