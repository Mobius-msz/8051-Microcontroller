#include <REGX52.H>
sbit RCK=P3^5;
sbit SCK=P3^6;
sbit SER=P3^4;

#define MATRIX_LED_PORT P0;
/** @brief 74HC595写入一个字节
  * @param 要写入的一个字节
  * @retval
  */
void _74HC595_WriteByte(unsigned char Byte){
	unsigned char i;
	for(i=0;i<8;i++){
		SER=Byte&(0x80>>i);			//SER压入数据，串行输入，并行输出，高位先入
		SCK=1;						//上升沿
		SCK=0;
	}
	RCK=1;
	RCK=0;
}

/** @brief LED点阵显示一列数据
  * @param Column要选择的列，范围：0-7，0是最左边
  * @param Data选择列要显示的数据，高位在上，1为亮，0为暗
  * @retval 无
  */
void MatrixLED_ShowColumn(unsigned char Column,Data){
	P0=0xFF;						//列消影，避免旧列残留
	_74HC595_WriteByte(Data);
	P0=~(0x80>>Column);				//0接通
}
/** @brief 
  * @param
  * @retval
  */
void Matrix_Init(void){
	SCK=0;
	RCK=0;
	_74HC595_WriteByte(0x00);		//595上电后存储寄存器状态不确定，先清零，避免点阵乱亮
}