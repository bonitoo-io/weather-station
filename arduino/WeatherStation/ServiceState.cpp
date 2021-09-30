#include "ServiceState.h"

// Must be the same number and the same order like SyncServices enum
static const char *serviceNames[] PROGMEM = {"location", "clock", "update","astronomy","current_weather","forecast","iot_center"};

ServicesStatusTracker::ServicesStatusTracker() {
}

void ServicesStatusTracker::updateServiceState(SyncServices service, ServiceState state) {
  ServiceStatistic *stat = &_statistics.services[service];
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
  Serial.printf_P(PSTR("Read from RTC mem, size %d\n"), sizeof(_statistics));
  if(ESP.rtcUserMemoryRead(0, (uint32_t*) &_statistics, sizeof(_statistics))) {
     uint32_t crcOfData = calculateCRC32((uint8_t*) &_statistics.services[0], sizeof(_statistics.services));
     ok = _statistics.crc32 == crcOfData;
     if(ok) {
       printStatistics();
     } else {
       Serial.println(F("RTC memory data CRC error"));
     }
  } else {
    Serial.println(F("Failed reading RTC memory"));
  }
  if(!ok) {
    reset();
  }
}

void ServicesStatusTracker::save(bool print) {
  Serial.printf_P(PSTR("Write to RTC mem, size %d\n"), sizeof(_statistics));
  _statistics.crc32 = calculateCRC32((uint8_t*) &_statistics.services[0], sizeof(_statistics.services));
  // Write struct to RTC memory
  if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &_statistics, sizeof(_statistics))) {
    if(print) {
      printStatistics();
    }
  } else {
    Serial.println(F("Failed writing to RTC memory"));
  }
}

void ServicesStatusTracker::reset() {
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
    resetServiceStatistic(&_statistics.services[i]);
  }
}

void ServicesStatusTracker::toPoint(Point *point) {
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
      serviceStatisticToPoint((SyncServices)i, point);
  }
}

void ServicesStatusTracker::serviceStatisticToPoint(SyncServices service, Point *point) {
  const __FlashStringHelper *serviceName = FPSTR(getServiceName(service));
  point->addField(String(serviceName)+F("_state"),(uint8_t) _statistics.services[service].state);
  if(_statistics.services[service].state != ServiceState::NotRun) {
    point->addField(String(serviceName)+F("_before_mem_free"), _statistics.services[service].memBefore.freeMem);
    point->addField(String(serviceName)+F("_before_mem_max_free_block"), _statistics.services[service].memBefore.maxFreeBlock);
    point->addField(String(serviceName)+F("_before_mem_framentation"), _statistics.services[service].memBefore.heapFragmentation);
    if(_statistics.services[service].state != ServiceState::SyncStarted) {
      point->addField(String(serviceName)+F("_after_mem_free"), _statistics.services[service].memAfter.freeMem);
      point->addField(String(serviceName)+F("_after_mem_max_free_block"), _statistics.services[service].memAfter.maxFreeBlock);
      point->addField(String(serviceName)+F("_after_mem_framentation"), _statistics.services[service].memAfter.heapFragmentation);
    }
  }
}

void ServicesStatusTracker::printStatistics() {
  Serial.println(" Services statistics: ");
  for(uint8_t i = 0; i < SyncServices::ServiceLastMark; i++) {
      Serial.print(F("\t"));
      Serial.print(FPSTR(getServiceName((SyncServices)i)));
      Serial.printf_P(PSTR(" - State %u, memory: \n"), _statistics.services[i].state);
      Serial.printf_P(PSTR("\t\tbefore: free %u, block: %u, fragmentation: %u\n"),_statistics.services[i].memBefore.freeMem,_statistics.services[i].memBefore.maxFreeBlock, _statistics.services[i].memBefore.heapFragmentation);
      Serial.printf_P(PSTR("\t\tafter: free %u, block: %u, fragmentation: %u\n"),_statistics.services[i].memAfter.freeMem,_statistics.services[i].memAfter.maxFreeBlock, _statistics.services[i].memAfter.heapFragmentation);
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
