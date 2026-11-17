#include <REGX52.H>
#include "AT24C02.h"
extern unsigned char Key[],Enter[],NEW1[],NEW2[];
unsigned char Check_Password(void){
	unsigned char i;
	for(i=0;i<6;i++){
		if(Key[i]==Enter[i]) continue;
		return 0;
	}
	return 1;
}
unsigned char Check_Password_NEW(void){
	unsigned char i;
	for(i=0;i<6;i++){
		if(NEW1[i]==NEW2[i]) continue;
		return 0;
	}
	return 1;
}
unsigned char Change_Key(void){	//双区保存：先写另一区并校验，最后单字节切换标志，断电不会出现半个密码
	unsigned char i,bank,base;
	bank=(AT24C02_ReadByte(7)==0)?1:0;	//当前有效区是0就写1区，反之写0区
	base=bank?12:1;						//0区地址1~6，1区地址12~17
	for(i=0;i<6;i++){
		AT24C02_WriteByte(base+i,NEW1[i]);
		AT24C02_WaitStandby();
	}
	for(i=0;i<6;i++){					//回读校验
		if(AT24C02_ReadByte(base+i)!=NEW1[i]) return 0;
	}
	AT24C02_WriteByte(7,bank);			//提交点：7=0用1~6，7=1用12~17
	AT24C02_WaitStandby();
	if(AT24C02_ReadByte(7)!=bank) return 0;
	for(i=0;i<6;i++){
		Key[i]=NEW1[i];
	}
	return 1;
}
void Init_Password(unsigned char base){
	unsigned char i;
	for(i=0;i<6;i++){
		Key[i]=AT24C02_ReadByte(base+i);
	}
}
