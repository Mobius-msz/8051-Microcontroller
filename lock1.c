#include <REGX52.H>
#include "MatrixLED.h"
unsigned char code photo[]={0x1F,0x7F,0x9F,0x91,0x91,0x9F,0x7F,0x1F};
//只在定时器中断里调用，每次显示一列
void ShowLock1(unsigned char Column){
	MatrixLED_ShowColumn(Column,photo[Column]);
}