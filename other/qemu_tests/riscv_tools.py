import argparse
import os
import shlex
import shutil
import subprocess
import sys

from pathlib import Path
from typing import NoReturn

ASSEMBLER = "riscv64-elf-as"
LINKER = "riscv64-elf-ld"
OBJCOPY = "riscv64-elf-objcopy"
OBJDUMP = "riscv64-elf-objdump"
QEMU = "qemu-system-riscv64"
SOCAT = "socat"

DEFAULT_OBJECT = Path("out.o")
DEFAULT_ELF = Path("out.elf")
DEFAULT_BINARY = Path("out.bin")


def tool_path(program: str):
  if (prog := shutil.which(program)) is not None:
    return Path(prog)
  else:
    return None


def please_install(program: str):
  if tool_path(program) is None:
    raise Exception(
      f'"{program}" could not be found on your system and is required to run this utility'
    )


def check_input(files: list[Path] | Path):
  if isinstance(files, Path):
    files = [files]

  for file in files:
    if not file.exists():
      raise ValueError(f"File {file} cannot be found")


def run_linux_program(process_name: str, commands: list[str], timeout: float = 5):
  proc = subprocess.Popen(
    commands,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
  )

  try:
    res = proc.wait(timeout)
  except subprocess.TimeoutExpired as e:
    proc.kill()
    raise Exception(f"{process_name} took longer than {timeout} seconds") from e

  if res != 0:
    out, err = proc.communicate()
    print(f"Command <{' '.join(commands)}> failed with return code {res}")
    print(f"Stdout: \n\t{out.decode(errors='backslash').replace('\n', '\n\t')}")
    print(f"Stderr: \n\t{err.decode(errors='backslash').replace('\n', '\n\t')}")
    raise Exception(f"{process_name} failed to run to completion")


def assemble(inputs: list[Path], output: Path):
  cmd = [
    ASSEMBLER,
    *[shlex.quote(str(x)) for x in inputs],
    "-o",
    shlex.quote(str(output)),
  ]
  run_linux_program("Assembler", cmd)


def link(inputs: list[Path], output: Path):
  cmd = [
    LINKER,
    "-melf64lriscv",
    "-nostdlib",
    "-Ttext=0x80000000",
    *[shlex.quote(str(x)) for x in inputs],
    "-o",
    shlex.quote(str(output)),
  ]
  run_linux_program("Linker", cmd)


def copy(input: Path, output: Path):
  cmd = [
    OBJCOPY,
    shlex.quote(str(input)),
    "-O",
    "binary",
    shlex.quote(str(output)),
  ]
  run_linux_program("Linker", cmd)


def dump(file: Path, output: Path | None):
  cmd = [OBJDUMP, "--disassembler-color=extended", "-m", "riscv:rv64", "-D", shlex.quote(str(file))]

  with open(file, "rb") as f:
    perhaps_elf_magic = f.read(4)
    if perhaps_elf_magic != b"\x7f\x45\x4c\x46":
      cmd += ["-b", "binary"]

  if output is not None:
    cmd.pop(1)  # Remove the colors...

  stdout = subprocess.PIPE
  if output is not None:
    stdout = open(output, "wb")

  proc = subprocess.Popen(
    cmd,
    stdout=stdout,
    stderr=subprocess.PIPE,
  )

  try:
    res = proc.wait(5)
  except subprocess.TimeoutExpired as e:
    proc.kill()
    raise Exception("Objdump took longer than 5 seconds") from e

  out, err = proc.communicate()
  if res != 0:
    print(f"Command <{' '.join(cmd)}> failed with return code {res}")
    if output is None:
      print(f"Stdout: \n\t{out.decode(errors='backslash').replace('\n', '\n\t')}")
    print(f"Stderr: \n\t{err.decode(errors='backslash').replace('\n', '\n\t')}")
    raise Exception("Objdump failed to run to completion")
  else:
    if output is None:
      print(out.decode())


def run_qemu(input: Path, monitor: str, graphical: bool) -> NoReturn:
  args = [
    QEMU,
    "-machine",
    "virt",
    "-m",
    "16",
    "-smp",
    "1",
    "-cpu",
    "rv64",
    "-monitor",
    shlex.quote(monitor),
    "-device",
    shlex.quote(f"loader,file={input},addr=0x80000000"),
    "-bios",
    "none",
  ]
  sys.stdout.flush()
  os.execvp(QEMU, args)


def run_socat(monitor: str) -> NoReturn:
  args = [SOCAT, "-,echo=0,icanon=0", shlex.quote(monitor)]
  sys.stdout.flush()
  os.execvp(SOCAT, args)


def compile_handler(ns: argparse.Namespace) -> int | NoReturn:
  files: list[Path] = ns.asm_files
  output: Path = ns.output
  and_run: bool = ns.and_also_run
  monitor: str = ns.monitor
  graphical: bool = ns.graphical

  please_install(ASSEMBLER)
  please_install(LINKER)
  please_install(OBJCOPY)
  if and_run:
    please_install(QEMU)

  check_input(files)

  assemble(files, DEFAULT_OBJECT)
  link([DEFAULT_OBJECT], DEFAULT_ELF)
  copy(DEFAULT_ELF, output)

  if and_run:
    run_qemu(output, monitor, graphical)

  return 0


