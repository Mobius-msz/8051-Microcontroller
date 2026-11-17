#include <REGX52.H>
#include "Delay.h"
#include "Nixie.h"
#include "Timer0.h"
#include "UART.h"
#include "MatrixKey.h"
#include "MatrixLED.h"
#include "I2C.h"
#include "AT24C02.h"
#include "CheckPassword.h"
#include "Cmd_Process.h"
unsigned char Key[6]={0,1,2,3,4,5},Wrong_Num=0,mode=0;//密码，错误次数，当前模式
/*0-LOCKED 1-UNLOCKED 2-NEW1 3-NEW2 4-WAIT 5-PROTECT 6-UPGRADE*/
unsigned char WAIT_Time=30,PROTECT_Time=60;
unsigned char Lock_Time=15;								//15秒自动上锁倒计时
unsigned char Reply_ON=1;								//键盘操作时不主动发串口
/*固件标识：0=A版(D8常暗) 1=B版(D8常亮)，两份HEX仅此不同*/
#define FW_B 0
/*声光反馈节奏表（单位ms，on/off交替，从响开始）*/
volatile unsigned char Fb_Req=0;	//动作写请求
volatile unsigned char Fb_Pat=0;	//正在播放的等级，0=空闲
volatile unsigned char Fb_Step=0;	//当前第几段
volatile unsigned char Fb_Timer=0;	//当前段剩余ms
static const unsigned char code Fb_Tab[]  ={30, 50,50,50, 200, 100,100,100,100,100};
static const unsigned char code Fb_Start[]={0,0,1,4,5};
static const unsigned char code Fb_Len[]  ={0,1,3,1,5};
unsigned char Enter[6];//输入值
unsigned char NEW1[6],NEW2[6];
volatile unsigned char KeyNumber=0;//矩阵键盘
unsigned char count0;
unsigned int count1;
unsigned char tp=1;//当前输入位置1-6
volatile unsigned char k;//k临时存放KeyNumber

char UART_RxBuffer[20];
volatile unsigned char UART_RxFlag=0;
volatile unsigned char UART_RxOverflow=0;				//本行超过19字符
volatile unsigned char rx_index=0;
unsigned char ch;
volatile unsigned int ISP_Wait=0;						//升级准备剩余毫秒
volatile unsigned char ISP_Ready=0;						//500ms到，请求软复位
sfr ISP_CONTR=0xE7;										//STC ISP控制寄存器
sbit Buzzer=P2^5;		//高电平响
sbit D7=P2^6;			//声光反馈灯，低电平亮
sbit D8=P2^7;			//A/B区分灯，低电平亮

/*数码管：第1位状态码，第2位错误次数，后6位输入/倒计时，空位为"-"*/
void Display_Update(void){
	unsigned char n,t;
	Nixie_Set(1,mode);
	Nixie_Set(2,Wrong_Num);
	if(mode==4||mode==5){			//限制状态：六位补零的剩余秒数
		t=(mode==4)?WAIT_Time:PROTECT_Time;
		Nixie_Set(3,t/100000%10);
		Nixie_Set(4,t/10000%10);
		Nixie_Set(5,t/1000%10);
		Nixie_Set(6,t/100%10);
		Nixie_Set(7,t/10%10);
		Nixie_Set(8,t%10);
	}
	else if(mode==1){				//解锁：--- -15 形式的自动上锁倒计时
		Nixie_Set(3,16);
		Nixie_Set(4,16);
		Nixie_Set(5,16);
		Nixie_Set(6,16);
		Nixie_Set(7,Lock_Time/10);
		Nixie_Set(8,Lock_Time%10);
	}
	else if(mode==6){				//升级准备
		for(n=3;n<=8;n++) Nixie_Set(n,16);
	}
	else{							//锁定/改密：后六位实时显示输入，空位为"-"
		for(n=0;n<6;n++){
			if(n<tp-1){
				if(mode==0) t=Enter[n];
				else if(mode==2) t=NEW1[n];
				else t=NEW2[n];
				Nixie_Set(n+3,t);
			}
			else Nixie_Set(n+3,16);
		}
	}
}

/*WAIT/PROTECT 到期处理：先处理到期，再处理同一时刻的命令*/
void State_Timer_Loop(void){
	if(mode==4&&WAIT_Time==0){
		WAIT_Time=30;
		mode=0;						//保留错误次数（3次）
		AT24C02_WriteByte(10,Wrong_Num);
		AT24C02_WaitStandby();
		AT24C02_WriteByte(11,0);
		AT24C02_WaitStandby();
	}
	else if(mode==5&&PROTECT_Time==0){
		PROTECT_Time=60;
		mode=0;
		Wrong_Num=0;				//保护结束清零
		AT24C02_WriteByte(10,0);
		AT24C02_WaitStandby();
		AT24C02_WriteByte(11,0);
		AT24C02_WaitStandby();
	}
}

