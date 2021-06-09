#include "FSPersistance.h"

FSPersistence::FSPersistence(FS *fs):_fs(fs) { 
    
}

void FSPersistence::readFromFS(Settings *s) {
    Serial.printf("FS: opening %s for reading",s->filePath().c_str());
    File settingsFile = _fs->open(s->filePath(), "r");

    if (settingsFile) {
        Serial.println(" - ok");
        DynamicJsonDocument jsonDocument = DynamicJsonDocument(DEFAULT_BUFFER_SIZE);
        DeserializationError error = deserializeJson(jsonDocument, settingsFile);
        if (error == DeserializationError::Ok && jsonDocument.is<JsonObject>()) {
            Serial.println("  deserialize ok");
            JsonObject jsonObject = jsonDocument.as<JsonObject>();
            s->load(jsonObject);
            settingsFile.close();
            return;
        }
        Serial.println(" deserialize error");
        settingsFile.close();
    } else {
        Serial.println(" - not exist");
    }
    // File doesn't exist, write defaults
    writeToFS(s);
}

bool FSPersistence::writeToFS(Settings *s) {
    // create and populate a new json object
    DynamicJsonDocument jsonDocument = DynamicJsonDocument(DEFAULT_BUFFER_SIZE);
    JsonObject jsonObject = jsonDocument.to<JsonObject>();
    s->save(jsonObject);

    // serialize it to filesystem
    File settingsFile = _fs->open(s->filePath(), "w");
    Serial.printf("FS: opening %s for writing",s->filePath().c_str());
    // failed to open file, return false
    if (!settingsFile) {
        Serial.println(" not exist");
        return false;
    }
    Serial.println(" ok");

    // serialize the data to the file
    serializeJson(jsonDocument, settingsFile);
    settingsFile.close();
    return true;
}

void FSPersistence::begin() {
    _fs->begin();
}