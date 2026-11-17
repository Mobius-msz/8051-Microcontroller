#ifndef __UART_INIT_H__
#define __UART_INIT_H__
extern volatile unsigned char UART_TxDone;
void UART_Init(void);
void UART_SendByte(unsigned char Byte);
void UART_SendBytes(unsigned char code *buf);
void UART_SendNum(unsigned char Byte);
#endif