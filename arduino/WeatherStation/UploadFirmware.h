#ifndef UploadFirmwareService_h
#define UploadFirmwareService_h

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

typedef std::function<void(void)> FWUploadStartedCallback;
typedef std::function<void(bool)> FWUploadFinishedCallback;

#define UPLOAD_FIRMWARE_PATH "/api/uploadFirmware"

class UploadFirmwareEndpoint {
 public:
  explicit UploadFirmwareEndpoint(AsyncWebServer* server);
  void setCallbacks(FWUploadStartedCallback startedCallback, FWUploadFinishedCallback finishedCallback) {
    _startedCallback = startedCallback;
    _finishedCallback = finishedCallback;
   }
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
  void handleEarlyDisconnect();
 private:
  uint8_t _notify = 0; //1 -notify started, 2 - notify succes, 3 -notify error
  FWUploadStartedCallback _startedCallback = nullptr;
  FWUploadFinishedCallback _finishedCallback = nullptr;
};

#endif  // end UploadFirmwareService_h
