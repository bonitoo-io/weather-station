import time
import network
try:
    import uasyncio as asyncio
except ImportError:
    pass

wlan = network.WLAN(network.STA_IF)
wlan.disconnect()

class WifiManager(object):
    webrepl_triggered = False
    _ap_start_policy = "never"

    def __init__(self, config):
        self.cfg = config

    # Checks the status and configures if needed
    async def manage(self):
        while True:
            status = self.wlan().status()
            # ESP32 does not currently return
            if (status != network.STAT_GOT_IP) or \
            (self.wlan().ifconfig()[0] == '0.0.0.0'):  # temporary till #3967
                print("Network not connected: managing")
                # Ignore connecting status for now.. ESP32 is a bit strange
                # if status != network.STAT_CONNECTING: <- do not care yet
                self.setup_network()
            await asyncio.sleep(10)  # Pause 5 seconds

    def wlan(self):
        return network.WLAN(network.STA_IF)


    def accesspoint(self):
        return network.WLAN(network.AP_IF)


    def wants_accesspoint(self) -> bool:
        static_policies = {"never": False, "always": True}
        if self._ap_start_policy in static_policies:
            return static_policies[self._ap_start_policy]
        # By default, that leaves "Fallback"
        return self.wlan().status() != network.STAT_GOT_IP  # Discard intermediate states and check for not connected/ok


    def setup_network(self) -> bool:
        # now see our prioritised list of networks and find the first available network
        self.preferred_networks = self.cfg['known_networks']
        self.ap_config = self.cfg["access_point"]


        # set things up
        self.webrepl_triggered = False  # Until something wants it
        self.wlan().active(True)

        # scan what’s available
        available_networks = []
        for network in self.wlan().scan():
            ssid = network[0].decode("utf-8")
            bssid = network[1]
            strength = network[3]
            available_networks.append(dict(ssid=ssid, bssid=bssid, strength=strength))
        # Sort fields by strongest first in case of multiple SSID access points
        available_networks.sort(key=lambda station: station["strength"], reverse=True)

        # Get the ranked list of BSSIDs to connect to, ranked by preference and strength amongst duplicate SSID
        candidates = []
        for aPreference in self.preferred_networks:
            for aNetwork in available_networks:
                if aPreference["ssid"] == aNetwork["ssid"]:
                    connection_data = {
                        "ssid": aNetwork["ssid"],
                        "bssid": aNetwork["bssid"],  # NB: One day we might allow collection by exact BSSID
                        "password": aPreference["password"],
                        "enables_webrepl": aPreference["enables_webrepl"]}
                    candidates.append(connection_data)

        for new_connection in candidates:
            print("Attempting to connect to network {0}...".format(new_connection["ssid"]))
            # Micropython 1.9.3+ supports BSSID specification so let's use that
            if self.connect_to(ssid=new_connection["ssid"], password=new_connection["password"],
                              bssid=new_connection["bssid"]):
                print("Successfully connected {0}".format(new_connection["ssid"]))
                self.webrepl_triggered = new_connection["enables_webrepl"]
                break  # We are connected so don't try more


        # Check if we are to start the access point
        self._ap_start_policy = self.ap_config.get("start_policy", "never")
        should_start_ap = self.wants_accesspoint()
        self.accesspoint().active(should_start_ap)
        if should_start_ap:  # Only bother setting the config if it WILL be active
            print("Enabling your access point...")
            self.accesspoint().CONFIG(**self.ap_config["config"])
            self.webrepl_triggered = self.ap_config["enables_webrepl"]
        self.accesspoint().active(self.wants_accesspoint())  # It may be DEACTIVATED here

        # may need to reload the config if access points trigger it

        # start the webrepl according to the rules
        if self.webrepl_triggered:
            import webrepl
            webrepl.start()

        # return the success status, which is ultimately if we connected to managed and not ad hoc wifi.
        return self.wlan().isconnected()


    def connect_to(self, *, ssid, password, **kwargs) -> bool:
        self.wlan().connect(ssid, password, **kwargs)

        for check in range(0, 10):  # Wait a maximum of 10 times (10 * 500ms = 5 seconds) for success
            if self.wlan().isconnected():
                return True
            time.sleep_ms(500)
        return False
