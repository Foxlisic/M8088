
        org     0

; ------------------------------------------------------------------------------
start:  cli
        xor     ax, ax
        mov     ss, ax
        xor     sp, sp
        mov     ax, $B800
        mov     es, ax
        mov     ax, cs
        mov     ds, ax

        mov     ah, $17
        mov     si, s1
        mov     di, 0

@@:     lodsb
        and     al, al
        je      $
        stosw
        jmp     @b


s1:     db      "HELLO WORLD!",0

; ------------------------------------------------------------------------------
        times   65536-16-$ db 0
        jmp     far $f000:start     ; EA 00 00 00 F0
        times   11 db 0
