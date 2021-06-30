from json import load
import network

try:
    with open('config.json', 'r') as json_file:
        CFG = load(json_file)
except Exception as e:
    print('Failed to load config file')


def collect_gc():
    import gc
    gc.collect()
    gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())


def debug(value):
    # value=None - turn off vendor O/S debugging messages
    # value=0 - redirect vendor O/S debugging messages to UART(0)
    import esp
    esp.osdebug(value)


wlan = network.WLAN(network.STA_IF)
wlan.disconnect()
collect_gc()
debug(None)
