#include "FSPersistance.h"

FSPersistence::FSPersistence(FS *fs):_fs(fs) { 
    
}

bool FSPersistence::readFromFS(Settings *s) {
    Serial.printf("FS: opening %s for reading",s->getFilePath().c_str());
    File settingsFile = _fs->open(s->getFilePath(), "r");

    if (settingsFile) {
        Serial.println(F(" - ok"));
        DynamicJsonDocument jsonDocument = DynamicJsonDocument(DEFAULT_BUFFER_SIZE);
        DeserializationError error = deserializeJson(jsonDocument, settingsFile);
        if (error == DeserializationError::Ok && jsonDocument.is<JsonObject>()) {
            Serial.println();
            JsonObject jsonObject = jsonDocument.as<JsonObject>();
            s->load(jsonObject);
            settingsFile.close();
            return true;
        }
        Serial.println(F(" deserialize error"));
        settingsFile.close();
        return false;
    } else {
        Serial.println(F(" - not exist"));
    }
    // File doesn't exist, write defaults
    return writeToFS(s);
}

bool FSPersistence::writeToFS(Settings *s) {
    // create and populate a new json object
    DynamicJsonDocument jsonDocument = DynamicJsonDocument(DEFAULT_BUFFER_SIZE);
    JsonObject jsonObject = jsonDocument.to<JsonObject>();
    s->save(jsonObject);

    // serialize it to filesystem
    File settingsFile = _fs->open(s->getFilePath(), "w");
    Serial.printf("FS: opening %s for writing",s->getFilePath().c_str());
    // failed to open file, return false
    if (!settingsFile) {
        Serial.println(F(" not exist"));
        return false;
    }
    Serial.println();

    // serialize the data to the file
    serializeJson(jsonDocument, settingsFile);
    settingsFile.close();
    return true;
}

bool FSPersistence::begin() {
  if(!_fs->begin()) {
    Serial.println(F(" Error mounting filesystem!"));
    return false;
  }
  return true;
}

void FSPersistence::removeConfigs() {
  traverseConfigs([this](const String &path, const String &fileName){
    removeConfig(path + "/" + fileName);
  }, FS_CONFIG_DIRECTORY);  
}

bool FSPersistence::removeConfig(const String &getFilePath) {
  return _fs->remove(getFilePath);
}

std::vector<String> FSPersistence::listConfigs(const String &root, bool fileNameOnly) {
  std::vector<String> list;  
  traverseConfigs([&](const String &path, const String &fileName){
    list.push_back(fileNameOnly?fileName:path + "/" + fileName);
  }, root);  
  return list;
}

void FSPersistence::traverseConfigs(std::function<void(const String &path, const String &fileName)> callback, const String &root) {
  Dir configDirectory = _fs->openDir(root);
  while (configDirectory.next()) {
    if(configDirectory.isDirectory()) {
      traverseConfigs(callback, root + ((!root.length() || root[root.length()-1] != '/')?"/":"") + configDirectory.fileName());
    } else {
      callback(root, configDirectory.fileName());
    }
  }
}