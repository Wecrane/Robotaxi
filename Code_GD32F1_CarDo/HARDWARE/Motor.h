#ifndef __PWM_H__
#define __PWM_H__

/*-----------------------------------------  I N C L U D E S  -----------------------------------------*/
#include "main.h"

/*---------------------------------------  D E F I N I T I O N  ---------------------------------------*/

#define  MOTOR_PWM_MAX   			1500		//OCR=95%,禁止满占空比输出，造成MOS损坏
#define  MOTOR_PWM_MIN			    -1500		//OCR=95%
#define  MOTOR_SPEED_MAX		    10.0f	 	//电机最大转速(m/s) (0.017,8.04)
#define  PI					        3.141593f   //π
#define  MOTOR_CONTROL_CYCLE	    0.01f    	//电机控制周期T：10ms
#define  MOTOR_ENCODER_FAULT_PWM_THRESHOLD 100  //编码器故障检测PWM阈值
#define  MOTOR_ENCODER_FAULT_CYCLES 150         //150*10ms=1.5s，避免大地面阻力起步误触发
#define  MOTOR_ENCODER_RECOVER_PROBE_PWM 160    //编码器恢复探测PWM，略高于故障阈值
#define  MOTOR_ENCODER_RECOVER_PROBE_CYCLES 20  //20*10ms=200ms，短时探测
#define  MOTOR_ENCODER_RECOVER_OK_CYCLES 2      //连续2个周期有脉冲才清故障

// Keep the original continuous speed PID behavior when SpeedSet is 0, but cap
// the zero-speed PID output so stopping is firm without commanding full PWM.
#define  MOTOR_ZERO_BRAKE_PWM_LIMIT      700

/**
* @brief    电机相关
**/
typedef struct 
{
	float ReductionRatio ;					    //电机减速比
	float EncoderLine ; 						//编码器线数=光栅数16*4
	signed int EncoderValue;				    //编码器实时速度
	float DiameterWheel;						//轮子直径：mm
	bool CloseLoop;							    //开环模式
	uint16_t Counter;							//线程计数器
	signed int PwmOutput;						//[P0-3] 实际PWM输出值(用于故障检测)
	uint8_t FaultCnt;							//[P0-3] 编码器断线连续计数
	bool FaultLatched;							//[P0-3] 故障锁存标志
	bool FaultRecoverRequested;					//编码器故障恢复探测请求
	uint8_t FaultRecoverCnt;					//恢复探测周期计数
	uint8_t FaultRecoverOkCnt;					//恢复探测有效脉冲计数
}MotorStruct;


extern MotorStruct motorStr;


void MOTOR_Init(void);
void MOTOR_SetPwmValue(signed int pwm);
void MOTOR_ControlLoop(float speed);
void MOTOR_Timer(void);
void MOTOR_RequestEncoderFaultRecovery(void);
void MOTOR_ClearEncoderFault(void);


//===========================================  End Of File  ===========================================//
#endif


