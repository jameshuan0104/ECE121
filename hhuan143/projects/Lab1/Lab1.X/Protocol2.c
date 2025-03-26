#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <assert.h>
#include <string.h>

#include "Protocol2.h"
#include "uart.h"
#include "BOARD.h"
#include "MessageIDs.h"

#include <sys/attribs.h> 

extern Buffer rxBuffer;
extern Buffer txBuffer; //using the rx and tx already defined in uart.c1

static Packet myPacket;
static PacketBuffer myPacketBuffer; //define my global variables

typedef struct PacketObj{
    uint8_t id;      
    uint8_t len;
    uint8_t checkum; 
    unsigned char payload[MAXPAYLOADLENGTH];
}PacketObj;

typedef struct PacketBufferObj{
    Packet buffer[PACKETBUFFERSIZE];
    int head;
    int tail;
    int full; 
    int err;
}PacketBufferObj;

Packet Packet_Init(){  
    myPacket = (Packet)malloc(sizeof(struct PacketObj)); //dynamically allocate memory
    assert(myPacket != NULL);
    myPacket->id = 0;      
    myPacket->len = 0;
    myPacket->checkum = 0; 
    return myPacket;
}

PacketBuffer PacketBuffer_Init(){  
    myPacketBuffer = (PacketBuffer)malloc(sizeof(struct PacketBufferObj));
    assert(myPacketBuffer != NULL);
    myPacketBuffer->head = 0;      
    myPacketBuffer->tail = 0;
    myPacketBuffer->full = 0;
    myPacketBuffer->err = 0;
    for (int i = 0; i < PACKETBUFFERSIZE; i++){
        myPacketBuffer->buffer[i] = Packet_Init(); //allocate mem for each packet that exist in the buffer
    }
    return myPacketBuffer;
}

int main(void){
    Protocol_Init(115200);
    LEDS_INIT();
    Buffer_InitStatic(rxBuffer);
    Buffer_InitStatic(txBuffer);
    myPacket = Packet_Init();
    myPacketBuffer = PacketBuffer_Init(); //initialize all ADTs
    while(1){
        //check if a packet has been parsed by the state machine
        if (BuildRxPacket() == 1){
            uint8_t id = 0;
            uint8_t len = 0;
            unsigned char payload[MAXPAYLOADLENGTH];
            if (Protocol_ReadNextPacketID() == ID_LEDS_SET){ //check if the packet type set leds
                Protocol_GetInPacket(&id, &len, payload); //read packet off buffer, payload is updated
                LEDS_SET(myPacket->payload[0]); //get the first byte that hold the hex to indicate which leds to set
            }
            else if(Protocol_ReadNextPacketID() == ID_LEDS_GET){ //check if packet is ID_LEDS_GET, have to send led status packet back
                Protocol_GetInPacket(&id, &len, payload);
                uint8_t ledState = LEDS_GET(); //get the current status of leds
                Protocol_SendPacket(0x02, ID_LEDS_STATE, &ledState); //send a packet out containing led status               
            }
            else if (Protocol_ReadNextPacketID() == ID_PING){ //check is packet type is PING
                Protocol_GetInPacket(&id, &len, payload); //read packet off the buffer
                /*converting unsigned char array to an unsigned int manually, this way of explicitly constructing the byte in a specific 
                order makes it so it can work on any devices regardless of its endianness*/
                unsigned int var = (unsigned int)payload[0] << 24 | // shift the first byte left by 24 bits
                        (unsigned int)payload[1] << 16 | // shift the second byte left by 16 bits
                        (unsigned int)payload[2] << 8  | // shift the third byte left by 8 bits
                        (unsigned int)payload[3]; //add the fourth byte to the end
              
                var = var / 2;
                
                unsigned char data[4];
                //converting the unsigned int back to unsigned char, again explicitly managing the byte order. 
                data[0] = (var >> 24) & 0xFF; // extracts the first (most significant) byte
                data[1] = (var >> 16) & 0xFF; // extracts the second byte
                data[2] = (var >> 8) & 0xFF;  // extracts the third byte
                data[3] = var & 0xFF;  // extracts the fourth (least significant) byte
                Protocol_SendPacket(0x5, ID_PONG, &data); //construct packet and send it out
            }
            
        }
        
    }
    
    
}




