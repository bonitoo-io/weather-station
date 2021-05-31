import ntptime

try:
    import utime as time
except:
    import time


def set_rtc(host):
    # if needed, overwrite default time server
    ntptime.host = host

    try:
        print('UTC time before sync：{0}'.format(time.gmtime()))
        # make sure to have internet connection
        ntptime.settime()
        print('UTC time after sync：{0}'.format(time.gmtime()))
    except:
        print('Error syncing time.')
