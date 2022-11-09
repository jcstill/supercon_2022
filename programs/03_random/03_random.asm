        ;mov R0,[0xFF]
        ;mov [0xF1],R0

        mov r0, 4        ; Set CPU speed
        mov [0xF1], r0   ;

        mov r0, 0b1000
        mov [0xF3],R0

        mov r0, 2        ; Set matrix page
        mov [0xF0], r0   ;

        mov R0,[0xF0]
        mov r2,R0        ; Set memory row and columns 
        mov r3,R0        ;
        inc R3
        mov r4, 0        ;

        mov R0,0xF
        mov [0xF9],R0
        jr 0
        jr 0
        jr 0
        mov r0, [0xFF]   ; Read random number
        mov [r3:r4], r0  ; Move into position on matrix
        mov r0, [0xFF]   ;
        mov [r2:r4], r0  ; Repeat for other side
        mov r0, r4       ;
        inc r4           ; Move to next row
        jr -12           ; Loop forever








