#ifndef UPDATER_H
#define UPDATER_H

#include "Settings.h"
#include <ESPGithubUpdater.h>
#include "RegionalSettings.h"
#include "AdvancedSettings.h"


typedef std::function<void(const char *newVersion, int progress)> FWUpdateProgressCallback;
typedef std::function<void(const char *newVersion)> FWUpdateStartedCallback;
typedef std::function<void(bool success, const char *error)> FWUpdateFinishedCallback;


class Updater {
 public:
  Updater();
  void init(AdvancedSettings *settings, const char *currentVersion);
  bool checkUpdate();
  void setUpdateCallbacks(FWUpdateStartedCallback startCb, FWUpdateProgressCallback progCb, FWUpdateFinishedCallback endCb) {
    _startCb = startCb;
    _progCb = progCb;
    _endCb = endCb;
  }
 private:
  AdvancedSettings *_settings;
  const char *_currentVersion = nullptr;
  FWUpdateStartedCallback _startCb = nullptr;
  FWUpdateProgressCallback _progCb = nullptr;
  FWUpdateFinishedCallback _endCb = nullptr;
};

#endif //UPDATER_H
