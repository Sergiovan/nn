from _logger import LOGGER
from reader import Reader
from tokenizer import Tokenizer
import time
import _ast

if __name__ == '__main__':
    r = Reader('examples/mastermind2.nn', True)
    t = Tokenizer(r)
    while not r.finished:
        LOGGER.info(str(t.read()))