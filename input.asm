.org 0x1000
start:
  addi x1, x0, 10
  addi x2, x0, 5
  add  x3, x1, x2
  sub  x4, x3, x2
  beq  x4, x1, match
  addi x5, x0, 1

match:
  lw x6, 0(x10)
  addi x7, x0, 99

halt:
  jal x0, halt

.org 0x2000
.word 1234
.word 5678
