#include "Inspector.h"


UsbStruct usbInspector;
InspectorStr inspectorStructure;

/**
* @brief        USB/UART初始化
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Inspector_Init(void)
{  
    //UART2初始化
    USART_InitTypeDef USART_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    //PA2,PA3 复用IO口功能用于配置串口。因此要使能AFIO（复用功能IO）时钟。
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); 
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		//浮空输入模式	   
    GPIO_Init(GPIOA, &GPIO_InitStructure);					 
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;			  	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    USART_InitStructure.USART_BaudRate = 115200;	
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;			//数据位8位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;					//停止位1位
    USART_InitStructure.USART_Parity = USART_Parity_No;							//无校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;  //无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(USART2, &USART_InitStructure);//配置串口参数函数
    USART_Cmd(USART2, ENABLE); 
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure); 
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);            			//使能接收中断
        
    //USB数据初始化
    usbInspector.counter = 0;
    usbInspector.receiveFinished = false;
    usbInspector.receiveStart = false;
    usbInspector.receiveIndex = 0;
}

/**
* @brief        USB-TypeC(监测软件)发送一个字节
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Inspector_TransmitByte(uint8_t data)
{
    USART2->SR;           
    USART_SendData(USART2, data);
    while(USART_GetFlagStatus(USART2,USART_FLAG_TC) != SET);	//等待发送结束
}


/**
* @brief        USB/UART接收中断函数
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USART2_IRQHandler(void)  
{	 
    uint8_t Uart1Res;
    if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {   
        Uart1Res = USART_ReceiveData(USART2); 
        if(Uart1Res == USB_FRAME_HEAD && !usbInspector.receiveStart)//监测帧头
        {
            usbInspector.receiveStart = true;
            usbInspector.receiveBuff[0] = Uart1Res;
            usbInspector.receiveBuff[2] = USB_FRAME_LENMIN;
            usbInspector.receiveIndex = 1;
        }
        else if(usbInspector.receiveIndex == 2)	//接收帧长度
        {
            usbInspector.receiveBuff[usbInspector.receiveIndex] = Uart1Res;
            usbInspector.receiveIndex++;
            
            if(Uart1Res > USB_FRAME_LENMAX || Uart1Res < USB_FRAME_LENMIN) //帧长错误
            {
                usbInspector.receiveBuff[2] = USB_FRAME_LENMIN;
                usbInspector.receiveIndex = 0;
                usbInspector.receiveStart = false;
            }
        }
        else if(usbInspector.receiveStart && usbInspector.receiveIndex < USB_FRAME_LENMAX)
        {
            usbInspector.receiveBuff[usbInspector.receiveIndex] = Uart1Res;
            usbInspector.receiveIndex++;
        }
        
        //接收帧完毕
        if((usbInspector.receiveIndex >= USB_FRAME_LENMAX || usbInspector.receiveIndex >= usbInspector.receiveBuff[2]) && usbInspector.receiveIndex > USB_FRAME_LENMIN)
        {
            uint8_t check = 0;
            uint8_t length = USB_FRAME_LENMIN;
        
            length = usbInspector.receiveBuff[2];
            for(int i=0;i<length-1;i++)
                check += usbInspector.receiveBuff[i];
            
            if(check == usbInspector.receiveBuff[length-1])//校验位
            {
                memcpy(usbInspector.receiveBuffFinished,usbInspector.receiveBuff,USB_FRAME_LENMAX);	
                usbInspector.receiveFinished = true;
                
                //智能车控制指令特殊处理
                if(USB_ADDR_CONTROL  == usbInspector.receiveBuffFinished[1])
                {
                    Bint16_Union bint16_Union;
                    Bint32_Union bint32_Union;
                    for(int i=0;i<4;i++)
                        bint32_Union.U8_Buff[i] = usbInspector.receiveBuffFinished[3+i];
                    
                    bint16_Union.U8_Buff[0] = usbInspector.receiveBuffFinished[7];
                    bint16_Union.U8_Buff[1] = usbInspector.receiveBuffFinished[8];
                    
                    SERVO_SetPwmValueCorrect(bint16_Union.U16);
                    icarStr.ServoPwmSet = bint16_Union.U16;
                    if(motorStr.CloseLoop)//闭环速控
                        icarStr.SpeedSet = bint32_Union.Float;   
                    else
                        icarStr.SpeedSet = MOTOR_PWM_MAX /100.f*bint32_Union.Float; //开环：百分比%
                    GPIO_BuzzerEnable(BuzzerOk);
                }
            }
            
            usbInspector.receiveIndex = 0;
            usbInspector.receiveStart = false;
        }
    }

    USART_ClearFlag(USART2, USART_IT_RXNE); 
}



/**
* @brief        USB通信处理函数
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Inspector_Handle(void)
{
    
    //-----------------------[数据接收处理]-----------------------------
    if(usbInspector.receiveFinished)																//接收成功
    {
        usbInspector.receiveFinished = false;
        Bint32_Union bint32_Union;
        Bint16_Union bint16_Union;
           
        if(usbInspector.receiveBuffFinished[1] & 0x80)	//读数据
        {
            uint8_t Addr = (uint8_t)(usbInspector.receiveBuffFinished[1] & 0x7F);
            switch(Addr)
            {
                case USB_ADDR_BATTERY :      //电池信息
                    break;
                
                case USB_ADDR_SERVOTHRESHOLD :   //舵机阈值
                    break;
            }
        }
        else //写数据
        {
            switch(usbInspector.receiveBuffFinished[1])
            {
                case USB_ADDR_HEART :    //监测软件心跳
                    inspectorStructure.enable = true;
                    inspectorStructure.counterDrop = 0;
                    break;
                
                case USB_ADDR_SERVOTHRESHOLD :   //舵机阈值
                    if(usbInspector.receiveBuffFinished[3] == 1)          //左转阈值
                    {
                        bint16_Union.U8_Buff[0] = usbInspector.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbInspector.receiveBuffFinished[5];
                        servoStr.thresholdLeft = bint16_Union.U16;
                        SERVO_SetPwmValue(servoStr.thresholdLeft);
                        GPIO_BuzzerEnable(BuzzerDing);
                        flashSaveEnable = true;
                    }
                    else if(usbInspector.receiveBuffFinished[3] == 2)     //右转阈值
                    {
                        bint16_Union.U8_Buff[0] = usbInspector.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbInspector.receiveBuffFinished[5];
                        servoStr.thresholdRight = bint16_Union.U16;
                        SERVO_SetPwmValue(servoStr.thresholdRight);
                        GPIO_BuzzerEnable(BuzzerDing);
                        flashSaveEnable = true;
                    }
                    else if(usbInspector.receiveBuffFinished[3] == 3)     //中值
                    {
                        bint16_Union.U8_Buff[0] = usbInspector.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbInspector.receiveBuffFinished[5];
                        servoStr.thresholdMiddle = bint16_Union.U16;
                        SERVO_SetPwmValue(servoStr.thresholdMiddle);
                        GPIO_BuzzerEnable(BuzzerDing);
                        flashSaveEnable = true;
                    }
                    break;
                
                case USB_ADDR_BUZZER :      //蜂鸣器音效
                    if(usbInspector.receiveBuffFinished[3] == 1)          //OK
                        GPIO_BuzzerEnable(BuzzerOk);
                    else if(usbInspector.receiveBuffFinished[3] == 1)     //Warnning
                        GPIO_BuzzerEnable(BuzzerWarnning);
                    else if(usbInspector.receiveBuffFinished[3] == 1)     //Finish
                        GPIO_BuzzerEnable(BuzzerFinish);
                    else if(usbInspector.receiveBuffFinished[3] == 1)     //Ding
                        GPIO_BuzzerEnable(BuzzerDing);
                    else if(usbInspector.receiveBuffFinished[3] == 1)     //SystemStart
                        GPIO_BuzzerEnable(BuzzerSysStart);
                    
                    break;
                
                case USB_ADDR_LIGHT :         //LED灯效
                    for(int i=0;i<4;i++)
                        bint32_Union.U8_Buff[i] = usbInspector.receiveBuffFinished[i+3];
                
                    RGB_SetAllColor((unsigned long)bint32_Union.U32);
                    rgbStr.lastColor = (unsigned long)bint32_Union.U32;
                
                    break;
                
                case USB_ADDR_SPEEDMODE :        //速控模式切换
                    if(usbInspector.receiveBuffFinished[3] == 1)    //开环模式
                        motorStr.CloseLoop = false;                    
                    else
                        motorStr.CloseLoop = true;
                    
                    icarStr.SpeedSet = 0;
                    GPIO_BuzzerEnable(BuzzerDing);
                    break;
                    
                case USB_ADDR_SELFCHECK :     //智能车自检
                {
                    ICAR_SelfcheckControl(usbInspector.receiveBuffFinished[3]);
                    break;
                }
            }
        }
    }
    
    //-----------------------[数据发送处理]-----------------------------
    if(inspectorStructure.enable && inspectorStructure.counterSend > 150)//150ms
    {
        USB_SendToInspector_ServoThreshold(1);
        Delay_Ms(1);
        USB_SendToInspector_ServoThreshold(2);
        Delay_Ms(1);
        USB_SendToInspector_ServoThreshold(3);
        Delay_Ms(1);
        USB_SendToInspector_BatteryInfo();
        Delay_Ms(1);
        USB_SendToInspector_CarSpeed();
        inspectorStructure.counterSend = 0; 
    }
}
	


/**
* @brief        监测软件线程控制器
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Inspector_Timer(void)
{
    if(inspectorStructure.enable)
    {
        inspectorStructure.counterSend++;
        inspectorStructure.counterDrop++;
        if(inspectorStructure.counterDrop >3000)//3s
        {
            inspectorStructure.enable = false;
            icarStr.selfcheckEnable = false;
        }
    }
    
}

//----------------------------------------------[UNIT-下位机与监测软件通信]----------------------------------------------------------

/**
* @brief        发送舵机阈值
* @param        chanel: 1/左转阈值，2/右转阈值，3/中值
* @ref          
* @author       Leo
* @note         
**/
void USB_SendToInspector_ServoThreshold(uint8_t chanel)
{
    if(chanel<1 || chanel>3)
        return;
    
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[9];
    buff[0] = 0x42; //帧头
    buff[1] = USB_ADDR_SERVOTHRESHOLD ; //地址
    buff[2] = 0x07; //帧长
    buff[3] = chanel; //通道
    
    switch(chanel)
    {
        case 1:
        {  
            bint16_Union.U16 = servoStr.thresholdLeft;
            buff[4] = bint16_Union.U8_Buff[0];
            buff[5] = bint16_Union.U8_Buff[1];
            break;
        }
        case 2:
        {
            bint16_Union.U16 = servoStr.thresholdRight;
            buff[4] = bint16_Union.U8_Buff[0];
            buff[5] = bint16_Union.U8_Buff[1];
            break;
        }
        case 3:
        {
            bint16_Union.U16 = servoStr.thresholdMiddle;
            buff[4] = bint16_Union.U8_Buff[0];
            buff[5] = bint16_Union.U8_Buff[1];
            break;
        }
    }
    
    for(int i=0;i<6;i++)
        check += buff[i];
    
    buff[6] = check;
    
    for(int i=0;i<9;i++)
        USB_Inspector_TransmitByte(buff[i]);
}