/**
 * @Function Protocol_Init(baudrate)
 * @param Legal Uart baudrate
 * @return SUCCESS (true) or ERROR (false)
 * @brief Initializes Uart1 for stream I/O to the lab PC via a USB 
 *        virtual comm port. Baudrate must be a legal value. 
 * @author instructor W2022 */
int Protocol_Init(unsigned long baudrate){
    Uart_Init(baudrate);
    return 1;
}

/**
 * @Function unsigned char Protocol_QueuePacket()
 * @param none
 * @return the buffer full flag: 1 if full
 * @brief Place in the main event loop (or in a timer) to continually check 
 *        for completed incoming packets and then queue them into 
 *        the RX circular buffer. The buffer's size is set by constant
 *        PACKETBUFFERSIZE.
 * @author instructor W2023 */
uint8_t Protocol_QueuePacket(){
    if (myPacketBuffer->full == 1){
        return myPacketBuffer->full;
    }
    myPacketBuffer->buffer[myPacketBuffer->tail] = myPacket; //buffer not full, write to tail
    myPacketBuffer->tail = (myPacketBuffer->tail + 1) % PACKETBUFFERSIZE; //increment tail
    if (myPacketBuffer->tail == myPacketBuffer->head){
        myPacketBuffer->full = 1; //if buffer BECOMES full, set full flag
    }
    return myPacketBuffer->full;
}

/**
 * @Function int Protocol_GetInPacket(uint8_t *type, uint8_t *len, uchar *msg)
 * @param *type, *len, *msg
 * @return SUCCESS (true) or WAITING (false)
 * @brief Reads the next packet from the packet Buffer 
 * @author instructor W2022 */
int Protocol_GetInPacket(uint8_t *type, uint8_t *len, unsigned char *msg){
    //checks if head and tail caught up, and it is not full (meaning empty buffer nothing to read)
    if ((myPacketBuffer->head == myPacketBuffer->tail) && (!myPacketBuffer->full)){
        myPacketBuffer->err = 1;
        return 0; 
    }
    myPacket = myPacketBuffer->buffer[myPacketBuffer->head]; //get the packet off the buffer, set it to myPacket
    *type = myPacket->id; //set the id and len
    *len = myPacket->len;
    for (int i = 0; i < myPacket->len - 1; i++){
        msg[i] = myPacket->payload[i]; //read the payload of packet one character at a time   
    }
    myPacketBuffer->head = (myPacketBuffer->head + 1) % PACKETBUFFERSIZE; //increment head after it has been read
    if (myPacketBuffer->full){
        myPacketBuffer->full = 0; //if the buffer was full, after reading it it should have become not full 
    }
    return 1;
}

/**
 * @Function int Protocol_SendPacket(unsigned char len, void *Payload)
 * @param len, length of full <b>Payload</b> variable
 * @param Payload, pointer to data
 * @return SUCCESS (true) or ERROR (false)
 * @brief composes and sends a full packet
 * @author instructor W2022 */
int Protocol_SendPacket(unsigned char len, unsigned char ID, void *Payload){
    unsigned char checksum = 0;
    unsigned char* payload = Payload;

    PutChar(txBuffer, HEAD);
    PutChar(txBuffer, len);
    PutChar(txBuffer, ID);
    checksum = Protocol_CalcIterativeChecksum(ID, checksum); //construct a complete packet
    //parse through the packet one character at a time and calc checksum as we parse
    for (int i = 0; i < len - 1; i++){
        unsigned char byte = payload[i]; 
        PutChar(txBuffer, byte);
        checksum = Protocol_CalcIterativeChecksum(byte, checksum);
    }
    PutChar(txBuffer, TAIL);
    PutChar(txBuffer, checksum);
    PutChar(txBuffer, '\r');
    PutChar(txBuffer, '\n');
}

/**
 @Function unsigned char Protocol_ReadNextID(void)
 * @param None
 * @return Reads the ID of the next available Packet
 * @brief Returns ID or 0 if no packets are available
 * @author instructor W2022 */
