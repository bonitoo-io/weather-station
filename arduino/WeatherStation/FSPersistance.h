#ifndef FS_PERSISTENCE_H
#define FS_PERSISTENCE_H
#include <FS.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include <vector>

class FSPersistence {
public:
    FSPersistence(FS *fs);
    void readFromFS(Settings *s);
    bool writeToFS(Settings *s);
    void begin();
    void removeConfigs();
    void removeConfig(const String &filename);
    void traverseConfigs(std::function<void(const String &path, const String &fileName)> callback, const String &root = FS_CONFIG_DIRECTORY);
    std::vector<String> listConfigs(const String &root = FS_CONFIG_DIRECTORY, bool fileNameOnly = false);
    FS *getFS() { return _fs; }
private:
    FS* _fs;
};
#endif  // FS_PERSISTENCE_H
