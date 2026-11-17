#ifndef __CHECKPASSWORD_H__
#define __CHECKPASSWORD_H__
unsigned char Check_Password(void);
unsigned char Check_Password_NEW(void);
void Init_Password(unsigned char base);	//base=1读1~6，base=12读12~17
unsigned char Change_Key(void);	//返回1=保存并校验成功，0=保存失败
#endif