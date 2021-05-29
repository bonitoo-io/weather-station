from influxdb2 import Point, Client, WritePrecision
from wm import WifiManager
from machine import Pin, I2C, reset
from ssd1306 import SSD1306_I2C
from dht import DHT11
from micropython import const
import ntp
import gc

try:
    from urandom import getrandbits
    import utime as time
    import uasyncio as asyncio
except:
    from random import getrandbits
    import time
    import asyncio



INTERVAL_MS = CFG["config"]["interval"]
NTP_INTERVAL = CFG["config"]["ntp_interval"]
OLED_WIDTH = const(128)
OLED_HEIGHT = const(64)
TEXT_SIZE = {"x": const(8), "y": const(8)}
Y_SPACING = const(6)
oled = None
i2c = None
dht11 = None
led2 = None
loop = None


def show_text_line(x, y, text, c=1, timeout=0):
    if c == 0:
        fill_c = 1
    else:
        fill_c = 0
    oled.text(text, x, y, c)
    oled.show()
    if timeout > 0:
        time.sleep(timeout)
        oled.fill_rect(x, y, (TEXT_SIZE["x"] * len(text)), TEXT_SIZE["y"], fill_c)
    else:
        return (x, y, TEXT_SIZE["x"] * len(text), TEXT_SIZE["y"], fill_c)


def clear_text_line(rect):
    oled.fill_rect(*rect)


def init():
    global i2c, oled, led2, dht11
    # init buses
    i2c = I2C(scl=Pin(14), sda=Pin(2), freq=400000)
    oled = SSD1306_I2C(OLED_WIDTH, OLED_HEIGHT, i2c, addr=0x3c)
    show_text_line(12, 30, "InfluxData WS", 1, 3)
    # init devices
    line = show_text_line(5, 30, "Initialize", 1)
    led2 = Pin(4, Pin.OUT)
    dht11 = DHT11(Pin(5))
    clear_text_line(line)
    show_text_line(5, 30, "Connecting WiFi", 1)


def randrange(start, stop=None):
    if stop is None:
        stop = start
        start = 0
    upper = stop - start
    bits = 0
    pwr2 = 1
    while upper > pwr2:
        pwr2 <<= 1
        bits += 1
    while True:
        r = getrandbits(bits)
        if r < upper:
            break
    return r + start


def restart():
    oled.fill(0)
    show_text_line(5, 30, "OSError, rebooting", 1, 3)
    reset()


def get_t_rh():
    dht11.measure()
    t = dht11.temperature()  # eg. 23 (°C)
    h = dht11.humidity()  # eg. 41 (% RH)
    return t, h


async def show_data(screens):
    global x_random, y_random
    paging_ms = INTERVAL_MS // len(screens) - 1000
    oled.fill(0)
    for screen, lines in screens.items():
        y_max = OLED_HEIGHT - ((Y_SPACING * (len(lines) - 1)) + (TEXT_SIZE["y"] * len(lines)))
        x_random = randrange(0, OLED_WIDTH - len(screens[screen][0]) * TEXT_SIZE["x"])
        y_random = randrange(0, y_max)
        if y_max < 0:
            raise ValueError("More lines than display can handle")
        else:
            for num, line in enumerate(lines, start=0):
                x = x_random
                y = y_random + (Y_SPACING * num) + (TEXT_SIZE["y"] * num)
                oled.text(line, x, y)
            oled.show()
            if paging_ms < INTERVAL_MS:
                await asyncio.sleep_ms(paging_ms)


def define_screens(data):
    screens = {}
    # Set-up screens
    screen1_line1 = 'Temp: {0} C'.format(data["t"])
    screen1_line2 = 'RH: {0} %'.format(data["rh"])
    screens["screen1"] = [screen1_line1, screen1_line2]
    return screens


def publish(data, client):
    point = Point("dht11") \
        .field("temperature", data["t"]) \
        .field("humidity", data["rh"]) \
        .tag("location", "office") \
        .time(time.time_ns() + WritePrecision.DELTA_NS)
    res = client.write(point)
    if res:
        print("Writing data failed: {0}".format(res))


async def run(interval, client):
    ntp.set_rtc()
    t0 = time.ticks_ms()
    try:
        while True:
            data = {}
            oled.fill(0)
            show_text_line(5, 30, "Loading data", 1)
            gc.collect()
            print(gc.mem_free())
            led2.value(0)
            data["t"], data["rh"] = get_t_rh()
            oled.fill(0)
            show_text_line(5, 30, "Publishing data", 1)
            publish(data, client)
            await show_data(define_screens(data))
            led2.value(1)

            # sleep
            iv = interval * 1000
            while True:
                t1 = time.ticks_ms()
                dt = time.ticks_diff(t1, t0)
                if dt >= iv:
                    break
                await asyncio.sleep_ms(min(iv - dt, 500))
            if iv <= dt < iv * 3 / 2:
                t0 = time.ticks_add(t0, iv)
            else:
                t0 = time.ticks_ms()
    except OSError as e:
        print("OSError: {0}".format(e))
        restart()
    finally:
        oled.poweroff()


async def schedule(cbk, t, *args, **kwargs):
    while True:
        await asyncio.sleep(t)
        cbk(*args, **kwargs)


def main():
    global loop
    loop = asyncio.get_event_loop()
    influxdb_client = Client(CFG["influxdb2"]["url"],
                             CFG["influxdb2"]["bucket"],
                             CFG["influxdb2"]["org"],
                             precision=WritePrecision.NS,
                             token=CFG["influxdb2"]["token"])

    init()
    loop.create_task(WifiManager(CFG).manage())
    loop.create_task(schedule(ntp.set_rtc, NTP_INTERVAL))
    loop.create_task(run(INTERVAL_MS, influxdb_client))
    loop.run_forever()


if __name__ == "__main__":
    print("Welcome on InfluxData WeatherStation!")
    main()
