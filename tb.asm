
        org     0
        mov     cx, 2
L0:     loop    L0
        jcxz    L1
        mov     ax, $AAAA
L1:     mov     ax, $FFFF
        hlt
A1:     dw      $BFEA

