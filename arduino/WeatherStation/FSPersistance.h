#ifndef FS_PERSISTENCE_H
#define FS_PERSISTENCE_H

#include <FS.h>
#include <ArduinoJson.h>
#include "Settings.h"


class FSPersistence {
public:
    FSPersistence(FS *fs);
    void readFromFS(Settings *s);
    bool writeToFS(Settings *s);
    void begin();
private:
    FS* _fs;
};

#endif  // FS_PERSISTENCE_H
