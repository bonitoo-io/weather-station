#include "ServiceState.h"

// Must be the same number and the same order like SyncServices enum
static const char *serviceNames[] PROGMEM = {"location", "clock", "update","astronomy","current_weather","forecast","iot_center","db_write_status","db_write_data","db_validate"};

ServicesStatusTracker ServicesTracker;

ServicesStatusTracker::ServicesStatusTracker() {
  _statistics.data.resetCount = 0;
}

void ServicesStatusTracker::updateServiceState(SyncServices service, ServiceState state) {
  ServiceStatistic *stat = &_statistics.data.services[service];
  stat->state = state;
  switch(state) {
    case ServiceState::NotRun:
      resetServiceStatistic(stat);
      break;
    case ServiceState::SyncStarted:
      updateMemoryStatistic(&stat->memBefore);
      break;
    case ServiceState::SyncFailed:
    case ServiceState::SyncOk:
      updateMemoryStatistic(&stat->memAfter);
      break;
  }
}

void ServicesStatusTracker::load() {
  bool ok = false;
  if(ESP.rtcUserMemoryRead(0, (uint32_t*) &_statistics, sizeof(_statistics))) {
    Serial.printf_P(PSTR("Load RTC memory: data len %d\n"), sizeof(_statistics.data));
     uint32_t crcOfData = calculateCRC32((uint8_t*) &_statistics.data, sizeof(_statistics.data));
     ok = _statistics.crc32 == crcOfData;
     if(ok) {
       printStatistics(F("Load"));
     } else {
       Serial.println(F("Load RTC memory: data CRC error"));
     }
  } else {
    Serial.println(F("Failed reading RTC memory"));
  }
  if(!ok) {
    reset();
    clearResetCount();
  }
  ++_statistics.data.resetCount;
}

void ServicesStatusTracker::save(bool print) {
  _statistics.crc32 = calculateCRC32((uint8_t*) &_statistics.data, sizeof(_statistics.data));
  // Write struct to RTC memory
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &_statistics, sizeof(_statistics))) {
    if(print) {
      printStatistics(F("Save"));
    }
  } else {
    Serial.println(F("Failed writing to RTC memory"));
  }
}

void ServicesStatusTracker::reset() {
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
    resetServiceStatistic(&_statistics.data.services[i]);
  }
}

void ServicesStatusTracker::clearResetCount() {
  _statistics.data.resetCount = 0;
}

void ServicesStatusTracker::serviceStatisticToPoint(SyncServices service, Point *point) {
  point->addField(F("state"),(uint8_t) _statistics.data.services[service].state);
  if(_statistics.data.services[service].state != ServiceState::NotRun) {
    point->addField(F("before_mem_free"), _statistics.data.services[service].memBefore.freeMem);
    point->addField(F("before_mem_max_free_block"), _statistics.data.services[service].memBefore.maxFreeBlock);
    point->addField(F("before_mem_framentation"), _statistics.data.services[service].memBefore.heapFragmentation);
    if(_statistics.data.services[service].state != ServiceState::SyncStarted) {
      point->addField(F("after_mem_free"), _statistics.data.services[service].memAfter.freeMem);
      point->addField(F("after_mem_max_free_block"), _statistics.data.services[service].memAfter.maxFreeBlock);
      point->addField(F("after_mem_framentation"), _statistics.data.services[service].memAfter.heapFragmentation);
    }
  }
}

Point *ServicesStatusTracker::serviceStatisticToPoint(SyncServices service) {
  Point *point = new Point(F("service_status"));
  point->addTag(F("service"), String(FPSTR(getServiceName(service))));
  serviceStatisticToPoint(service, point);
  return point;
}

void ServicesStatusTracker::printStatistics(const String &title) {
  Serial.print(title);
  Serial.println(F(" services statistics:"));
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
      Serial.print(F("\t"));
      Serial.print(FPSTR(getServiceName((SyncServices)i)));
      Serial.printf_P(PSTR(" - State %u, memory: \n"), _statistics.data.services[i].state);
      Serial.printf_P(PSTR("\t\tbefore: free %u, block: %u, fragmentation: %u\n"),_statistics.data.services[i].memBefore.freeMem,_statistics.data.services[i].memBefore.maxFreeBlock, _statistics.data.services[i].memBefore.heapFragmentation);
      Serial.printf_P(PSTR("\t\tafter: free %u, block: %u, fragmentation: %u\n"),_statistics.data.services[i].memAfter.freeMem,_statistics.data.services[i].memAfter.maxFreeBlock, _statistics.data.services[i].memAfter.heapFragmentation);
  }
}

void resetServiceStatistic(ServiceStatistic *stat) {
  stat->state = ServiceState::NotRun;
  resetMemoryStatistic(&stat->memBefore);
  resetMemoryStatistic(&stat->memAfter);
}

void resetMemoryStatistic(MemoryStatistic *stat) {
  stat->freeMem = 0;
  stat->maxFreeBlock = 0;
  stat->heapFragmentation = 0;
}

void updateMemoryStatistic(MemoryStatistic *stat) {
  ESP.getHeapStats(&stat->freeMem, &stat->maxFreeBlock, &stat->heapFragmentation);
};

const char *getServiceName(SyncServices service) {
  return serviceNames[service];
}

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
  uint32_t crc = 0xffffffff;
  while (length--) {
    uint8_t c = *data++;
    for (uint32_t i = 0x80; i > 0; i >>= 1) {
      bool bit = crc & 0x80000000;
      if (c & i) {
        bit = !bit;
      }
      crc <<= 1;
      if (bit) {
        crc ^= 0x04c11db7;
      }
    }
  }
  return crc;
}
