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
    motorStr.PwmOutput = 0;
    motorStr.FaultCnt = 0;
    motorStr.FaultLatched = false;
    motorStr.FaultRecoverRequested = false;
    motorStr.FaultRecoverCnt = 0;
    motorStr.FaultRecoverOkCnt = 0;
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
    signed int pwm;
    uint8_t zeroSpeedCommand = 0;

    if(speed > MOTOR_SPEED_MAX)
        speed = MOTOR_SPEED_MAX;
    else if(speed < -MOTOR_SPEED_MAX)
        speed = -MOTOR_SPEED_MAX;

    if(speed > -0.001f && speed < 0.001f)
        zeroSpeedCommand = 1;
    
    pidStr.vi_Ref = (float)(speed*MOTOR_CONTROL_CYCLE / motorStr.DiameterWheel / PI * motorStr.EncoderLine * 4.0f * motorStr.ReductionRatio);

    pwm = PID_MoveCalculate(&pidStr);
    if(zeroSpeedCommand)
    {
        if(pwm > MOTOR_ZERO_BRAKE_PWM_LIMIT)
            pwm = MOTOR_ZERO_BRAKE_PWM_LIMIT;
        else if(pwm < -MOTOR_ZERO_BRAKE_PWM_LIMIT)
            pwm = -MOTOR_ZERO_BRAKE_PWM_LIMIT;
    }

    MOTOR_SetPwmValue(pwm);
}


/**
* @brief        清除编码器故障锁存
* @param
* @ref
* @author       Codex
* @note         仅在恢复探测确认编码器有脉冲后调用
**/
void MOTOR_ClearEncoderFault(void)
{
    MOTOR_SetPwmValue(0);
    icarStr.SpeedSet = 0.0f;
    icarStr.errorCode &= ~(1 << 4);

    motorStr.FaultLatched = false;
    motorStr.FaultCnt = 0;
    motorStr.FaultRecoverRequested = false;
    motorStr.FaultRecoverCnt = 0;
    motorStr.FaultRecoverOkCnt = 0;
    pidStr.vi_Ref = 0.0f;
    pidStr.vi_FeedBack = 0.0f;
    pidStr.vi_PreError = 0.0f;
    pidStr.vi_PreDerror = 0.0f;
    pidStr.vl_PreU = 0.0f;
    TIM3->CNT = 0;
}


/**
* @brief        请求编码器故障恢复探测
* @param
* @ref
* @author       Codex
* @note         由USB 0x0C命令触发；未锁存时只清计数和错误位
**/
void MOTOR_RequestEncoderFaultRecovery(void)
{
    icarStr.SpeedSet = 0.0f;
    MOTOR_SetPwmValue(0);

    motorStr.FaultRecoverCnt = 0;
    motorStr.FaultRecoverOkCnt = 0;
    TIM3->CNT = 0;

    pidStr.vi_Ref = 0.0f;
    pidStr.vi_FeedBack = 0.0f;
    pidStr.vi_PreError = 0.0f;
    pidStr.vi_PreDerror = 0.0f;
    pidStr.vl_PreU = 0.0f;

    if(motorStr.FaultLatched)
    {
        motorStr.FaultRecoverRequested = true;
    }
    else
    {
        motorStr.FaultCnt = 0;
        icarStr.errorCode &= ~(1 << 4);
    }
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
        signed int pwmAbs;
        ENCODER_RevSample();								//编码器采样

        //[P0-3] 编码器断线故障检测：EncoderValue==0且PWM持续较大→紧急停车
        pwmAbs = motorStr.PwmOutput >= 0 ? motorStr.PwmOutput : -motorStr.PwmOutput;
        if(!motorStr.FaultLatched) //[审查修复] 锁存后不再重复检测，保留FaultCnt证据
        {
            if(motorStr.EncoderValue == 0 && pwmAbs > MOTOR_ENCODER_FAULT_PWM_THRESHOLD)
            {
                motorStr.FaultCnt++;
                if(motorStr.FaultCnt >= MOTOR_ENCODER_FAULT_CYCLES) //连续1.5s无编码器反馈
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

        //[P0-3] 故障锁存：默认强制停车；收到恢复请求后短时低PWM探测编码器
        if(motorStr.FaultLatched)
        {
            if(motorStr.FaultRecoverRequested)
            {
                if(motorStr.FaultRecoverCnt > 0 && motorStr.EncoderValue != 0)
                    motorStr.FaultRecoverOkCnt++;
                else if(motorStr.FaultRecoverCnt > 0)
                    motorStr.FaultRecoverOkCnt = 0;

                if(motorStr.FaultRecoverOkCnt >= MOTOR_ENCODER_RECOVER_OK_CYCLES)
                {
                    MOTOR_ClearEncoderFault();
                    motorStr.Counter = 0;
                    return;
                }

                if(motorStr.FaultRecoverCnt < MOTOR_ENCODER_RECOVER_PROBE_CYCLES)
                {
                    motorStr.FaultRecoverCnt++;
                    MOTOR_SetPwmValue(MOTOR_ENCODER_RECOVER_PROBE_PWM);
                }
                else
                {
                    motorStr.FaultRecoverRequested = false;
                    motorStr.FaultRecoverCnt = 0;
                    motorStr.FaultRecoverOkCnt = 0;
                    MOTOR_SetPwmValue(0);
                }
            }
            else
            {
                MOTOR_SetPwmValue(0);
            }
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

