;init:
        ; set the led matrix to display page C+1 and C
        ;mov     R0,0xC          ; 0x90C         // set R0 to 0xC
        ;mov     [0xF0],R0       ; 0xCF0         // move value in R0 to data address 0xF0
        
        ;; init for random data
        ;mov     R0,[0xF0]       ; 0xDF0         // get the display address
        ;mov     R9,0x6          ; 0x996         // move 0x6 address to R9
        ;xor     R9,R0           ; 0x790         // xor the display address in R0 and 0x6 in R0 to get the "procssing address" (0xA in this case)
        ;mov     R6,R0           ; 0x860         // move processing address into R6
        ;mov     R5,0x0          ; 0x950         // load 0 to R5

        ;; write random data to the display
        ;mov     R0,[0xFF]       ; 0xDFF         // read from RNG
        ;mov     [R6:R5],R0      ; 0xA64         // write to cell (in memory)
        ;inc     R5              ; 0x025         // increment R5
        ;skip    c,1             ; 0x0F1         // skip if carry
        ;jr      -5              ; 0xFFB         // jump back to do the rest of the first page
        ;inc     R6              ; 0x026         // increment R6
        ;mov     R0,[0xFF]       ; 0xDFF         // read from RNG
        ;mov     [R6:R5],R0      ; 0xA64         // write to cell (in memory)
        ;inc     R5              ; 0x025         // increment R5
        ;skip    c,1             ; 0x0F1         // skip if carry
        ;jr      -5              ; 0xFFB         // jump back to do the rest of the second page
        
        ;ret     R0,0



;start:
;        ; The address ranges 0xC0:0xDF and 0xA0:0xBF are either a frame being displayed
;        ; or a new frame being proccessed. Since the address of the frame being
;        ; proccessed is dynamic, we need to read the from the page register (0xF0) and 
;        ; xor it with 0x6 to get the address of the new frame to process. the address of
;        ; the new frame is stored in register 9 and updated every frame.
;        ;
;        ; Data in the page register (0xF0) is xor'd with 0x6 and written back
;        ;
;        ; The page register (0xF0) is initialized to 0xC. The xor with 0x6 will give us
;        ; 0xA - the address of our next frame.
;
;        ; get the address to start proccessing as the new frame (store in R9)
;        mov     R0,[0xF0]       ; 0xDF0         // move value in data address 0xF0 to R0
;        mov     R9,0x6          ; 0x996         // set R9 to 0x6
;        xor     R9,R0           ; 0x790         // xor the data in R9 and the data in R0
;
;main:
;        ; do frame proccessing here - soubroutine??
;
;
;        ; switch the display to the new frame by "exchanging" the page we are viewing
;        mov     R0,R9
;        mov     [0xF0],R0       ; 0xCF0         // move value in R0 to data address 0xF0
;
;        ; jump to top - just write new PC data - not sure if this is the best way...
;        ; this literally jumps to the third instruction in this program
;        mov     PCH,0x0         ; 0x9F0         // set PCH to 0x0
;        mov     PCM,0x0         ; 0x9E0         // set PCM to 0x0
;        mov     PCL,0x2         ; 0x9D2         // set PCL to 0x2

        mov R0,0xC
        mov [0xF0],R0

        GOSUB   iter
        ret     R0,0




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
        jr      5
        mov     R6,1            ; start with page n + 1
        GOSUB   rules           ; jump to ckeck bit in page n + 1
        GOTO    iter_bit1       ; jump back to top
iter_bit0:
        rrc     R7              ; Roll R7 with carry
        skip    nc,1            ; skip if jump if looped
        jr      5
        mov     R6,0            ; page n
        GOSUB   rules           ; jump to ckeck bit in page n
        GOTO    iter_bit0       ; jump back to top
        inc     R8              ; increment col
        skip    c,2             ; skip GOTO if cary
        GOTO    iter_row        ; GOTO top of row iteration
        ret     R0,0            ; end - change to return from subroutine



; rules subroutine
rules:
        ;jr 0            ; placholder

        ;mov     R0,[R5:R8]      ; get data at memory
        ;and     R0,R7           ; check if bit is set
        ;GOSUB   rules           ; jump to check bit
        ;skip    NZ, 1
        ;jr      -#

        mov     R0,[0xF0]
        add     R0,R6
        mov     R3,R0
        mov     R0,[R3:R8]
        or      R0, r7
        mov     [R3:R8],R0

        ret     R0,0




