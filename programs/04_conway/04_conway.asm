; mask_tbl
; 1. Page 5 contains col bit-masks for bit-status
; 2. Page 4 contains left nibble masks
; 3. Page 3 contains right nibble masks
; 4. Row in mask table corresponds to column
mask_tbl:
        ; col 8 masks
        mov     R0,0b1000       ; col bit-mask
        mov     [5:8],R0
        mov     R0,0b1100       ; left nibble mask
        mov     [4:8],R0
        mov     R0,0b0001       ; right nibble mask
        mov     [3:8],R0

        ; column 7 masks
        mov     R0,0b0100       ; col bit-mask
        mov     [5:7],R0
        mov     R0,0b1110       ; left nibble mask
        mov     [4:7],R0
        mov     R0,0b0000       ; right nibble mask
        mov     [3:7],R0

        ; column 6 masks
        mov     R0,0b0010       ; col bit-mask
        mov     [5:6],R0
        mov     R0,0b0111       ; left nibble mask
        mov     [4:6],R0
        mov     R0,0b0000       ; right nibble mask
        mov     [3:6],R0

        ; column 5 masks
        mov     R0,0b0001       ; col bit-mask
        mov     [5:5],R0
        mov     R0,0b0011       ; left nibble mask
        mov     [4:5],R0
        mov     R0,0b1000       ; right nibble mask
        mov     [3:5],R0

        ; column 4 masks
        mov     R0,0b1000       ; col bit-mask
        mov     [5:4],R0
        mov     R0,0b0001       ; left nibble mask
        mov     [4:4],R0
        mov     R0,0b1100       ; right nibble mask
        mov     [3:4],R0

        ; column 3 masks
        mov     R0,0b0100       ; col bit-mask
        mov     [5:3],R0
        mov     R0,0b0000       ; left nibble mask
        mov     [4:3],R0
        mov     R0,0b1110       ; right nibble mask
        mov     [3:3],R0

        ; column 2 masks
        mov     R0,0b0010       ; col bit-mask
        mov     [5:2],R0
        mov     R0,0b0000       ; left nibble mask
        mov     [4:2],R0
        mov     R0,0b0111       ; right nibble mask
        mov     [3:2],R0

        ; column 1 masks
        mov     R0,0b0001       ; col bit-mask
        mov     [5:1],R0
        mov     R0,0b1000       ; left nibble mask
        mov     [4:1],R0
        mov     R0,0b0011       ; right nibble mask
        mov     [3:1],R0

; init subroutine
; [0xF0] - page n (src)
; R9 - dest page n
; R8 - dest page n+1
; R7 - src page n
; R6 - src page n+1
init:
        ; set the led matrix to display pages 0xC0 and 0xD0
        mov     R0,0xC          ; set R0 to 0xC
        mov     [0xF0],R0       ; move value in R0 to data address 0xF0

        mov     R0,[0xF0]       ; move disp addr into R0
        mov     R9,6            ; move 6 into R9
        xor     R9,R0           ; xor R0 and R9 to get dest page n (0xA)
        mov     R8,R9           ; move R9 into R8
        inc     R8              ; incr to get dest page n+1 (0xB)
        mov     R7,R0           ; move R0 into R7 to get src page n (0xC)
        mov     R6,R7           ; move R7 into R6
        inc     R6              ; incr to get src page n+1 (0xD)

        ; write random data to the display
        mov     R5,0            ; move 0 into R5
        mov     R0,[0xFF]       ; Read random number
        mov     [R7:R5],R0      ; Move Random into page n
        mov     R0,[0xFF]       ; Read Random number
        mov     [R6:R5],R0      ; Move Random into page n + 1
        inc     R5              ; Move to next row
        skip    C,1             ; Skip after row 15
        jr      -7              ; Loop to next row

