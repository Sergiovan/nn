.globl _start

_start:
  call uart_init
  la a0, STRING_VALUE
  call uart_puts
  ld a0, 3
  call print_number
  ebreak

STRING_VALUE: .string "Value in a0 is "
