#include "Delay.h"
#include <REGX52.H>
unsigned char NixieTable[]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x40,0x3F,0x73,0x79,0x54,0x50,0x40};	//0-9，10不显示，11=O 12=P 13=E 14=n 15=r 16=-
unsigned char NixieBuf[8]={10,10,10,10,10,10,10,10};								//缓冲区，10表示不显示
/** @brief 设置某一位数码管
  * @param	loc为位置，从左到右1-8
  * @param number为要显示的数字
  * @retval 无
  */
void Nixie_Init(void){
	unsigned char i;
	for(i=0;i<8;i++) NixieBuf[i]=10;
}
void Nixie_Set(unsigned char loc,unsigned char number){
	if(loc>=1&&loc<=8&&number<=16) NixieBuf[loc-1]=number;
}

/** @brief 将缓冲区的数据扫描显示
  * @param	无
  * @retval	显示缓冲区数据
  */
void Nixie_Scan(void){
	static unsigned char i=0;			
	P0=0x00;								// 消影
	switch(i+1){							// 位选
		case 1:P2_4=1;P2_3=1;P2_2=1;break;
		case 2:P2_4=1;P2_3=1;P2_2=0;break;
		case 3:P2_4=1;P2_3=0;P2_2=1;break;
		case 4:P2_4=1;P2_3=0;P2_2=0;break;
		case 5:P2_4=0;P2_3=1;P2_2=1;break;
		case 6:P2_4=0;P2_3=1;P2_2=0;break;
		case 7:P2_4=0;P2_3=0;P2_2=1;break;
		case 8:P2_4=0;P2_3=0;P2_2=0;break;
	}
	P0=NixieTable[NixieBuf[i]];				// 段选
	i++;
	if(i>=8) i=0;
}