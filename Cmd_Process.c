#include <REGX52.H>
#include "UART.h"
#include "Nixie.h"
#include "AT24C02.h"
#include "CheckPassword.h"
#include "Cmd_Process.h"
#include <string.h>

extern char UART_RxBuffer[20];
extern volatile unsigned char UART_RxFlag;
extern volatile unsigned char UART_RxOverflow;
extern volatile unsigned char rx_index;
extern unsigned char mode;
extern unsigned char tp;
extern unsigned char Enter[];//输入值
extern unsigned char NEW1[],NEW2[];
extern unsigned char Wrong_Num;
extern unsigned char WAIT_Time,PROTECT_Time,Lock_Time;
extern unsigned int count1;				//1秒节拍计数（Timer0中断）
extern volatile unsigned char Fb_Req;	//声光反馈请求：0空闲 1普通 2成功 3错误 4限制
extern volatile unsigned int ISP_Wait;	//升级准备剩余时间（ms）
extern unsigned char Reply_ON;			//0=键盘操作，不主动发串口

/*回复格式：OK S N E R I / ERR CODE S N E R I*/
static void Send_State(void){
	switch(mode){
		case 0: UART_SendBytes("LOCKED"); break;
		case 1: UART_SendBytes("UNLOCKED"); break;
		case 2: UART_SendBytes("NEW1"); break;
		case 3: UART_SendBytes("NEW2"); break;
		case 4: UART_SendBytes("WAIT"); break;
		case 5: UART_SendBytes("PROTECT"); break;
		default: UART_SendBytes("UPGRADE"); break;
	}
}

static void Send_Num2(unsigned char n){		//0~99，无前导零
	if(n>=10) UART_SendByte('0'+n/10);
	UART_SendByte('0'+n%10);
}

static void Send_Fields(void){				//S N E R I
	UART_SendByte(' ');
	Send_State();
	UART_SendByte(' ');
	UART_SendNum(tp-1);						//N：输入位数
	UART_SendByte(' ');
	UART_SendNum(Wrong_Num);				//E：错误次数
	UART_SendByte(' ');
	if(mode==4) Send_Num2(WAIT_Time);		//R：限制剩余秒数
	else if(mode==5) Send_Num2(PROTECT_Time);
	else UART_SendNum(0);
	UART_SendByte(' ');
	if(mode>=1&&mode<=3) Send_Num2(Lock_Time);	//I：自动上锁剩余秒数
	else UART_SendNum(0);
	UART_SendBytes("\r\n");
}

static void Reply_OK(void){
	if(!Reply_ON) return;				//无主动日志
	UART_SendBytes("OK");
	Send_Fields();
}

static void Reply_Err(unsigned char code *err){
	if(!Reply_ON) return;				//无主动日志
	UART_SendBytes("ERR ");
	UART_SendBytes(err);
	Send_Fields();
}

/*内部工具封装*/
static void Clear_New(void){
	unsigned char n;
	for(n=0;n<6;n++){NEW1[n]=0;NEW2[n]=0;}
}

static void Save_Phase(unsigned char p){		//11地址：限制阶段
	AT24C02_WriteByte(11,p);
	AT24C02_WaitStandby();
}

static void Save_Count(unsigned char n){		//10地址：错误次数
	AT24C02_WriteByte(10,n);
	AT24C02_WaitStandby();
}

static void Restart_Tick(void){					//1秒节拍从当前时刻重新起算
	EA=0; count1=0; EA=1;
}

static void Refresh_Timer(void){				//有效输入/阶段变化刷新15秒
	if(mode==1||mode==2||mode==3){
		Lock_Time=15;
		Restart_Tick();							//保证剩余秒数向上取整
	}
}

