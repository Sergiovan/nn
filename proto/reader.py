from nnlogger import LOGGER
import os

class Reader:
    def __init__(self, str, is_file=False):
        self.finished = False
        self.index = 0
        self.column = 1
        self.line = 1
        self.filename = str if is_file else "<user input>"
        if is_file:
            try:
                with open(str) as f:
                    self.content = f.read()
            except IOError as e:
                LOGGER.error("Error reading file ", str, ": ", e, e.__traceback__)
                exit(1)
        else:
            self.content = str


    def next(self):
        if self.finished:
            return ''
        if self.index >= len(self.content):
            self.finished = True
            return ''
        c = self.content[self.index]
        self.index += 1
        self.column += 1
        if c == '\n':
            self.column = 1
            self.line += 1
        return c

    def peek(self):
        if self.index >= len(self.content):
            return ''
        return self.content[self.index]

    def get_line(self):
        line_start = self.index - (self.column + 1)
        x = 0
        while self.index + x < len(self.content) and self.content[x + self.index] != '\n':
            x += 1
        line = self.content[line_start:self.index + x]
        return line

    def get_pretty_pos(self):
        return "{}:{}:{}".format(self.filename, self.line, self.column)