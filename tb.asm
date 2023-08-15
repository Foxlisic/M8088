
        org     0

        mov     dx, $abac
        in      ax, dx
        push    ax
        pop     word [bx]
        shl     ax, cl
L1:     dw      $0102
