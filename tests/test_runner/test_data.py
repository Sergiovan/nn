from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

from serde import serde, field as serde_field

COLOR_PASS = "\x1b[38;2;34;181;115m"
COLOR_SKIP = "\x1b[38;2;255;204;0m"
COLOR_FAIL = "\x1b[38;2;204;51;0m"

LOADING_CHARS = ["⠇", "⠋", "⠙", "⠸", "⢰", "⣠", "⣄", "⡆"]


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
    return self.done or self.text == ""

  def print(self):
    if self.text:
      print(self.text, end="")
    else:
      print("Waiting", end="")
      return

    if self.done:
      print(": Done", end="")
    else:
      print(f": {LOADING_CHARS[self.state]}", end="")
    print("\x1b[0K", end="", flush=True)

  def advance(self):
    self.state = (self.state + 1) & 7


class TestResult(Enum):
  PASS = auto()
  FAIL = auto()
  ERROR = auto()
  SKIP = auto()
  XFAIL = auto()
  XPASS = auto()
  UNKNOWN = auto()

  def small_repr(self, color: bool = True) -> str:
    match self:
      case TestResult.PASS:
        return f"{COLOR_PASS}P" if color else "P"
      case TestResult.FAIL:
        return f"{COLOR_FAIL}F" if color else "F"
      case TestResult.ERROR:
        return f"{COLOR_FAIL}E" if color else "E"
      case TestResult.SKIP:
        return f"{COLOR_SKIP}>" if color else ">"
      case TestResult.XFAIL:
        return f"{COLOR_PASS}F" if color else "F"
      case TestResult.XPASS:
        return f"{COLOR_FAIL}P" if color else "P"
      case TestResult.UNKNOWN:
        return f"{COLOR_SKIP}?" if color else "?"

  def repr(self, color: bool = True) -> str:
    match self:
      case TestResult.PASS:
        return f"{COLOR_PASS}PASS" if color else "PASS"
      case TestResult.FAIL:
        return f"{COLOR_FAIL}FAIL" if color else "FAIL"
      case TestResult.ERROR:
        return f"{COLOR_FAIL}ERROR" if color else "ERROR"
      case TestResult.SKIP:
        return f"{COLOR_SKIP}SKIP" if color else "SKIP"
      case TestResult.XFAIL:
        return f"{COLOR_PASS}XFAIL" if color else "XFAIL"
      case TestResult.XPASS:
        return f"{COLOR_FAIL}XPASS" if color else "XPASS"
      case TestResult.UNKNOWN:
        return f"{COLOR_SKIP}UNKNOWN" if color else "UNKNOWN"

  def is_pass(self) -> bool:
    return self in (TestResult.PASS, TestResult.XFAIL)

  def __str__(self) -> str:
    return self.name

  @staticmethod
  def from_str(string: str) -> TestResult:
    VALUES = {str(x): x for x in TestResult}
    try:
      return VALUES[string]
    except KeyError as e:
      raise ValueError(f"Invalid value for Test Result: {string}") from e


class CompilerPhase(Enum):
  LEX = auto()
  PARSE = auto()
  CODEGEN = auto()
  RUN = auto()

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

  def __str__(self) -> str:
    return self.name

  @staticmethod
  def from_str(string: str) -> CompilerPhase:
    VALUES = {str(x): x for x in CompilerPhase}
    try:
      return VALUES[string]
    except KeyError as e:
      raise ValueError(f"Invalid value for Compiler Phase: {string}") from e


@dataclass
class ProgramOutput:
  retcode: int = -1
  stdout: str = ""
  stderr: str = ""


@serde
class TestExpectations:
  stage_reached: CompilerPhase = serde_field(
    default=CompilerPhase.LEX, serializer=CompilerPhase.__str__, deserializer=CompilerPhase.from_str
  )

  # Only having these sentinels because python's toml capabilities
  # are quite pathetic and they constantly trip on Nones
  retcode: int | None = serde_field(default=None, skip_if_default=True)
  stdout_exact: bool = False
  stdout: str | None = serde_field(default=None, skip_if_default=True)

  xfail: bool = False
