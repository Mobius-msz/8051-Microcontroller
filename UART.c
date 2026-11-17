#include <REGX52.H>

volatile unsigned char UART_TxDone=1;	//1=上一字节发送完成，由串口中断置位

void UART_Init(void)	//9600bps@11.0592MHz
{
	PCON|=0x80;		//使能波特率倍速位SMOD
	SCON=0x50;		//8位数据,可变波特率
	TMOD&=0x0F;		//设置定时器模式
	TMOD|=0x20;		//设置定时器模式
	TL1=0xFA;		//设置定时初始值9600
	TH1=0xFA;		//设置定时重载值9600
	ET1=0;			//禁止定时器中断
	ES=1;			//串口中断
	EA=1;
	TR1=1;			//定时器1开始计时
}

void UART_SendByte(unsigned char Byte){
	UART_TxDone=0;
	SBUF=Byte;
	while(!UART_TxDone);		//TI不能在主程序里清，否则和中断抢；由中断清TI并置标志
}

void UART_SendNum(unsigned char Byte){
	UART_SendByte(Byte+'0');
}

void UART_SendBytes(unsigned char code *buf)			//输出字符串
{
    while(*buf != 0x00){
        UART_SendByte(*buf++);
    }
}
