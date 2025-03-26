#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "BOARD.h"
//#define pt1
#define pt2 

int main(int argc, char** argv)
{
    TRISE = 0b0; //set Port E to be an output port, so we can see the RE0-7 or all LEDs 
    //TRISF = 0b1;
    //TRISD = 0b1; //doesnt work TA doesnt know why :(
    LATE = 0b0; //set all Port E to 0s, turn off all LEDs 
    
#ifdef pt1
    while(1){
        //define each buttons using its corresponding port bits, if pressed, light up LEDs. 
        if (PORTFbits.RF1){
            LATE = 0b00000001;
        }else if (PORTDbits.RD5){
            LATE = 0b00000010;
        }else if (PORTDbits.RD6){
            LATE = 0b00000100;  
        }else if (PORTDbits.RD7){
            LATE = 0b00001000;
        }
    }
#endif
    
#ifdef pt2 
    LATE = 0b00000001;
    while(1){
        LATE++;
        if (PORTFbits.RF1 || PORTDbits.RD5 || PORTDbits.RD6 || PORTDbits.RD7){
            LATE = 0b00000001;
        }
        if (LATE == 0b00000000){
            LATE = 0b00000001;
        }
        //6500 NOP is about 5ms
        asm("NOP");
        for (int i = 0; i < 250000; i++){
            asm("NOP");
        }
        asm("NOP");
        
    }
#endif
    
}


