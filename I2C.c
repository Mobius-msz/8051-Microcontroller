#include <REGX52.H>

sbit I2C_SCL=P2^1;
sbit I2C_SDA=P2^0;
/** @brief I2C开始
  * @param 无
  * @retval	无
  */
void I2C_Start(void){
	I2C_SDA=1;		//初始化-》先置SDA因为SDA的数据可能为1/0,而SCL不读写一定置0,SLC置1时读入
	I2C_SCL=1;		//初始化
	I2C_SDA=0;		//满足SCL高电平时SDA从1切换为0的开始条件
	I2C_SCL=0;		//SCL置1时读写数据，置0时为了初始化
}

/** @brief I2C结束
  * @param	无
  * @retval	无
  */
void I2C_Stop(void){
	I2C_SDA=0;
	I2C_SCL=1;
	I2C_SDA=1;
}

/** @brief I2C发送一个字节
  * @param	Byte 要发送的字节
  * @retval	无
  */
void I2C_SendByte(unsigned char Byte){
	unsigned char i;
	for(i=0;i<8;i++){
		I2C_SDA=Byte&(0x80>>i);
		I2C_SCL=1;
		I2C_SCL=0;
	}
	
}

/** @brief I2C接受一个字节
  * @param	无
  * @retval	接受到的一个字节数据
  */
unsigned char I2C_ReceiveByte(void){
	unsigned char Byte=0,i;
	I2C_SDA=1;				//I2C接受从机数据时，先把总线释放，再在下方if判断0/1
	for(i=0;i<8;i++){
		I2C_SCL=1;
		if(I2C_SDA){Byte|=(0x80>>i);}
		I2C_SCL=0;
	}
	return Byte;
}

/** @brief I2C发送应答
  * @param	AckBit 应答位，0为应答，1为非应答
  * @retval	无
  */
void I2C_SendAck(unsigned char AckBit){
	I2C_SDA=AckBit;
	I2C_SCL=1;
	I2C_SCL=0;
}

/** @brief I2C接受应答
  * @param	无
  * @retval	接受到的应答位，0为应答，1为非应答
  */
unsigned char I2C_ReceiveAck(void){
	unsigned char AckBit;
	I2C_SDA=1;
	I2C_SCL=1;
	AckBit=I2C_SDA;
	I2C_SCL=0;
	return AckBit;
}