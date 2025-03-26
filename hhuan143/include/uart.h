/***
 * File:    uart.h
 * Author:  James Huang
 * Created: ECE121 winter 2024
 * This library implements a true UART device driver that enforces 
 * I/O stream abstraction between the physical and application layers.
 * All stream accesses are on a per-character or byte basis. 
 */
#include <stdbool.h>
#ifndef UART_H
#define UART_H
#define BufferSize 16

typedef struct BufferObj* Buffer;

typedef struct BufferObj{
    unsigned char buffer[BufferSize];
    int head;
    int tail;
    int full; //0 == not full 1 == full
    int err; //0 == no err 1 == err
}BufferObj;

void Buffer_InitStatic(Buffer cb);
/**
 * @Function Uart_Init(unsigned long baudrate)
 * @param baudrate
 * @return none
 * @brief  Initializes UART1 to baudrate N81 and creates circ buffers
 * @author instructor ece121 W2022 */
void Uart_Init(unsigned long baudRate);

/**
 * @Function int PutChar(char ch)
 * @param ch - the character to be sent out the serial port, buffer
 * @return True if successful, else False if the buffer is full or busy.
 * @brief  adds char to the end of the TX circular buffer
 * @author instrutor ECE121 W2022 */
int PutChar(Buffer cb, char ch);

/**
 * @Function unsigned char GetChar(void)
 * @param buffer, char
 * @return Char in the argument.
 * @brief  dequeues a character from the RX buffer,
 * @author instructor, ECE121 W2022 */
unsigned char GetChar(Buffer cb, unsigned char *);

#endif // UART_H