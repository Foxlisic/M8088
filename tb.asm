
        org     0
        into
        lds     sp, [A1]
        xlatb
        test    ax, $0000
        add     word [A1], -1
        hlt
A1:     dw      $4004
        dw      $BEEF
L1:     retf

