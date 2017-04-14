SECTION .data
	msg: db "Enter a number:", 0Ah
	.len: equ $-msg	;The length of the message
	lb: db 0Ah
	.len: equ $-lb
	;color
	color_red:	db  1Bh, '[31;1m', 0
	.len: equ $ - color_red
	color_blue:	db  1Bh, '[34;1m', 0
	.len: equ $ - color_blue

SECTION .bss
	number1 resd 1
	number2 resd 1
	total resw 1

	;for printing number
	temp resd 1
	count resd 1
	digit resd 1
	color resd 1

	;for reading number
	tempNum: resb 2

SECTION .text
	global _start

_start:
	;set color to 0
	mov eax, 0
	mov dword[color], eax

	;print prompt msg
	mov eax, 4
	mov ebx, 1
	mov ecx, msg
	mov edx, msg.len
	int 0x80

	;read number
	call readNum

	;init f(-1) and f(0)
	mov dword[number1], 0
	mov dword[number2], 1
	;print f(0)
	mov dword[temp], 1
	call printNum

	;reset eax as a counter
	mov eax, 0

forLoop:
	;whether to finish loop
	movzx ebx, word[total]
	;if n = 1
	cmp ebx, 1
	jne cont
	mov dword[temp], 1
	je finished
cont:
	dec ebx
	cmp eax, ebx
	je finished

	;load f(n-1) and f(n-2)
	mov ecx, dword[number1]
	mov edx, dword[number2]

	;calculate f(n)
	add ecx, edx

	;save f(n-1) and f(n)
	mov dword[number2], ecx
	mov dword[number1], edx
	
	;print f(n)
	mov dword[temp], ecx

	;recover counter
	;add counter
	add eax, 1

	call printNum
	
	jmp forLoop
finished:
	mov eax, 1
	mov ebx, 0
	int 0x80
	
printNum:
	pusha
	;in dword[temp]: real number
	mov dword[count], 0

	;change color
	call changeColor
calLoop:
	mov eax, dword[temp]
	mov ecx, 10
	cdq
	div ecx
	mov dword[temp], eax
	add edx, 48
	;push quotient
	push edx
	inc dword[count]

	;save remainder to temp
	cmp dword[temp], 0
	jne calLoop
printLoop:
	pop dword[digit]

	mov eax, 4
	mov ebx, 1
	mov ecx, digit
	mov edx, 4
	int 0x80

	dec dword[count]
	cmp dword[count], 0
	jne	printLoop

	mov eax, 4
	mov ebx, 1
	mov ecx, lb
	mov edx, lb.len
	int 0x80
	
	popa	
	ret

readNum:
  	pusha
  	mov word[total], 0
loopReadNum:
    mov eax, 3
    mov ebx, 0
    mov ecx, tempNum
    mov edx, 1
    int 80h
    
    cmp byte[tempNum], 0Ah
    je endReadNum

    mov ax, word[total]
    mov bx, 10
    mul bx
    mov bl, byte[tempNum]
    sub bl, 48
    mov bh, 0
    add ax, bx

    mov word[total], ax
    jmp loopReadNum 
 endReadNum:

 	popa
 	ret

changeColor:
	pusha
	mov eax, dword[color]
	cmp eax, 0

	push eax

	mov eax, 4
    mov ebx, 1
    mov ecx, color_blue
    mov edx, color_blue.len
    int 80h

    pop eax

	je incColor
	dec eax
	jmp endColor
incColor:
	push eax

	mov eax, 4
    mov ebx, 1
    mov ecx, color_red
    mov edx, color_red.len
    int 80h

    pop eax

	inc eax
endColor:
	mov dword[color], eax
	popa
	ret