/*状态机动作（键盘、串口共用*/
void Act_Digit(unsigned char n){
	if(mode==0||mode==2||mode==3){
		if(tp<7){							//超过六位不覆盖
			if(mode==0) Enter[tp-1]=n;
			else if(mode==2) NEW1[tp-1]=n;
			else NEW2[tp-1]=n;
			Nixie_Set(tp+2,n);
			tp++;
			Refresh_Timer();
			Fb_Req=FB_NORMAL;				//普通操作30ms
			Reply_OK();
		}
		else { Fb_Req=FB_ERROR; Reply_Err("FULL"); }
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Del(void){
	if(mode==0||mode==2||mode==3){
		if(tp>1){							//空删除也回OK，但不刷新15秒
			tp--;
			Nixie_Set(tp+2,10);
			Refresh_Timer();
		}
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Clear(void){
	if(mode==0||mode==2||mode==3){
		if(tp>1){
			tp=1;
			Nixie_Init();
			Refresh_Timer();
		}
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Ok(void){
	if(mode==0){
		if(tp<7){ Fb_Req=FB_ERROR; Reply_Err("INCOMPLETE"); return; }	//不足六位不计错
		if(Check_Password()){							//解锁成功
			Fb_Req=FB_SUCCESS;							//解锁成功：两次50ms
			mode=1;
			Wrong_Num=0;
			tp=1;
			Refresh_Timer();
			Nixie_Init();
			Save_Count(0);								//正确解锁清零
			Save_Phase(0);
			Reply_OK();
		}
		else{											//密码错误：清空输入并累计次数
			Wrong_Num++;
			tp=1;
			Nixie_Init();
			if(Wrong_Num==3){
				mode=4; WAIT_Time=30;
				Restart_Tick();							//倒计时从收到命令这一刻起算，不能等EEPROM写完
				Fb_Req=FB_RESTRICT;						//进入限制：三次100ms
			}
			else if(Wrong_Num==5){
				mode=5; PROTECT_Time=60;
				Restart_Tick();
				Fb_Req=FB_RESTRICT;
			}
			else Fb_Req=FB_ERROR;
			Save_Count(Wrong_Num);
			if(Wrong_Num==3) Save_Phase(4);				//第3次暂停30秒
			else if(Wrong_Num==5) Save_Phase(5);		//第5次保护60秒
			Reply_Err("WRONG");
		}
	}
	else if(mode==2){									//NEW1满六位：暂存并清空
		if(tp<7){ Fb_Req=FB_ERROR; Reply_Err("INCOMPLETE"); return; }
		tp=1;
		mode=3;
		Refresh_Timer();
		Nixie_Init();
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else if(mode==3){									//NEW2满六位：比较并保存
		if(tp<7){ Fb_Req=FB_ERROR; Reply_Err("INCOMPLETE"); return; }
		tp=1;
		mode=1;
		Refresh_Timer();
		Nixie_Init();
		if(Check_Password_NEW()){
			if(Change_Key()){ Fb_Req=FB_SUCCESS; Reply_OK(); }	//保存成功：两次50ms
			else { Fb_Req=FB_ERROR; Reply_Err("STORAGE"); }		//保存失败不报告成功
		}
		else { Fb_Req=FB_ERROR; Reply_Err("MISMATCH"); }
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Change(void){
	if(mode==1){
		mode=2;
		tp=1;
		Refresh_Timer();
		Clear_New();
		Nixie_Init();
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Cancel(void){
	if(mode==0||mode==1){								//成功但无变化，不改输入
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else if(mode==2||mode==3){							//丢弃改密暂存，回解锁
		tp=1;
		Clear_New();
		mode=1;
		Refresh_Timer();
		Nixie_Init();
		Fb_Req=FB_NORMAL;
		Reply_OK();
	}
	else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
}

void Act_Lock(void){
	if(mode==4||mode==5){			//原态，且不重置限制时间
		Fb_Req=FB_NORMAL;
		Reply_OK();
		return;
	}
	tp=1;
	Clear_New();
	mode=0;
	Nixie_Init();
	Fb_Req=FB_NORMAL;
	Reply_OK();
}

void Act_Status(void){
	Fb_Req=FB_NORMAL;				//查询：普通提示
	Reply_OK();						//原态，字段中带R/I
}

/*串口指令解析*/
void Cmd_Process(void){
	unsigned char n,len;

	if(UART_RxOverflow){			//超长行
		UART_RxOverflow=0;
		Fb_Req=FB_ERROR;
		Reply_Err("BAD_CMD");
		return;
	}
	len=0;							//去掉行尾空格
	while(UART_RxBuffer[len]!=0) len++;
	while(len>0&&UART_RxBuffer[len-1]==' ') UART_RxBuffer[--len]=0;

	if(strncmp(UART_RxBuffer,"DIGIT ",6)==0){		//DIGIT 后恰好一个空格+一位数字
		if(UART_RxBuffer[6]>='0'&&UART_RxBuffer[6]<='9'&&UART_RxBuffer[7]==0)
			Act_Digit(UART_RxBuffer[6]-'0');
		else { Fb_Req=FB_ERROR; Reply_Err("BAD_CMD"); }
	}
	else if(strcmp(UART_RxBuffer,"DEL")==0) Act_Del();
	else if(strcmp(UART_RxBuffer,"CLEAR")==0) Act_Clear();
	else if(strcmp(UART_RxBuffer,"OK")==0) Act_Ok();
	else if(strcmp(UART_RxBuffer,"CHANGE")==0) Act_Change();
	else if(strcmp(UART_RxBuffer,"CANCEL")==0) Act_Cancel();
	else if(strcmp(UART_RxBuffer,"LOCK")==0) Act_Lock();
	else if(strcmp(UART_RxBuffer,"STATUS")==0) Act_Status();
	else if(strcmp(UART_RxBuffer,"VERSION")==0){
		Fb_Req=FB_NORMAL;
		UART_SendBytes("OK VERSION 1.0\r\n");
	}
	else if(strcmp(UART_RxBuffer,"ISP")==0){
		if(mode==1){				//仅普通解锁允许
			mode=6;					//升级准备
			Nixie_Set(1,6);			//状态码6=升级准备
			for(n=3;n<=8;n++) Nixie_Set(n,16);
			Fb_Req=FB_NORMAL;
			UART_SendBytes("OK ISP\r\n");	//特殊回复，不带状态字段
			EA=0;					//必须在串口实际发送完成后再计时500ms
			ISP_Wait=500;			//非阻塞等待，到点由主循环软复位
			EA=1;
		}
		else { Fb_Req=FB_ERROR; Reply_Err("DENIED"); }
	}
	else { Fb_Req=FB_ERROR; Reply_Err("BAD_CMD"); }		//非法行
}
