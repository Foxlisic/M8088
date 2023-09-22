
        org     0

        cli                         ; Стандартный сброс
        xor     ax, ax              ; SS=0, SP=1000h
        mov     ss, ax
        mov     sp, $1000
        mov     ax, $B800
        mov     es, ax              ; ES=B800
        mov     ax, cs
        mov     ds, ax              ; DS=CS

        ; Кошатина
        xor     di, di
        mov     ah, $07
        mov     cx, 667
@@:     mov     al, '-'
        stosw
        mov     al, '_'
        stosw
        stosw
        loop    @b
        jmp     $
