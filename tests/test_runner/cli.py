from __future__ import annotations

from . import TEST_DIR, THIS_DIR
from .test_data import CompilerPhase

from dataclasses import field
from enum import Enum, auto
from pathlib import Path

from serde import serde, field as serde_field


class RerunType(Enum):
  NO_RERUN = auto()
  ALL = auto()
  FAILED = auto()

  def __str__(self) -> str:
    return self.name

  @staticmethod
  def from_str(string: str) -> RerunType:
    VALUES = {str(x): x for x in RerunType}
    try:
      return VALUES[string]
    except KeyError as e:
      raise ValueError(f"Invalid value for Rerun Type: {string}") from e


def compiler_path_factory() -> Path:
  return (
    (TEST_DIR / Path("..") / "output" / "Debug" / "nn")
    .resolve()
    .relative_to(THIS_DIR, walk_up=True)
  )


def past_runs_toml_path_factory() -> Path:
  return (TEST_DIR / "past_runs.toml").resolve().relative_to(THIS_DIR)


@serde
class Arguments:
  test_filter: list[str] = field(default_factory=list[str])
  test_files: list[Path] = field(default_factory=list[Path])
  compiler_path: Path = field(default_factory=compiler_path_factory)
  show_passes: bool = False
  print_last_run: bool = False
  past_runs_toml_path: Path = field(default_factory=past_runs_toml_path_factory)
  past_runs_limit: int = 100
  rerun_previous: RerunType = serde_field(
    default=RerunType.NO_RERUN, serializer=RerunType.__str__, deserializer=RerunType.from_str
  )
  stop_after: CompilerPhase = serde_field(
    default=CompilerPhase.RUN, serializer=CompilerPhase.__str__, deserializer=CompilerPhase.from_str
  )

  @staticmethod
  def parse_args() -> Arguments:
    import argparse

    p = argparse.ArgumentParser(
      prog="NN Compiler test runner",
      usage=Path(__file__).name,
      description="Test runner for the nn compiler",
      formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    def path_from_this_dir(arg: str) -> Path:
      p = Path(arg).resolve().relative_to(THIS_DIR)
      return p

    # Choose what to run
    p.add_argument(
      "test_filter",
      nargs="*",
      type=str,
      help="Filters for tests to run. All tests that include any of the text in any of the filters in their path will be run",
    )
    # Test one or more files directly instead of globbing
    p.add_argument(
      "--test-file",
      dest="test_files",
      type=path_from_this_dir,
      action="append",
      help="Run test on specific file(s) explicitly",
    )
    # Path to the compiler to use
    p.add_argument(
      "--compiler",
      dest="compiler_path",
      type=path_from_this_dir,
      default=compiler_path_factory(),
      help="Path to compiler to use",
    )
    # Show tests that were successful in the output
    p.add_argument(
      "--show-passes",
      dest="show_passes",
      action="store_true",
      default=False,
      help="Show passed tests in addition to failures",
    )
    # Show tests that were successful in the output
    p.add_argument(
      "--print-last-run",
      dest="print_last_run",
      action="store_true",
      default=False,
      help="Print last run's output",
    )
    # Where to store data about previous runs, and how many to keep
    p.add_argument(
      "--past-runs",
      dest="past_runs_toml_path",
      type=path_from_this_dir,
      default=past_runs_toml_path_factory(),
      help="Path to past runs toml",
    )
    p.add_argument(
      "--past-runs-limit",
      dest="past_runs_limit",
      type=int,
      default=100,
      help="Amount of past runs to keep in toml file",
    )
    # If we should rerun the last set of tests
    p.add_argument(
      "--rerun",
      dest="rerun_previous",
      type=RerunType.from_str,
      choices=RerunType,
      default=RerunType.NO_RERUN,
      help="If previous run should be done",
    )
    p.add_argument(
      "--stop-after",
      dest="stop_after",
      type=CompilerPhase.from_str,
      choices=CompilerPhase,
      default=CompilerPhase.RUN,
      help="Stop all test cases after the given compiler phase",
    )

    args = p.parse_args(namespace=Arguments())

    for test_file in args.test_files:
      if not test_file.exists():
        p.error(f'Test file "{test_file}" does not exist')

    if args.compiler_path and not args.compiler_path.exists():
      p.error(f'Compiler "{args.compiler_path}" does not exist')

    return args
