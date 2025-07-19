from dataclasses import dataclass
from enum import Enum, auto
from pathlib import Path

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


@dataclass
class ProgramOutput:
  retcode: int = -1
  stdout: bytes = b""
  stderr: bytes = b""


@dataclass
class TestExpectations:
  stage_reached: CompilerPhase = CompilerPhase.LEX

  retcode: int | None = None
  stdout_exact: bool = False
  stdout: str | None = None

  xfail: bool = False
