#ifndef __CMD_PROCESS_H__
#define __CMD_PROCESS_H__
/*声光反馈等级：动作里写 Fb_Req，1ms中断里按节奏播放*/
#define FB_NONE		0
#define FB_NORMAL	1		//普通操作：响/亮30ms
#define FB_SUCCESS	2		//解锁、保存成功：两次50ms，中间隔50ms
#define FB_ERROR	3		//错误/拒绝：200ms
#define FB_RESTRICT	4		//进入限制：三次100ms，中间隔100ms
void Cmd_Process(void);
/*键盘与串口共用的状态机动作*/
void Act_Digit(unsigned char n);
void Act_Del(void);
void Act_Clear(void);
void Act_Ok(void);
void Act_Change(void);
void Act_Cancel(void);
void Act_Lock(void);
void Act_Status(void);
#endif
