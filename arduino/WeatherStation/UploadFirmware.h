#ifndef UploadFirmwareService_h
#define UploadFirmwareService_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

typedef std::function<void(void)> FWUploadFinishedCallback;

#define UPLOAD_FIRMWARE_PATH "/api/uploadFirmware"

class UploadFirmwareEndpoint {
 public:
  UploadFirmwareEndpoint(AsyncWebServer* server);
  void setCallback(FWUploadFinishedCallback callback) { _callback = callback; }
  void loop();
 private:
  void handleUpload(AsyncWebServerRequest* request,
                    const String& filename,
                    size_t index,
                    uint8_t* data,
                    size_t len,
                    bool final);
  void uploadComplete(AsyncWebServerRequest* request);
  void handleError(AsyncWebServerRequest* request, int code);
  static void handleEarlyDisconnect();
 private:
  bool _notify = false;
  FWUploadFinishedCallback _callback = nullptr;
};

#endif  // end UploadFirmwareService_h
