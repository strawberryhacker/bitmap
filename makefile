main:
	@gcc -Wall -Wno-unused-variable bitmap.c main.c -o bitmap
	@./bitmap
	@rm -f bitmap
