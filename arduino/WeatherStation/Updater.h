#ifndef UPDATER_H
#define UPDATER_H

#include <ESPGithubUpdater.h>

typedef std::function<void(const char *newVersion, int progress)> FWUpdateProgressCallback;
typedef std::function<void(const char *newVersion)> FWUpdateStartedCallback;
typedef std::function<void(bool success, const char *error)> FWUpdateFinishedCallback;

class Updater {
 public:
  Updater();
  void init(const char *owner, const char *repo, const char *currentVersion, bool checkBeta = false);
  void checkUpdate();
  void setUpdateCallbacks(FWUpdateStartedCallback startCb, FWUpdateProgressCallback progCb, FWUpdateFinishedCallback endCb) {
     _startCb = startCb;
     _progCb = progCb;
     _endCb = endCb;
  }
 private:
  const char *_owner = nullptr;
  const char *_repo = nullptr;
  const char *_currentVersion = nullptr;
  bool _checkBeta = false;
  FWUpdateStartedCallback _startCb = nullptr;
  FWUpdateProgressCallback _progCb = nullptr;
  FWUpdateFinishedCallback _endCb = nullptr;
};

#endif //UPDATER_H
