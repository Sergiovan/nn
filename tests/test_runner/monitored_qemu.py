import asyncio
import shlex
import time

from pathlib import Path


class MonitoredQemu:
  def __init__(self, binary: Path, monitor_socket: Path):
    self.binary: Path = binary
    self.monitor_socket: Path = monitor_socket
    self.at_prompt: bool = False

    self.program_ret: int = -1
    self.error: Exception | None = None
    self.program_stdout: str = ""
    self.program_stderr: str = ""

  async def complete(self):
    qemu_proc = await asyncio.create_subprocess_exec(
      *[
        "qemu-system-riscv64",
        "-machine",
        "virt",
        "-m",
        "16",
        "-smp",
        "1",
        "-cpu",
        "rv64",
        "-serial",
        "stdio",
        "-nographic",
        "-monitor",
        shlex.quote(f"unix:{self.monitor_socket},server,nowait"),
        "-device",
        shlex.quote(f"loader,file={self.binary},addr=0x80000000"),
        "-bios",
        "none",
      ],
      stdout=asyncio.subprocess.PIPE,
      stderr=asyncio.subprocess.PIPE,
    )

    try:
      await self.wait_for_socket()
      self.reader, self.writer = await asyncio.open_unix_connection(self.monitor_socket)
      await self.wait_for_prompt()
      timeout = 5  # Seconds
      while timeout > 0:
        loop_start = time.time()
        try:
          # Not particularly precise, but that's ok
          # TODO Do not assume we're on CPU 0
          registers = (await asyncio.wait_for(self._read_registers(), timeout))[0]
          if registers["mscratch"] == 1:
            # We're done. Output is in a0
            self.program_ret = registers["a0"]
            if self.program_ret & (1 << 63) != 0:
              self.program_ret = self.program_ret - (1 << 64)  # negate that bastard
            break
          await asyncio.sleep(0.1)
          timeout -= time.time() - loop_start
        except asyncio.TimeoutError as e:
          self.error = e
          raise Exception("Reading registers timed out") from e
      else:
        raise Exception("Execution timed out after 5 seconds")
    finally:
      try:
        qemu_proc.kill()
      except OSError:
        pass
      self.program_stdout, self.program_stderr = (
        x.decode("ascii") for x in await qemu_proc.communicate()
      )
      try:
        self.writer.close()
      finally:
        pass

  async def wait_for_socket(self):
    start_time = time.time()
    while time.time() - start_time < 5:
      if self.monitor_socket.exists():
        return
      await asyncio.sleep(0.1)
    else:
      raise Exception(f"Socket {self.monitor_socket} was not created within 5 seconds")

  async def wait_for_prompt(self):
    if self.at_prompt:
      return ""
    res = await self.reader.readuntil(b"(qemu) ")
    self.at_prompt = True
    return res.decode("ascii")[:-7]  # Remove prompt

  async def _send_command(self, command: str):
    if not self.at_prompt:
      raise Exception(f'Trying to send command "{command}" before waiting for prompt')
    self.writer.write(command.encode("utf-8") + b"\n")
    await self.reader.readline()  # Echo
    self.at_prompt = False

  async def _read_registers(self) -> list[dict[str, int]]:
    await self._send_command("info registers")
    res = await self.wait_for_prompt()
    register_data: list[dict[str, int]] = []
    for line in res.splitlines():
      line = line.strip()
      if "=" in line or not line:
        continue
      if "CPU#" in line:
        # TODO make a proper structure...
        register_data.append({})
        continue
      words = line.split()
      while len(words) >= 2:
        regs = words.pop(0)
        val = int(words.pop(0), 16)
        for reg in regs.split("/"):
          register_data[-1][reg] = val
      if words:
        raise Exception(f"Invalid register line read: {line}")
    return register_data

  def get_result(self) -> int:
    return self.program_ret

  def get_output(self) -> tuple[str, str]:
    return (self.program_stdout, self.program_stderr)
