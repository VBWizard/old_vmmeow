ASM_START
  ;; and function
  landl:
  landul:
    SEG SS
      and ax,[di]
    SEG SS
      and bx,2[di]
    ret

  ;; add function
  laddl:
  laddul:
    SEG SS
      add ax,[di]
    SEG SS
      adc bx,2[di]
    ret

  ;; cmp function
  lcmpl:
  lcmpul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    shr ebx, #16
    SEG SS
      cmp eax, dword ptr [di]
    ret

  ;; sub function
  lsubl:
  lsubul:
    SEG SS
    sub ax,[di]
    SEG SS
    sbb bx,2[di]
    ret

  ;; mul function
  lmull:
  lmulul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    SEG SS
    mul eax, dword ptr [di]
    mov ebx, eax
    shr ebx, #16
    ret

  ;; dec function
  ldecl:
  ldecul:
    SEG SS
    dec dword ptr [bx]
    ret

  ;; or function
  lorl:
  lorul:
    SEG SS
    or  ax,[di]
    SEG SS
    or  bx,2[di]
    ret

  ;; inc function
  lincl:
  lincul:
    SEG SS
    inc dword ptr [bx]
    ret

  ;; tst function
  ltstl:
  ltstul:
    and eax, #0x0000FFFF
    shl ebx, #16
    or  eax, ebx
    shr ebx, #16
    test eax, eax
    ret

  ;; sr function
  lsrul:
    mov  cx,di
    jcxz lsr_exit
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
  lsr_loop:
    shr  eax, #1
    loop lsr_loop
    mov  ebx, eax
    shr  ebx, #16
  lsr_exit:
    ret

  ;; sl function
  lsll:
  lslul:
    mov  cx,di
    jcxz lsl_exit
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
  lsl_loop:
    shl  eax, #1
    loop lsl_loop
    mov  ebx, eax
    shr  ebx, #16
  lsl_exit:
    ret

  idiv_:
    cwd
    idiv bx
    ret

  idiv_u:
    xor dx,dx
    div bx
    ret

  ldivul:
    and  eax, #0x0000FFFF
    shl  ebx, #16
    or   eax, ebx
    xor  edx, edx
    SEG SS
    mov  bx,  2[di]
    shl  ebx, #16
    SEG SS
    mov  bx,  [di]
    div  ebx
    mov  ebx, eax
    shr  ebx, #16
    ret

    ;; imodu doesnt preserve dx (returns quotient in it)
imodu:
	xor	dx,dx
	div	bx
	mov	ax,dx		
	ret

ASM_END
