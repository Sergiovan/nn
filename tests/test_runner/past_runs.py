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
      return from_toml(PastRuns, open(file, "r").read())

  def store(self, file: Path, run_limit: int = 100):
    as_toml = to_toml(PastRuns(self.runs[-run_limit:]))
    open(file, "w").write(as_toml)
