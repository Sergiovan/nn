import re
from pathlib import Path
from typing import Callable


from .test_data import TestExpectations, CompilerPhase, SkipTest


class TestSpecificationException(Exception):
  pass


class TestParser:
  UNESCAPED_SEMICOLON = re.compile(r"(?<!\\);")
  UNESCAPED_COMMA = re.compile(r"(?<!\\),")
  LINE_STARTER = "//!"

  def __init__(self, lines: list[str], file: Path):
    self.lines = lines
    self.file = file

    self.stage_reached: None | CompilerPhase = None
    self.retcode: None | int = None
    self.xfail: bool = False
    self.skip: str = ""
    self.stdout: str | None = None
    self.stdout_exact: bool = False

    self.description: str | None = None

  def parse(self) -> TestExpectations:
    res = TestExpectations()

    if len(self.lines) == 0:
      return res

    has_found_expectations = False
    for line in self.lines:
      line = line.strip()
      if has_found_expectations and not line.startswith(TestParser.LINE_STARTER):
        break
      has_found_expectations = True

      calls = [x.strip() for x in re.split(TestParser.UNESCAPED_SEMICOLON, line[3:])]

      for call in calls:
        if not call:
          continue
        fn, args = self._split_fn(call)
        match fn:
          case "LEX":
            self._verify_arg_count(fn, args, 0)
            self._set_stage_reached(CompilerPhase.LEX)
            break
          case "PARSE":
            self._verify_arg_count(fn, args, 0)
            self._set_stage_reached(CompilerPhase.PARSE)
            break
          case "CODEGEN":
            self._verify_arg_count(fn, args, 0)
            self._set_stage_reached(CompilerPhase.CODEGEN)
            break
          case "RUN":
            self._verify_arg_count(fn, args, 1)
            retcode = self._verify_arg(fn, args[0], int)
            self._set_stage_reached(CompilerPhase.RUN)
            self.retcode = retcode
            break
          case "XFAIL":
            self._verify_arg_count(fn, args, 0)
            self._set_xfail()
            break
          case "SKIP":
            self._verify_arg_count(fn, args, 1)
            reason = self._verify_arg(fn, args[0], str, lambda x: len(x) > 0)
            self._set_skip(reason)
            break
          case "OUT":
            if len(args) > 0:
              text = self._verify_arg(fn, args[0], str, lambda x: len(x) > 0)
            else:
              text = ""
            self._set_stdout_exact(text)
            break
          case "OUTISH":
            self._verify_arg_count(fn, args, 1)
            text = self._verify_arg(fn, args[0], str, lambda x: len(x) > 0)
            self._set_stdoutish(text)
            break
          case "DESC":
            self._verify_arg_count(fn, args, 1)
            text = self._verify_arg(fn, args[0], str, lambda x: len(x) > 0)
            self._set_description(text)
            break
          case o:
            raise TestSpecificationException(
              f'Unknown compiler test directive "{o}" in {self.file}'
            )

    if not self.stage_reached:
      raise TestSpecificationException(f"Did not specify test goal for {self.file}")

    if self.skip:
      raise SkipTest(self.skip)

    return TestExpectations(
      stage_reached=self.stage_reached,
      retcode=self.retcode,
      stdout_exact=self.stdout_exact,
      stdout=self.stdout,
      xfail=self.xfail,
    )

  def _split_fn(self, token: str) -> tuple[str, tuple[str, ...]]:
    token = token.strip()
    if not token.endswith(")"):
      if "(" in token:
        raise TestSpecificationException(
          f"Invalid test spec call is missing closing paren in {self.file}: {token}"
        )
      return (token, ())

    if "(" not in token:
      raise TestSpecificationException(
        f"Invalid test spec call is missing an opening paren in {self.file}: {token}"
      )

    head, call = token[:-1].split("(", 1)
    ret_params = [x.strip() for x in re.split(TestParser.UNESCAPED_COMMA, call)]

    return (head, tuple(ret_params))

  def _set_stage_reached(self, new_stage: CompilerPhase):
    if self.stage_reached is not None:
      raise TestSpecificationException(
        f"Setting stage reached twice in {self.file}: was {self.stage_reached} and setting to {new_stage}"
      )
    self.stage_reached = new_stage

  def _set_xfail(self):
    if self.xfail:
      raise TestSpecificationException(f"XFAIL set twice in {self.file}")
    self.xfail = True

  def _set_skip(self, skip_reason: str):
    if self.skip:
      raise TestSpecificationException(f"SKIP set twice in {self.file}")
    self.skip = skip_reason

  def _set_stdout_exact(self, stdout: str):
    if self.stdout:
      raise TestSpecificationException(f"OUT or OUTISH set twice in {self.file}")
    self.stdout = stdout
    self.stdout_exact = True

  def _set_stdoutish(self, stdout: str):
    if self.stdout:
      raise TestSpecificationException(f"OUT or OUTISH set twice in {self.file}")
    self.stdout = stdout
    self.stdout_exact = False

  def _set_description(self, description: str):
    if self.description:
      raise TestSpecificationException(f"DESC set twice in {self.file}")
    self.description = description

  def _verify_arg_count(self, fn: str, args: tuple[str, ...], amount: int):
    if len(args) != amount:
      raise TestSpecificationException(
        f"Invalid amount of arguments for {fn} in {self.file}: Expected {amount} but got {len(args)}"
      )

  def _verify_arg[T](
    self,
    fn: str,
    arg: str,
    type: Callable[[str], T],
    validate: Callable[[T], bool] = lambda x: True,
  ) -> T:
    res = type(arg)
    if not validate(res):
      raise TestSpecificationException(f"Invalid parameter {arg} for {fn} in {self.file}")

    return res
