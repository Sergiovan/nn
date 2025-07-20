# Running a binary

This will run a binary blob. Also see https://www.qemu.org/docs/master/system/generic-loader.html#loading-files and https://wiki.osdev.org/QEMU

`qemu-system-riscv64 -machine virt -m 16 -smp 1 -cpu rv64 -serial stdio -nographic -monitor unix:qemu-monitor-socket,server,nowait -device loader,file=test.bin,addr=0x80000000`

- `qemu-system-riscv64`: QEMU RISCV64 emulator
- `-machine virt`: Use the virtual machine type (does not correspond to a real chip) https://www.qemu.org/docs/master/system/riscv/virt.html
- `-m 16`: 16 MB RAM memory (more than enough for tests)
- `-smp 1`: Run on 1 singular CPU
- `-cpu rv64`: Run on an RV64
- `-serial stdio`: Create a serial port and "bind" it to stdio. This is apparently the default for `-nographic` QEMU instances.
- `-nographic`: Do not start a window, instead use the console.
- `-monitor unix:qemu-monitor-socket,server,nowait`: Open a monitor on unix socket `qemu-monitor-socket`. Don't really know what the other options do...
- `-device loader,file=test.bin,addr=0x80000000`: Load the data of file `test.bin` at address `0x80000000`. This address is where QEMU will start execution.

## Other options

- `-s`: Same as -gdb tcp::1234. Opens a gdb server connection at the given port
- `-S`: Starts suspended, requiring the continue command to be sent via either the monitor or gdb

# Connecting to a running monitor

If running with a monitor, use `socat -,echo=0,icanon=0 unix-connect:qemu-monitor-socket` to connect to the socket.

# RISCV

https://projectf.io/posts/riscv-cheat-sheet

# Compiling

`riscv64-elf-as test.S -o test.o` to compile assembly in `test.S` to an ELF in `test.o`

# ELF to binary blob

`riscv64-elf-objcopy test.o -O binary test.bin` to convert `test.o` from an ELF file into a binary blob with just the assembly. Useful to make bootloaders and such

# Inspect binaries

`riscv64-elf-objdump --disassembler-color=extended -b binary -m riscv:rv64 -D test.bin` Disassembly of `test.bin` in the console
