GLOBAL  _read_msw,_lidt
GLOBAL  _int_08_hand
GLOBAL  _mascaraPIC1,_mascaraPIC2,_Cli,_Sti
GLOBAL  _debug
GLOBAL _int_00_hand
GLOBAL _int_09_hand
GLOBAL _int_80_hand
GLOBAL _int_0e_hand
GLOBAL __write
GLOBAL __read
GLOBAL _cursor
GLOBAL _moveCursor
GLOBAL _speaker
GLOBAL _clock
GLOBAL _setPaging
GLOBAL _getStackSize
GLOBAL _out
GLOBAL _in
GLOBAL _readSector
GLOBAL _writeSector

EXTERN	seed
EXTERN 	int_00
EXTERN  int_08
EXTERN  int_09
EXTERN	int_80
EXTERN  int_0e
EXTERN  waiti
EXTERN  printBuffer
EXTERN  printasd
EXTERN  printa

EXTERN LoadESP
EXTERN SaveESP
EXTERN GetTemporaryESP
EXTERN GetNextProcess


SECTION .text


_Cli:
	cli			; limpia flag de interrupciones
	ret

_Sti:

	sti			; habilita interrupciones por flag
	ret

_mascaraPIC1:			; Escribe mascara del PIC 1
	push    ebp
        mov     ebp, esp
        mov     ax, [ss:ebp+8]  ; ax = mascara de 16 bits
        out	21h,al
        pop     ebp
        retn

_mascaraPIC2:			; Escribe mascara del PIC 2
	push    ebp
        mov     ebp, esp
        mov     ax, [ss:ebp+8]  ; ax = mascara de 16 bits
        out	0A1h,al
        pop     ebp
        retn

_read_msw:
        smsw    ax		; Obtiene la Machine Status Word
        retn


_lidt:				; Carga el IDTR
        push    ebp
        mov     ebp, esp
        push    ebx
        mov     ebx, [ss: ebp + 6] ; ds:bx = puntero a IDTR 
	rol	ebx,16		    	
	lidt    [ds: ebx]          ; carga IDTR
        pop     ebx
        pop     ebp
        retn

_int_00_hand:

	push    ds
        push    es                      ; Se salvan los registros
        pusha                           ; Carga de DS y ES con el valor del selector
        mov     ax, 10h			; a utilizar.
        mov     ds, ax
        mov     es, ax                  
        call    int_00 
jmp $               
	popa                            
        pop     es
        pop     ds
        iret


_int_08_hand:				; Handler de INT 8 ( Timer tick)
	cli
	pushad
		push    ds
		push    es                      ; Se salvan los registros                         
		mov     ax, 10h			; Carga de DS y ES con el valor del selector
		mov     ds, ax			; a utilizar.
		mov     es, ax                  
		call    int_08                 
		pop     es
		pop     ds
		mov eax, esp
		push eax
		call SaveESP
		pop eax
		call GetTemporaryESP
		mov esp, eax
		call GetNextProcess
		push eax
		call LoadESP
		pop ebx
		mov esp,eax
	popad

	mov	al,20h			; Envio de EOI generico al PIC
	out	20h,al                          
	sti
        iret

_int_09_hand:				; Handler de INT 9 ( Teclado )
	cli
        push    ds
        push    es                      ; Se salvan los registros
        pusha                           ; Carga de DS y ES con el valor del selector
        mov     ax, 10h			; a utilizar.
        mov     ds, ax
        mov     es, ax
	mov 	eax, 0
	in	al, 60h			;saca el scan code del buffer del teclado.
	push 	eax
        call    int_09  
	pop	eax
        mov	al,20h			; Envio de EOI generico al PIC
	out	20h,al
	popa                            
        pop     es
        pop     ds
	sti
        iret

_int_0e_hand:
	push    ds
        push    es                      ; Se salvan los registros
        pusha                           ; Carga de DS y ES con el valor del selector
        mov     ax, 10h			; a utilizar.
        mov     ds, ax
        mov     es, ax                  
        call    int_0e 
jmp $             
	popa                            
        pop     es
        pop     ds
        iret

_int_80_hand:

      push    ds
      push    es                      ; Se salvan los registros
      pusha                           ; Carga de DS y ES con el valor del selector a utilizar.
      push	eax
      mov     ax, 10h			
      mov     ds, ax
      mov     es, ax
      pop	eax

      push	edx
      push      ecx
      push	ebx
      push	eax

      call 	int_80
      pop 	eax
      pop	ebx
      pop	ecx
      pop	edx
      popa                            
      pop     es
      pop     ds
      iret

__write:
   ;Armado de stack frame
   push ebp
   mov ebp, esp
   push    ds
   push    es                      ; Se salvan los registros
   pusha                          
   mov     eax,0
   mov ebx, [ebp+8] ; file descriptor
   mov ecx, [ebp + 12] ; buffer
   mov edx, [ebp + 16] ; cantidad de caracteres a escribir.
   int 80h  
   popa                            
   pop     es
   pop     ds
   ;Desarmado de stack frame
   mov esp,ebp
   pop ebp   
   ret
  
