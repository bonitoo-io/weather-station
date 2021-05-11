import logging
import wifi_mgr as wifimgr

def gc():
    import gc
    gc.collect()
    gc.threshold(gc.mem_free() // 4 + gc.mem_alloc())

log = logging.getLogger("boot")

# wlan_ap = network.WLAN(network.AP_IF)
# wlan_sta = network.WLAN(network.STA_IF)
#
# if wlan_sta.isconnected():
#     wlan_sta.disconnect()
# wlan_ap.active(False)
# wlan_sta.active(False)

wlan = wifimgr.get_connection()
if wlan is None:
    log.info("Could not initialize the network connection.")
    while True:
        pass  # you shall not pass :D

gc()




