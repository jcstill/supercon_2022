// 
// game of life - this will play Conway's Game of Life on the display 
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
        mov     R0,0xC          // 0x90C        // set R0 to 0xC
        mov     [0xF0],R0       // 0xCF0        // move value in R0 to data address 0xF0
        
        // I can't figure out if the RAM is cleared to 0 on program start. If it isn't, 
        // that would be helpful in initializing a random patern on the board - otherwise
        // we might have to use the rand register to initialize the pattern

// start program

        // The address ranges 0xC0:0xDF and 0xA0:0xBF are either a frame being displayed
        // or a new frame being proccessed. Since the address of the frame being
        // proccessed is dynamic, we need to read the from the page register (0xF0) and 
        // xor it with 0x6 to get the address of the new frame to process. the address of
        // the new frame is stored in register 9 and updated every frame.
        //
        // Data in the page register (0xF0) is xor'd with 0x6 and written back
        //
        // The page register (0xF0) is initialized to 0xC. The xor with 0x6 will give us
        // 0xA - the address of our next frame.

        // get the address to start proccessing as the new frame (store in R9)
        mov     R0,[0xF0]       // 0xDF0        // move value in data address 0xF0 to R0
        mov     R9,0x6          // 0x996        // set R9 to 0x6
        xor     R9,R0           // 0x790        // xor the data in R9 and the data in R0


        // do frame proccessing here - soubroutine??


        // switch the display to the new frame by "exchanging" the page we are viewing
        mov     R0,R9
        mov     [0xF0],R0       // 0xCF0        // move value in R0 to data address 0xF0

        // jump to top - just write new PC data - not sure if this is the best way...
        // this literally jumps to the third instruction in this program
        mov     PCH,0x0          // 0x9F0       // set PCH to 0x0
        mov     PCM,0x0          // 0x9E0       // set PCM to 0x0
        mov     PCL,0x2          // 0x9D2       // set PCL to 0x2

        
