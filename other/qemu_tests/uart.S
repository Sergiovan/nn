.globl uart_init
.globl uart_putc
.globl uart_getc
.globl uart_puts
.globl print_number

.equ UART_ADDRESS,              0x10000000
.equ LINE_STATUS_REGISTER,      0x5
.equ LINE_CONTROL_REGISTER,     0x3
.equ FIFO_CONTROL_REGISTER,     0x2
.equ INTERRUPT_ENABLE_REGISTER, 0x1
.equ LINE_STATUS_DATA_READY,    0x1

.equ FIRST_NUMBER, '0'
.equ FIRST_HEX, 'A' - 0xA

uart_init:
    li  t0, UART_ADDRESS

    # 0x3 -> 8 bit word length
    li  t1, 0x3
    sb  t1, LINE_CONTROL_REGISTER(t0)

    # 0x1 -> enable FIFOs
    li  t1, 0x1
    sb  t1, LINE_CONTROL_REGISTER(t0)

    # 0x1 -> enable reciever interrupts
    sb  t1, INTERRUPT_ENABLE_REGISTER(t0)

    ret

uart_getc:
    li      t0, UART_ADDRESS

    lbu     t1, LINE_STATUS_REGISTER(t0)
    andi    t1, t1, LINE_STATUS_DATA_READY

    # jump to _uart_read if UART is ready to read from
    bnez    t1, _uart_read

    # otherwise, return 0
    mv      a0, zero
    j       _uart_get_end

_uart_read:
    # load character at UART address into a0
    lbu     a0, (t0)
    j       _uart_get_end

_uart_get_end:
    ret

uart_putc:
    li  t0, UART_ADDRESS

    # store character at UART address in return register
    sb  a0, (t0)
    ret

uart_puts:
    # a0 is char*
    li   t0, UART_ADDRESS
_puts_loop_start:
    lbu  t1, (a0)
    beqz t1, _puts_loop_end
    sb   t1, (t0)
    addi a0, a0, 1
    j    _puts_loop_start
_puts_loop_end:
    ret

print_number:
  # Force printing of 0x
  li t0, UART_ADDRESS
  li t1, '0'
  sb t1, (t0)
  li t1, 'x'
  sb t1, (t0)

  mv t1, a0 # t1 contains the number to print

  li t2, 0xA # t2 contains 0xA
  li t3, 64 # t3 contains the loop counter. 64 (-=4) until < 0
  li t4, 0 # t4 contains zero only if we've only seen zeroes
  mv t5, ra # Return address, since we're calling something else here
_loop_start:
  addi t3, t3, -4 # Update loop counter
  bltz t3, _loop_end # If loop counter < 0, we're done
  mv a0, t1 # Copy number to print to a0 (we'll call uart_putc soon enough)
  # Shift right logical by the loop counter. This effectively gets the 
  # n-th most significant nibble, where n is the amount of iterations
  srl a0, a0, t3
  andi a0, a0, 0xF # Remove other data
  or t4, t4, a0 # Check if we have seen anything other than 0s before
  beqz t4, _loop_start
  bltu a0, t2, _is_number # If the number is less than 0xA, print a number
  addi a0, a0, FIRST_HEX # Otherwise, print A-F by adding 'A' to the number
  j _do_putc
_is_number:
  addi a0, a0, FIRST_NUMBER # Add '0' to the number
_do_putc:
  call uart_putc # Actually send the character to serial
  j _loop_start # Back to the start of the loop
_loop_end:
  mv ra, t5
  ret

.section .note.GNU-stack,"",@progbits
