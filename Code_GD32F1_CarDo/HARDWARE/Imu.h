#ifndef _IMU_H_
#define _IMU_H_

#include "main.h"


// 定义MPU6050内部地址
//****************************************
#define	SMPLRT_DIV		0x19		//陀螺仪采样率，典型值：0x07(125Hz)
#define	CONFIG			0x1A		//低通滤波频率，典型值：0x06(5Hz)
#define	GYRO_CONFIG		0x1B		//陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define	ACCEL_CONFIG	0x1C		//加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define	ACCEL_XOUT_H	0x3B
#define	ACCEL_XOUT_L	0x3C
#define	ACCEL_YOUT_H	0x3D
#define	ACCEL_YOUT_L	0x3E
#define	ACCEL_ZOUT_H	0x3F
#define	ACCEL_ZOUT_L	0x40
#define	TEMP_OUT_H		0x41
#define	TEMP_OUT_L		0x42

#define	GYRO_XOUT_H		0x43
#define	GYRO_XOUT_L		0x44
#define	GYRO_YOUT_H		0x45
#define	GYRO_YOUT_L		0x46
#define	GYRO_ZOUT_H		0x47
#define	GYRO_ZOUT_L		0x48

#define	PWR_MGMT_1		0x6B	//电源管理，典型值：0x00(正常启用)
#define	WHO_AM_I		  0x75	//IIC地址寄存器(默认数值0x68，只读)


//****************************

#define	MPU6050_Addr      0xD0	  //默认IIC写地址，AD0=0
#define	MPU6050_Addr_ALT  0xD2	  //备用IIC写地址，AD0=1

// PB14 is used by MOTOR direction on this board. Keep IMU bus disabled
// until the real SDA/SCL pins are confirmed and changed below.
#define IMU_SOFT_IIC_ENABLE 0

//************************************
/*模拟IIC端口输出输入定义*/
#define SCL_H         GPIOB->BSRR = GPIO_Pin_14
#define SCL_L         GPIOB->BRR  = GPIO_Pin_14

#define SDA_H         GPIOB->BSRR = GPIO_Pin_13
#define SDA_L         GPIOB->BRR  = GPIO_Pin_13

#define SCL_read      GPIOB->IDR  & GPIO_Pin_14
#define SDA_read      GPIOB->IDR  & GPIO_Pin_13


typedef struct  
{
	uint16_t Counter;											//线程计数器
	short AacX;												    //X轴加速度
	short AacY;												    //Y轴加速度
	short AacZ;													//Z轴加速度
	short GyroX;												//X轴角速度
	short GyroY;												//Y轴角速度
	short GyroZ;												//Z轴角速度
	short GyroRawZ;											//Z轴角速度原始值
	float GyroZDps;												//Z轴角速度 deg/s
	float GyroZOffset;											//Z轴零偏 deg/s
	float YawDeg;												//相对航向角 deg
	float CalibSum;												//零偏标定累计
	uint16_t CalibCount;										//零偏标定计数
	unsigned char Address;										//当前MPU6050 IIC写地址
	unsigned char WhoAmI;										//WHO_AM_I寄存器
	bool Calibrated;											//零偏标定完成
	bool Present;												//IMU芯片在线
	bool ReadOk;												//最近一次采样读寄存器成功
	bool Valid;													//IMU数据有效
}IMU_STA;


extern IMU_STA ImuStructure;


void IMU_Init(void);
void IMU_Handle(void);
void IMU_Timer(void);


#endif


