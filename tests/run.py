from __future__ import annotations

import asyncio
from dataclasses import dataclass, field
from datetime import datetime
import enum
from enum import auto
import glob
import termios
import time
import os
import sys

from pathlib import Path

from serde import serde
from serde.toml import from_toml, to_toml

TEST_DIR = Path(__file__).resolve().parent
THIS_DIR = Path(os.getcwd()).resolve()

COLOR_PASS = '\x1b[38;2;34;181;115m'
COLOR_SKIP = '\x1b[38;2;255;204;0m'
COLOR_FAIL = '\x1b[38;2;204;51;0m'

LOADING_CHARS = ["⠇", "⠋", "⠙", "⠸", "⢰", "⣠", "⣄", "⡆"]

def set_echo(enabled: bool) -> None:
  stdin = sys.stdin.fileno()
  stdin_attr = termios.tcgetattr(stdin)
  if enabled:
    stdin_attr[3] |= termios.ECHO
  else:
    stdin_attr[3] &= ~termios.ECHO

  termios.tcsetattr(stdin, termios.TCSANOW, stdin_attr)

class TestSpecificationException(Exception):
  pass

class SkipTest(Exception):
  pass

class RerunType(enum.Enum):
  NO_RERUN = auto()
  ALL = auto()
  FAILED = auto()

  @staticmethod
  def from_string(string: str) -> RerunType:
    VALUES = {str(x): x for x in RerunType}
    try:
      return VALUES[string]
    except KeyError as e:
      raise ValueError(f"Invalid value for Rerun Type: {string}") from e

  def __str__(self) -> str:
    return str(self.name)

class TestResult(enum.Enum):
  PASS = enum.auto()
  FAIL = enum.auto()
  ERROR = enum.auto()
  SKIP = enum.auto()
  XFAIL = enum.auto()
  XPASS = enum.auto()
  UNKNOWN = enum.auto()

  def small_repr(self, color: bool = True) -> str:
    match self: # TODO This should be a method of the enum
      case TestResult.PASS:
        return f'{COLOR_PASS}P' if color else 'P'
      case TestResult.FAIL:
        return f'{COLOR_FAIL}F' if color else 'F'
      case TestResult.ERROR:
        return f'{COLOR_FAIL}E' if color else 'E'
      case TestResult.SKIP:
        return f'{COLOR_SKIP}>' if color else '>'
      case TestResult.XFAIL:
        return f'{COLOR_PASS}F' if color else 'F'
      case TestResult.XPASS:
        return f'{COLOR_FAIL}P' if color else 'P'
      case TestResult.UNKNOWN:
        return f'{COLOR_SKIP}?' if color else '?'

  def repr(self, color: bool = True) -> str:
    match self: # TODO This should be a method of the enum
      case TestResult.PASS:
        return f'{COLOR_PASS}PASS' if color else 'PASS'
      case TestResult.FAIL:
        return f'{COLOR_FAIL}FAIL' if color else 'FAIL'
      case TestResult.ERROR:
        return f'{COLOR_FAIL}ERROR' if color else 'ERROR'
      case TestResult.SKIP:
        return f'{COLOR_SKIP}SKIP' if color else 'SKIP'
      case TestResult.XFAIL:
        return f'{COLOR_PASS}XFAIL' if color else 'XFAIL'
      case TestResult.XPASS:
        return f'{COLOR_FAIL}XPASS' if color else 'XPASS'
      case TestResult.UNKNOWN:
        return f'{COLOR_SKIP}UNKNOWN' if color else 'UNKNOWN'

  def is_pass(self) -> bool:
    return self in (TestResult.PASS, TestResult.XFAIL)

class CompilerPhase(enum.Enum):
  LEX = enum.auto()
  PARSE = enum.auto()
  CODEGEN = enum.auto()
  RUN = enum.auto()

  def compiler_retcode(self) -> int:
    match self:
      case CompilerPhase.LEX:
        return 1
      case CompilerPhase.PARSE:
        return 2
      case CompilerPhase.CODEGEN:
        return 3
      case CompilerPhase.RUN:
        return 0

@dataclass
class ProgramOutput:
  retcode: int = -1
  stdout: bytes = b""
  stderr: bytes = b""

@dataclass
class TestData:
  compiler_path: Path

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

@dataclass
class TestExpectations:
  stage_reached: CompilerPhase = CompilerPhase.LEX

  retcode: int | None = None
  stdout_exact: bool = False
  stdout: str | None = None

  xfail: bool = False

def compiler_path_factory() -> Path:
  return (TEST_DIR / Path('..') / 'output' / 'Debug' / 'nn').resolve().relative_to(THIS_DIR)

