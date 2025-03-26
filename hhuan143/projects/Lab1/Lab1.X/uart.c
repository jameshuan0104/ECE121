#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <assert.h>
#include "uart.h"
#include "BOARD.h"

 // Be sure this include is added to your uart.c code.
#include <sys/attribs.h> 

//#define task1
#define task2


struct BufferObj rxBufferStatic;
struct BufferObj txBufferStatic; //allocate memory for the BufferObj struct
Buffer rxBuffer = &rxBufferStatic;
Buffer txBuffer = &txBufferStatic; //set the pointer to the address of memory
 
void Buffer_InitStatic(Buffer cb){  
    //instead of memory allocation in the init function, it takes the already allocated memory and set values to it.
    cb->head = 0;
    cb->tail = 0;
    cb->full = 0;
    cb->err = 0; //set all 0s, init    
}


/*void main(){
    Uart_Init(115200);
#ifdef task1
    while(1){
        if (U1STAbits.URXDA){
            U1TXREG = U1RXREG; 
        }
    }
#endif

#ifdef task2
    Buffer_InitStatic(rxBuffer);
    Buffer_InitStatic(txBuffer);
    /*setbuf(stdout, NULL);
    printf("hello world");
    printf("0987654321");
    printf("c");
    printf("h");
    printf("a");
    printf("t");*/
    /*while(1){
        unsigned char var;
        var = GetChar(rxBuffer, &var);
        if (var!= '?'){
            PutChar(txBuffer, var);
        }
    }
    
#endif     
}*/
void Uart_Init(unsigned long baudRate){
    BOARD_Init();
    U1MODE = 0; //clear UART
    U1BRG = BOARD_GetPBClock() / (16 * baudRate) - 1;
    U1MODEbits.PDSEL = 0b00; //8N1
    U1MODEbits.STSEL = 0b0; //1 stop bit
    U1MODEbits.ON = 1; //enable UART
    
    U1STAbits.UTXEN = 1; //enable TX pin
    U1STAbits.URXEN = 1; //enable RX pin
    
    U1STAbits.URXISEL = 0b00; //config the interrupt register for rx
    U1STAbits.UTXISEL = 0b00; //config the interrupt register for tx
    IEC0bits.U1RXIE = 1;
    IEC0bits.U1TXIE = 1; //enable flags
    IPC6bits.U1IP = 0b100; //set priority for interrupts, since only one device doesnt rly matter  
}

int PutChar(Buffer cb, char ch){
    if (cb->full){
        cb->err = 1; //if full, set error, cant write to buffer
        return 0;
    }
    cb->buffer[cb->tail] = ch; //buffer not full, write to tail
    cb->tail = (cb->tail + 1) % BufferSize; //increment tail
    if (cb->tail == cb->head){
        cb->full = 1; //if buffer BECOMES full, set full flag
    }
    IFS0bits.U1TXIF = 1;
    return 1;
}

unsigned char GetChar(Buffer cb, unsigned char *data){
    //checks if head and tail caught up, and it is not full (meaning empty buffer nothing to read)
    if ((cb->head == cb->tail) && (!cb->full)){
        cb->err = 1;
        return '?'; //if seg fault check here
    }
    //otherwise read the data from the head and set it to data
    *data = cb->buffer[cb->head];
    cb->head = (cb->head + 1) % BufferSize; //increment head after it has been read
    if (cb->full){
        cb->full = 0; //if the buffer was full, after reading it it should have become not full 
    }
    IFS0bits.U1RXIF = 1;
    return *data;
}                                                     
 //******************************************************************************/

void _mon_putc(char c){
 //your code goes here
    while(!PutChar(txBuffer, c)){ 
        ;
    }
}


/****************************************************************************
 * Function: IntUart1Handler
 * Parameters: None.
 * Returns: None.
 * The PIC32 architecture calls a single interrupt vector for both the 
 * TX and RX state machines. Each IRQ is persistent and can only be cleared
 * after "removing the condition that caused it". This function is declared in
 * sys/attribs.h. 
 ****************************************************************************/
void __ISR(_UART1_VECTOR) IntUart1Handler(void) {
    //your interrupt handler code goes here
    //check if rx flag has been raised
    if (IFS0bits.U1RXIF == 1){
        //check if rx has data
        IFS0bits.U1RXIF = 0; //clear the rx flag, located at RXIF 
        while (U1STAbits.URXDA){
            //only write to rxBuffer it is not full
            if (rxBuffer->err != 1){
                PutChar(rxBuffer, U1RXREG);
            }else{
                rxBuffer->err = 0;
            }
        }
    }
        
    //check if tx flag has been raised
    if (IFS0bits.U1TXIF == 1){
        //check if tx buffer has at least one more space
        IFS0bits.U1TXIF = 0; // clears the tx flag, located at TXIF
        if (!U1STAbits.UTXBF){
            unsigned char var; //intermediate variable due to data not passed in correctly
            GetChar(txBuffer, &var);//GetChar reads data from txBuffer and set it to "var"
            if (txBuffer->err != 1){
                U1TXREG = var;
            }else{
                txBuffer->err = 0; //clear txBuffer err flag 
            }       
        }
    }

}   
