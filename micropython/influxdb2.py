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
    MS = 'ms'
    S = 's'
    US = 'us'
    NS = 'ns'


DELTA_S = const(946684800)
DEFAULT_WRITE_PRECISION = WritePrecision.S


class Point(object):

    @staticmethod
    def measurement(measurement):
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
            return ''
        _time = _append_time(self._time, self._write_precision)

        return '{0}{1}{2}{3}'.format(_measurement, _tags, _fields, _time)

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
    return ' {0}'.format(int(_convert_timestamp(time, write_precision)))


def _convert_timestamp(timestamp, precision=DEFAULT_WRITE_PRECISION):
    if isinstance(timestamp, int):
        if precision is None or precision == WritePrecision.S:
            return timestamp + DELTA_S

        elif precision == WritePrecision.MS:
            return timestamp + DELTA_S * const(1000)

        elif precision == WritePrecision.US:
            return timestamp + DELTA_S * const(1000000)

        # If nanosecond precision is needed use timestamp=utime.time_ns().
        elif precision == WritePrecision.NS:
            return timestamp + DELTA_S * const(1000000000)

    elif isinstance(timestamp, str):
        raise NotImplementedError('String timestamp format is not supported')

    else:
        raise ValueError(timestamp)


class Client(object):
    def __init__(self, endpoint, bucket, org, user=None, password=None, token=None):
        self.endpoint = endpoint
        self.token = token
        self.org = org
        self.bucket = bucket
        self.precision = WritePrecision.S
        if (user and len(user) > 0) and (password and len(password) > 0):
            self.user_and_pass = str(binascii.b2a_base64('{0}:{1}'.format(user, password).encode())[:-1], 'utf-8')
        else:
            self.user_and_pass = None

    def _convert_point(self, point):
        if isinstance(point, Point):
            lp = point.to_line_protocol()
            self.precision = point.write_precision
        elif isinstance(point, str):
            lp = bytes(point, 'utf-8')
        elif isinstance(point, bytes):
            lp = point
        else:
            raise ValueError('Type: {0} is not supported'.format(type(point)))
        return lp

    @staticmethod
    def _post(url, headers, lp):
        response = requests.post(url, data=lp, headers=headers)
        return response

    @property
    def url(self):
        write_path = '/api/v2/write?org={0}&bucket={1}&precision={2}'.format(self.org, self.bucket, self.precision)
        return '{0}{1}'.format(self.endpoint, write_path)

    @property
    def headers(self):
        headers = {
            'Accept': 'application/json',
            'Content-type': 'text/plain'
            }

        if self.user_and_pass and not self.token:
            headers['Authorization'] = 'Basic {0}'.format(self.user_and_pass)
        elif self.token and not self.user_and_pass:
            headers['Authorization'] = 'Token {0}'.format(self.token)
        else:
            print('Use Token OR User/Password authentication')
            raise ValueError('Use Token OR User/Password')
        return headers

    def write(self, point):
        text = None
        data = self._convert_point(point)
        resp = Client._post(self.url, self.headers, data)
        if resp.status_code != 204:
            text = resp.reason.decode('utf-8')
        resp.close()
        return text