def past_runs_toml_path_factory() -> Path:
  return (TEST_DIR / 'past_runs.toml').resolve().relative_to(THIS_DIR)

@serde
class Arguments:
  show_passes: bool = False
  test_filter: list[str] = field(default_factory=list[str])
  test_files: list[Path] = field(default_factory=list[Path])
  compiler_path: Path = field(default_factory=compiler_path_factory)
  past_runs_toml_path: Path = field(default_factory=past_runs_toml_path_factory)
  past_runs_limit: int = 100
  rerun_previous: RerunType = RerunType.NO_RERUN 

  @staticmethod
  def parse_args() -> Arguments:
    import argparse
    p = argparse.ArgumentParser(
      prog="NN Compiler test runner", 
      usage=Path(__file__).name, 
      description="Test runner for the nn compiler",
      formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    def path_from_this_dir(arg: str) -> Path:
      p = Path(arg).resolve().relative_to(THIS_DIR)
      return p

    # Choose what to run
    p.add_argument("test_filter", nargs="*", type=str, help="Filters for tests to run. All tests that include any of the text in any of the filters in their path will be run")
    # Show tests that were successful in the output
    p.add_argument("--show-passes", dest="show_passes", action="store_true", default=False, help="Show passed tests in addition to failures")
    # Test one or more files directly instead of globbing
    p.add_argument("--test-file", dest="test_files", type=path_from_this_dir, action="append", help="Run test on specific file(s) explicitly")
    # Path to the compiler to use
    p.add_argument("--compiler", dest="compiler_path", type=path_from_this_dir, default=compiler_path_factory(), help="Path to compiler to use")
    # Where to store data about previous runs, and how many to keep
    p.add_argument("--past-runs", dest="past_runs_toml_path", type=path_from_this_dir, default=past_runs_toml_path_factory(), help="Path to past runs toml")
    p.add_argument("--past-runs-limit", dest="past_runs_limit", type=int, default=100, help="Amount of past runs to keep in toml file")
    # If we should rerun the last set of tests
    p.add_argument("--rerun", dest="rerun_previous", type=RerunType.from_string, choices=RerunType, default=RerunType.NO_RERUN, help="If previous run should be done")

    args = p.parse_args(namespace=Arguments())

    for test_file in args.test_files:
      if not test_file.exists():
        p.error(f"Test file \"{test_file}\" does not exist")

    if args.compiler_path and not args.compiler_path.exists():
      p.error(f"Compiler \"{args.compiler_path}\" does not exist")

    return args

@serde
class PastSingleFileTest:
  file: Path
  result: TestResult
  start_time: datetime
  end_time: datetime
  compiler_output: ProgramOutput
  compiled_program_output: ProgramOutput
  test_expectation: TestExpectations
  error: str

@serde
class PastRun:
  start_time: datetime
  run_args: Arguments
  file_tests: list[PastSingleFileTest] 

@serde
class PastRuns:
  runs: list[PastRun]

  @staticmethod
  def load(file: Path) -> PastRuns:
    if not file.exists():
      return PastRuns([])
    else:
      return from_toml(PastRuns, open(file, 'r').read())
    
  def store(self, file: Path, run_limit: int = 100):
    as_toml = to_toml(PastRuns(self.runs[:run_limit]))
    open(file, 'w').write(as_toml)

class SingleFileTest:
  def __init__(self, runner: TestRunner, file: Path, *, test_data: TestData):
    self.runner = runner
    self.file = file
    self.test_data = test_data

    self.result = TestResult.UNKNOWN

    self.compilation_command: str = ""

    self.compiler_output = ProgramOutput()
    self.compiled_program_output = ProgramOutput()
    self.test_expectations = TestExpectations()
    
    self.error: Exception | None = None

    self.start_time: int = -1
    self.end_time: int = -1

  async def run(self):
    # Set up cool vis
    processing = self.runner.add_processing(str(self.file))
    try:
      self.start_time = time.perf_counter_ns()
      # Read file
      text = open(self.file, 'r').readlines()
      self.find_expectations(text)
      # Run compiler on it
      self.result = TestResult.PASS # Just for checking

      compilation_params: list[str] = [str(self.test_data.compiler_path), str(self.file)]
      match self.test_expectations.stage_reached:
        case CompilerPhase.LEX:
          compilation_params.append("--lex")
        case CompilerPhase.PARSE:
          compilation_params.append("--parse")
        case CompilerPhase.CODEGEN:
          compilation_params.append("--codegen")
        case CompilerPhase.RUN:
          pass

      self.compilation_command = ' '.join(compilation_params)
      timeout = 5

      proc = await asyncio.create_subprocess_exec(*compilation_params, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE)
      try:
        self.compiler_output.stdout, self.compiler_output.stderr = await asyncio.wait_for(proc.communicate(), timeout)
      except asyncio.TimeoutError as e:
        proc.kill() # Finish him
        await proc.communicate()
        raise Exception(f"Test timed out after {timeout}s") from e

      # Compare result with expectation
      assert proc.returncode is not None
      self.compiler_output.retcode = proc.returncode

      if self.test_expectations.stage_reached == CompilerPhase.RUN:
        if self.compiler_output.retcode != 0:
          self.result = TestResult.FAIL
        # TODO Run the executable as well
      else:
        if self.compiler_output.retcode != self.test_expectations.stage_reached.compiler_retcode():
          self.result = TestResult.FAIL
        elif self.test_expectations.stdout is not None:
          if self.test_expectations.stdout_exact:
            if self.compiler_output.stdout.decode(errors="backslash").strip() != self.test_expectations.stdout:
              self.result = TestResult.FAIL
          else:
            if self.test_expectations.stdout not in self.compiler_output.stdout.decode(errors="backslash").strip():
              self.result = TestResult.FAIL

      if self.test_expectations.xfail:
        if self.result == TestResult.FAIL:
          self.result = TestResult.XFAIL
        elif self.result == TestResult.PASS:
          self.result = TestResult.XPASS
        
    except SkipTest as e:
      self.result = TestResult.SKIP
      self.error = e
    except Exception as e:
      self.result = TestResult.ERROR
      self.error = e
    finally:
      processing.done = True
      self.end_time = time.perf_counter_ns()

  def find_expectations(self, lines: list[str]):
    if len(lines) == 0:
      return
    line = lines[0].strip()
    if not line.startswith("//!"):
      return
    
    tokens = line[3:].split()

    while True:
      if len(tokens) == 0:
        raise TestSpecificationException(f"Empty compiler test directive in {self.file}: Must be LEX, PARSE, CODEGEN or RUN")

      head: str = tokens[0]
      match head:
        case "XFAIL":
          if self.test_expectations.xfail:
            raise TestSpecificationException(f"Invalid compiler test directive in {self.file}: XFAIL can only appear once")
          self.test_expectations.xfail = True
          tokens.pop(0)
        case "LEX":
          self.test_expectations.stage_reached = CompilerPhase.LEX
          break
        case "PARSE":
          self.test_expectations.stage_reached = CompilerPhase.PARSE
          break
        case "CODEGEN":
          self.test_expectations.stage_reached = CompilerPhase.CODEGEN
          break
        case "RUN":
          self.test_expectations.stage_reached = CompilerPhase.RUN
          self.test_expectations.retcode = 0
          break
        case "SKIP":
          raise SkipTest(line[line.find("SKIP")+4:])
        case o:
          raise TestSpecificationException(f"Unknown compiler test directive \"{o}\" in {self.file}: Must be LEX, PARSE, CODEGEN or RUN")

    tokens.pop(0)

    while True:
      if len(tokens) == 0:
        return
      
      head = tokens[0]

      match head:
        case "XFAIL":
          if self.test_expectations.xfail:
            raise TestSpecificationException(f"Invalid compiler test directive in {self.file}: XFAIL can only appear once")
          self.test_expectations.xfail = True
          tokens.pop(0)
        case ":":
          if self.test_expectations.retcode is not None:
            raise TestSpecificationException(f"Invalid compiler test directive return code in {self.file}: Only one return code is allowed")
          if self.test_expectations.stage_reached != CompilerPhase.RUN:
            raise TestSpecificationException(f"Invalid compiler test directive return code in {self.file}: Return code only allowed for RUN phases")
          if len(tokens) == 1:
            raise TestSpecificationException(f"Invalid compiler test directive return code in {self.file}: Must be present and be an integer between 0 and 255")
          try:
            ret = int(tokens[2])
            if ret < 0 or ret > 255:
              raise TestSpecificationException(f"Invalid compiler test directive return code in {self.file}: Must be between 0 and 255, is {ret}")
            self.test_expectations.retcode = ret
          except ValueError as e:
            raise TestSpecificationException(f"Invalid compiler test directive return code in {self.file}: Must be an integer, found \"{tokens[3]}\"") from e
          tokens.pop(0)
          tokens.pop(0)
        case "=":
          self.test_expectations.stdout = line[line.find("=")+1:].strip()
          self.test_expectations.stdout_exact = True
          return 
        case "~":
          self.test_expectations.stdout = line[line.find("~")+1:].strip()
          self.test_expectations.stdout_exact = False
          return
        case _:
          raise TestSpecificationException(f"Unknown compiler test directive \"{0}\" in {self.file}: Must be \":\" for return code, \"=\" for stdout equals or \"~\" for stdout contains")

  def print(self):
    print(f"===== \x1b[1m{self.file.name}\x1b[0m =====")
    print(f"Ran in \x1b[1m{(self.end_time - self.start_time) / 1_000_000_000:.3f}\x1b[0ms")
    print(f"Result: \x1b[1m{self.result.repr()}\x1b[0m")

    if self.result == TestResult.SKIP:
      assert self.error is not None 
      print(f"Reason: \x1b[1m{self.error.args[0]}\x1b[0m")
      return
    if self.result.is_pass():
      return

    print(f"Command executed: {self.compilation_command or "<NO COMMAND EXECUTED>"}")
    if self.result == TestResult.ERROR:
      assert self.error is not None
      print(self.error)
    else:
      
      if self.compiled_program_output.retcode == -1: # Didn't run
        print(f"Compiler return: Expected {self.test_expectations.stage_reached.compiler_retcode()}, got {self.compiler_output.retcode}")
        print("Compiler stdout:")
        if self.compiler_output.stdout:
          print("\t" + self.compiler_output.stdout.decode(errors="backslash").replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectations.stdout is not None:
          print(f"Expected {"exactly" if self.test_expectations.stdout_exact else "to find"} \"{self.test_expectations.stdout}\"")
      else:
        print(f"Program return: Expected 0, got {self.compiled_program_output.retcode}")
        print("Program stdout:")
        if self.compiled_program_output.stdout:
          print("\t" + self.compiled_program_output.stdout.decode(errors="backslash").replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectations.stdout is not None:
          print(f"Expected {"exactly" if self.test_expectations.stdout_exact else "to find"} \"{self.test_expectations.stdout}\"")
    
class TestRunner:
  def __init__(self):
    self.arguments = Arguments.parse_args()
    self.past_runs = PastRuns.load(self.arguments.past_runs_toml_path)

    match self.arguments.rerun_previous:
      case RerunType.NO_RERUN:
        self.test_files = self.arguments.test_files or [Path(x) for x in glob.glob(str((TEST_DIR / "test_files" / "**" / "*.nn").relative_to(THIS_DIR)))]
      case RerunType.FAILED:
        self.test_files = [x.file for x in self.past_runs.runs[-1].file_tests if not x.result.is_pass()]
      case RerunType.ALL:
        self.test_files = [x.file for x in self.past_runs.runs[-1].file_tests]

    self.test_data = TestData(self.arguments.compiler_path)
    
    self.task_limit = 8
    self.column_limit = 16

    self.result_line = 0
    self.result_column = 0

    self.finished_tests = 0
    self.test_time = 0

    self.processing_lines = [ProcessingData() for _ in range(self.task_limit)]

  def run(self) -> int:
    start_time = time.perf_counter_ns()
    res = asyncio.run(self._run())
    end_time = time.perf_counter_ns()

    print(f"Finished in {(end_time - start_time) / 1_000_000_000:.3f}s, real test time was {self.test_time / 1_000_000_000:.3f}s")

    return res

  def add_processing(self, text: str) -> ProcessingData:
    for processing in self.processing_lines:
      if processing.is_available:
        processing.text = text
        processing.state = 0
        processing.done = False
        return processing
    
    raise Exception("add_processing called with no slots free")

  async def _run(self) -> int:
    # TODO Filter tests
    if not self.arguments.test_filter:
      filtered_tests = self.test_files
    else:
      filtered_tests_set: set[Path] = set[Path]()
      for filter in self.arguments.test_filter:
        # Terrible time complexity over here
        for test_file in self.test_files:
          if filter in str(test_file):
            filtered_tests_set.add(test_file)
      filtered_tests: list[Path] = list(filtered_tests_set)
    if len(filtered_tests) == 0:
      print("Nothing to do")
      return 0

    tests = asyncio.Queue[SingleFileTest]()
    file_tests = [SingleFileTest(self, test, test_data=self.test_data) for test in filtered_tests]
    for test in file_tests:
      tests.put_nowait(test)

    tasks = [asyncio.create_task(self._single_runner(tests)) for _ in range(self.task_limit)]
    try:
      set_echo(False)
      self._setup_screen()
      await asyncio.gather(self._screen_updater(len(filtered_tests)), *tasks)
      self._update_screen()
      print(flush=True)
    finally:
      set_echo(True)

    file_tests.sort(key=lambda t: not t.result.is_pass())
    if not self.arguments.show_passes:
      file_tests = [test for test in file_tests if not test.result.is_pass()]
    
    for test in file_tests:
      test.print()
      self.test_time += (test.end_time - test.start_time) 

    return 0 if all(test.result.is_pass() for test in file_tests) else 1

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
    print(result.small_repr(), end='')
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