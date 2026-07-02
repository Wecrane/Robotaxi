#include "motor.h"
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

MotorStruct motorStr;


/**
* @brief        电机控制初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_Init(void)
{
    //PWM-IO初始化
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);  
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8; 			//PWM
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14; 			//电机方向IO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;          
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_14);	
	
    //TIM初始化
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
    TIM_OCInitTypeDef TIM_OCInitStructure;  
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure; 
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseStructure.TIM_Prescaler = 2;  										
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
    TIM_TimeBaseStructure.TIM_Period = 2000-1;   //72M  3分频 =  12KHz PWM
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;   
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);    
      
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;   //设置PWM模式为向上计数模式 
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;  
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;  
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;  
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;  
    TIM_OCInitStructure.TIM_Pulse = 0;  
    //Set the Channel 1 of TIMER1 
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);   

    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;  
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;  
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;  
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;  
    TIM_OCInitStructure.TIM_Pulse = 0;  
		
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Disable; // Disable the Break function  
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_Low;  
    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable; //Enable Running State  
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable; //Enable Idle State  
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_OFF; //Set the lock level  
    TIM_BDTRInitStructure.TIM_DeadTime = 0x2B;  
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable; //Enable the Auto Outputting.  
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);  
      
    TIM_Cmd(TIM1, ENABLE);    
    TIM_CtrlPWMOutputs(TIM1, ENABLE);   
		
    MOTOR_SetPwmValue(0);
    
    //电机模型初始化
    motorStr.EncoderLine = 512.0f; 							//编码器线数=光栅数16*4				
    motorStr.ReductionRatio = 2.7f;							//电机减速比								
    motorStr.EncoderValue = 0;
    motorStr.DiameterWheel = 0.064f;//68cm					//轮子直径:m
    motorStr.CloseLoop = true;                              //默认闭环模式
}


/**
* @brief        电机输出PWM设置
* @param        pwm：-2000~2000
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_SetPwmValue(signed int pwm)
{   
    pwm = -pwm;
    if(pwm>=0)
    {
        GPIO_SetBits(GPIOB,GPIO_Pin_14);					
        if(pwm>MOTOR_PWM_MAX)
            pwm =MOTOR_PWM_MAX;
        
        TIM_SetCompare1(TIM1,pwm);
    }
    else if(pwm<0)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_14);
        if(pwm<MOTOR_PWM_MIN)
            pwm=MOTOR_PWM_MIN;
        
        pwm = -pwm;

        TIM_SetCompare1(TIM1,pwm);
    }	
    motorStr.PwmOutput = pwm; //[P0-3] 追踪实际PWM输出(用于故障检测)
}


/**
* @brief        电机闭环速控
* @param        speed：速度m/s
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_ControlLoop(float speed)
{	
    if(speed > MOTOR_SPEED_MAX)
        speed = MOTOR_SPEED_MAX;
    else if(speed < -MOTOR_SPEED_MAX)
        speed = -MOTOR_SPEED_MAX;
    
    pidStr.vi_Ref = (float)(speed*MOTOR_CONTROL_CYCLE / motorStr.DiameterWheel / PI * motorStr.EncoderLine * 4.0f * motorStr.ReductionRatio);
    
    MOTOR_SetPwmValue(PID_MoveCalculate(&pidStr));
}


/**
* @brief        电机控制线程
* @param        
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_Timer(void)
{
    motorStr.Counter++;
    if(motorStr.Counter >= 10)							    //速控:10ms
    {
        ENCODER_RevSample();								//编码器采样

        //[P0-3] 编码器断线故障检测：EncoderValue==0且PWM>100连续20周期→紧急停车
        if(!motorStr.FaultLatched) //[审查修复] 锁存后不再重复检测，保留FaultCnt证据
        {
            if(motorStr.EncoderValue == 0 && motorStr.PwmOutput > 100)
            {
                motorStr.FaultCnt++;
                if(motorStr.FaultCnt >= 20) //200ms连续故障
                {
                    motorStr.FaultLatched = true;
                    icarStr.errorCode |= 0x10; //bit4:编码器断线
                }
            }
            else
            {
                motorStr.FaultCnt = 0;
            }
        }

        //[P0-3] 故障锁存：一旦触发，每周期强制停车直至系统复位
        if(motorStr.FaultLatched)
        {
            MOTOR_SetPwmValue(0);
            motorStr.Counter = 0;
            return;
        }

        if(icarStr.sprintEnable || icarStr.selfcheckEnable || usbStr.connected) //[审查修复] 回退&&为||，增加selfcheckEnable确保自检可用
        {
            if(motorStr.CloseLoop)
            {
                MOTOR_ControlLoop(icarStr.SpeedSet);		//闭环速控
            }
            else//开环百分比控制
            {
                if(icarStr.SpeedSet > 100)
                    icarStr.SpeedSet = 100;
                else if(icarStr.SpeedSet < -100)
                    icarStr.SpeedSet = -100;
                signed int speedRate = MOTOR_PWM_MAX/100.f*icarStr.SpeedSet; //开环：百分比%
                
                MOTOR_SetPwmValue(speedRate);		//开环速控
            }
        }
        else
        {
            MOTOR_SetPwmValue(0);
        }
       
        motorStr.Counter = 0;
    }
}

