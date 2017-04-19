global add

global my_print

SECTION .bss
	char resb 1

SECTION .text

add:
    mov eax, [esp + 4]
    add eax, [esp + 8]
    ret

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