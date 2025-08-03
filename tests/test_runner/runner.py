import asyncio
import tempfile
import glob
import os
import shutil
from typing import cast
import serde
import sys
import termios
import time

from datetime import datetime
from pathlib import Path

from . import TEST_DIR, THIS_DIR

from .cli import Arguments, RerunType
from .past_runs import PastRuns, PastRun, PastSingleFileTest
from .test import SingleFileTest
from .test_data import (
  COLOR_PASS,
  COLOR_SKIP,
  COLOR_FAIL,
  COLOR_END,
  TestrunData,
  TestResult,
  ProcessingData,
)


def set_echo(enabled: bool):
  stdin = sys.stdin.fileno()
  stdin_attr = termios.tcgetattr(stdin)
  if enabled:
    stdin_attr[3] |= termios.ECHO
  else:
    stdin_attr[3] &= ~termios.ECHO

  termios.tcsetattr(stdin, termios.TCSANOW, stdin_attr)


class TestRunner:
  def __init__(self):
    self.arguments = Arguments.parse_args()
    try:
      self.past_runs = PastRuns.load(self.arguments.past_runs_toml_path)
    except serde.compat.SerdeError as e:
      raise Exception(
        f"Test format has changed. Delete or rename your past runs from {self.arguments.past_runs_toml_path}"
      ) from e

    match self.arguments.rerun_previous:
      case RerunType.NO_RERUN:
        self.test_files = self.arguments.test_files or [
          Path(x)
          for x in glob.glob(str((TEST_DIR / "test_files" / "**" / "*.nn").relative_to(THIS_DIR)))
        ]
      case RerunType.FAILED:
        if len(self.past_runs.runs) == 0:
          raise ValueError("No past runs to rerun")
        self.test_files = [
          x.file for x in self.past_runs.runs[-1].file_tests if not x.result.is_pass()
        ]
      case RerunType.ALL:
        if len(self.past_runs.runs) == 0:
          raise ValueError("No past runs to rerun")
        self.test_files = [x.file for x in self.past_runs.runs[-1].file_tests]

    self.stop_after = self.arguments.stop_after

    self.test_data = TestrunData(
      compiler_path=self.arguments.compiler_path, stop_after=self.stop_after
    )

    terminal_size = shutil.get_terminal_size()
    cpu_count = os.cpu_count() or 1

    self.task_limit = min(cpu_count // 2, max(len(self.test_files), 1))
    self.column_limit = min(terminal_size.columns - 1, 30)

    self.result_line = 0
    self.result_column = 0

    self.finished_tests = 0
    self.test_time = 0

    self.test_date = datetime.now()
    self.file_tests: list[SingleFileTest] = []

    self.processing_lines = [ProcessingData() for _ in range(self.task_limit)]

  def run(self) -> int:
    self.past_runs.runs = [
      run for run in self.past_runs.runs if run.run_args.stop_after == self.arguments.stop_after
    ]
    if self.arguments.print_last_run:
      if len(self.past_runs.runs) == 0:
        raise ValueError("Cannot print last run: There is no last run")

      last_run = self.past_runs.runs.pop()
      self.test_time = last_run.runtime

      self._print_full_run(0, [(x, x) for x in last_run.file_tests])

      return 0

    self.test_data.temp_directory = Path(tempfile.mkdtemp(prefix="nn_test_")).resolve()

    start_time = time.perf_counter_ns()
    res = asyncio.run(self._run())
    end_time = time.perf_counter_ns()

    archives: list[tuple[SingleFileTest, PastSingleFileTest]] = []

    for test in self.file_tests:
      archives.append((test, test.archive()))
      self.test_time += test.end_time - test.start_time

    self._print_full_run(
      end_time - start_time,
      cast(list[tuple[SingleFileTest | PastSingleFileTest, PastSingleFileTest]], archives),
    )

    this_run = PastRun(
      self.test_date, self.test_time, self.arguments, [archive for (_, archive) in archives]
    )
    self.past_runs.runs.append(this_run)
    self.past_runs.store(self.arguments.past_runs_toml_path, self.arguments.past_runs_limit)

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

      self.task_limit = max(min(len(filtered_tests), self.task_limit), 1)
      self.processing_lines = [ProcessingData() for _ in range(self.task_limit)]

    if len(filtered_tests) == 0:
      print("Nothing to do")
      return 0

    tests = asyncio.Queue[SingleFileTest]()
    self.file_tests = [
      SingleFileTest(self, test, testrun_data=self.test_data) for test in filtered_tests
    ]
    for test in self.file_tests:
      tests.put_nowait(test)

    tasks = [asyncio.create_task(self._single_runner(tests)) for _ in range(self.task_limit)]
    try:
      set_echo(False)
      self._setup_screen()
      await asyncio.gather(self._screen_updater(len(self.file_tests)), *tasks)
      self._update_screen()
      print(flush=True)
    finally:
      set_echo(True)

    return 0 if all(test.result.is_pass() for test in self.file_tests) else 1

  def _print_full_run(
    self,
    time_elapsed: int,
    archives: list[tuple[SingleFileTest | PastSingleFileTest, PastSingleFileTest]],
  ):
    shown_tests = archives[:]
    shown_tests.sort(key=lambda t: not t[0].result.is_pass())
    if not self.arguments.show_passes:
      shown_tests = [test for test in shown_tests if not test[0].result.is_pass()]

    print()
    for test, archived in shown_tests:
      archived.print(with_error=test.error if isinstance(test, SingleFileTest) else None)

    print()

    passes: list[str] = []
    xfails: list[str] = []
    xpasses: list[str] = []
    skipped: list[str] = []
    failures: list[str] = []
    errors: list[str] = []

    test_files_dir = (TEST_DIR / "test_files").relative_to(THIS_DIR)

    try:
      previous_run: list[PastSingleFileTest] = self.past_runs.runs[-1].file_tests
    except IndexError:
      previous_run = []

    for test, archived in archives:
      is_new = False
      try:
        in_past_run = next(run for run in previous_run if run.file.resolve() == test.file.resolve())
      except StopIteration:
        in_past_run = archived
        is_new = bool(previous_run)

      test_name = test.file.relative_to(test_files_dir)
      test_name = str(test_name) + (" (NEW)" if is_new else "")

      match test.result:
        case TestResult.PASS:
          if not in_past_run.result.is_pass():
            passes.append(f"{test_name} (FIXED)")
          else:
            passes.append(f"{test_name}")
        case TestResult.FAIL:
          if in_past_run.result.is_pass():
            failures.append(f"{test_name} (BROKEN)")
          else:
            failures.append(f"{test_name}")
        case TestResult.ERROR:
          errors.append(f"{test_name}")
        case TestResult.SKIP:
          skipped.append(f"{test_name}")
        case TestResult.XFAIL:
          if not in_past_run.result.is_pass():
            xfails.append(f"{test_name} (FIXED)")
          else:
            xfails.append(f"{test_name}")
        case TestResult.XPASS:
          if in_past_run.result.is_pass():
            xpasses.append(f"{test_name} (BROKEN)")
          else:
            xpasses.append(f"{test_name}")
        case _:
          errors.append(f"{test_name} ({test.result})")

    print(f"TESTS RUN: {len(archives)}")
    if passes:
      print(f"{COLOR_PASS}PASSES{COLOR_END}: {len(passes)}")
      for pass_ in passes:
        if self.arguments.show_passes or pass_.endswith(")"):
          print(f"  {pass_}")
    if xfails:
      print(f"{COLOR_PASS}XFAILS{COLOR_END}: {len(xfails)}")
      for xfail in xfails:
        if self.arguments.show_passes or xfail.endswith(")"):
          print(f"  {xfail}")
    if skipped:
      print(f"{COLOR_SKIP}SKIPS{COLOR_END}: {len(skipped)}")
      for skip in skipped:
        print(f"  {skip}")
    if failures:
      print(f"{COLOR_FAIL}FAILURES{COLOR_END}: {len(failures)}")
      for error in failures:
        print(f"  {error}")
    if xpasses:
      print(f"{COLOR_FAIL}UNEXPECTED PASSES{COLOR_END}: {len(xpasses)}")
      for xpass in xpasses:
        print(f"  {xpass}")
    if errors:
      print(f"{COLOR_FAIL}ERRORS{COLOR_END}: {len(errors)}")
      for error in errors:
        print(f"  {error}")

    print(
      f"Finished in {(time_elapsed) / 1_000_000_000:.3f}s, real test time was {self.test_time / 1_000_000_000:.3f}s"
    )

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
      print()  # Newline

    print("=" * self.column_limit, flush=True)  # And another newline

  def _update_screen(self):
    backtrack_amount = 2 + self.result_line + len(self.processing_lines) - 1
    visible_lines = shutil.get_terminal_size().lines
    backtrack_miss = backtrack_amount - visible_lines
    print(f"\x1b[{backtrack_amount}F", end="", flush=True)
    for processing in self.processing_lines:
      if backtrack_miss > 0:
        backtrack_miss -= 1
        continue
      if not processing.done:
        processing.advance()
      processing.print()
      print("\x1b[1E", end="")
    print(f"\x1b[{2 + self.result_line}E", end="", flush=True)
    if self.result_column:
      print(f"\x1b[{self.result_column}C", end="", flush=True)

  def _print_result(self, result: TestResult):
    print(result.small_repr(), end="")
    print("\x1b[0m", end="", flush=True)
    self.result_column += 1
    if self.result_column == self.column_limit:
      self.result_column = 0
      self.result_line += 1
      print()  # Newline

  def _read_args(self):
    pass
