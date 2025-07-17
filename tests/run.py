from __future__ import annotations

import asyncio
from dataclasses import dataclass
import enum
import glob
import time
import termios
import sys

from pathlib import Path

def set_echo(enabled: bool) -> None:
  stdin = sys.stdin.fileno()
  stdin_attr = termios.tcgetattr(stdin)
  if enabled:
    stdin_attr[3] |= termios.ECHO
  else:
    stdin_attr[3] &= ~termios.ECHO

  termios.tcsetattr(stdin, termios.TCSANOW, stdin_attr)

THIS_DIR = Path(__file__).absolute().parent

@dataclass
class TestData:
  compiler_path: Path

LOADING_CHARS = ["⠇", "⠋", "⠙", "⠸", "⢰", "⣠", "⣄", "⡆"]



@dataclass
class ProcessingData:
  text: str = ""
  state: int = 0
  done: bool = False

  @property
  def is_available(self) -> bool:
    return self.done == True or self.text == ""
  
  def print(self):
    if self.text:
      print(self.text, end='')
    else:
      print("Waiting", end='')
      return
    
    if self.done:
      print(": Done", end='')
    else:
      print(f": {LOADING_CHARS[self.state]}", end='')
    print("\x1b[0K", end='', flush=True)
    
  def advance(self):
    self.state = (self.state + 1) & 7

class TestResult(enum.Enum):
  PASS = enum.auto()
  FAIL = enum.auto()
  ERROR = enum.auto()
  SKIP = enum.auto()
  XFAIL = enum.auto()
  XPASS = enum.auto()
  UNKNOWN = enum.auto()

class SingleFileTest:
  def __init__(self, runner: TestRunner, file: str, *, test_data: TestData):
    self.runner = runner
    self.file = file
    self.test_data = test_data

    self.result = TestResult.UNKNOWN

    self.compiler_retcode: int = -1
    self.compiler_stdout: bytes = b""
    self.compiler_stderr: bytes = b""
    
    self.program_retcode: int = -1
    self.program_stdout: bytes = b""
    self.compiler_stderr: bytes = b""
    
    self.error: Exception | None = None

    self.start_time: int = -1
    self.end_time: int = -1

  async def run(self):
    import random
    processing = self.runner.add_processing(self.file)
    try:
      self.start_time = time.perf_counter_ns()
      await asyncio.sleep(random.random() * 2 + 1)
      self.result = random.choice(list(TestResult))
      # # Set up cool vis
      # processing = self.runner.add_processing(self.file)
      # # Read file
      # text = open(self.file, 'r').readlines()
      # # TODO Parse what should actually happen
      # _ = text
      # # Run compiler on it
      # proc = await asyncio.create_subprocess_exec(self.test_data.compiler_path, self.file, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
      # self.stdout, self.stderr = await proc.communicate()
      # # Compare result with expectation
      # ret = proc.returncode
      # assert ret is not None
      # self.retcode = ret

      # self.result = TestResult.PASS # Just for checking
    except Exception as e:
      self.result = TestResult.ERROR
      self.error = e
    finally:
      processing.done = True
      self.end_time = time.perf_counter_ns()

  def print(self):
    print(f"===== {self.file} =====")
    print(f"Result: {self.result}") # TODO This should be a method of the enum
    print(f"Ran in {(self.end_time - self.start_time) / 1_000_000_000}s")
    print("Command executed: <TODO>")
    print(f"Compiler return: Expected 0, got {self.compiler_retcode}")
    print("Compiler stdout:")
    print("\t" + self.compiler_stdout.decode(errors="backslash").replace("\n", "\n\t"))
    print("Compiler stderr:")
    print("\t" + self.compiler_stderr.decode(errors="backslash").replace("\n", "\n\t"))
    print(f"Program return: Expected 0, got {self.program_retcode}")
    print("Program stdout:")
    print("\t" + self.compiler_stdout.decode(errors="backslash").replace("\n", "\n\t"))
    print("Program stderr:")
    print("\t" + self.compiler_stderr.decode(errors="backslash").replace("\n", "\n\t"))
    
class TestRunner:
  def __init__(self):
    self.test_files = [str(x) for x in range(100)] # glob.glob(str(THIS_DIR / "test_files"))
    self.test_data = TestData(THIS_DIR / '..' / 'output' / 'Debug' / 'nn')
    
    self.task_limit = 8
    self.column_limit = 16

    self.result_line = 0
    self.result_column = 0

    self.finished_tests = 0

    self.processing_lines = [ProcessingData() for _ in range(self.task_limit)]

  def run(self) -> int:
    return asyncio.run(self._run())

  def add_processing(self, text: str) -> ProcessingData:
    for processing in self.processing_lines:
      if processing.is_available:
        processing.text = text
        processing.state = 0
        processing.done = False
        return processing
    
    raise Exception("add_processing called with no slots free")

  async def _run(self) -> int:
    tests = asyncio.Queue[SingleFileTest]()
    file_tests = [SingleFileTest(self, test, test_data=self.test_data) for test in self.test_files]
    for test in file_tests:
      tests.put_nowait(test)

    tasks = [asyncio.create_task(self._single_runner(tests)) for _ in range(self.task_limit)]
    try:
      set_echo(False)
      self._setup_screen()
      await asyncio.gather(self._screen_updater(len(self.test_files)), *tasks)
      self._update_screen()
    finally:
      set_echo(True)

    for test in file_tests:
      test.print()

    return 0

  def _finish_test(self):
    self.finished_tests += 1

  async def _single_runner(self, task_queue: asyncio.Queue[SingleFileTest]):
    while True:
      try:
        test = task_queue.get_nowait()
        await test.run()
        self._finish_test()
        self._print_result(test.result)
      except asyncio.QueueEmpty:
        break

  async def _screen_updater(self, test_amount: int):
    while test_amount > self.finished_tests:
      self._update_screen()

      await asyncio.sleep(0.1)

  def _setup_screen(self):
    print("=" * self.column_limit)
    for processing in self.processing_lines:
      processing.print()
      print() # Newline

    print("=" * self.column_limit, flush=True) # And another newline

  def _update_screen(self):
    print(f"\x1b[{ 2 + self.result_line + len(self.processing_lines) - 1}F", end='', flush=True)
    for processing in self.processing_lines:
      if not processing.done:
        processing.advance()
      processing.print()
      print("\x1b[1E", end='')
    print(f"\x1b[{2 + self.result_line}E", end='', flush=True)
    if self.result_column:
      print(f"\x1b[{self.result_column}C", end='', flush=True)

  def _print_result(self, result: TestResult):
    match result: # TODO This should be a method of the enum
      case TestResult.PASS:
        print('\x1b[38;2;34;181;115mP', end='')
      case TestResult.FAIL:
        print('\x1b[38;2;204;51;0mF', end='')
      case TestResult.ERROR:
        print('\x1b[38;2;204;51;0mE', end='')
      case TestResult.SKIP:
        print('\x1b[38;2;255;204;0m>', end='')
      case TestResult.XFAIL:
        print('\x1b[38;2;34;181;115mF', end='')
      case TestResult.XPASS:
        print('\x1b[38;2;204;51;0mP', end='')
      case TestResult.UNKNOWN:
        print('\x1b[38;2;255;204;0m?', end='')
    print('\x1b[0m', end='', flush=True)
    self.result_column += 1
    if self.result_column == self.column_limit:
      self.result_column = 0
      self.result_line += 1
      print() # Newline

  def _read_args(self):
    pass


if __name__ == "__main__":
  exit(TestRunner().run())