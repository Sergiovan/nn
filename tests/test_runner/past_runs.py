from __future__ import annotations

from .cli import Arguments
from .test_data import TestResult, ProgramOutput, TestExpectations

from datetime import datetime
from pathlib import Path

from serde import serde, field as serde_field
from serde.toml import from_toml, to_toml


@serde
class PastSingleFileTest:
  file: Path
  description: str
  result: TestResult = serde_field(serializer=TestResult.__str__, deserializer=TestResult.from_str)
  start_time: datetime
  runtime: int
  compilation_command: str
  compiler_output: ProgramOutput
  compiled_program_output: ProgramOutput
  test_expectation: TestExpectations
  error: str

  def print(self, with_error: Exception | None = None):
    import traceback

    test_files_dir = self.file
    while test_files_dir.name and test_files_dir.name != "test_files":
      test_files_dir = test_files_dir.parent

    print(f"===== \x1b[1m{self.file.relative_to(test_files_dir)}\x1b[0m =====")
    if self.description:
      print(self.description)
    print(f"Ran in \x1b[1m{(self.runtime) / 1_000_000_000:.3f}\x1b[0ms")
    print(f"Result: \x1b[1m{self.result.repr()}\x1b[0m")

    if self.result == TestResult.SKIP:
      if with_error:
        print(f"Reason: \x1b[1m{with_error.args[0]}\x1b[0m")
      else:
        print(f"Reason: \x1b[1m{self.error}\x1b[0m")
      return
    if self.result.is_pass():
      return

    print(f"Command executed: {self.compilation_command or '<NO COMMAND EXECUTED>'}")
    if self.result == TestResult.ERROR:
      if with_error:
        error_text = traceback.format_exception(with_error)
        for line in error_text:
          print(f"{line}", end="")
      else:
        print(self.error)
      if self.compiler_output.stdout:
        print("Compiler stdout:")
        print("\t" + self.compiler_output.stdout.replace("\n", "\n\t"))
      if self.compiled_program_output.stdout:
        print("Program stdout:")
        print("\t" + self.compiled_program_output.stdout.replace("\n", "\n\t"))
    else:
      if self.compiled_program_output.retcode == -(1 << 65):  # Didn't run
        print(
          f"Compiler return: Expected {self.test_expectation.stage_reached.compiler_retcode()}, got {self.compiler_output.retcode}"
        )
        print("Compiler stdout:")
        if self.compiler_output.stdout:
          print("\t" + self.compiler_output.stdout.replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectation.stdout is not None:
          print(
            f'Expected {"exactly" if self.test_expectation.stdout_exact else "to find"} "{self.test_expectation.stdout}"'
          )
      else:
        print(
          f"Program return: Expected {self.test_expectation.retcode}, got {self.compiled_program_output.retcode}"
        )
        print("Program stdout:")
        if self.compiled_program_output.stdout:
          print("\t" + self.compiled_program_output.stdout.replace("\n", "\n\t"))
        else:
          print("\t <BLANK>")
        if self.test_expectation.stdout is not None:
          print(
            f'Expected {"exactly" if self.test_expectation.stdout_exact else "to find"} "{self.test_expectation.stdout}"'
          )


@serde
class PastRun:
  start_time: datetime
  runtime: int
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
      return from_toml(PastRuns, open(file, "r").read())

  def store(self, file: Path, run_limit: int = 100):
    as_toml = to_toml(PastRuns(self.runs[-run_limit:]))
    open(file, "w").write(as_toml)
