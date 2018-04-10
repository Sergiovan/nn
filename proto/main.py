from nnlogger import LOGGER
from reader import Reader
from tokenizer import Tokenizer
from nnparser import Parser
import time
import nnast


if __name__ == '__main__':
    r = Reader('examples/mm.nn', True)
    t = Tokenizer(r)
    p = Parser(t)
    ret = p.program()
    p.tt.print()