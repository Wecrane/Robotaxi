#ifndef __PWM_H__
#define __PWM_H__

/*-----------------------------------------  I N C L U D E S  -----------------------------------------*/
#include "main.h"

/*---------------------------------------  D E F I N I T I O N  ---------------------------------------*/

#define  MOTOR_PWM_MAX   			1500		//OCR=95%,��ֹ��ռ�ձ���������MOS��
#define  MOTOR_PWM_MIN			    -1500		//OCR=95%
#define  MOTOR_SPEED_MAX		    10.0f	 	//������ת��(m/s) (0.017,8.04)
#define  PI					        3.141593f   //��
#define  MOTOR_CONTROL_CYCLE	    0.01f    	//�����������T��10ms

/**
* @brief    ������
**/
typedef struct 
{
	float ReductionRatio ;					    //������ٱ�
	float EncoderLine ; 						//����������=��դ��16*4
	signed int EncoderValue;				    //������ʵʱ�ٶ�
	float DiameterWheel;						//����ֱ����mm
	bool CloseLoop;							    //����ģʽ
	uint16_t Counter;							//�̼߳�����
	signed int PwmOutput;						//[P0-3] 当前PWM输出值
	uint16_t FaultCnt;							//[P0-3] 编码器故障连续计数
	bool FaultLatched;							//[P0-3] 故障锁存标志
}MotorStruct;


extern MotorStruct motorStr;


void MOTOR_Init(void);
void MOTOR_SetPwmValue(signed int pwm);
void MOTOR_ControlLoop(float speed);
void MOTOR_Timer(void);


//===========================================  End Of File  ===========================================//
#endif


