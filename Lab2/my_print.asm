global my_print
global my_print_str

SECTION .bss
	char resd 1
	stra resd 1
	color resd 1

SECTION .data
	lb: db 0Ah
	.len: equ $-lb
	;color
	color_red:	db  1Bh, '[31;1m', 0
	.len: equ $ - color_red
	color_blue:	db  1Bh, '[34;1m', 0
	.len: equ $ - color_blue
	color_green:  db  1Bh, '[32;1m', 0
	.len: equ $ - color_green

SECTION .text

;print only 1 char
my_print:
	;create stack frame
	push ebp
    mov ebp, esp
    ;get the argument
    mov ecx, [ebp + 8]
    ;move to 'char' in memory
    mov dword[char], ecx
    ;print 'char'
	mov eax, 4
	mov ebx, 1
	mov ecx, char
	mov edx, 4
	int 0x80
	; restore the base pointer
    pop ebp
	ret

;NASM uses little-endian storage!!!
;Example:
;	string:		"abc"						eax
;	in memory:	0x06	'\0' 	register:	
;				0x07	'c'					
;				0x08	'b'					ah
;				0x09	'a'					al

;print a string
my_print_str:
	;save stack register
	push ebp
	mov ebp, esp
	;move start address of string to stra
	mov ecx, [ebp + 8]
	mov dword[stra], ecx
	;move type to edx

	mov edx, [ebp + 12]
	;set color to type
	;add edx, 48
	mov dword[color], edx


	call changeColor

loop_print:
	;load start address of string to ecx
	mov ecx, dword[stra]
	;whether to end loop                           
	mov dl, 0xFF		;dl:	1111 1111
	mov eax, dword[ecx]	;al:	last char in byte
	and dl, al			;compare dl and al
	cmp dl, 0			;if dl == 0, end of string
	je end_print
	;save next byte start address to stra
	inc ecx
	mov dword[stra], ecx
	;get the char
	and eax, 0x0000FF
	mov dword[char], eax
	mov ecx, char
	;print the char
	mov eax, 4
	mov ebx, 1
	mov edx, 4
	int 0x80
	jmp loop_print

end_print:
	;print new line
	mov eax, 4
	mov ebx, 1
	mov ecx, lb
	mov edx, lb.len
	int 0x80
	;recover stack register
	pop ebp
	ret

changeColor:
	pusha
	mov eax, dword[color]
	cmp eax, 0
	jne tryBlue
	call printRed
	jmp endColor
tryBlue:
	cmp eax, 1
	jne tryGreen
	call printBlue
	jmp endColor
tryGreen:
	call printGreen
endColor:
	popa
	ret

printRed:
	push eax

	mov eax, 4
    mov ebx, 1
    mov ecx, color_red
    mov edx, color_red.len
    int 80h

    pop eax
    ret

printBlue:
	push eax

	mov eax, 4
    mov ebx, 1
    mov ecx, color_blue
    mov edx, color_blue.len
    int 80h

    pop eax
    ret

printGreen:
	push eax

	mov eax, 4
    mov ebx, 1
    mov ecx, color_green
    mov edx, color_green.len
    int 80h

    pop eax
    ret
