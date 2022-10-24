// 
// fibonacci - this will display the (8 bit) fibonacci sequence on the first
// row of the display
// 
// Copyright (C) 2022 Jacob Still jacobcstill@gmail.com
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 

// initialize the display
        // set the led matrix to display page C+1 and C
        // this will be our "MSB" (C+1) and "LSB" (C)
        mov     R0,0xC          // 0x90C        // set R0 to 0xC
        mov     [0xF0],R0       // 0xCF0        // move value in R0 to data address 0xF0
        // clear the first line on the display
        mov     R0,0x0          // 0x900        // set R0 to 0x0
        mov     [0xC0],R0       // 0xCC0        // move value in R0 to data address 0xC0
        mov     [0xD0],R0       // 0xCD0        // move value in R0 to data address 0xD0

// initialize DATA
        // Jump here to reset fibonacci sequence
        mov     [0x20],R0       // 0xC20        // move value in R0 to data address 0x20
        mov     [0x21],R0       // 0xC21        // move value in R0 to data address 0x21
        mov     [0x22],R0       // 0xC22        // move value in R0 to data address 0x22
        mov     R0,0x1          // 0x911        // set R0 to 0x1
        mov     [0x23],R0       // 0xC23        // move value in R0 to data address 0x23

// start loop
        // jump here to get next number in sequence
        // get data from 20:21 and 22:23 into registers
        mov     R0,[0x21]       // 0xD21        // move value in data address 0x21 to R0
        mov     R1,R0           // 0x810        // move value in R0 to R1
        mov     R0,[0x22]       // 0xD22        // move value in data address 0x22 to R0
        mov     R2,R0           // 0x820        // move value in R0 to R2
        mov     R0,[0x23]       // 0xD23        // move value in data address 0x23 to R0
        mov     R3,R0           // 0x830        // move value in R0 to R3
        mov     R0,[0x20]       // 0xD20        // move value in data address 0x20 to R0
        // add the two numbers
        add     R1,R3           // 0x113        // fist 4 bits add
        adc     R0,R2           // 0x202        // second 4 bits add (with carry)
        // store result in D0:C0
        mov     [0xD0],R0       // 0xCD0        // move value in R0 to data address 0xD0
        mov     R0,R1           // 0x801        // move value in R1 to R0
        mov     [0xC0],R0       // 0xCC0        // move value in R0 to data address 0xC0
        // shift data in 22:23 to 20:21
        mov     R0,[0x22]       // 0xD22        // move value in data address 0x22 to R0
        mov     [0x20],R0       // 0xC20        // move value in R0 to data address 0x20
        mov     R0,[0x23]       // 0xD23        // move value in data address 0x23 to R0
        mov     [0x21],R0       // 0xC21        // move value in R0 to data address 0x21
        // copy data in D0:C0 to 22:23
        mov     R0,[0xD0]       // 0xDD0        // move value in data address 0xD0 to R0
        mov     [0x22],R0       // 0xC22        // move value in R0 to data address 0x22
        mov     R0,[0xC0]       // 0xDC0        // move value in data address 0xC0 to R0
        mov     [0x23],R0       // 0xC23        // move value in R0 to data address 0x23
        // conditional logic
        skip    nc,#1           // 0x0F5        // skip the next 1 line if carry not set
        jr      -27             // 0xFE5        // jump back 27 instructions (inclusive)
        jr      -23             // 0xFE9        // jump back 23 instructions (inclusive)
