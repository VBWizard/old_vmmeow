;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PMode tutorials in C and Asm                          ;;
;; Copyright (C) 2000 Alexei A. Frounze                  ;;
;; The programs and sources come under the GPL           ;;
;; (GNU General Public License), for more information    ;;
;; read the file gnu-gpl.txt (originally named COPYING). ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

USE32 386

.SECT .TEXT ;PUBLIC CLASS=CODE USE16

_exc_common:
        push    bp
        mov     bp, sp
        push    ds
        push    es                      ; saving segment registers and
        pushad                          ; other regs because it's an ISR
        mov     bx, #0x10
        mov     ds, bx
        mov     es, bx                  ; load ds and es with valid selector
        mov     bx, ax
	add	bx, _exc_has_error
	mov	al,byte ptr [bx]
        cmp     al,0
        je      .isr1
	SEG SS
        push    word [bp+4]          ; error code
	SEG SS
        push    dword [bp+8]         ; ip
	SEG SS
        push    word [bp+12]         ; cs
        jmp     .isr2
.isr1:
        push    word 0                  ; error code
        SEG SS
	push    dword [bp+4]         ; ip
	SEG SS        
	push    word [bp+8]          ; cs
.isr2:
        push    ax                      ; exception no
; void exc_handler (no, cs, ip, error)
        call    _exc_handler            ; call actual ISR code
        add     sp, #10
        popad                           ; restoring the regs
        pop     es
        pop     ds
        pop     bp
        pop     ax
        iret
_isr_00_wrapper:
        push    ax
        mov     ax, #0x00                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_01_wrapper:
        push    ax
        mov     ax, #0x01                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_02_wrapper:
        push    ax
        mov     ax, #0x02                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_03_wrapper:
        push    ax
        mov     ax, #0x03                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_04_wrapper:
        push    ax
        mov     ax, #0x04                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_05_wrapper:
        push    ax
        mov     ax, #0x05                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_06_wrapper:
        push    ax
        mov     ax, #0x06                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_07_wrapper:
        push    ax
        mov     ax, #0x07                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_08_wrapper:
        push    ax
        mov     ax, #0x08                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_09_wrapper:
        push    ax
        mov     ax, #0x09                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0A_wrapper:
        push    ax
        mov     ax, #0x0a                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0B_wrapper:
        push    ax
        mov     ax, #0x0b                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0C_wrapper:
        push    ax
        mov     ax, #0x0c                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0D_wrapper:
        push    ax
        mov     ax, #0x0d                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0E_wrapper:
        push    ax
        mov     ax, #0x0e                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_0F_wrapper:
        push    ax
        mov     ax, #0x0f                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_10_wrapper:
        push    ax
        mov     ax, #0x10                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_11_wrapper:
        push    ax
        mov     ax, #0x11                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_12_wrapper:
        push    ax
        mov     ax, #0x12                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_13_wrapper:
        push    ax
        mov     ax, #0x13                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_14_wrapper:
        push    ax
        mov     ax, #0x14                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_15_wrapper:
        push    ax
        mov     ax, #0x15                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_16_wrapper:
        push    ax
        mov     ax, #0x16                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_17_wrapper:
        push    ax
        mov     ax, #0x17                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_18_wrapper:
        push    ax
        mov     ax, #0x18                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_19_wrapper:
        push    ax
        mov     ax, #0x19                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1A_wrapper:
        push    ax
        mov     ax, #0x1a                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1B_wrapper:
        push    ax
        mov     ax, #0x1b                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1C_wrapper:
        push    ax
        mov     ax, #0x1c                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1D_wrapper:
        push    ax
        mov     ax, #0x1d                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1E_wrapper:
        push    ax
        mov     ax, #0x1e                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_1F_wrapper:
        push    ax
        mov     ax, #0x1f                  ; save exception number
        jmp     _exc_common             ; jump to the common code
_isr_20_wrapper:
        push    ds
        push    es                      ; saving segment registers and
        pushad                          ; other regs because it's an ISR
        mov     bx, #0x10
        mov     ds, bx
        mov     es, bx                  ; load ds and es with valid selector
        call    _timer_handler          ; call actual ISR code
        popad                           ; restoring the regs
        pop     es
        pop     ds
        iretd

_isr_21_wrapper:
        push    ds
        push    es                      ; saving segment registers and
        pushad                          ; other regs because it's an ISR
        mov     bx, #0x10
        mov     ds, bx
        mov     es, bx                  ; load ds and es with valid selector
        call    _kbd_handler            ; call actual ISR code
        popad                           ; restoring the regs
        pop     es
        pop     ds
        iretd

_call_gate_wrapper:
        push    ds
        push    es                      ; saving segment registers and
        pushad                          ; other regs because it's an ISR
        call    _call_gate_proc
        popad                           ; restoring the regs
        pop     es
        pop     ds
        retf

.DATA ;PUBLIC CLASS=DATA
        _exc_has_error:  db      0,0,0,0,0,0,0,0, 1,0,1,1,1,1,1,0
                        db      0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0