def assemble_handler(ns: argparse.Namespace) -> int:
  files: list[Path] = ns.asm_files
  output: Path = ns.output

  please_install(ASSEMBLER)

  check_input(files)

  assemble(files, output)
  return 0


def link_handler(ns: argparse.Namespace) -> int:
  files: list[Path] = ns.object_files
  output: Path = ns.output

  please_install(LINKER)

  check_input(files)

  link(files, output)
  return 0


def copy_handler(ns: argparse.Namespace) -> int:
  file: Path = ns.elf_file
  output: Path = ns.output

  please_install(ASSEMBLER)

  check_input(file)

  copy(file, output)
  return 0


def dump_handler(ns: argparse.Namespace) -> int:
  file: Path = ns.file
  output: Path | None = ns.output

  please_install(OBJDUMP)
  check_input(file)

  dump(file, output)

  return 0


def run_handler(ns: argparse.Namespace) -> NoReturn:
  file: Path = ns.binary_file
  monitor: str = ns.monitor
  graphical: bool = ns.graphical

  please_install(QEMU)
  check_input(file)

  run_qemu(file, monitor, graphical)


def monitor_handler(ns: argparse.Namespace) -> NoReturn:
  address: str = ns.address

  please_install(SOCAT)

  run_socat(address)


def main() -> int:
  parser = argparse.ArgumentParser()
  subparsers = parser.add_subparsers(required=True)

  parser_compile = subparsers.add_parser("compile", help="Fully compile files into a binary object")
  parser_compile.add_argument("asm_files", nargs="+", type=Path, help="Files to compile")
  parser_compile.add_argument(
    "-o", "--output", type=Path, default=DEFAULT_BINARY, help="Final binary"
  )
  parser_compile.add_argument(
    "--and-also-run",
    action="store_true",
    default=False,
    help="Also invoke qemu on the final binary",
  )
  parser_compile.add_argument(
    "--monitor",
    default="unix:qemu-monitor-socket,server,nowait",
    help="If running, where to host the monitor connection",
  )
  parser_compile.add_argument(
    "--graphical",
    action="store_true",
    help="If running, whether a QEMU graphical session should be started",
  )
  parser_compile.set_defaults(handler=compile_handler)

  parser_as = subparsers.add_parser("assemble", help="Only assemble files")
  parser_as.add_argument("asm_files", nargs="+", type=Path, help="Files to compile")
  parser_as.add_argument(
    "-o", "--output", type=Path, default=DEFAULT_OBJECT, help="Compiled object code"
  )
  parser_as.set_defaults(handler=assemble_handler)

  parser_ld = subparsers.add_parser("link", help="Link object code files together")
  parser_ld.add_argument(
    "object_files",
    nargs="+",
    type=Path,
    default=[DEFAULT_OBJECT],
    help="Files to link together",
  )
  parser_ld.add_argument("-o", "--output", type=Path, default=DEFAULT_ELF, help="Compiled ELF")
  parser_ld.set_defaults(handler=link_handler)

  parser_objcopy = subparsers.add_parser("copy", help="Convert an ELF file into a binary blob")
  parser_objcopy.add_argument("elf_file", type=Path, default=DEFAULT_ELF, help="File to convert")
  parser_objcopy.add_argument(
    "-o", "--output", type=Path, default=DEFAULT_BINARY, help="Binary file"
  )
  parser_objcopy.set_defaults(handler=copy_handler)

  parser_objdump = subparsers.add_parser(
    "dump", help="Print the contents of a binary or elf for convenient reading"
  )
  parser_objdump.add_argument("file", type=Path, default=DEFAULT_BINARY, help="File to dump")
  parser_objdump.add_argument(
    "--output",
    type=Path,
    help="File to dump content to instead of printing to output",
  )
  parser_objdump.set_defaults(handler=dump_handler)

  parser_qemu = subparsers.add_parser("run", help="Run a binary file through RISCV64 QEMU")
  parser_qemu.add_argument("binary_file", type=Path, default=DEFAULT_BINARY, help="Binary to run")
  parser_qemu.add_argument(
    "--monitor",
    default="unix:qemu-monitor-socket,server,nowait",
    help="Where to host the monitor connection",
  )
  parser_qemu.add_argument(
    "--graphical",
    action="store_true",
    help="If a QEMU graphical session should be started",
  )
  parser_qemu.set_defaults(handler=run_handler)

  parser_socat = subparsers.add_parser("monitor", help="Monitor a running QEMU instance")
  parser_socat.add_argument(
    "address",
    nargs="?",
    default="unix-connect:qemu-monitor-socket",
    help="Socket to connect to",
  )
  parser_socat.set_defaults(handler=monitor_handler)

  args = parser.parse_args()

  if not args.handler:
    print("No handler set up?")
    return 1

  return args.handler(args)


if __name__ == "__main__":
  exit(main())