void main(void){
	/*初始化*/
	ch=AT24C02_ReadByte(7);					//7=0→1~6为最新密码，7=1→12~17为最新密码
	if(ch==0) Init_Password(1);
	else if(ch==1) Init_Password(12);
	mode=AT24C02_ReadByte(11);				//4/5阶段断电保留，重启重新等待完整时长
	if(mode!=4&&mode!=5) mode=0;
	Wrong_Num=AT24C02_ReadByte(10);
	if(Wrong_Num>5){			//空白EEPROM读出来是0xFF，必须纠正
		Wrong_Num=0;
		AT24C02_WriteByte(10,0);
		AT24C02_WaitStandby();
	}
	P0=0x00;
	Buzzer=0;			//上电静音
	D7=1;				//反馈灯灭
#if FW_B
	D8=0;				//B版：D8常亮
#else
	D8=1;				//A版：D8常暗
#endif
	Nixie_Init();
	Matrix_Init();
	UART_Init();
	Timer0_Init();
	//

	while(1){
		if(ISP_Ready) ISP_CONTR=0x60;		//非阻塞500ms到，软件复位进入ISP
		State_Timer_Loop();					//先处理计时到期
		if(UART_RxFlag){
			if(ISP_Wait==0) Cmd_Process();	//升级准备期间业务暂停，只扫描
			UART_RxFlag=0;
		}
		EA=0;					//读+清零必须原子，否则会覆盖中断刚写入的键值
		k=KeyNumber;
		KeyNumber=0;
		EA=1;
		/*键盘映射到同一套状态机；升级准备期间业务暂停，只扫描*/
		if(k!=0&&ISP_Wait==0){
			Reply_ON=0;					//无主动日志：键盘操作不回串口
			if(k<=10) Act_Digit(k%10);		//1~10 → 0~9
			else if(k==11) Act_Clear();
			else if(k==12) Act_Cancel();
			else if(k==13) Act_Ok();
			else if(k==14) Act_Del();
			else if(k==15) Act_Change();
			else if(k==16) Act_Lock();
			Reply_ON=1;
		}
		Display_Update();					//实时刷新数码管
		k=0;	//按键事件只生效一次
	}
}

void Timer0_Routine() interrupt 1
{
	TL0 = 0x8A;				//1ms@11.0592MHz：0xFC8A；模式1不自动重装，中断入口已耗掉约36周期，重装值必须相应补偿
	TH0 = 0xFC;
	count0++;
	count1++;
	if(ISP_Wait){							//升级准备非阻塞延时
		ISP_Wait--;
		if(ISP_Wait==0) ISP_Ready=1;
	}
	if(count0>=20){KeyNumber=MatrixKey_Loop();count0=0;}
	Nixie_Scan();
	if(count1>=1000){
		count1=0;
		if(mode==4){ if(WAIT_Time) WAIT_Time--; }
		else if(mode==5){ if(PROTECT_Time) PROTECT_Time--; }
		else if(mode==1||mode==2||mode==3){	//15秒无有效操作自动上锁
			if(Lock_Time) Lock_Time--;
			if(Lock_Time==0){
				unsigned char j;
				mode=0;
				tp=1;
				for(j=0;j<6;j++){NEW1[j]=0;NEW2[j]=0;}
			}
		}
	}
	if(Fb_Req){						//动作请求了新的声光反馈
		unsigned char lv=Fb_Req;
		Fb_Req=0;
		if(!(Fb_Pat==FB_RESTRICT&&lv!=FB_RESTRICT)){	//限制提示优先，密集事件合并
			Fb_Pat=lv;
			Fb_Step=0;
			Fb_Timer=Fb_Tab[Fb_Start[lv]];
			Buzzer=1;				//高电平响
			D7=0;					//低电平亮
		}
	}
	if(Fb_Pat){						//按节奏表推进，不阻塞主程序
		if(Fb_Timer) Fb_Timer--;
		if(Fb_Timer==0){
			Fb_Step++;
			if(Fb_Step>=Fb_Len[Fb_Pat]){
				Fb_Pat=0;
				Buzzer=0;
				D7=1;
			}else{
				if(Fb_Step&1){Buzzer=0;D7=1;}
				else{Buzzer=1;D7=0;}
				Fb_Timer=Fb_Tab[Fb_Start[Fb_Pat]+Fb_Step];
			}
		}
	}
}

void UART_Routine() interrupt 4 {
    if(RI){
        RI=0; // 必须手动清零
        ch=SBUF;
        if(ch=='\n'){ 				// 收到换行代表一帧数据结束
            UART_RxBuffer[rx_index]='\0';
            rx_index=0;
            UART_RxFlag=1; 			// 收到完整数据
        }else if(ch!='\r'){			// 忽略回车，串口助手发\r\n也不会多出字符
            if(rx_index<16){			//正文最多16字节
                UART_RxBuffer[rx_index++]=ch;
            }else{
                UART_RxOverflow=1;	//超长行丢弃至LF后回 BAD_CMD
            }
        }
    }
    if(TI){						// 发送完成：清标志并通知UART_SendByte
        TI=0;
        UART_TxDone=1;
    }
}
