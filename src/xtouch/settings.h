#ifndef _XLCD_SETTINGS
#define _XLCD_SETTINGS

void xtouch_settings_save(bool onlyRoot = false)
{
    DynamicJsonDocument doc(384);
    const char *languages[] = {"zh", "zh-TW", "en", "fr", "ru", "ja"};
    doc["language"] = languages[xtouch_language_get()];
    doc["backlight"] = xTouchConfig.xTouchBacklightLevel;
    doc["tftOff"] = xTouchConfig.xTouchTFTOFFValue;
    doc["tftInvert"] = xTouchConfig.xTouchTFTInvert;
    doc["ota"] = xTouchConfig.xTouchOTAEnabled;
    doc["wop"] = xTouchConfig.xTouchWakeOnPrint;
    doc["chamberTempDiff"] = xTouchConfig.xTouchChamberSensorReadingDiff;
    doc["chamberTemp"] = xTouchConfig.xTouchChamberSensorEnabled;
    doc["auxFan"] = xTouchConfig.xTouchAuxFanEnabled;
    doc["chamberFan"] = xTouchConfig.xTouchChamberFanEnabled;

    if (!xtouch_storage_write(xtouch_paths_settings, doc)) ConsoleError.println("[STORE] settings save failed");
}

void xtouch_settings_loadSettings()
{
    if (!xtouch_storage_has(xtouch_paths_settings))
    {
        DynamicJsonDocument doc(256);
        xTouchConfig.xTouchBacklightLevel = 128;
        xTouchConfig.xTouchTFTOFFValue = 15;
        xTouchConfig.xTouchTFTInvert = false;
        xTouchConfig.xTouchOTAEnabled = false;
        xTouchConfig.xTouchWakeOnPrint = true;
        xTouchConfig.xTouchChamberSensorReadingDiff = 0;
        xTouchConfig.xTouchChamberSensorEnabled = false;
        xTouchConfig.xTouchAuxFanEnabled = false;
        xTouchConfig.xTouchChamberFanEnabled = false;
        xtouch_settings_save(true);
    }

    DynamicJsonDocument settings = xtouch_storage_read(xtouch_paths_settings, 2048);
    const char *language = settings["language"] | "zh";
    const char *languages[]={"zh","zh-TW","en","fr","ru","ja"};
    xtouch_language_set(0);for(unsigned i=0;i<6;++i)if(!strcmp(language,languages[i]))xtouch_language_set(i);

    xTouchConfig.xTouchBacklightLevel = settings.containsKey("backlight") ? settings["backlight"].as<int>() : 128;
    xTouchConfig.xTouchTFTOFFValue = settings.containsKey("tftOff") ? settings["tftOff"].as<int>() : 15;
    xTouchConfig.xTouchTFTInvert = settings.containsKey("tftInvert") ? settings["tftInvert"].as<bool>() : false;
    xTouchConfig.xTouchOTAEnabled = settings.containsKey("ota") ? settings["ota"].as<bool>() : false;
    xTouchConfig.xTouchWakeOnPrint = settings.containsKey("wop") ? settings["wop"].as<bool>() : true;
    xTouchConfig.xTouchChamberSensorReadingDiff = settings.containsKey("chamberTempDiff") ? settings["chamberTempDiff"].as<int8_t>() : 0;
    xTouchConfig.xTouchChamberSensorEnabled = settings.containsKey("chamberTemp") ? settings["chamberTemp"].as<bool>() : false;
    xTouchConfig.xTouchAuxFanEnabled = settings.containsKey("auxFan") ? settings["auxFan"].as<bool>() : false;
    xTouchConfig.xTouchChamberFanEnabled = settings.containsKey("chamberFan") ? settings["chamberFan"].as<bool>() : false;

    bool isTFTFlipped = xtouch_screen_getTFTFlip();
    tft.setRotation(isTFTFlipped ? 3 : 1);
    x_touch_touchScreen.setRotation(isTFTFlipped ? 3 : 1);
    xtouch_screen_setBrightness(xTouchConfig.xTouchBacklightLevel);

    xtouch_screen_invertColors();
}

#endif
