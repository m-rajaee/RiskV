beq x7, x8, mylabel
not x3 , x1
mylabel:
		addi x1, x2, 23
		lw x5, 4(x10)
		sw x6, 2(x1)