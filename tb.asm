
        org     0

        mov     di, L2
        mov     si, L1
        mov     ax, 2
        scasw
        ;rep lodsw
        hlt
        push    ax
        pop     word [bx]
        shl     ax, cl
L1:     dw      $AECE, $FABE
L2:     dw      $AEFA
