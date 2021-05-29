from math import isfinite
from micropython import const
try:
    from utime import mktime
    import ubinascii as binascii
    import urequests as requests
except:
    from time import mktime
    import binascii
    import requests


class WritePrecision(object):
    """
    allowed enum values
    """
    MS = "ms"
    S = "s"
    US = "us"
    NS = "ns"
    DELTA_NS = const(946684800000000000)


DEFAULT_WRITE_PRECISION = WritePrecision.NS

class Point(object):

    @staticmethod
    def measurement(measurement):
        """Create a new Point with specified measurement name."""
        p = Point(measurement)
        return p

    @staticmethod
    def from_dict(dictionary: dict, write_precision: WritePrecision = DEFAULT_WRITE_PRECISION):
        point = Point(dictionary['measurement'])
        if 'tags' in dictionary:
            for tag_key, tag_value in dictionary['tags'].items():
                point.tag(tag_key, tag_value)
        for field_key, field_value in dictionary['fields'].items():
            point.field(field_key, field_value)
        if 'time' in dictionary:
            point.time(dictionary['time'], write_precision=write_precision)
        return point

    def __init__(self, measurement_name):
        self._tags = {}
        self._fields = {}
        self._name = measurement_name
        self._time = None
        self._write_precision = DEFAULT_WRITE_PRECISION
        pass


    def time(self, time, write_precision=DEFAULT_WRITE_PRECISION):
        self._write_precision = write_precision
        self._time = time
        return self

    def tag(self, key, value):
        self._tags[key] = value
        return self

    def field(self, field, value):
        self._fields[field] = value
        return self

    def to_line_protocol(self):
        _measurement = self._name
        _tags = _append_tags(self._tags)
        _fields = _append_fields(self._fields)
        if not _fields:
            return ""
        _time = _append_time(self._time, self._write_precision)

        return "{0}{1}{2}{3}".format(_measurement, _tags, _fields, _time)

    @property
    def write_precision(self):
        return self._write_precision


def _append_tags(tags):
    _return = []
    for tag_key, tag_value in sorted(iter(tags.items())):

        if tag_value is None:
            continue

        tag = tag_key
        value = tag_value
        if tag != '' and value != '':
            _return.append('{0}={1}'.format(tag, value))

    if _return:
        return ',{0} '.format(','.join(_return))
    else:
        return ''


def _append_fields(fields):
    _return = []

    for field, value in sorted(iter(fields.items())):
        if value is None:
            continue

        if isinstance(value, float):
            if not isfinite(value):
                continue
            s = str(value)
            if s.endswith('.0'):
                s = s[:-2]
            _return.append('{0}={1}'.format(field, s))
        elif isinstance(value, int) and not isinstance(value, bool):
            _return.append('{0}={1}i'.format(field, str(value)))
        elif isinstance(value, bool):
            _return.append('{0}={1}'.format(field, str(value).lower()))
        elif isinstance(value, str):
            _return.append('{0}="{1}"'.format(field, value))
        else:
            raise ValueError('Type: "{0}" of field: "{1}" is not supported.'.format(type(value), field))

    return ','.join(_return)


def _append_time(time, write_precision):
    if time is None:
        return ''
    return " {0}".format(int(_convert_timestamp(time, write_precision)))


def _convert_timestamp(timestamp, precision=DEFAULT_WRITE_PRECISION):
    if isinstance(timestamp, int):
        # If nanosecond precision is needed use timestamp=utime.time_ns() + WritePrecision.DELTA_NS.
        return timestamp  # assume precision is correct if timestamp is int

    elif isinstance(timestamp, str):
        raise NotImplementedError("String is not supported timestamp format")

    elif isinstance(timestamp, tuple):
        # Micropython's utime.localtime() and utime.gmtime() returns
        # tuples with calendar date format with second precision.
        timestamp = mktime(timestamp)
        ns = (timestamp + WritePrecision.DELTA_NS)

        if precision is None or precision == WritePrecision.NS:
            return ns

        elif precision == WritePrecision.US:
            return ns / 1e3

        elif precision == WritePrecision.MS:
            return ns / 1e6

        elif precision == WritePrecision.S:
            return ns / 1e9

    else:
        raise ValueError(timestamp)


class Client(object):
    def __init__(self, endpoint, bucket, org, precision=WritePrecision.NS, user=None, password=None, token=None):
        self.tcp_socket = None
        self.endpoint = endpoint
        self.token = token
        self.org = org
        self.bucket = bucket
        self.precision = precision
        if user and password:
            self.user_and_pass = str(binascii.b2a_base64("{0}:{1}".format(user, password).encode())[:-1], 'utf-8')
        else:
            self.user_and_pass = None

    def write(self, point):
        text = None
        if isinstance(point, Point):
            lp = point.to_line_protocol()
        elif isinstance(point, str):
            lp = bytes(point, 'utf-8')
        elif isinstance(point, bytes):
            lp = point
        else:
            return ValueError("Type: {0} is not supported.".format(type(point)))
        print(lp)
        write_path = '/api/v2/write?org={0}&bucket={1}&precision={2}'.format(self.org, self.bucket, self.precision)
        headers = {'Accept': 'application/json',
                   'Content-type': 'text/plain'
                   }

        if self.user_and_pass:
            headers["Authorization"] = "Basic {0}".format(self.user_and_pass)
        elif self.token:
            headers["Authorization"] = "Token {0}".format(self.token)
        else:
            pass

        response = requests.post("{0}{1}".format(self.endpoint, write_path),
                                 data=lp,
                                 headers=headers)
        if response.status_code != 204:
            text = response.reason.decode("utf-8")
        response.close()
        return text
