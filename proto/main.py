from nnlogger import LOGGER
from reader import Reader
from tokenizer import Tokenizer
from nnparser import Parser
import time
import nnast


if __name__ == '__main__':
    r = Reader('examples/mastermind2.nn', True)
    t = Tokenizer(r)
    p = Parser(t)
    ret = p.program()
    nnast.print_tree(ret)
    #p.tt.print()