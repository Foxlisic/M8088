
        org     0
        mov     bl, $11
        add     word [A1], -1
        hlt
A1:     dw      $BFEA
L1:     ret     2

