#include "timer.h"
/*
********************************************************************************************************
*                                               示例代码
*                                             EXAMPLE  CODE                                             
*
*                             (c) Copyright 2021; SaiShu.Lcc.; Leo
*                                 版权所属[北京赛曙科技有限公司]
*
*               The code is for internal use only, not for commercial transactions(开源学习,请勿商用).
*               The code ADAPTS the corresponding hardware circuit board(代码使用CarDo智控板), 
*               the specific details consult the professional(欢迎联系我们).
*********************************************************************************************************
*/


/**
* @brief        定时器TIM2初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);              //时钟使能

    TIM_TimeBaseStructure.TIM_Period = 1000-1;                        //设置在下一个更新事件装入活动的自动重装载寄存器周期的值  -> 10 * 1us = 10us
    TIM_TimeBaseStructure.TIM_Prescaler = 72-1;                       //设置用来作为TIMx时钟频率除数的预分频值  -> 72M/(71+1) = 1us
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;           //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;       //TIM向上计数模式
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);                   //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
    TIM_ITConfig(TIM2, TIM_IT_Update,ENABLE); 													//允许定时器7更新中断
    TIM_Cmd(TIM2, ENABLE);                                            //使能TIM7

    TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;                   //TIM7中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;         //先占优先级1级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;                //从优先级3级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;                   //IRQ通道被使能
    NVIC_Init(&NVIC_InitStructure);                                   //根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器
}


/**
* @brief        TIM2定时中断服务
* @param        
* @ref          
* @author       Leo
* @note         
**/
void TIM2_IRQHandler(void)    //1ms触发一次
{
    if(TIM_GetITStatus(TIM2, TIM_IT_CC4) == SET)      //蜂鸣器连续音调比较中断
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC4);
        GPIO_BuzzerToneIrq();
    }
    
    if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)		//溢出中断
    {	
        GPIO_Timer();			//GPIO外设线程
        MOTOR_Timer();			//电机控制线程
        SOC_Timer();			//电量计监测线程
        ICAR_Timer();           //智能车综合处理线程计数器
        USB_Edgeboard_Timr();   //USB通信线程
    }
    
    TIM_ClearITPendingBit(TIM2, TIM_IT_Update);				//清除中断标志位
}