/**
* @brief        发送按键响应信息
* @param        time: 按下时长/ms
* @ref          
* @author       Leo
* @note         
**/
void USB_SendToInspector_KeyPress(uint16_t time)
{
    if(time<100)
        return;
    
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[8];
    buff[0] = 0x42; //帧头
    buff[1] = USB_ADDR_KEYINPUT ; //地址
    buff[2] = 0x06; //帧长
        
    bint16_Union.U16 = time;
    buff[3] = bint16_Union.U8_Buff[0];
    buff[4] = bint16_Union.U8_Buff[1];
    
    for(int i=0;i<5;i++)
        check += buff[i];
    
    buff[5] = check;
    
    for(int i=0;i<8;i++)
        USB_Inspector_TransmitByte(buff[i]);
}


/**
* @brief        发送电池信息
* @ref          
* @author       Leo
* @note         
**/
void USB_SendToInspector_BatteryInfo(void)
{
    Bint32_Union bint32_Union;
    uint8_t check = 0;
    uint8_t buff[11];
    buff[0] = 0x42; //帧头
    buff[1] = USB_ADDR_BATTERY ; //地址
    buff[2] = 0x09; //帧长
        
    buff[3] = icarStr.Electricity; //电量
    bint32_Union.Float = icarStr.Voltage;
    buff[4] = bint32_Union.U8_Buff[0];
    buff[5] = bint32_Union.U8_Buff[1];
    buff[6] = bint32_Union.U8_Buff[2];
    buff[7] = bint32_Union.U8_Buff[3];

    for(int i=0;i<8;i++)
        check += buff[i];
    
    buff[8] = check;
    
    for(int i=0;i<11;i++)
        USB_Inspector_TransmitByte(buff[i]);
}

/**
* @brief        发送自检信息
* @ref          
* @author       Leo
* @note         
**/
void USB_SendToInspector_Selfcheck(uint8_t step)
{
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[9];
    buff[0] = 0x42; //帧头
    buff[1] = USB_ADDR_SELFCHECK ; //地址
    buff[2] = 0x07; //帧长
        
    buff[3] = step;
    
    bint16_Union.U16 = icarStr.errorCode;
    buff[4] = bint16_Union.U8_Buff[0];
    buff[5] = bint16_Union.U8_Buff[1];

    for(int i=0;i<6;i++)
        check += buff[i];
    
    buff[6] = check;
    
    for(int i=0;i<9;i++)
        USB_Inspector_TransmitByte(buff[i]);
}
//------------------------------------------------[END]-------------------------------------------------------------




