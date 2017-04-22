global add

global my_print

global my_print_str

SECTION .bss
	char resd 1
	stra resd 1

SECTION .data
	lb: db 0Ah
	.len: equ $-lb

SECTION .text

add:
    mov eax, [esp + 4]
    add eax, [esp + 8]
    ret

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
	mov ecx, [ebp + 8]
	mov dword[stra], ecx

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
