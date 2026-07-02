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
* @brief    蜂鸣器音效
**/
typedef enum 
{
    BuzzerOk = 0,						//确认提示音
	BuzzerWarnning,						//报警提示音
	BuzzerSysStart,						//开机提示音
    BuzzerDing,                         //叮=====(￣▽￣*)
    BuzzerFinish,                       //结束提示音
}BuzzerEnum;


/**
* @brief    按键和LED相关
**/
typedef struct 
{
	bool KeyPress;					    //按键输入-B
	uint16_t CounterLed;				//LED闪烁计数器
}GpioStruct;


/**
* @brief    蜂鸣器相关
**/
typedef struct 
{
	bool Enable;						//使能标志
	uint16_t Times;					    //鸣叫次数
	uint16_t Counter;				    //计数器
	uint16_t Cut;					    //间隔时间
	bool Silent;						//是否禁用蜂鸣器
}BuzzerStruct;


extern GpioStruct gpioStr;
extern BuzzerStruct buzzerStr;

void GPIO_Initialize(void);
void GPIO_Timer(void);
void GPIO_Handle(void);
void GPIO_BuzzerEnable(BuzzerEnum buzzer);

#endif

//===========================================  End Of File  ===========================================//

