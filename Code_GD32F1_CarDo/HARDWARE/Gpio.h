#ifndef __GPIO_H__
#define __GPIO_H__

/*-----------------------------------------  I N C L U D E S  -----------------------------------------*/

#include "main.h"

/*---------------------------------------  D E F I N I T I O N  ---------------------------------------*/

#define LED_ON          (GPIO_ResetBits(GPIOB,GPIO_Pin_12)) 		
#define LED_OFF         (GPIO_SetBits(GPIOB,GPIO_Pin_12)) 			
#define LED_REV         (GPIOB->ODR ^= GPIO_Pin_12)							

#define BUZZER_ON        (GPIO_SetBits(GPIOA,GPIO_Pin_3))  			
#define BUZZER_OFF       (GPIO_ResetBits(GPIOA,GPIO_Pin_3))  		
#define BUZZER_REV       (GPIOA->ODR ^= GPIO_Pin_3)						


/*---------------------------------------  D E F I N I T I O N  ---------------------------------------*/
/**
* @brief    ��������Ч
**/
typedef enum 
{
    BuzzerOk = 0,						//ȷ����ʾ��
	BuzzerWarnning,						//������ʾ��
	BuzzerSysStart,						//������ʾ��
    BuzzerDing,                         //��=====(������*)
    BuzzerFinish,                       //������ʾ��
}BuzzerEnum;


/**
* @brief    ������LED���
**/
typedef struct 
{
	bool KeyPress;					    //按键标志-B
	uint16_t CounterLed;				//LED闪烁计数器
	uint16_t KeyDebounce;				//按键消抖计数器(ms) [P3-4]
}GpioStruct;


/**
* @brief    ���������
**/
typedef struct 
{
	bool Enable;						//ʹ�ܱ�־
	uint16_t Times;					    //���д���
	uint16_t Counter;				    //������
	uint16_t Cut;					    //���ʱ��
	bool Silent;						//�Ƿ���÷�����
}BuzzerStruct;


extern GpioStruct gpioStr;
extern BuzzerStruct buzzerStr;

void GPIO_Initialize(void);
void GPIO_Timer(void);
void GPIO_Handle(void);
void GPIO_BuzzerEnable(BuzzerEnum buzzer);

#endif

//===========================================  End Of File  ===========================================//

