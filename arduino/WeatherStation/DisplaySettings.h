#ifndef WS_DISPLAY_SETTINGS_H
#define WS_DISPLAY_SETTINGS_H

#include <Arduino.h>
#include "Settings.h"
#include "AdvancedSettings.h"
#include "RegionalSettings.h"

#define DISPLAY_DEFAUT_SCREEN_ROTATE_INTERVAL 10
#define DISPLAY_SETTINGS_ENDPOINT_PATH "/api/displaySettings"
#define DISPLAY_DEFAUT_NIGHT_MODE_BEGIN 2200  //HHMM
#define DISPLAY_DEFAUT_NIGHT_MODE_END 700  //HHMM
#define DISPLAY_DEFAUT_NIGHT_MODE_USE_BIG_FONT false

class DisplaySettings : public Settings {
  public:
    // Interval of screens changes, in seconds
    uint8_t screenRotateInterval;
    // Letters of visible screens
    String screens;
    // Begin night mode
    uint16_t nightModeBegin;
    // End night mode
    uint16_t nightModeEnd;
    // Use bigger font in night mode
    bool nightModeBigFont;
  public:
    DisplaySettings();
    virtual ~DisplaySettings() {};
    virtual int save(JsonObject& root) override;
    virtual int load(JsonObject& root) override;
    virtual void print(const __FlashStringHelper *title) override;
    virtual String getFilePath() override { return F(FS_CONFIG_DIRECTORY "/displaySettings.json"); }
};

class DisplaySettingsEndpoint : public SettingsEndpoint {
public:
  DisplaySettingsEndpoint(FSPersistence *pPersistence, DisplaySettings *pSettings, RegionalSettings *pRegionalSettings);
private:
  RegionalSettings *_pRegionalSettings;
};

#endif //WS_DISPLAY_SETTINGS_H
