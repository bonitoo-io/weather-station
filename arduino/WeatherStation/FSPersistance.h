#ifndef FS_PERSISTENCE_H
#define FS_PERSISTENCE_H
#include <FS.h>
#include <ArduinoJson.h>
#include "Settings.h"
#include <vector>

class FSPersistence {
public:
    FSPersistence(FS *fs);
    bool readFromFS(Settings *s);
    bool writeToFS(Settings *s);
    bool begin();
    void removeConfigs();
    bool removeConfig(const String &filename);
    void traverseConfigs(std::function<void(const String &path, const String &fileName)> callback, const String &root = FS_CONFIG_DIRECTORY);
    std::vector<String> listConfigs(const String &root = FS_CONFIG_DIRECTORY, bool fileNameOnly = false);
    FS *getFS() { return _fs; }
private:
    FS* _fs;
};
#endif  // FS_PERSISTENCE_H
