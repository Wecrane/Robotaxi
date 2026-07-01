#include "motor.h"
/*
********************************************************************************************************
*                                               ʾ������
*                                             EXAMPLE  CODE                                             
*
*                             (c) Copyright 2021; SaiShu.Lcc.; Leo
*                                 ��Ȩ����[��������Ƽ����޹�˾]
*
*               The code is for internal use only, not for commercial transactions(��Դѧϰ,��������).
*               The code ADAPTS the corresponding hardware circuit board(����ʹ��CarDo�ǿذ�), 
*               the specific details consult the professional(��ӭ��ϵ����).
*********************************************************************************************************
*/

MotorStruct motorStr;


/**
* @brief        ������Ƴ�ʼ��?* @param        
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_Init(void)
{
    //PWM-IO��ʼ��
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);  
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_8; 			//PWM
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14; 			//�������IO
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;          
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_SetBits(GPIOB,GPIO_Pin_14);	
	
    //TIM��ʼ��
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  
    TIM_OCInitTypeDef TIM_OCInitStructure;  
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure; 
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    TIM_TimeBaseStructure.TIM_Prescaler = 2;  										
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
    TIM_TimeBaseStructure.TIM_Period = 2000-1;   //72M  3��Ƶ =  12KHz PWM
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;   
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);    
      
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;   //����PWMģʽΪ���ϼ���ģʽ 
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
    
    //���ģ�ͳ�ʼ��?    motorStr.EncoderLine = 512.0f; 							//����������=��դ��16*4				
    motorStr.ReductionRatio = 2.7f;							//������ٱ�?							
    motorStr.EncoderValue = 0;
    motorStr.DiameterWheel = 0.064f;//68cm					//����ֱ��:m
    motorStr.CloseLoop = true;                              //Ĭ�ϱջ�ģʽ
    motorStr.PwmOutput = 0;
    motorStr.FaultCnt = 0;
    motorStr.FaultLatched = false;
}


/**
* @brief        ������PWM����
* @param        pwm��-2000~2000
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

    //[P0-3] 追踪当前PWM输出值（限幅后的实际写入值，用于编码器故障检测）
    motorStr.PwmOutput = pwm;
}


/**
* @brief        ����ջ��ٿ�?* @param        speed���ٶ�m/s
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
* @brief        ��������߳�?* @param        
* @ref          
* @author       Leo
* @note         
**/
void MOTOR_Timer(void)
{
    motorStr.Counter++;
    if(motorStr.Counter >= 10)							    //主控周期:10ms
    {
        ENCODER_RevSample();								//编码器采�?
        //[P0-4] &&替代||：必须同时满足冲刺使能、上位机连接，防止单条件绕过保护
        if(!motorStr.FaultLatched && (icarStr.sprintEnable || icarStr.selfcheckEnable) && usbStr.connected)
        {
            if(motorStr.CloseLoop)
            {
                MOTOR_ControlLoop(icarStr.SpeedSet);		//闭环控制
            }
            else//开环百分比控制
            {
                if(icarStr.SpeedSet > 100)
                    icarStr.SpeedSet = 100;
                else if(icarStr.SpeedSet < -100)
                    icarStr.SpeedSet = -100;
                signed int speedRate = MOTOR_PWM_MAX/100.f*icarStr.SpeedSet; //输出速度百分�?
                
                MOTOR_SetPwmValue(speedRate);		//开环控制
            }
        }
        else
        {
            MOTOR_SetPwmValue(0);
        }

        //[P0-3] 编码器故障检测：EncoderValue==0且PwmOutput>100，连续20周期(200ms)触发紧急停车
        //       启动豁免：电机曾转动过(abs(EncoderValue)>5)后才使能检测
        if(!motorStr.FaultLatched)
        {
            if(motorStr.EncoderValue == 0 && motorStr.PwmOutput > 100)
            {
                motorStr.FaultCnt++;
                if(motorStr.FaultCnt >= 20)
                {
                    motorStr.FaultLatched = true;			//锁存故障
                    MOTOR_SetPwmValue(0);					//紧急停车
                    icarStr.errorCode |= 0x10;				//故障码bit4:编码器断线
                    motorStr.FaultCnt = 0;
                }
            }
            else
            {
                motorStr.FaultCnt = 0;						//条件不满足立即清�?防偶发零值累�?
            }
        }
       
        motorStr.Counter = 0;
    }
}

