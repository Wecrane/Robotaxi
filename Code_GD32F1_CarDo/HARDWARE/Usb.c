#include "Usb.h"
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

UsbStruct usbStr;


/**
* @brief        USB/UART��ʼ��
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_Init(void)
{  
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;	
    NVIC_InitTypeDef NVIC_InitStruct;		
    //UART1��ʼ����PA9,PA10	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
    
    GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin=GPIO_Pin_9;	
    GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;	
    GPIO_Init(GPIOA,&GPIO_InitStruct); 	
    GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Pin=GPIO_Pin_10;	
    GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStruct);
    		
    USART_InitStruct.USART_BaudRate	= 115200;		//������
    USART_InitStruct.USART_HardwareFlowControl=USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode=USART_Mode_Rx|USART_Mode_Tx;	
    USART_InitStruct.USART_Parity=USART_Parity_No;
    USART_InitStruct.USART_StopBits=USART_StopBits_1;
    USART_InitStruct.USART_WordLength=USART_WordLength_8b;
    USART_Init(USART1,&USART_InitStruct);	
    NVIC_InitStruct.NVIC_IRQChannel=USART1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd=ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority=2;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority=1;
    NVIC_Init(&NVIC_InitStruct);
    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART1,ENABLE);
         
    //USB���ݳ�ʼ��
    usbStr.counter = 0;
    usbStr.receiveFinished = false;
    usbStr.receiveStart = false;
    usbStr.receiveIndex = 0;
    usbStr.connected = false;
    usbStr.inspectorEnable = false;
}


/**
* @brief        USB-Edgeboard����һ���ֽ�����
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_TransmitByte(uint8_t data)
{
    USART1->SR;           
    USART_SendData(USART1, data);
    while(USART_GetFlagStatus(USART1,USART_FLAG_TC) != SET);	//�ȴ����ͽ���
}

/**
* @brief        USB/UART�����жϺ���
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USART1_IRQHandler(void)  
{	 
    uint8_t Uart1Res;
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {   
        Uart1Res = USART_ReceiveData(USART1); 
        if(Uart1Res == USB_FRAME_HEAD && !usbStr.receiveStart)//���֡ͷ
        {
            usbStr.receiveStart = true;
            usbStr.receiveBuff[0] = Uart1Res;
            usbStr.receiveBuff[2] = USB_FRAME_LENMIN;
            usbStr.receiveIndex = 1;
        }
        else if(usbStr.receiveIndex == 2)	//����֡����
        {
            usbStr.receiveBuff[usbStr.receiveIndex] = Uart1Res;
            usbStr.receiveIndex++;
            
            if(Uart1Res > USB_FRAME_LENMAX || Uart1Res < USB_FRAME_LENMIN) //֡������
            {
                usbStr.receiveBuff[2] = USB_FRAME_LENMIN;
                usbStr.receiveIndex = 0;
                usbStr.receiveStart = false;
            }
        }
        else if(usbStr.receiveStart && usbStr.receiveIndex < USB_FRAME_LENMAX)
        {
            usbStr.receiveBuff[usbStr.receiveIndex] = Uart1Res;
            usbStr.receiveIndex++;
        }
        
        //����֡���
        if((usbStr.receiveIndex >= USB_FRAME_LENMAX || usbStr.receiveIndex >= usbStr.receiveBuff[2]) && usbStr.receiveIndex > USB_FRAME_LENMIN)
        {
            uint8_t check = 0;
            uint8_t length = USB_FRAME_LENMIN;
        
            length = usbStr.receiveBuff[2];
            for(int i=0;i<length-1;i++)
                check += usbStr.receiveBuff[i];
            
            if(check == usbStr.receiveBuff[length-1])//У��λ
            {
                memcpy(usbStr.receiveBuffFinished,usbStr.receiveBuff,USB_FRAME_LENMAX);	
                usbStr.receiveFinished = true;
                
                //���ܳ�����ָ�����⴦��������ʵʱ�ԣ�
                if(USB_ADDR_CONTROL  == usbStr.receiveBuffFinished[1])
                {
                    Bint16_Union bint16_Union;
                    Bint32_Union bint32_Union;
                    for(int i=0;i<4;i++)
                        bint32_Union.U8_Buff[i] = usbStr.receiveBuffFinished[3+i];
                    
                    bint16_Union.U8_Buff[0] = usbStr.receiveBuffFinished[7];
                    bint16_Union.U8_Buff[1] = usbStr.receiveBuffFinished[8];
                    
                    SERVO_SetPwmValueCorrect(bint16_Union.U16);
                    icarStr.ServoPwmSet = bint16_Union.U16;         //����
                    icarStr.SpeedSet = bint32_Union.Float;          //�ٶ�				
                }
				
                if(!usbStr.connected)//��λ����������ͨ��
                {
                    RGB_SetAllColor(RGB_COLOR_GREEN);
                    GPIO_BuzzerEnable(BuzzerOk);
                    usbStr.connected = true;
                }
                
                usbStr.counterDrop = 0;
            }
            
            usbStr.receiveIndex = 0;
            usbStr.receiveStart = false;
        }
    }

    USART_ClearFlag(USART1,USART_IT_RXNE); 
}


/**
* @brief        ��������߳̿�����
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_Timr(void)
{
    if(usbStr.connected)//USBͨ�ŵ��߼��
    {
        usbStr.counterDrop++;
        if(usbStr.counterDrop >3000)//3s
        {
            usbStr.connected = false;
            usbStr.inspectorEnable = false;
            icarStr.selfcheckEnable = false;
            icarStr.sprintEnable = false;			//[P0-4] 掉线时清除冲刺使能
            icarStr.SpeedSet = 0;					//[P0-4] 掉线时清零速度设定
            SERVO_SetPwmValueCorrect(servoStr.thresholdMiddle);//[P0-5] 掉线时舵机回中
        }
        
        if(usbStr.inspectorEnable)
        {
            usbStr.counterSend++;
        }
    }
}


/**
* @brief        USBͨ�Ŵ�������
* @param        
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_Handle(void)
{
    if(usbStr.receiveFinished)																//���ճɹ�
    {
        usbStr.receiveFinished = false;
        Bint32_Union bint32_Union;
        Bint16_Union bint16_Union;
           
        if(usbStr.receiveBuffFinished[1] & 0x80)	//������
        {
            uint8_t Addr = (uint8_t)(usbStr.receiveBuffFinished[1] & 0x7F);
            switch(Addr)
            {
                case USB_ADDR_BATTERY :             //�����Ϣ
                    break;
                
                case USB_ADDR_SERVOTHRESHOLD :      //�����ֵ
                    break;
            }
        }
        else //д����
        {
            switch(usbStr.receiveBuffFinished[1])
            {
                case USB_ADDR_SERVOTHRESHOLD :   //�����ֵ
                    if(usbStr.receiveBuffFinished[3] == 1)          //��ת��ֵ
                    {
                        bint16_Union.U8_Buff[0] = usbStr.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbStr.receiveBuffFinished[5];
                        servoStr.thresholdLeft = bint16_Union.U16;
                        flashSaveEnable = true; //�ȴ�Flash�洢
                        SERVO_SetPwmValue(servoStr.thresholdLeft);
                        GPIO_BuzzerEnable(BuzzerDing);
                    }
                    else if(usbStr.receiveBuffFinished[3] == 2)     //��ת��ֵ
                    {
                        bint16_Union.U8_Buff[0] = usbStr.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbStr.receiveBuffFinished[5];
                        servoStr.thresholdRight = bint16_Union.U16;
                        flashSaveEnable = true; //�ȴ�Flash�洢
                        SERVO_SetPwmValue(servoStr.thresholdRight);
                        GPIO_BuzzerEnable(BuzzerDing);
                    }
                    else if(usbStr.receiveBuffFinished[3] == 3)     //��ֵ
                    {
                        bint16_Union.U8_Buff[0] = usbStr.receiveBuffFinished[4];
                        bint16_Union.U8_Buff[1] = usbStr.receiveBuffFinished[5];
                        servoStr.thresholdMiddle = bint16_Union.U16;
                        flashSaveEnable = true; //�ȴ�Flash�洢
                        SERVO_SetPwmValue(servoStr.thresholdMiddle);
                        GPIO_BuzzerEnable(BuzzerDing);
                    }
                    break;
                
                case USB_ADDR_BUZZER :      //��������Ч
                    if(usbStr.receiveBuffFinished[3] == 1)          //OK
                        GPIO_BuzzerEnable(BuzzerOk);
                    else if(usbStr.receiveBuffFinished[3] == 2)     //Warnning
                        GPIO_BuzzerEnable(BuzzerWarnning);
                    else if(usbStr.receiveBuffFinished[3] == 3)     //Finish
                        GPIO_BuzzerEnable(BuzzerFinish);
                    else if(usbStr.receiveBuffFinished[3] == 4)     //Ding
                        GPIO_BuzzerEnable(BuzzerDing);
                    else if(usbStr.receiveBuffFinished[3] == 5)     //SystemStart
                        GPIO_BuzzerEnable(BuzzerSysStart);
                    
                    break;
                
                case USB_ADDR_LIGHT :         //LED��Ч
                    for(int i=0;i<4;i++)
                        bint32_Union.U8_Buff[i] = usbStr.receiveBuffFinished[i+3];
                
                    RGB_SetAllColor((unsigned long)bint32_Union.U32);
                    rgbStr.lastColor = (unsigned long)bint32_Union.U32;
                
                    break;

                case USB_ADDR_SPEEDMODE:        //�ٿ�ģʽ�л�
                    if(usbStr.receiveBuffFinished[3] == 1)    //����ģʽ
                        motorStr.CloseLoop = false;                    
                    else
                        motorStr.CloseLoop = true;
                    
                    icarStr.SpeedSet = 0;
                    GPIO_BuzzerEnable(BuzzerDing);
                    break;
                
                    
                //-----------------------------[�Լ��������]-------------------------------------------
                case USB_ADDR_INSPECTOR :           //�Լ���������
                    usbStr.inspectorEnable = true;
                    break;
                
                case USB_ADDR_SELFCHECK :           //��ʼ�Լ�
                    ICAR_SelfcheckControl(usbStr.receiveBuffFinished[3]);
                    break;             
            }      
            
        }
    }
    
    
    //-----------------------[�Լ��������ݷ���]-----------------------------
    if(usbStr.inspectorEnable && usbStr.connected && usbStr.counterSend > 150)//150ms
    {
        USB_Edgeboard_ServoThreshold(1);        //���Ͷ����ֵ
        Delay_Ms(1);
        USB_Edgeboard_ServoThreshold(2);        
        Delay_Ms(1);
        USB_Edgeboard_ServoThreshold(3);
        Delay_Ms(1);
        USB_Edgeboard_BatteryInfo();            //发送电池信息
        Delay_Ms(1);
        USB_Edgeboard_CarSpeed();               //发送车速
        Delay_Ms(1);
        USB_Edgeboard_Selfcheck(icarStr.selfcheckStep); //发送自检状态（含errorCode）
        usbStr.counterSend = 0; 
    }
}

/**
* @brief        USB���Ͱ����ź�
* @param        time: ����ʱ��
* @ref
* @author       Leo
* @note
**/
void USB_Edgeboard_TransmitKey(uint16_t time)
{
    uint8_t check = 0;
    uint8_t buff[8];
    Bint16_Union bint16_Union;
    
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_KEYINPUT; //��ַ
    buff[2] = 0x06; //֡��

    bint16_Union.U16 = time;
    buff[3] = bint16_Union.U8_Buff[0];
    buff[4] = bint16_Union.U8_Buff[1];
    
    for(int i=0;i<5;i++)
        check += buff[i];

    buff[5] = check;

	for(int i=0;i<8;i++)
		USB_Edgeboard_TransmitByte(buff[i]);
}
	










