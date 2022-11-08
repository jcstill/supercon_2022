;  
; fibonacci - this will display the (8 bit) fibonacci sequence on the first
; row of the display
; 
; Copyright (C) 2022 Jacob Still jacobcstill@gmail.com
; 
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
; 
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; 

; initialize the display
        ; set the led matrix to display page C+1 and C
        ; this will be our "MSB" (C+1) and "LSB" (C)
        mov     R0,0xC          ; set R0 to 0xC
        mov     [0xF0],R0       ; move value in R0 to data address 0xF0 (screen)
        ; clear the first line on the display
        mov     R0,0x0          ; set R0 to 0x0
        mov     [0xC0],R0       ; move value in R0 to data address 0xC0
        mov     [0xD0],R0       ; move value in R0 to data address 0xD0

; initialize DATA
        ; Jump here to reset fibonacci sequence
        mov     [0x20],R0       ; move value in R0 to data address 0x20
        mov     [0x21],R0       ; move value in R0 to data address 0x21
        mov     [0x22],R0       ; move value in R0 to data address 0x22
        mov     R0,0x1          ; set R0 to 0x1
        mov     [0x23],R0       ; move value in R0 to data address 0x23

        mov     R0,0x9          ; set R0 to 0x9
        mov     [0xF1],R0       ; move value in R0 to data address 0xF1 (clock)

; start loop
        ; jump here to get next number in sequence
        ; get data from 20:21 and 22:23 into registers
        mov     R0,[0x21]       ; move value in data address 0x21 to R0
        mov     R1,R0           ; move value in R0 to R1
        mov     R0,[0x22]       ; move value in data address 0x22 to R0
        mov     R2,R0           ; move value in R0 to R2
        mov     R0,[0x23]       ; move value in data address 0x23 to R0
        mov     R3,R0           ; move value in R0 to R3
        mov     R0,[0x20]       ; move value in data address 0x20 to R0
        ; add the two numbers
        add     R1,R3           ; fist 4 bits add
        adc     R0,R2           ; second 4 bits add (with carry)
        ; store result in D0:C0
        mov     [0xD0],R0       ; move value in R0 to data address 0xD0
        mov     R0,R1           ; move value in R1 to R0
        mov     [0xC0],R0       ; move value in R0 to data address 0xC0
        ; shift data in 22:23 to 20:21
        mov     R0,[0x22]       ; move value in data address 0x22 to R0
        mov     [0x20],R0       ; move value in R0 to data address 0x20
        mov     R0,[0x23]       ; move value in data address 0x23 to R0
        mov     [0x21],R0       ; move value in R0 to data address 0x21
        ; copy data in D0:C0 to 22:23
        mov     R0,[0xD0]       ; move value in data address 0xD0 to R0
        mov     [0x22],R0       ; move value in R0 to data address 0x22
        mov     R0,[0xC0]       ; move value in data address 0xC0 to R0
        mov     [0x23],R0       ; move value in R0 to data address 0x23
        ; conditional logic
        skip    nc,#1           ; skip the next 1 line if carry not set
        jr      -27             ; jump back 27 instructions (inclusive)
        jr      -23             ; jump back 23 instructions (inclusive)

