.globl _start

_start:
  # Init uart
  call uart_init
  # Put "Value in a0 is "
  la a0, STRING_VALUE
  call uart_puts

  # Write to mscratch
  csrrwi zero, mscratch, 1
  
  # Print 0x3
  li a0, 3
  call print_number
  ebreak

STRING_VALUE: .string "Value in a0 is "