//----------------------------------------------[UNIT-���������Լ�����ͨ�����ݣ��˲���δ��Դ��]----------------------------------------------------------
/**
* @brief        ���Ͷ����ֵ
* @param        chanel: 1/��ת��ֵ��2/��ת��ֵ��3/��ֵ
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_ServoThreshold(uint8_t chanel)
{
    if(chanel<1 || chanel>3)
        return;
    
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[9];
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_SERVOTHRESHOLD; //��ַ
    buff[2] = 0x07; //֡��
    buff[3] = chanel; //ͨ��
    
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
        USB_Edgeboard_TransmitByte(buff[i]);
}

/**
* @brief        ���Ͱ�����Ӧ��Ϣ
* @param        time: ����ʱ��/ms
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_KeyPress(uint16_t time)
{
    if(time<100)
        return;
    
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[8];
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_KEYINPUT; //��ַ
    buff[2] = 0x06; //֡��
        
    bint16_Union.U16 = time;
    buff[3] = bint16_Union.U8_Buff[0];
    buff[4] = bint16_Union.U8_Buff[1];
    
    for(int i=0;i<5;i++)
        check += buff[i];
    
    buff[5] = check;
    
    for(int i=0;i<8;i++)
        USB_Edgeboard_TransmitByte(buff[i]);
}


/**
* @brief        ���͵����Ϣ
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_BatteryInfo(void)
{
    Bint32_Union bint32_Union;
    uint8_t check = 0;
    uint8_t buff[11];
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_BATTERY; //��ַ
    buff[2] = 0x09; //֡��
        
    buff[3] = icarStr.Electricity; //����
    bint32_Union.Float = icarStr.Voltage;
    buff[4] = bint32_Union.U8_Buff[0];
    buff[5] = bint32_Union.U8_Buff[1];
    buff[6] = bint32_Union.U8_Buff[2];
    buff[7] = bint32_Union.U8_Buff[3];

    for(int i=0;i<8;i++)
        check += buff[i];
    
    buff[8] = check;
    
    for(int i=0;i<11;i++)
        USB_Edgeboard_TransmitByte(buff[i]);
}

/**
* @brief        ���ͳ�����Ϣ
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_CarSpeed(void)
{
    Bint32_Union bint32_Union;
    uint8_t check = 0;
    uint8_t buff[10];
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_SPEEDBACK; //��ַ
    buff[2] = 0x08; //֡��
        
    bint32_Union.Float = icarStr.SpeedFeedback;
    buff[3] = bint32_Union.U8_Buff[0];
    buff[4] = bint32_Union.U8_Buff[1];
    buff[5] = bint32_Union.U8_Buff[2];
    buff[6] = bint32_Union.U8_Buff[3];

    for(int i=0;i<7;i++)
        check += buff[i];
    
    buff[7] = check;
    
    for(int i=0;i<10;i++)
        USB_Edgeboard_TransmitByte(buff[i]);
}

/**
* @brief        �����Լ���Ϣ
* @ref          
* @author       Leo
* @note         
**/
void USB_Edgeboard_Selfcheck(uint8_t step)
{
    Bint16_Union bint16_Union;
    uint8_t check = 0;
    uint8_t buff[9];
    buff[0] = 0x42; //֡ͷ
    buff[1] = USB_ADDR_SELFCHECK; //��ַ
    buff[2] = 0x07; //֡��
        
    buff[3] = step;
    
    bint16_Union.U16 = icarStr.errorCode;
    buff[4] = bint16_Union.U8_Buff[0];
    buff[5] = bint16_Union.U8_Buff[1];

    for(int i=0;i<6;i++)
        check += buff[i];
    
    buff[6] = check;
    
    for(int i=0;i<9;i++)
        USB_Edgeboard_TransmitByte(buff[i]);
}

//------------------------------------------------[END]-------------------------------------------------------------











