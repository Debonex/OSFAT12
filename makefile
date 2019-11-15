main:my_print.obj main.cpp
	g++ -o main main.cpp my_print.obj
my_print.obj:my_print.asm
	nasm -f win32 -o my_print.obj my_print.asm
run:main
	.\main