unsigned char Protocol_ReadNextPacketID(void){
    myPacket = myPacketBuffer->buffer[myPacketBuffer->head]; //read the next packet from buffer, but doesnt take it off the buffer
    return myPacket->id; //we want to read the id of the next packet
}
/*******************************************************************************
 * PRIVATE FUNCTIONS
 * Generally these functions would not be exposed but due to the learning nature 
 * of the class some are are noted to help you organize the code internal 
 * to the module. 
 ******************************************************************************/

/* BuildRxPacket() should implement a state machine to build an incoming
 * packet incrementally and return it completed in the called argument packet
 * structure (rxPacket is a pointer to a packet struct). The state machine should
 * progress through discrete states as each incoming byte is processed.
 * 
 * Now consider how to create another structure for use as a circular buffer
 * containing a PACKETBUFFERSIZE number of these rxpT packet structures.
 ******************************************************************************/
typedef enum {
    WAIT_FOR_HEAD,
    READ_LENGTH,
    READ_ID,
    READ_PAYLOAD,
    READ_TAIL,
    READ_CHECKSUM,
    PROCESS_PACKET
} State; //define my states
static uint8_t payloadLen = 0; //payload length counter
static unsigned char checksum = 0; //checksum counter
static State currState = WAIT_FOR_HEAD; //initialize first state

uint8_t BuildRxPacket (){
    unsigned char byte; 
    while (GetChar(rxBuffer, &byte) != '?'){ //GetChar read data in 
        switch(currState){
            case WAIT_FOR_HEAD: //checks for head to start reading
                if (byte == HEAD){
                    currState = READ_LENGTH;
                }
                break;
            case READ_LENGTH: //checks for length byte
                myPacket->len = byte; //set relevant data to our packet
                currState = READ_ID; //move next state
                break;
            case READ_ID: //checks for id byte
                checksum = Protocol_CalcIterativeChecksum(byte, checksum); //add to checksum counter
                myPacket->id = byte; //set packet id 
                payloadLen++; //keep track length of the payload
                if (payloadLen == myPacket->len){
                    currState = READ_TAIL; //check the case if len = 1, packet contains only ID in payload, no actual payload to read. goes straight to tail
                }else{
                    currState = READ_PAYLOAD;// otherwise there will be actual data in payload to be read 
                }
                break;
            case READ_PAYLOAD: //reads the payload
                checksum = Protocol_CalcIterativeChecksum(byte, checksum); //add to checksum counter
                myPacket->payload[payloadLen-1] = byte; //update packet's payload (-1 because payload len also accounts for ID, but we want to add to the buffer starting at index 0)
                payloadLen++; //add to payload length counter
                if (payloadLen == myPacket->len){
                    currState = READ_TAIL; //if expected payload is reached, we look for tail
                    break;
                }
                break;
            case READ_TAIL: //checks for tail byte 
                if (byte == TAIL){
                    currState = READ_CHECKSUM;
                }
                break;
            case READ_CHECKSUM: //checks the checksum byte 
                if (byte == checksum){
                    myPacket->checkum = byte;
                    currState = PROCESS_PACKET;
                }
                break;
            case PROCESS_PACKET: 
                //packet should have been build, myPacket now contains all info as we parsed through data
                checksum = 0; //clear checksum for next packet
                payloadLen = 0; //clear payload length for next packet
                Protocol_QueuePacket(); //add packet to packet buffer
                currState = WAIT_FOR_HEAD; //return to starting state for next packet
                return 1;
                break;
        }       
    }
}

/**
 * @Function char Protocol_CalcIterativeChecksum(unsigned char charIn, unsigned char curChecksum)
 * @param charIn, new char to add to the checksum
 * @param curChecksum, current checksum, most likely the last return of this function, can use 0 to reset
 * @return the new checksum value
 * @brief Returns the BSD checksum of the char stream given the curChecksum and the new char
 * @author mdunne */
unsigned char Protocol_CalcIterativeChecksum(unsigned char charIn, unsigned char checksum){
    checksum = (checksum >> 1) | (checksum << 7); //BSD checksum 
    checksum += charIn;
    checksum &= 0xFF; //ensures checksum is 8bit, althought it should be already
    return checksum;   
}