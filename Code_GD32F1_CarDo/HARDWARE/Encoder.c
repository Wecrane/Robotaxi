#include "encoder.h"
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
* @brief        编码器初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void ENCODER_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;   
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_StructInit(&GPIO_InitStructure);

    TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
    TIM_TimeBaseStructure.TIM_Prescaler = 0x0; // No prescaling 
    TIM_TimeBaseStructure.TIM_Period = 0xffff; 
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;   
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
    TIM_ICStructInit(&TIM_ICInitStructure);
    TIM_ICInitStructure.TIM_ICFilter = 6;
    TIM_ICInit(TIM3, &TIM_ICInitStructure);

    TIM3->CNT = 0;
    TIM_Cmd(TIM3, ENABLE); 
}


/**
* @brief        编码器转速采样
* @param        
* @ref          
* @author       Leo
* @note         
**/
void ENCODER_RevSample(void)
{
    motorStr.EncoderValue = TIM3->CNT;
    TIM3->CNT = 0;

    if(motorStr.EncoderValue > 32767)
        motorStr.EncoderValue = motorStr.EncoderValue - 65536;
    
    //PID负反馈数据输入
    pidStr.vi_FeedBack = motorStr.EncoderValue;
    
    //计算实际速度	---		m/s
    icarStr.SpeedFeedback = (float)(motorStr.EncoderValue * PI * motorStr.DiameterWheel)/ MOTOR_CONTROL_CYCLE / motorStr.EncoderLine / 4.0f / motorStr.ReductionRatio; //  m/s
    
    
    if(icarStr.SpeedFeedback > 0 && icarStr.SpeedFeedback > icarStr.SpeedMaxRecords)
        icarStr.SpeedMaxRecords = icarStr.SpeedFeedback;
        
    else if(icarStr.SpeedFeedback < 0 && -icarStr.SpeedFeedback > icarStr.SpeedMaxRecords)
        icarStr.SpeedMaxRecords = -icarStr.SpeedFeedback;
}