main:
        ; The address ranges 0xC0:0xDF and 0xA0:0xBF are either a frame being
        ; displayed or a new frame being proccessed. Since the address of the
        ; frame being proccessed is dynamic, we need to read the from the page
        ; register (0xF0) and xor it with 0x6 to get the address of the new
        ; frame to process. the address of the new frame is stored in R9 and
        ; updated every frame.
        ;
        ; Data in the page register (0xF0) is xor'd with 0x6 and written back.
        ; The page register (0xF0) is initialized to 0xC. The xor with 0x6 will
        ; give us 0xA - the address of our next frame.

        ; get the address to start proccessing as the new frame (store in R9)
        mov     R0,[0xF0]       ; move value in data address 0xF0 to R0
        mov     R9,0x6          ; set R9 to 0x6
        xor     R9,R0           ; xor the data in R9 and the data in R0

        ; do frame proccessing here
        GOSUB   iter

        ; exchange buffer display by exchanging the page we are viewing
        mov     R0,R9
        mov     [0xF0],R0       ; move value in R0 to data address 0xF0

        GOTO    main            ; Return to top of loop

; iter subroutine
; [0xF0] - page n (src)
; R9 - dest page n
; R8 - dest page n+1
; R7 - src page n (same as [0xF0])
; R6 - src page n+1
; R5 - row
; R4 - row above
; R3 - row below
; R2 - col
; E0:E5 -
;
;                  Page n+1    (Top)    Page n
;           ┌────┬────┬────┬────┐┌────┬────┬────┬────┐
;        E0 │ b3*│ b2*│ b1 │ b0 ││ b3 │ b2 │ b1 │ b0*│ E1           <-- above
;           ├────┼────┼────┼────┤├────┼────┼────┼────┤
; (Left) E2 │ b3*│ b2*│ b1 │ b0 ││ b3 │ b2 │ b1 │ b0*│ E3 (Right)   <-- row
;           ├────┼────┼────┼────┤├────┼────┼────┼────┤
;        E4 │ b3*│ b2*│ b1 │ b0 ││ b3 │ b2 │ b1 │ b0*│ E5           <-- below
;           └────┴────┴────┴────┘└────┴────┴────┴────┘
;             ∧              (Bottom)
;             │
;            col #8 (1-based index)
;
; 1. Apply mask to all six registers based on col index
; 2. Use popcount subroutine to count neighboring bits in all 6 registers
; 3. Apply rules to determine bit value in dest pages
iter:
        ; init src, dest, and row regs
        mov     R8, R9          ; move dest page n into R8
        inc     R8              ; incr to set dest page n+1
        mov     R0,[0xF0]       ; move src page n into R0
        mov     R7,R0           ; move R0 into R7 for src page n
        mov     R6,R7           ; move R7 into R6
        inc     R6              ; incr R6 to set src page n+1
        mov     R5,0            ; init row reg (R5) to zero

iter_row:
        ; init above, below, and col reg
        mov     R4,R5           ; move R5 into R4
        dec     R4              ; decr R4 to set row above (use rollover)
        mov     R3,R5           ; move R5 into R3
        inc     R3              ; incr R3 to set row below (use rollover)
        mov     R2,8            ; init col reg (left-to-right iter)

        ; populate E0 through E5 registers
        mov     R0,[R6:R4]      ; get top left nibble
        mov     [0xE0],R0       ; move into E0
        mov     R0,[R7:R4]      ; get top right nibble
        mov     [0xE1],R0       ; move into E1
        mov     R0,[R6:R5]      ; get left nibble
        mov     [0xE2],R0       ; move into E2
        mov     R0,[R7:R5]      ; get right nibble
        mov     [0xE3],R0       ; move into E3
        mov     R0,[R6:R3]      ; get bottom-left nibble
        mov     [0xE4],R0       ; move into E4
        mov     R0,[R7:R3]      ; get bottom-right nibble
        mov     [0xE5],R0       ; move into E5

