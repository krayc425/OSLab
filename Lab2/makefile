.PHONY : start
start : main.c my_print.asm
	nasm -f elf -o my_print.o my_print.asm
	gcc -o main main.c my_print.o
	./main