#ifndef _WS_SERVICE_STATE_H_
#define _WS_SERVICE_STATE_H_

#include <Arduino.h>
#include <InfluxDbClient.h>

enum SyncServices : uint8_t {
  ServiceLocationDetection = 0,
  ServiceClock,
  ServiceFWUpdate,
  ServiceAstronomy,
  ServiceCurrentWeather,
  ServiceForecast,
  ServiceIoTCenter,
  ServiceDBWriteStatus,
  ServiceDBWriteData,
  ServiceDBValidate,
  ServiceLastMark
};


enum class ServiceState : uint8_t {
  NotRun = 0,
  SyncStarted,
  SyncOk,
  SyncFailed
};

// 8 bytes
struct MemoryStatistic {
  uint32_t freeMem;
  uint16_t maxFreeBlock;
  uint8_t heapFragmentation;
};

// 20 bytes
struct ServiceStatistic {
  ServiceState state;
  MemoryStatistic memBefore;
  MemoryStatistic memAfter;
};

struct DataBlock {
  ServiceStatistic services[SyncServices::ServiceLastMark];
  uint8_t resetCount;
};

//204 bytes
struct RtcMemBlock {
  DataBlock data;
  uint32_t crc32;
};

class ServicesStatusTracker {
 public:
  ServicesStatusTracker();
  void updateServiceState(SyncServices service, ServiceState status);
  // Load from RTC memory
  void load();
  // Store to RTC memory
  void save(bool print = false);
  // Reset all values to initial
  void reset();
  void serviceStatisticToPoint(SyncServices service, Point *point);
  Point *serviceStatisticToPoint(SyncServices service);
  void printStatistics(const String &title);
  ServiceStatistic& getServiceStatics(SyncServices service) { return _statistics.data.services[service]; }
  void clearResetCount();
  uint8_t getResetCount() { return _statistics.data.resetCount; }
private:
  uint8_t _bootCount = 0;
  RtcMemBlock _statistics;
};

extern ServicesStatusTracker ServicesTracker;

uint32_t calculateCRC32(const uint8_t *data, size_t length);
void resetServiceStatistic(ServiceStatistic *stat);
void resetMemoryStatistic(MemoryStatistic *stat);
void updateMemoryStatistic(MemoryStatistic *stat);
const char *getServiceName(SyncServices service);



#endif //_WS_SERVICE_STATE_H_