iter_col:
        ; apply left nibble mask based on col for E0, E2, E4
        mov     R1,4            ; move 4 into R1
        mov     R0,[R1:R2]      ; move left nibble mask into R0
        mov     R1,R0           ; move left nibble mask from R0 to R1
        mov     R0,[0xE0]       ; move E0 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xE7],R0       ; move masked E0 nibble into E7
        mov     R0,[0xE2]       ; move E2 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xE9],R0       ; move masked E2 nibble into E9
        mov     R0,[0xE4]       ; move E4 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xEB],R0       ; move masked E4 nibble into EB

        ; apply right nibble mask based on col for E1, E3, E5
        mov     R1,3            ; move 3 into R1
        mov     R0,[R1:R2]      ; move right nibble mask into R0
        mov     R1,R0           ; move right nibble mask from R0 to R1
        mov     R0,[0xE1]       ; move E1 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xE8],R0       ; move masked E1 nibble into E8
        mov     R0,[0xE3]       ; move E3 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xEA],R0       ; move masked E3 nibble into EA
        mov     R0,[0xE5]       ; move E5 into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xEC],R0       ; move masked E5 nibble into EC

        ; get popcount for reg E7, store in reg E7
        mov     R0,[0xE7]       ; move E7 to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of E7
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xE7],R0       ; move R0 to E7

        ; get popcount for reg E8, store in reg E8
        mov     R0,[0xE8]       ; move E8 to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of E8
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xE8],R0       ; move R0 to E8

        ; get popcount for reg E9, store in reg E9
        mov     R0,[0xE9]       ; move E9 to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of E9
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xE9],R0       ; move R0 to E9

        ; get popcount for reg EA, store in reg EA
        mov     R0,[0xEA]       ; move EA to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of EA
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xEA],R0       ; move R0 to EA

        ; get popcount for reg EB, store in reg EB
        mov     R0,[0xEB]       ; move EB to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of EB
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xEB],R0       ; move R0 to EB

        ; get popcount for reg EC, store in EC
        mov     R0,[0xEC]       ; move EC to R0
        mov     [0xE6],R0       ; move R0 to E6
        GOSUB   popcount        ; popcount of EC
        mov     R0,[0xE6]       ; move E6 to R0
        mov     [0xEC],R0       ; move R0 to EC

        ; sum registers E7 through EC, store sum in ED
        mov     R1,0            ; init R1 to zero (accumulator)
        mov     R0,[0xE7]       ; move E7 into R0
        add     R1,R0           ; R1 <- R1 + E7
        mov     R0,[0xE8]       ; move E8 into R0
        add     R1,R0           ; R1 <- R1 + E8
        mov     R0,[0xE9]       ; move E9 into R0
        add     R1,R0           ; R1 <- R1 + E9
        mov     R0,[0xEA]       ; move EA into R0
        add     R1,R0           ; R1 <- R1 + EA
        mov     R0,[0xEB]       ; move EB into R0
        add     R1,R0           ; R1 <- R1 + EB
        mov     R0,[0xEC]       ; move EC into R0
        add     R1,R0           ; R1 <- R1 + EC
        mov     R0,R1           ; move sum into R0
        mov     [0xED],R0       ; move R0 into ED

        ; given row and col regs, store bit status in EE
        mov     R0,R2           ; move col reg into R0
        cp      R0,5            ; R0 - 5
        skip    C,2             ; if R0 >= 5, then skip next 2 inst
        mov     R0,[R7:R5]      ; move page n nibble into R0
        jr      1               ; skip next inst
        mov     R0,[R6:R5]      ; move page n+1 nibble into R0
        mov     R1,R0           ; move current nibble into R1
        mov     R0,5            ; move 7 into R0
        mov     R0,[R0:R2]      ; move col bit-mask into R0
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [0xEE],R0       ; move R0 into EE

; egocentric rule #1: if all-field sum is 3, then next inner-field state is life
rule1:
        mov     R0,[0xED]       ; move count into R0
        cp      R0,3            ; R0 - 3
        skip    Z,2             ; if R0 = 3, then skip next 2 inst
        GOTO    rule2           ; else goto rule2
        GOTO    set_bit         ; set bit

; egocentric rule #2: if all-field sum is 4, then inner-field state is constant
rule2:
        cp      R0,4            ; R0 - 4
        skip    Z,2             ; if R0 = 4, then skip next 2 inst
        GOTO    rule3           ; else goto rule3
        mov     R0,[0xEE]       ; load bit-status
        cp      R0,0            ; R0 - 0
        skip    NZ,2            ; if R0 > 0, then skip next 2 inst
        GOTO    clear_bit       ; else clear bit
        GOTO    set_bit         ; set bit

