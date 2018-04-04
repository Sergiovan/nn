import datetime

class bcolors:
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    ERROR = '\033[91m'
    CLEAR = '\033[0m'

def format_timedelta(timedelta: datetime.timedelta):
    d = timedelta.days
    s = timedelta.seconds
    us = timedelta.microseconds
    h = s // 3600 + d * 24
    s -= h * 3600
    m = s // 60
    s -= m * 60
    return "{:0>4}:{:0>2}:{:0>2}.{:0>6}".format(h, m, s, us)

class Logger:
    def __init__(self):
        self.start = datetime.datetime.now()

    def pretty_time(self, time=None):
        if time is None:
            time = datetime.datetime.now()
        d = time - self.start
        return format_timedelta(d)

    def info(self, *args):
        print(self.pretty_time(), " - ", bcolors.BLUE, "[INFO ] ", *args, bcolors.CLEAR, sep='')

    def warn(self, *args):
        print(self.pretty_time(), " - ", bcolors.WARNING, "[WARN ] ", *args, bcolors.CLEAR, sep='')

    def error(self, *args):
        print(self.pretty_time(), " - ", bcolors.ERROR, "[ERROR] ", *args, bcolors.CLEAR, sep='')

    def debug(self, *args):
        print(self.pretty_time(), " - [DEBUG] ", *args, sep='')


LOGGER = Logger()