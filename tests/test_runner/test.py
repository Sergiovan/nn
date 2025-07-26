from __future__ import annotations

import asyncio
import os
import time

from datetime import datetime
from pathlib import Path
from typing import TYPE_CHECKING

from .test_parser import TestParser
from .monitored_qemu import MonitoredQemu
from .past_runs import PastSingleFileTest
from .test_data import (
  CompilerPhase,
  TestrunData,
  TestResult,
  ProgramOutput,
  TestExpectations,
  SkipTest,
)

if TYPE_CHECKING:
  from .runner import TestRunner


class SingleFileTest:
  def __init__(self, runner: TestRunner, file: Path, *, testrun_data: TestrunData):
    assert testrun_data.temp_directory is not None

    self.runner = runner
    self.file = file
    self.description: str = ""
    self.testrun_data = testrun_data

    self.output_dir = testrun_data.temp_directory / self.file.stem
    self.output_bin = self.output_dir / "out.bin"

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
      self.parse_test_data(text)
      # Run compiler on it
      self.result = TestResult.PASS  # Just for checking

      compilation_params: list[str] = [
        str(self.testrun_data.compiler_path),
        str(self.file),
        "--silent",  # Please no extraneous output
      ]
      match self.test_expectations.stage_reached:
        case CompilerPhase.LEX:
          compilation_params.append("--lex")
        case CompilerPhase.PARSE:
          compilation_params.append("--parse")
        case CompilerPhase.CODEGEN:
          compilation_params.append("--codegen")
        case CompilerPhase.RUN:
          os.makedirs(self.output_dir, exist_ok=True)
          compilation_params += ["-o", str(self.output_bin.parent / self.output_bin.stem)]

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
        try:
          proc.kill()  # Finish him
          await proc.communicate()
        except OSError:
          pass
        raise Exception(f"Test timed out after {timeout}s") from e

      # Compare result with expectation
      assert proc.returncode is not None
      self.compiler_output.retcode = proc.returncode

      self.check_compiler_expectations()

      if self.test_expectations.stage_reached == CompilerPhase.RUN and self.result.is_pass():
        await self.run_executable()
        self.check_program_expectations()

    except SkipTest as e:
      self.result = TestResult.SKIP
      self.error = e
    except Exception as e:
      self.result = TestResult.ERROR
      self.error = e
    finally:
      processing.done = True
      self.end_time = time.perf_counter_ns()

  async def run_executable(self):
    monitor = MonitoredQemu(self.output_bin, self.output_dir / "socket")
    await monitor.complete()
    self.compiled_program_output.retcode = monitor.get_result()
    self.compiled_program_output.stdout, self.compiled_program_output.stderr = monitor.get_output()

  def parse_test_data(self, lines: list[str]):
    parser = TestParser(lines, self.file)
    self.test_expectations = parser.parse()
    self.description = parser.description or ""

  def check_compiler_expectations(self):
    if self.test_expectations.stage_reached == CompilerPhase.RUN:
      if self.compiler_output.retcode != 0:
        self.result = TestResult.FAIL
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

  def check_program_expectations(self):
    if self.test_expectations.stage_reached != CompilerPhase.RUN:
      self.result = TestResult.FAIL
    elif (
      self.test_expectations.retcode
      and self.compiled_program_output.retcode != self.test_expectations.retcode
    ):
      self.result = TestResult.FAIL
    elif self.test_expectations.stdout is not None:
      if self.test_expectations.stdout_exact:
        if self.compiled_program_output.stdout != self.test_expectations.stdout:
          self.result = TestResult.FAIL
      else:
        if self.test_expectations.stdout not in self.compiled_program_output.stdout:
          self.result = TestResult.FAIL

    if self.test_expectations.xfail:
      if self.result == TestResult.FAIL:
        self.result = TestResult.XFAIL
      elif self.result == TestResult.PASS:
        self.result = TestResult.XPASS

  def print(self):
    import traceback

    print(f"===== \x1b[1m{self.file.name}\x1b[0m =====")
    if self.description:
      print(self.description)
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
      error_text = traceback.format_exception(self.error)
      for line in error_text:
        print(f"{line}", end="")
      if self.compiler_output.stdout:
        print("Compiler stdout:")
        print("\t" + self.compiler_output.stdout.replace("\n", "\n\t"))
      if self.compiled_program_output.stdout:
        print("Program stdout:")
        print("\t" + self.compiled_program_output.stdout.replace("\n", "\n\t"))
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
      self.description,
      self.result,
      self.start_date,
      self.end_time - self.start_time,
      self.compiler_output,
      self.compiled_program_output,
      self.test_expectations,
      str(self.error),
    )
