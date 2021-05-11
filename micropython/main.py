import logging
import dht
import machine
from ssd1306 import SSD1306_I2C

log = logging.getLogger("main")
log.setLevel(logging.DEBUG)
interval_ms = 5000
oled_width = 128
oled_height = 64
text_size = {"x": 8, "y": 10}
yspacing = 6

try:
    import usocket as socket
    import uselect as select
    import uasyncio as asyncio
    import utime as time
    import urandom as random
except:
    import socket
    import select
    import asyncio
    import time
    import random

# init interfaces
i2c = machine.I2C(scl=machine.Pin(14), sda=machine.Pin(2), freq=400000)
log.info('Scan i2c bus...')
devices = i2c.scan()

if len(devices) == 0:
    log.error("No i2c device !")
else:
    for device in devices:
        log.info("i2c device found: {0}".format(hex(device)))

led2 = machine.Pin(4, machine.Pin.OUT)

# init devices
try:
    dht11 = dht.DHT11(machine.Pin(5))
    oled = SSD1306_I2C(oled_width, oled_height, i2c, addr=0x3c)
    oled.fill(1)
    oled.fill_rect(10, 10, 108, 44, 0)
    oled.text("InfluxData", 20, 20, 1)
    oled.show()
    time.sleep(2)
except Exception as e:
    log.error("Init of devices failed: {0}".format(e))


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
        r = random.getrandbits(bits)
        if r < upper:
            break
    return r + start


def restart():
    machine.reset()


def get_t_rh():
    dht11.measure()
    t = dht11.temperature()  # eg. 23 (°C)
    h = dht11.humidity()  # eg. 41 (% RH)
    return t, h


async def show_data(screens):
    paging_ms = interval_ms // len(screens) - 1000
    oled.fill(0)
    for screen, lines in screens.items():
        y_max = oled_height - ((yspacing * (len(lines) - 1)) + (text_size["y"] * len(lines)))
        log.debug(y_max)
        xrandom = randrange(0, (oled_width) - len(screens[screen][0]) * text_size["x"])
        yrandom = randrange(0, y_max)
        if y_max < 0:
            raise ValueError("More llines than display can handle")
        else:
            for num, line in enumerate(lines, start=0):
                x = xrandom
                y = yrandom + (yspacing * num) + (text_size["y"] * num)
                log.debug("line_text: {0}, x: {1}, y: {2}".format(line, x, y))
                oled.text(line, x, y)
            oled.show()
            if paging_ms < interval_ms:
                await asyncio.sleep_ms(paging_ms)


def define_screens(data):
    screens = {}
    # Set-up screens
    screen1_line1 = 'Temp: {0} C'.format(data["t"])
    screen1_line2 = 'RH: {0} %'.format(data["rh"])
    screens["screen1"] = [screen1_line1, screen1_line2]
    log.debug(screens)
    return screens


async def run(interval):
    t0 = time.ticks_ms()
    while True:
        data = {}
        led2.value(0)
        data["t"], data["rh"] = get_t_rh()
        log.debug(data)
        led2.value(1)
        await show_data(define_screens(data))

        # sleep
        iv = interval
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


def main():
    global loop
    try:
        loop = asyncio.get_event_loop()
        loop.create_task(run(interval_ms))
        loop.run_forever()
    except OSError:
        loop.stop()
        oled.poweroff()
        restart()
    except KeyboardInterrupt:
        loop.stop()
        oled.poweroff()


if __name__ == "__main__":
    log.info("Running main.py on Bonitoo WS")
    main()
