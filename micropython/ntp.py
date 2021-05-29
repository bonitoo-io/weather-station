import ntptime

try:
    import utime as time
except:
    import time


def set_rtc():
    # if needed, overwrite default time server
    ntptime.host = "1.europe.pool.ntp.org"

    try:
        print("Local time before sync：{0}".format(time.localtime()))
        # make sure to have internet connection
        ntptime.settime()
        print("Local time after sync：{0}".format(time.localtime()))
    except:
        print("Error syncing time.")

