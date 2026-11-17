#include <REGX52.H>
/** @brief 定时器0初始化，1ms@11.0592MHz
  * @param
  * @retval
  */

void Timer0_Init(void){
	TMOD &= 0xF0;//设置定时器模式 不该定时器1，清零定时器0
	TMOD |= 0x01;//定时器0，模式1（16位）
	TL0 = 0x8A;				//1ms@11.0592MHz：0xFC8A，与中断内的重装值保持一致
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0=1;                  //中断
	EA=1;					//中断（总）
	PT0=0;					//优先级
}
