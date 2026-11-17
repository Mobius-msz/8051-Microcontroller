#include <REGX52.H>
#include "MatrixLED.h"
unsigned char code photo1[]={0x1F,0xDF,0xBF,0x11,0x91,0xDF,0x1F,0x1F};
//只在定时器中断里调用，每次显示一列
void ShowLock2(unsigned char Column){
	MatrixLED_ShowColumn(Column,photo1[Column]);
}