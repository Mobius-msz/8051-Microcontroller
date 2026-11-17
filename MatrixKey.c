#include "Delay.h"
#include <REGX52.H>
//键盘扫描
unsigned char MatrixKey(void){
	P1=0xFF; P1_3=0;			// 扫描第1列
	if(P1_7==0) return 1;
	if(P1_6==0) return 4;
	if(P1_5==0) return 7;
	if(P1_4==0) return 11;
	
	P1=0xFF; P1_2=0;			// 扫描第2列
	if(P1_7==0) return 2;
	if(P1_6==0) return 5;
	if(P1_5==0) return 8;
	if(P1_4==0) return 10;
	
	P1=0xFF; P1_1=0;			// 扫描第3列
	if(P1_7==0) return 3;
	if(P1_6==0) return 6;
	if(P1_5==0) return 9;
	if(P1_4==0) return 12;
	
	P1=0xFF; P1_0=0;			// 扫描第4列
	if(P1_7==0) return 13;
	if(P1_6==0) return 14;
	if(P1_5==0) return 15;
	if(P1_4==0) return 16;
	
	return 0;					// 如果都没按，返回 0
}
static unsigned char last_Key,now_Key;
//在循环中对矩阵键盘进行扫描
unsigned char MatrixKey_Loop(void){
	last_Key=now_Key;			//0，1按下即生效
	now_Key=MatrixKey();
	if(last_Key==0&&now_Key!=0){
		return now_Key;
	}
	return 0;
}