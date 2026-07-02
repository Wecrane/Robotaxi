#ifndef __SOC_H__
#define __SOC_H__

/*-----------------------------------------  I N C L U D E S  -----------------------------------------*/
#include "main.h"


/*---------------------------------------  D E F I N I T I O N  ---------------------------------------*/
//IIC
#define SOC_SDA_IN()  					{GPIOC->CRH&=0XFF0FFFFF;GPIOC->CRH|=8<<20;}	//PC13输入模式
#define SOC_SDA_OUT() 					{GPIOC->CRH&=0XFF0FFFFF;GPIOC->CRH|=3<<20;} //PC13输出模式 
#define SOC_IIC_SCL    					PCout(14) 		//SCL
#define SOC_IIC_SDA    					PCout(13) 		//SDA	 
#define SOC_READ_SDA   					PCin(13)  		//输入SDA 


#define	READ_CW2015							0xc5
#define	WRITE_CW2015						0xc4

#define REG_VERSION             0x0
#define REG_VCELL               0x2
#define REG_SOC                 0x4
#define REG_RRT_ALERT           0x6
#define REG_CONFIG              0x8
#define REG_MODE                0xA
#define REG_BATINFO             0x10

#define MODE_SLEEP_MASK         (0x3<<6)
#define MODE_SLEEP              (0x3<<6)
#define MODE_NORMAL             (0x0<<6)
#define MODE_QUICK_START        (0x3<<4)
#define MODE_RESTART            (0xf<<0)
#define CONFIG_UPDATE_FLG       (0x1<<1)
#define ATHD                    (0x0<<3)        //ATHD = 0%

#define SIZE_BATINFO        		64

#define BATTERY_UP_MAX_CHANGE   720             // the max time allow battery change quantity
#define BATTERY_DOWN_MIN_CHANGE 60              // the min time allow battery change quantity when run
#define BATTERY_DOWN_MIN_CHANGE_SLEEP 1800      // the min time allow battery change quantity when run 30min
//#define BAT_LOW_INTERRUPT    1

/*电池建模信息，拿到自己的电池匹配的建模信息后替换*/
static unsigned char cw_bat_config_info[SIZE_BATINFO] = {
0x15  ,0x4C  ,0x5D  ,0x5D  ,0x5A  ,0x59  ,0x55  ,
0x51  ,0x4E  ,0x48  ,0x46  ,0x41  ,0x3C  ,0x39  ,
0x33  ,0x2D  ,0x25  ,0x1E  ,0x19  ,0x19  ,0x1A  ,
0x2C  ,0x44  ,0x4A  ,0x43  ,0x40  ,0x0C  ,0xCD  ,
0x22  ,0x43  ,0x56  ,0x82  ,0x78  ,0x6F  ,0x62  ,
0x60  ,0x42  ,0x19  ,0x37  ,0x31  ,0x00  ,0x1D  ,
0x59  ,0x85  ,0x8F  ,0x91  ,0x91  ,0x18  ,0x58  ,
0x82  ,0x94  ,0xA5  ,0xFF  ,0xAF  ,0xE8  ,0xCB  ,
0x2F  ,0x7D  ,0x72  ,0xA5  ,0xB5  ,0xC1  ,0x46  ,
0xAE
};

//****************************struct*********************************/
/**
* @brief    电量计相关
**/
typedef struct  
{
	unsigned char UsbOnline;					            //USB插入状态
	unsigned int Capacity;						            //电量
	unsigned int voltage;							        //电压值
	uint32_t Counter;									    //计数器
}SocStruct;


extern SocStruct socStr;

void SOC_IIC_Init(void);									//IIC-IO初始化
void SOC_IIC_Start(void);									//产生IIC起始信号
void SOC_IIC_Stop(void);									//产生IIC停止信号
u8 SOC_IIC_Wait_Ack(void);									//等待应答信号到来
void SOC_IIC_Ack(void);										//产生ACK应答
void SOC_IIC_NAck(void);									//不产生ACK应答	
void SOC_IIC_Send_Byte(u8 txd);							    //IIC发送一个字节
u8 SOC_IIC_Read_Byte(unsigned char ack);		            //读1个字节
u8 SOC_Write_Len(u8 reg,u8 len,u8 *buf);		            //IIC连续写指定长度数据
u8 SOC_Write(u8 reg,u8 *buf);								//IIC写入数据
u8 SOC_Read_Len(u8 reg,u8 len,u8 *buf);			            //IIC连续读取指定长度数据
u8 SOC_Read(u8 reg,u8 *buf);								//IIC读取指定地址的数据

unsigned char SOC_HardwareInit(void);				        //电量计底层初始化
unsigned char SOC_UpdataConfigInfo(void);		            //更新电池信息
int SOC_Por(void);											//电量计上电复位
int SOC_GetCapacity(void);									//获取电量
unsigned int SOC_GetVol(void);							    //获取电压值
void SOC_UpdateCapacity(void);							    //SOC更新电池电量
void SOC_UpdateVol(void);									//SOC更新电池电压
void SOC_UpdateUsbOnline(void);							    //USB插入状态检测
void SOC_BatWork(void);									    //SOC控制器

unsigned char SOC_Init(void);								//SOC初始化
void SOC_Timer(void);										//电量计IC控制时序
void SOC_Handle(void);										//电量计IC控制逻辑
#endif


