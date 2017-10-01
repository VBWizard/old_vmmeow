/* filename : isr_wrapper.asm */
.intel_syntax noprefix
#.globl   _isr_wrapper
.align   4
.code32

.extern timer_handler
.extern	kbd_handler
.extern call_gate_proc
.extern exc_CS
.extern exc_IP

.global _isr_common
_isr_common:
        push    bp
        mov     bp, sp
        push    ds
        push    es                      # saving segment registers and
        pushad                          # other regs because its an ISR
        mov     bx, 0x38
        mov     ds, bx
        mov     es, bx                  # load ds and es with valid selector
		mov bx, [bp+8]
		mov _exc_CS, bx
		mov bx, [bp+4]
		mov _exc_IP, bx
        mov     bx, ax
		mov	    _exception_number, ax
	add	bx, _isr_has_error
	mov	al,byte ptr [bx]
        cmp     al,0
        je      .isr1
	SS
        pushw     [bp+4]          # error code
	SS
        pushd     [bp+8]         # ip
	SS
        pushw    [bp+12]         # cs
        jmp     .isr2
.isr1:
		mov		bx,0
        pushw    bx                  # error code
        SS
	pushd    [bp+4]         # ip
	SS
	pushw    [bp+8]          # cs
.isr2:
        pushw    ax                      # exception no
# void exc_handler (no, cs, ip, error)
        call    host_isr_handler            # call actual ISR code
        add     sp, 10
        popad                           # restoring the regs
        pop     es
        pop     ds
        pop     bp
        pop     ax
        pop		bp
        iret

.global _isr_00_wrapper        
_isr_00_wrapper:
        push    ax
        mov     ax, 0x00                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_01_wrapper        
_isr_01_wrapper:
        push    ax
        mov     ax, 0x01                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_02_wrapper        
_isr_02_wrapper:
        push    ax
        mov     ax, 0x02                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_03_wrapper        
_isr_03_wrapper:
        push    ax
        mov     ax, 0x03                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_04_wrapper        
_isr_04_wrapper:
        push    ax
        mov     ax, 0x04                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_05_wrapper        
_isr_05_wrapper:
        push    ax
        mov     ax, 0x05                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_06_wrapper        
_isr_06_wrapper:
        push    ax
        mov     ax, 0x06                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_07_wrapper        
_isr_07_wrapper:
        push    ax
        mov     ax, 0x07                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_08_wrapper        
_isr_08_wrapper:
        push    ds
        push    es                      # saving segment registers and
	push    gs
        pushad                          # other regs because its an ISR
        mov     bx, 0x10
        mov     ds, bx
        mov     es, bx                  # load ds and es with valid selector
	mov	bx, 0x38
	mov     gs, bx
        call    timer_handler          # call actual ISR code
        popad                           # restoring the regs
	pop	gs
        pop     es
        pop     ds
        iretd
.global _isr_09_wrapper        
_isr_09_wrapper:
        push    ds
        push    es                      # saving segment registers and
        pushad                          # other regs because its an ISR
        mov     bx, 0x10
        mov     ds, bx
        mov     es, bx                  # load ds and es with valid selector
        call    kbd_handler            # call actual ISR code
        popad                           # restoring the regs
        pop     es
        pop     ds
        iretd
.global _isr_0A_wrapper        
_isr_0A_wrapper:
        push    ax
        mov     ax, 0x0a                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_0B_wrapper        
_isr_0B_wrapper:
        push    ax
        mov     ax, 0x0b                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_0C_wrapper        
_isr_0C_wrapper:
        push    ax
        mov     ax, 0x0c                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_0D_wrapper        
_isr_0D_wrapper:
        push    ax
        mov     ax, 0x0d                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_0E_wrapper        
_isr_0E_wrapper:
        push    ax
        mov     ax, 0x0e                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_0F_wrapper        
_isr_0F_wrapper:
        push    ax
        mov     ax, 0x0f                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_10_wrapper        
_isr_10_wrapper:
        push    ax
        mov     ax, 0x10                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_11_wrapper        
_isr_11_wrapper:
        push    ax
        mov     ax, 0x11                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_12_wrapper        
_isr_12_wrapper:
        push    ax
        mov     ax, 0x12                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_13_wrapper        
_isr_13_wrapper:
        push    ax
        mov     ax, 0x13                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_14_wrapper        
_isr_14_wrapper:
        push    ax
        mov     ax, 0x14                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_15_wrapper        
_isr_15_wrapper:
        push    ax
        mov     ax, 0x15                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_16_wrapper        
_isr_16_wrapper:
        push    ax
        mov     ax, 0x16                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_17_wrapper        
_isr_17_wrapper:
        push    ax
        mov     ax, 0x17                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_18_wrapper        
_isr_18_wrapper:
        push    ax
        mov     ax, 0x18                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_19_wrapper        
_isr_19_wrapper:
        push    ax
        mov     ax, 0x19                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1A_wrapper        
_isr_1A_wrapper:
        push    ax
        mov     ax, 0x1a                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1B_wrapper        
_isr_1B_wrapper:
        push    ax
        mov     ax, 0x1b                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1C_wrapper        
_isr_1C_wrapper:
        push    ax
        mov     ax, 0x1c                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1D_wrapper        
_isr_1D_wrapper:
        push    ax
        mov     ax, 0x1d                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1E_wrapper        
_isr_1E_wrapper:
        push    ax
        mov     ax, 0x1e                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_1F_wrapper        
_isr_1F_wrapper:
        push    ax
        mov     ax, 0x1f                  # save exception number
        jmp     _isr_common             # jump to the common code
.global _isr_20_wrapper        
_isr_20_wrapper:
        push    ds
        push    es                      # saving segment registers and
        pushad                          # other regs because its an ISR
        mov     bx, 0x10
        mov     ds, bx
        mov     es, bx                  # load ds and es with valid selector
        call    timer_handler          # call actual ISR code
        popad                           # restoring the regs
        pop     es
        pop     ds
        iretd

.global _isr_21_wrapper        
_isr_21_wrapper:
        push    ds
        push    es                      # saving segment registers and
        pushad                          # other regs because its an ISR
        mov     bx, 0x10
        mov     ds, bx
        mov     es, bx                  # load ds and es with valid selector
        call    kbd_handler            # call actual ISR code
        popad                           # restoring the regs
        pop     es
        pop     ds
        iretd

.global _call_gate_wrapper
_call_gate_wrapper:
        push    ds
        push    es                      # saving segment registers and
        pushad                          # other regs because its an ISR
        call    call_gate_proc
        popad                           # restoring the regs
        pop     es
        pop     ds
        retf

        _isr_has_error:  .byte      0,0,0,0,0,0,0,0, 1,0,1,1,1,1,1,0
                        .byte      0,1,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
.global _exception_number
_exception_number: .word 0
.global _ltr
_ltr:
		push ax
		mov ax, 0x58
        ltr     ax
        pop ax
        ret

