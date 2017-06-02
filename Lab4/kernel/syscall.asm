
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               syscall.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Forrest Yu, 2005
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

%include "sconst.inc"

;  Modified here
_NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
_NR_disp_str_2      equ 1
_NR_process_sleep	equ	2
INT_VECTOR_SYS_CALL equ 0x90


; 导出符号
global	get_ticks
global	disp_str_2
global	process_sleep

bits 32
[section .text]

; ====================================================================
;                              get_ticks
; ====================================================================
get_ticks:
	mov	eax, _NR_get_ticks
	int	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                             disp_str_2
; ====================================================================
disp_str_2:
	mov 	eax, _NR_disp_str_2
	mov 	ebx, [esp + 4]	; One argument 
	int 	INT_VECTOR_SYS_CALL
	ret

; ====================================================================
;                             process_sleep
; ====================================================================
process_sleep:
	mov 	eax, _NR_process_sleep
	mov 	ebx, [esp + 4]	; One argument 
	int 	INT_VECTOR_SYS_CALL
	ret