; egocentric rule #3; for every other sum, next inner-field state is death
rule3:
        GOTO    clear_bit

set_bit:
        mov     R0,R2           ; move col reg into R0
        cp      R0,5            ; R0 - 5
        skip    C,2             ; if R0 >= 5, then skip next 2 inst
        GOTO    set_bit_pg0     ; set bit in page n
        GOTO    set_bit_pg1     ; set bit in page n+1

set_bit_pg0:
        mov     R0,[R9:R5]      ; move dest page n nibble into R0
        mov     R1,R0           ; move R0 into R1
        mov     R0,5            ; move 5 into R0
        mov     R0,[R0:R2]      ; move col bit-mask into R0
        or      R0,R1           ; R0 <- R0 bitwise-or R1
        mov     [R9:R5],R0      ; move R0 into dest page n nibble
        GOTO    next_iter

set_bit_pg1:
        mov     R0,[R8:R5]      ; move dest page n+1 nibble into R0
        mov     R1,R0           ; move R0 into R1
        mov     R0,5            ; move 5 into R0
        mov     R0,[R0:R2]      ; move col bit-mask into R0
        or      R0,R1           ; R0 <- R0 bitwise-or R1
        mov     [R8:R5],R0      ; move R0 into dest page n+1 nibble
        GOTO    next_iter

clear_bit:
        mov     R0,R2           ; move col reg into R0
        cp      R0,5            ; R0 - 5
        skip    C,2             ; if R0 >= 5, then skip next 2 inst
        GOTO    clear_bit_pg0   ; clear bit in page n
        GOTO    clear_bit_pg1   ; clear bit in page n+1

clear_bit_pg0:
        mov     R0,[R9:R5]      ; move dest page n nibble into R0
        mov     R1,R0           ; move R0 into R1
        mov     R0,5            ; move 5 into R0
        mov     R0,[R0:R2]      ; move col bit-mask into R0
        xor     R0,0xF          ; bitwise-compl of col bit-mask
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [R9:R5],R0      ; move R0 into dest page n nibble
        GOTO    next_iter

clear_bit_pg1:
        mov     R0,[R8:R5]      ; move dest page n+1 nibble into R0
        mov     R1,R0           ; move R0 into R1
        mov     R0,5            ; move 5 into R0
        mov     R0,[R0:R2]      ; move col bit-mask into R0
        xor     R0,0xF          ; bitwise-compl of col bit-mask
        and     R0,R1           ; R0 <- R0 bitwise-and R1
        mov     [R8:R5],R0      ; move R0 into dest page n+1 nibble
        GOTO    next_iter

next_iter:
        ; move to next col
        dec     R2              ; decr col reg
        skip    Z,2             ; skip if col reg is zero
        GOTO    iter_col

        ; move to next row
        inc     R5              ; incr row reg
        skip    C,2             ; skip if carry reg rollover
        GOTO    iter_row

        ; iter subroutine return
        ret     R0,0

; popcount subroutine
; 1. get popcount of reg E6
; 2. put result in reg E6
popcount:
        mov     R0,[0xE6]       ; move E6 into R0
        and     R0,0x5          ; mask unshifted bits
        mov     R1,R0           ; move R0 into R1
        mov     R0,[0xE6]       ; move E6 into R0
        and     R0,0xF          ; clear carry flag
        rrc     R0              ; roll R0
        and     R0,0x5          ; mask shifted bits
        add     R0,R1           ; R0 <- R0 + R1
        mov     [0xE6],R0       ; move R0 into E6
        and     R0,0x3          ; mask unshifted bits
        mov     R1,R0           ; move R0 into R1
        mov     R0,[0xE6]       ; move E6 into R0
        and     R0,0xF          ; clear carry flag
        rrc     R0              ; roll R0
        rrc     R0              ; roll R0
        and     R0,0x3          ; mask shifted bits
        add     R0,R1           ; R0 <- R0 + R1
        mov     [0xE6],R0       ; move R0 into E6
        ret     R0,0            ; popcount subroutine return
