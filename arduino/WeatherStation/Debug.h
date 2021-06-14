#ifndef DEBUG_H
#define DEBUG_H

#define WS_DEBUG_ENABLE

#ifdef WS_DEBUG_ENABLE
# define WS_DEBUG_RAM(text) { Serial.printf_P(PSTR(text ": free_heap %d, max_alloc_heap %d, heap_fragmentation  %d\n"), ESP.getFreeHeap(), ESP.getMaxFreeBlockSize(), ESP.getHeapFragmentation()); }
#else
# define WS_DEBUG_RAM(text)
#endif //WS_DEBUG_ENABLE
#endif //DEBUG_H