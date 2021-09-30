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
  ServiceLastMark
};


enum class ServiceState : uint8_t {
  NotRun = 0,
  SyncStarted,
  SyncOk,
  SyncFailed
};

// 8bytes
struct MemoryStatistic {
  uint32_t freeMem;
  uint16_t maxFreeBlock;
  uint8_t heapFragmentation;
};

// 20bytes
struct ServiceStatistic {
  ServiceState state;
  MemoryStatistic memBefore;
  MemoryStatistic memAfter;
};

struct RtcMemBlock {
  ServiceStatistic services[SyncServices::ServiceLastMark];
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
  // Store values to line protocol
  void toPoint(Point *point);
  void serviceStatisticToPoint(SyncServices service, Point *point);
  void printStatistics();
  ServiceStatistic& getServiceStatics(SyncServices service) { return _statistics.services[service]; }
private:
  RtcMemBlock _statistics;
};


uint32_t calculateCRC32(const uint8_t *data, size_t length);
void resetServiceStatistic(ServiceStatistic *stat);
void resetMemoryStatistic(MemoryStatistic *stat);
void updateMemoryStatistic(MemoryStatistic *stat);
const char *getServiceName(SyncServices service);



#endif //_WS_SERVICE_STATE_H_