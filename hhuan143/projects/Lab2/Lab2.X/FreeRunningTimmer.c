#include <stdio.h>
#include <stdlib.h>
#include "FreeRunningTimer.h"
#include "BOARD.h"
#include <xc.h>
#include <sys/attribs.h> 
#include "uart.h"

#define PBCLK (BOARD_GetPBClock())
#define freq 1000

static unsigned int microSTimer = 0;
static unsigned int mSTimer = 0;
static unsigned int currTime1 = 0;

extern Buffer rxBuffer;
extern Buffer txBuffer;



int main(void){
    Uart_Init(115200);
    LEDS_INIT();
    FreeRunningTimer_Init();
    Buffer_InitStatic(rxBuffer);
    Buffer_InitStatic(txBuffer);
    setbuf(stdout, NULL);
    while(1){
        if (FreeRunningTimer_GetMilliSeconds() - currTime1 == 2000){
            currTime1 = mSTimer;
            //currTime1 = FreeRunningTimer_GetMicroSeconds();
            if (LEDS_GET() == 0x01)
                LEDS_SET(0x00);
            else{
                LEDS_SET(0x01);
            }
            printf("%d\n\r", FreeRunningTimer_GetMilliSeconds());
            printf("%d\n\r", FreeRunningTimer_GetMicroSeconds());

        }
        
    }
}

void FreeRunningTimer_Init(void){
    T5CON = 0; //clear timer 5 register 
    T5CONbits.ON = 0; //stops clock
    
    //T5CONSET //stop timer
    T5CONbits.TCKPS = 1; // 1:2 prescale
    
    TMR5 = 0;
    PR5 = (PBCLK / freq) >> 1; //rollover every mS
    
    IEC0bits.T5IE = 1; //interrup enbale
    IFS0bits.T5IF = 0; //interrupt flag
    IPC5bits.T5IP = 3; //set priority
    
    
    T5CONbits.ON = 1; //turn on clock
    
    //T5CONSET = 0x8000; //start timer
}

/**
 * Function: FreeRunningTimer_GetMilliSeconds
 * @param None
 * @return the current MilliSecond Count
   */
unsigned int FreeRunningTimer_GetMilliSeconds(void){
    return mSTimer; 
}

/**
 * Function: FreeRunningTimer_GetMicroSeconds
 * @param None
 * @return the current MicroSecond Count
   */
unsigned int FreeRunningTimer_GetMicroSeconds(void){
    return ((mSTimer * 1000) + TMR5 / 20);  
}


void __ISR(_TIMER_5_VECTOR, ipl3auto) Timer5IntHandler(void){
    IFS0bits.T5IF = 0;
    mSTimer++;
    microSTimer += 1000;
}