__read:
   ;Armado de stack frame
   push ebp
   mov ebp, esp
   push    ds
   push    es                      ; Se salvan los registros
   pusha                          
   mov     eax,1
   mov ebx, [ebp+8] ; file descriptor
   mov ecx, [ebp + 12] ; buffer
   mov edx, [ebp + 16] ; cantidad de caracteres a escribir.
   int 80h  
   popa                            
   pop     es
   pop     ds
   ;Desarmado de stack frame
   mov esp,ebp
   pop ebp   
   ret


_cursor:

	;Stack frame y guardado de registros.
	push ebp
	mov ebp, esp
	push ds
	push es
	pusha

	mov dx, 03cch
	in al, dx
	and al, 0FEh
	mov dx, 03c2h
	out dx, al
	mov dx, 03b4h
	in al, dx
	mov al, 0eh
	mov dx, 03b4h
	out dx, al
	mov dx, 03b5h
	mov ax, [ebp+8]
	mov al, ah
	out dx, al

	mov al, 0fh
	mov dx, 03b4h
	out dx, al
	mov eax, [ebp+8]
	mov dx, 03b5h
	out dx, al

	popa
	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret

_moveCursor:
    
	;Stack frame y guardado de registros.
	push ebp
	mov ebp, esp
	push ds
	push es
	pusha
	mov dx, 03cch
	in al, dx
	and al, 0FEh
	mov dx, 03c2h
	out dx, al
	mov al, 0fh
	mov dx, 03b4h
	out dx, al
	mov dx, 03b5h
	in al, dx
	cmp al, 0FFh
	jnz incAl
	mov al, 0
	out dx, al
	mov al, 0eh
	mov dx, 03b4h
	out dx, al
	mov dx, 03b5h
	in al, dx
incAl:	inc al
	out dx, al
	popa
	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret

_clock:

	;Stack frame y guardado de registros.
	push ebp
	mov ebp, esp
	push ds
	push es

	mov al, [ebp+8] ; memory location
	out 70h, al
	in al, 71h

	and eax, 0FFh

	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret

_getStackSize:

	;Stack frame y guardado de registros.
	push ebp
	mov ebp, esp
	push ds
	push es

    
	mov eax, esp
	mov ebx, ss
	sub eax, ebx

	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret



_speaker:
	push ebp
	mov ebp, esp
	push ds
	push es
	pusha
	mov     bx,[ebp+8]             ; frecuencia

	mov     al, 0b6h    
	out     43h, al     

	mov     ax, bx      

	out     42h, al     
	mov     al, ah      
	out     42h, al     

	in      al, 61h          
	or      al, 03h    
	out     61h, al    
                         
	mov 	eax, [ebp+12]	;duracion
	push	eax
	call	waiti
	pop	eax

	in      al,61h   
	and     al,0fch  
	out     61h,al   
                        
	popa
	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret

; Debug para el BOCHS, detiene la ejecució; Para continuar colocar en el BOCHSDBG: set $eax=0
;

_setPaging:
	push ebp
	mov ebp, esp
	push ds
	push es
	pusha
	mov eax, [ebp+8]
	mov cr3, eax
	mov eax, 80000000h
	mov ebx, cr0
	or ebx, eax;//HABILITO PAGING
	mov cr0, ebx
	popa
	pop es
	pop ds
	mov esp, ebp
	pop ebp
	ret

_out:
	push ebp
	mov ebp, esp
	mov edx, [ebp + 8]
	mov eax, [ebp + 12]
	out dx, ax
	mov esp, ebp
	pop ebp
	ret

_in:
	push ebp
	mov ebp, esp
	mov edx, [ebp + 8]
	in ax, dx
	mov esp, ebp
	pop ebp
	ret

_readSector:	
	push ebp
	mov ebp, esp
	
	mov ecx, [ebp+12]
	mov dx, 1f0h
	mov eax, ds
	;mov es, eax
	mov edi, [ebp+8]
	rep insw
	mov esp, ebp
	pop ebp
	ret

_writeSector:
	push ebp
	mov ebp, esp
	mov ecx, 256
	mov dx, 1f0h
	mov eax, ds
	mov es, eax
	mov edi, [ebp+8]
repeat: rep outsw
	;;jmp $+2
	;;nop
	;;sub edi, 2 
	;mov eax, ecx
	;push eax
	;call printasd
	;pop eax
	;mov eax, edi
	;push ecx
	;push eax
	;call printa
	;pop eax
	;pop ecx
	;;dec ecx
	;;jnz repeat
	mov esp, ebp
	pop ebp

	ret


_debug:
        push    bp
        mov     bp, sp
        push	ax
vuelve:	mov     ax, 1
        cmp	ax, 0
	jne	vuelve
	pop	ax
	pop     bp
        retn
