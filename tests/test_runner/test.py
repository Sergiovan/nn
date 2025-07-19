from __future__ import annotations

import asyncio
import time

from datetime import datetime
from pathlib import Path
from typing import TYPE_CHECKING

from .past_runs import PastSingleFileTest
from .test_data import CompilerPhase, TestData, TestResult, ProgramOutput, TestExpectations

if TYPE_CHECKING:
  from .runner import TestRunner


class TestSpecificationException(Exception):
  pass


class SkipTest(Exception):
  pass


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
    self.start_date: datetime = datetime.now()
    self.end_time: int = -1

  async def run(self):
    # Set up cool vis
    processing = self.runner.add_processing(str(self.file))
    try:
      self.start_time = time.perf_counter_ns()
      self.start_date = datetime.now()
      # Read file
      text = open(self.file, "r").readlines()
      self.find_expectations(text)
      # Run compiler on it
      self.result = TestResult.PASS  # Just for checking

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

      self.compilation_command = " ".join(compilation_params)
      timeout = 5

      proc = await asyncio.create_subprocess_exec(
        *compilation_params, stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.PIPE
      )
      try:
        stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout)
        self.compiler_output.stdout = stdout.decode(errors="backslash").strip()
        self.compiler_output.stderr = stderr.decode(errors="backslash").strip()
      except asyncio.TimeoutError as e:
        proc.kill()  # Finish him
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
            if self.compiler_output.stdout != self.test_expectations.stdout:
              self.result = TestResult.FAIL
          else:
            if self.test_expectations.stdout not in self.compiler_output.stdout:
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
        raise TestSpecificationException(
          f"Empty compiler test directive in {self.file}: Must be LEX, PARSE, CODEGEN or RUN"
        )

      head: str = tokens[0]
      match head:
        case "XFAIL":
          if self.test_expectations.xfail:
            raise TestSpecificationException(
              f"Invalid compiler test directive in {self.file}: XFAIL can only appear once"
            )
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
          raise SkipTest(line[line.find("SKIP") + 4 :])
        case o:
          raise TestSpecificationException(
            f'Unknown compiler test directive "{o}" in {self.file}: Must be LEX, PARSE, CODEGEN or RUN'
          )

    tokens.pop(0)

    while True:
      if len(tokens) == 0:
        return

      head = tokens[0]

      match head:
        case "XFAIL":
          if self.test_expectations.xfail:
            raise TestSpecificationException(
              f"Invalid compiler test directive in {self.file}: XFAIL can only appear once"
            )
          self.test_expectations.xfail = True
          tokens.pop(0)
        case ":":
          if self.test_expectations.retcode is not None:
            raise TestSpecificationException(
              f"Invalid compiler test directive return code in {self.file}: Only one return code is allowed"
            )
          if self.test_expectations.stage_reached != CompilerPhase.RUN:
            raise TestSpecificationException(
              f"Invalid compiler test directive return code in {self.file}: Return code only allowed for RUN phases"
            )
          if len(tokens) == 1:
            raise TestSpecificationException(
              f"Invalid compiler test directive return code in {self.file}: Must be present and be an integer between 0 and 255"
            )
          try:
            ret = int(tokens[2])
            if ret < 0 or ret > 255:
              raise TestSpecificationException(
                f"Invalid compiler test directive return code in {self.file}: Must be between 0 and 255, is {ret}"
              )
            self.test_expectations.retcode = ret
          except ValueError as e:
            raise TestSpecificationException(
              f'Invalid compiler test directive return code in {self.file}: Must be an integer, found "{tokens[3]}"'
            ) from e
          tokens.pop(0)
          tokens.pop(0)
        case "=":
          self.test_expectations.stdout = line[line.find("=") + 1 :].strip()
          self.test_expectations.stdout_exact = True
          return
        case "~":
          self.test_expectations.stdout = line[line.find("~") + 1 :].strip()
          self.test_expectations.stdout_exact = False
          return
        case _:
          raise TestSpecificationException(
            f'Unknown compiler test directive "{0}" in {self.file}: Must be ":" for return code, "=" for stdout equals or "~" for stdout contains'
          )

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

    print(f"Command executed: {self.compilation_command or '<NO COMMAND EXECUTED>'}")
    if self.result == TestResult.ERROR:
      assert self.error is not None
      print(self.error)
    else:
      if self.compiled_program_output.retcode == -1:  # Didn't run
        print(
          f"Compiler return: Expected {self.test_expectations.stage_reached.compiler_retcode()}, got {self.compiler_output.retcode}"
        )
        print("Compiler stdout:")
        if self.compiler_output.stdout:
          print("\t" + self.compiler_output.stdout.replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectations.stdout is not None:
          print(
            f'Expected {"exactly" if self.test_expectations.stdout_exact else "to find"} "{self.test_expectations.stdout}"'
          )
      else:
        print(f"Program return: Expected 0, got {self.compiled_program_output.retcode}")
        print("Program stdout:")
        if self.compiled_program_output.stdout:
          print("\t" + self.compiled_program_output.stdout.replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectations.stdout is not None:
          print(
            f'Expected {"exactly" if self.test_expectations.stdout_exact else "to find"} "{self.test_expectations.stdout}"'
          )

  def archive(self) -> PastSingleFileTest:
    return PastSingleFileTest(
      self.file,
      self.result,
      self.start_date,
      self.end_time - self.start_time,
      self.compiler_output,
      self.compiled_program_output,
      self.test_expectations,
      str(self.error),
    )
