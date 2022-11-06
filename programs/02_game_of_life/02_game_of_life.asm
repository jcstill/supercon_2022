        ; Set processor speed (temporary)
        mov R0,8
        mov [0xf1],R0
        GOTO init
setter: ;(temporary)
        mov     R0,[0xF0]
        add     R0,R6
        mov     R3,R0
        mov     R0,[R3:R8]
        or      R0,r7
        mov     [R3:R8],R0
        ret     R0,0

init:
        ; set the led matrix to display page C+1 and C
        mov     R0,0xC          ; set R0 to 0xC
        mov     [0xF0],R0       ; move value in R0 to data address 0xF0

        ; init for random data
        mov     R0,[0xF0]       ; get the display address
        mov     R9,0x6          ; move 0x6 address to R9
        xor     R9,R0           ; xor the display address in R0 and 0x6 in R0 to get the "procssing address" (0xA in this case)
        mov     R6,R0           ; move processing address into R6
        mov     R5,0x0          ; load 0 to R5

        ; write random data to the display
        mov     R0,[0xFF]       ; Read random number
        mov     [R6:R5],R0      ; Move Random into page n
        mov     R7,R6           ; copy page n
        inc     R7              ; get page n + 1
        mov     R0,[0xFF]       ; Read Random number
        mov     [R7:R5],R0      ; Move Random into page n + 1
        inc     R5              ; Move to next row
        skip    C,1             ; Skip after row 15
        jr      -9              ; Loop forever

main:
        ; The address ranges 0xC0:0xDF and 0xA0:0xBF are either a frame being displayed
        ; or a new frame being proccessed. Since the address of the frame being
        ; proccessed is dynamic, we need to read the from the page register (0xF0) and 
        ; xor it with 0x6 to get the address of the new frame to process. the address of
        ; the new frame is stored in register 9 and updated every frame.
        ;
        ; Data in the page register (0xF0) is xor'd with 0x6 and written back
        ;
        ; The page register (0xF0) is initialized to 0xC. The xor with 0x6 will give us
        ; 0xA - the address of our next frame.

        ; get the address to start proccessing as the new frame (store in R9)
        mov     R0,[0xF0]       ; move value in data address 0xF0 to R0
        mov     R9,0x6          ; set R9 to 0x6
        xor     R9,R0           ; xor the data in R9 and the data in R0

        ; do frame proccessing here - soubroutine??
        GOSUB   iter

        ; switch the display to the new frame by "exchanging" the page we are viewing
        mov     R0,R9
        mov     [0xF0],R0       ; move value in R0 to data address 0xF0

        GOTO    main            ; Return to top of loop

        ; iter subroutine
        ; [0xF0] - page n
        ; R9 - dest
        ; R8 - row
        ; R7 - col
        ; R6 - page offset
iter:
        mov     R8, 0           ; init row in R8
        mov     R7, 0           ; init col in R7
        mov     R0, [0xF0]      ; move from SFR (page) to R0
iter_row:
        or      R0, 0           ; just set carry flag HACK
iter_bit1:
        rrc     R7              ; Roll R7 with carry
        skip    nc,1            ; skip if jump if looped
        jr      6
        mov     R6,1            ; start with page n + 1
        GOSUB   rules           ; jump to ckeck bit in page n + 1
        and     R0,0xf          ; clear carry
        GOTO    iter_bit1       ; jump back to top
iter_bit0:
        rrc     R7              ; Roll R7 with carry
        skip    nc,1            ; skip if jump if looped
        jr      6
        mov     R6,0            ; page n
        GOSUB   rules           ; jump to ckeck bit in page n
        and     R0,0xf          ; clear carry
        GOTO    iter_bit0       ; jump back to top
        inc     R8              ; increment col
        skip    c,2             ; skip GOTO if cary
        GOTO    iter_row        ; GOTO top of row iteration
        ret     R0,0            ; end - change to return from subroutine

; rules subroutine
; [0xF0] - page n
; R9 - dest
; R8 - row
; R7 - col
; R6 - page offset
; R2 - edge cond nibble (b3 b2 b1 b0) -> (Top Bottom Left Right)
rules:
        GOSUB   setter

        mov     R2,0            ; init edge cond nibble

        ; check top edge cond
        mov     R0,R8           ; move row into R0
        cp      R0,0            ; check top edge cond
        skip    nz,2            ; skip 2 inst if not zero
        bset    R2,3            ; set top edge cond bit
        jr      3               ; jump to page offset check

        ; check bottom edge cond
        cp      R0,15           ; check bottom edge cond
        skip    nz,1            ; skip 1 inst if not zero
        bset    R2,2            ; set bottom edge cond bit

        ; dispatch based on page offset
        mov     R0,R6           ; move page offset intio R0
        cp      R0,1            ; check page offset
        skip    z,1             ; skip 1 inst if zero
        jr      5               ; jump to check right edge cond

        ; check left edge cond
        mov     R0,R7           ; move col into R0
        cp      R0,8            ; check left edge cond
        skip    nz,2            ; skip 2 inst if not zero
        bset    R2,1            ; set left edge cond
        jr      3               ; jump to applying rules

        ; check right edge cond
        cp      R0,1            ; check right edge cond
        skip    nz,1            ; skip 1 inst if not zero
        bset    R2,0            ; set right edge cond

        ; applying rules
        ret     R0,0
