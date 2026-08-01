#include "soc.h"
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

SocStruct socStr;

//----------------------------------------------[UNIT-SOC_IIC]----------------------------------------------------------
/********************************************************************************************************
Function Name: IIC_Init
Description  : IIC-IO初始化
********************************************************************************************************/
void SOC_IIC_Init(void)
{			
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14;		
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    SOC_IIC_SCL=1;
    SOC_IIC_SDA=1;
}


/**
* @brief				: 产生IIC起始信号
* @param        : None 
* @date					: None
* @author       : Fqy
* @note         : None
**/
void SOC_IIC_Start(void)
{
		SOC_SDA_OUT();     	//sda线输出
		SOC_IIC_SDA=1;	  	  
		SOC_IIC_SCL=1;
		Delay_Us(4);
		SOC_IIC_SDA=0;				//START:when CLK is high,DATA change form high to low 
		Delay_Us(4);
		SOC_IIC_SCL=0;				//钳住I2C总线，准备发送或接收数据 
}	  


/**
* @brief				: 产生IIC停止信号
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : None
**/
void SOC_IIC_Stop(void)
{
		SOC_SDA_OUT();				//sda线输出
		SOC_IIC_SCL=0;
		SOC_IIC_SDA=0;				//STOP:when CLK is high DATA change form low to high
		Delay_Us(4);
		SOC_IIC_SCL=1; 
		SOC_IIC_SDA=1;				//发送I2C总线结束信号
		Delay_Us(4);							   	
}    


/**
* @brief				: 等待应答信号到来
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : 返回值：1：接收应答失败 ，0：接收应答成功
**/

uint8_t SOC_IIC_Wait_Ack(void)
{
		uint8_t ucErrTime=0;
		SOC_SDA_IN();      					//SDA设置为输入  
		SOC_IIC_SDA=1;Delay_Us(1);	   
		SOC_IIC_SCL=1;Delay_Us(1);
	
		while(SOC_READ_SDA)
		{
			ucErrTime++;
			if(ucErrTime>250)
			{
				SOC_IIC_Stop();
				return 1;
			}
		}
		
		SOC_IIC_SCL=0;							//时钟输出0 	 
		
		return 0;  
} 


/**
* @brief				: 产生ACK应答
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : None
**/
void SOC_IIC_Ack(void)
{
		SOC_IIC_SCL=0;
		SOC_SDA_OUT();
		SOC_IIC_SDA=0;
		Delay_Us(2);
		SOC_IIC_SCL=1;
		Delay_Us(2);
		SOC_IIC_SCL=0;
}


/**
* @brief				: 不产生ACK应答	
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : None
**/
void SOC_IIC_NAck(void)
{
		SOC_IIC_SCL=0;
		SOC_SDA_OUT();
		SOC_IIC_SDA=1;
		Delay_Us(2);
		SOC_IIC_SCL=1;
		Delay_Us(2);
		SOC_IIC_SCL=0;
}	


/**
* @brief				: IIC发送一个字节
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : 返回从机有无应答，1：有应答，0：无应答	
**/		  
void SOC_IIC_Send_Byte(uint8_t txd)
{                        
    uint8_t t;   
		SOC_SDA_OUT(); 	    
    SOC_IIC_SCL=0;					//拉低时钟开始数据传输
	
    for(t=0;t<8;t++)
    {              
			SOC_IIC_SDA=(txd&0x80)>>7;
			txd<<=1; 	  
			Delay_Us(2);   				//对TEA5767这三个延时都是必须的
			SOC_IIC_SCL=1;
			Delay_Us(2); 
			SOC_IIC_SCL=0;	
			Delay_Us(2);
    }	 
} 	    


/**
* @brief				: 读1个字节
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : ack=1时，发送ACK，ack=0，发送nACK 
**/  
uint8_t SOC_IIC_Read_Byte(unsigned char ack)
{
		unsigned char i,receive=0;
		SOC_SDA_IN();								//SDA设置为输入
		
		for(i=0;i<8;i++ )
		{
			SOC_IIC_SCL=0; 
			Delay_Us(2);
			SOC_IIC_SCL=1;
			receive<<=1;
			
			if(SOC_READ_SDA)receive++;   
				Delay_Us(1); 
		}			
		
		if (!ack)
			SOC_IIC_NAck();//发送nACK
		else
			SOC_IIC_Ack(); //发送ACK  
		
		return receive;
}


/**
* @brief				: IIC连续写指定长度数据
* @param        : reg:寄存器地址，len:写入长度buff:数据
* @date					: None
* @author       : Fqy
* @note         : 返回值:0,：正常,其他：错误代码
**/
uint8_t SOC_Write_Len(uint8_t reg,uint8_t len,uint8_t *buff)
{
		uint8_t i; 
		SOC_IIC_Start(); 
		SOC_IIC_Send_Byte(WRITE_CW2015);
		if(SOC_IIC_Wait_Ack())							//等待应答
		{
			SOC_IIC_Stop();		 
			return 1;		
		}
		
		SOC_IIC_Send_Byte(reg);						//写寄存器地址
		SOC_IIC_Wait_Ack();								//等待应答
		
		for(i=0;i<len;i++)
		{
			SOC_IIC_Send_Byte(buff[i]);				//发送数据
			if(SOC_IIC_Wait_Ack())						//等待ACK
			{
				SOC_IIC_Stop();	 
				return 1;		 
			}		
		}    
		SOC_IIC_Stop();
		
		return 0;	
} 


/**
* @brief				: IIC写入数据
* @param        : None
* @date					: None
* @author       : Fqy
* @note         : None
**/
uint8_t SOC_Write(uint8_t reg,uint8_t *buf)
{
		SOC_IIC_Start(); 
		SOC_IIC_Send_Byte(WRITE_CW2015);
		if(SOC_IIC_Wait_Ack())							//等待应答
		{
			SOC_IIC_Stop();		 
			return 1;		
		}
		
		SOC_IIC_Send_Byte(reg);						//写寄存器地址
		SOC_IIC_Wait_Ack();								//等待应答
		
		SOC_IIC_Send_Byte(*buf);					//发送数据
		
		if(SOC_IIC_Wait_Ack())						//等待ACK
		{
			SOC_IIC_Stop();	 
			return 1;		 
		}  
		
		SOC_IIC_Stop();
		
		return 0;	
}


/**
* @brief				: IIC连续读取指定长度数据
* @param        : reg：要读取的寄存器地址，len:要读取的长度，buff:读取到的数据存储区
* @date					: None
* @author       : Fqy
* @note         : 返回值:0：正常，其他：错误代码
**/
uint8_t SOC_Read_Len(uint8_t reg,uint8_t len,uint8_t *buff)
{ 
		SOC_IIC_Start(); 
		SOC_IIC_Send_Byte(WRITE_CW2015);
		if(SOC_IIC_Wait_Ack())									//等待应答
		{
			SOC_IIC_Stop();		 
			return 1;		
		}
		SOC_IIC_Send_Byte(reg);								//写寄存器地址
		SOC_IIC_Wait_Ack();										//等待应答
		SOC_IIC_Start();
		SOC_IIC_Send_Byte(READ_CW2015);
		SOC_IIC_Wait_Ack();										//等待应答 
		
		while(len)
		{
			if(len==1)*buff=SOC_IIC_Read_Byte(0);	//读数据,发送nACK 
			else *buff=SOC_IIC_Read_Byte(1);			//读数据,发送ACK  
			len--;
			buff++; 
		}    
		SOC_IIC_Stop();												//产生一个停止条件 
		
		return 0;	
}


/**
* @brief				: IIC读取指定地址的数据
* @param        : reg：要读取的寄存器地址，buff:读取到的数据存储区
* @date					: None
* @author       : Fqy
* @note         : None
**/
uint8_t SOC_Read(uint8_t reg,uint8_t *buff)
{ 
		SOC_IIC_Start(); 
		SOC_IIC_Send_Byte(WRITE_CW2015);
		if(SOC_IIC_Wait_Ack())									//等待应答
		{
			SOC_IIC_Stop();		 
			return 1;		
		}
		SOC_IIC_Send_Byte(reg);								//写寄存器地址
		SOC_IIC_Wait_Ack();										//等待应答
		SOC_IIC_Start();
		SOC_IIC_Send_Byte(READ_CW2015);
		SOC_IIC_Wait_Ack();										//等待应答 
		*buff = SOC_IIC_Read_Byte(0);					//读数据,发送nACK 
		SOC_IIC_Stop();												//产生一个停止条件 
		
		return 0;	
}
//------------------------------------------------[END]-------------------------------------------------------------





//----------------------------------------------[UNIT-SOC-LOGIC]----------------------------------------------------------
int8_t no_charger_full_jump = 0;
unsigned int allow_no_charger_full =0;
unsigned int allow_charger_always_zero =0;
unsigned char if_quickstart =0;
unsigned char reset_loop =0;

/**
* @brief				: 更新电池信息
* @param        : None
* @date					: None
* @author       : None
* @note         : 这个函数的作用是更新ic内的电池profile信息，一般只有在ic VDD掉电后再上电时才执行 
									return 1 : i2c读写错， return 2 : 芯片处于sleep模式 return 3 : 写入的profile信息读出后与代码中的不一致
**/
unsigned char SOC_UpdataConfigInfo(void)
{
	int8_t ret = 0;
	unsigned char i;
	unsigned char reset_val;
	unsigned char reg_val;
	/* make sure no in sleep mode */
	ret = SOC_Read(REG_MODE, &reg_val);
	if(ret)
	{
		return 1;
	}
	if((reg_val & MODE_SLEEP_MASK) == MODE_SLEEP)
	{
		return 2;
	}
	/* update new battery info */
	for(i = 0; i < SIZE_BATINFO; i++)
	{
		reg_val = cw_bat_config_info[i];
		ret = SOC_Write(REG_BATINFO+i, &reg_val);
		if(ret)
		{
			return 1;
		}
	}

	/* readback & check */
	for(i = 0; i < SIZE_BATINFO; i++)
	{
		ret = SOC_Read(REG_BATINFO+i, &reg_val);
		if(ret)
		{
			return 1;
		}
		if(reg_val != cw_bat_config_info[i])
		{
			return 3;
		}
	}
	/* set cw2015/cw2013 to use new battery info */
	ret = SOC_Read(REG_CONFIG, &reg_val);
	if(ret)
	{
		return 1;
	}
	reg_val |= CONFIG_UPDATE_FLG;   /* set UPDATE_FLAG */
	reg_val &= 0x07;                /* clear ATHD */
	reg_val |= ATHD;                /* set ATHD */
	ret = SOC_Write(REG_CONFIG, &reg_val);
	if(ret)
	{
		return 1;
	}
	/* reset */
	reset_val = MODE_NORMAL;
	reg_val = MODE_RESTART;
	ret = SOC_Write(REG_MODE, &reg_val);
	if(ret)
	{
		return 1;
	}
	Delay_Us(100);  //delay 100us      
	ret = SOC_Write(REG_MODE, &reset_val);
	if(ret)
	{
		return 1;
	}   
	return 0;
}


/**
* @brief				: 电量计底层初始化
* @param        : None
* @date					: None
* @author       : None
* @note         : return 1 : i2c读写错， return 2 : 芯片处于sleep模式 return 3 : 写入的profile信息读出后与代码中的不一致 return 4 : 芯片启动后30s内读电量值一直异常
**/
unsigned char SOC_HardwareInit(void)
{
	unsigned ret;
	unsigned char i;
	unsigned char reg_val = MODE_NORMAL;

	/* wake up cw2015/13 from sleep mode */
	ret = SOC_Write(REG_MODE, &reg_val);
	if(ret)
	{
		return 1;
	}

	/* check ATHD if not right */
	ret = SOC_Read(REG_CONFIG, &reg_val);
	if(ret)
	{
		return 1;
	}
	if((reg_val & 0xf8) != ATHD)
	{
		//"the new ATHD need set"
		reg_val &= 0x07;    /* clear ATHD */
		reg_val |= ATHD;    /* set ATHD */
		ret = SOC_Write(REG_CONFIG, &reg_val);
		if(ret)
		{
			return 1;
		}
	}
	
	/* check config_update_flag if not right */
	ret = SOC_Read(REG_CONFIG, &reg_val);
	if(ret)
	{
		return 1;
	}
	if(!(reg_val & CONFIG_UPDATE_FLG))
	{
		//"update flag for new battery info need set"
		ret = SOC_UpdataConfigInfo();
		if(ret)
		{
			return ret;
		}
	}
	else
	{
		for(i = 0; i < SIZE_BATINFO; i++)
		{ 
			ret = SOC_Read(REG_BATINFO +i, &reg_val);
			if(ret)
			{
				return 1;
			}
			if(cw_bat_config_info[i] != reg_val)
			{
				break;
			}
		}
		if(i != SIZE_BATINFO)
		{
			//"update flag for new battery info need set"
			ret = SOC_UpdataConfigInfo();
			if(ret)
			{
				return ret;
			}
		}
	}
	/* check SOC if not eqaul 255 */
	for (i = 0; i < 30; i++) {
		ret = SOC_Read(REG_SOC, &reg_val);
		if (ret)
			return 1;
		else if (reg_val <= 100) 
			break;
		Delay_Ms(100);//delay 100ms
    }
	
    if (i >=30){
        reg_val = MODE_SLEEP;
        ret = SOC_Write(REG_MODE, &reg_val);
        // "cw2015/cw2013 input unvalid power error_2\n";
        return 4;
    } 
	return 0;
}

#ifdef BAT_LOW_INTERRUPT


/**
* @brief				: 释放alrt引脚
* @param        : None 
* @date					: None 
* @author       : None
* @note         : 当一次alrt 事件到来时，cw2015 ic会拉低arlt pin产生中断，这时需要对06寄存器的最高bit位清0，才能让cw2015 ic释放alrt pin 下面函数的作用是释放alrt pin
**/
unsigned char SOC_ReleaseAlrtPin(void)
{
		signed char ret = 0;
		unsigned int reg_val;
		unsigned char alrt;

		ret = SOC_Read(REG_RRT_ALERT, &reg_val);
		if(ret) 
		{
			return -1;
		}
		alrt = reg_val & 0x80;

		reg_val = reg_val & 0x7f;
		ret = SOC_Write(REG_RRT_ALERT, &reg_val);
		if(ret)
		{
			return -1;
		}

		return alrt;
}


/**
* @brief				: 更新低电量报警值
* @param        : None
* @date					: None
* @author       : None
* @note         : 函数的作用是更新新的低电告警值为上次的 -1， 比如我们的代码开始的时候设定的低电告警值是10，那当电量降到10后
									主控处理完中断后，我把新的低电告警值9写在了对应的寄存器中。 ATHD是08寄存器的前5个bit
**/
int8_t SOC_UpdateAthd()
{
	int8_t ret = 0;
	unsigned char reg_val;
	char new_athd = 0;
	
	ret = SOC_Read(REG_CONFIG, &reg_val);
	if(ret)
	{
		return -1;
	}
	new_athd = (reg_val >> 3) - 1;
	if(new_athd <= 0){
		new_athd = 0;
	}
	new_athd = new_athd << 3;

	//"the new ATHD need set"
	reg_val &= 0x07;    /* clear ATHD */
	reg_val |= new_athd;    /* set new ATHD */
	ret = SOC_Write(REG_CONFIG, &reg_val);
	if(ret)
	{
			return -1;
	}
	return 0;
}


/**
* @brief				: 中断服务函数
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
static void ALRT_ISR() //interrupt 
{
    /*User can do something when alrt */
	/*客户可以在这里加入当中断到来时想做的操作*/
    SOC_ReleaseAlrtPin();
		SOC_UpdateAthd();
    /*User can write new alrt to CONFIG resiger*/
}
#endif


/**
* @brief				: 电量计上电复位
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
int SOC_Por(void)
{
	int8_t ret = 0;
	unsigned char reset_val = 0;
	reset_val = MODE_SLEEP;             
	ret = SOC_Write(REG_MODE, &reset_val);
	if (ret)
		return -1;
	Delay_Us(100); //delay 100us
	
	reset_val = MODE_NORMAL;
	ret = SOC_Write(REG_MODE, &reset_val);
	if (ret)
		return -1;
	Delay_Us(100); //delay 100us
	
	ret = SOC_HardwareInit();
	if (ret) 
		return ret;
	return 0;
}


/**
* @brief				: SOC获取电池电量
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
int SOC_GetCapacity(void)
{
	int8_t ret = 0;
	unsigned char allow_capacity;
	unsigned char reg_val;
	//unsigned char reset_val;
	unsigned char cw_capacity;
	//int charge_time;

	ret = SOC_Read(REG_SOC, &reg_val);
	if(ret)
	{
		return -1;
	}
        
	cw_capacity = reg_val;
	/*假设ic出现问题，读取电量不在合理值范围内5次，重启ic。如果中间读到正确的值，那么5次的计数器清0，正确显示*/
	if (cw_capacity > 100)
		{
                // "get cw_capacity error; cw_capacity = %d\n"
        reset_loop++;
		if (reset_loop >5) { 
			ret = SOC_Por(); //por ic
			if(ret)
				return -1;
			reset_loop =0;               
		}                   
        return socStr.Capacity;
    }else {
        reset_loop =0;
    }
	
	/*如果是充电状态，电量应该是向上增加的，插入充电器时如果本次读取的电量比上次小，显示上次电量。*/
	/*什么情况下会出现这种问题呢，因为我司电量计有小数位，比如本次电量0x04寄存器读出的值是9.01，因为代码只读03寄存器，所以给客户显示的值是9，但下次读取的电量是8.99*/
	/*那么下次显示给客户的电量是8，从客户显示上来看不太好看，所以做了这个。*/
	/*放电时做了同样的处理*/
	if(((socStr.UsbOnline == 1) && (cw_capacity == (socStr.Capacity - 1)))
			|| ((socStr.UsbOnline == 0) && (cw_capacity == (socStr.Capacity + 1))))
	{
		// modify battery level swing
		if(!((cw_capacity == 0 && socStr.Capacity <= 2)||(cw_capacity == 100 && socStr.Capacity == 99)))
		{			
			cw_capacity = socStr.Capacity;
		}
	}
	
		/*这段是客户常见的问题，客户使用的charger ic（充电ic）精度不是很高，有些ic只能把电池充电到4.1V，如果提取电池曲线时满充电压提的是4.2V，那么久永远显示不了100%*/
		/*遇到这种问题有两种方法，1、如我写的代码这样显示给客户一个假的值，一点点增加到100。
		2、另外是修改profile，降低显示100%时的电压点（这样做后如果还是会有些charger ic只能让电池充到4.0就又有问题了）。
		以前的oppo的一款charger遇到这样的问题
		*/
	if((socStr.UsbOnline == 1) && (cw_capacity >= 95) && (cw_capacity <= socStr.Capacity) )
	{     
		// avoid not charge full
		allow_no_charger_full++;
		if(allow_no_charger_full >= BATTERY_UP_MAX_CHANGE)
		{
			allow_capacity = socStr.Capacity + 1; 
			cw_capacity = (allow_capacity <= 100) ? allow_capacity : 100;
			no_charger_full_jump =1;
			allow_no_charger_full =0;
		}
		else if(cw_capacity <= socStr.Capacity)
		{
			cw_capacity = socStr.Capacity; 
		}
	}
	/*这段的代码是当电量是通过上到代码修正上去的假的值，那么要用假的方法修正下来，避免100直接跳变回真实值的情况*/
    else if((socStr.UsbOnline == 0) && (cw_capacity <= socStr.Capacity ) && (cw_capacity >= 90) && (no_charger_full_jump == 1))
	{
		// avoid battery level jump to CW_BAT
		if(socStr.UsbOnline == 0) 
		   allow_no_charger_full++;
		if(allow_no_charger_full >= BATTERY_DOWN_MIN_CHANGE)
		{
			allow_capacity = socStr.Capacity - 1;
			allow_no_charger_full =0; 
			if (cw_capacity >= allow_capacity)
			{
				no_charger_full_jump =0;
			}
			else
			{
				cw_capacity = (allow_capacity > 0) ? allow_capacity : 0;
			}
		}
		else if(cw_capacity <= socStr.Capacity)
		{
			cw_capacity = socStr.Capacity;
		}
	}
	else
    {
  		allow_no_charger_full =0;
    }
	
	/*ic出错了，充了很久一直还是0%，一般我们用半个小时，那么重启下ic*/
	if((socStr.UsbOnline > 0) && (cw_capacity == 0))
	{		  
		allow_charger_always_zero++;
		if((allow_charger_always_zero >= BATTERY_DOWN_MIN_CHANGE_SLEEP) && (if_quickstart == 0))
		{
      ret = SOC_Por(); //por ic
			if(ret){
				return -1;
			}
			if_quickstart = 1;
			allow_charger_always_zero =0;
		}
	}
	else if((if_quickstart == 1)&&(socStr.UsbOnline == 0))
	{
		if_quickstart = 0;
	}

	return(cw_capacity);
}


/**
* @brief				: 获取电压值
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
unsigned int SOC_GetVol(void)
{
	int8_t ret = 0;
	unsigned char get_ad_times = 0;
	unsigned char reg_val[2] = {0 , 0};
	unsigned long ad_value = 0;
	unsigned int ad_buff = 0;
	unsigned int ad_value_min = 0;
	unsigned int ad_value_max = 0;

	for(get_ad_times = 0; get_ad_times < 3; get_ad_times++)
	{
		ret = SOC_Read(REG_VCELL, &reg_val[0]);
		if(ret)
		{
			return 1;
		}
		ret = SOC_Read(REG_VCELL + 1, &reg_val[1]);
		if(ret)
		{
			return 1;
		}
		ad_buff = (reg_val[0] << 8) + reg_val[1];

		if(get_ad_times == 0)
		{
			ad_value_min = ad_buff;
			ad_value_max = ad_buff;
		}
		if(ad_buff < ad_value_min)
		{
			ad_value_min = ad_buff;
		}
		if(ad_buff > ad_value_max)
		{
			ad_value_max = ad_buff;
		}
		ad_value += ad_buff;
	}
	ad_value -= ad_value_min;
	ad_value -= ad_value_max;
	ad_value = ad_value  * 305 / 1000;
	return(ad_value);       //14位ADC转换值
}


/**
* @brief				: SOC更新电池电量
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
void SOC_UpdateCapacity(void)
{
	int cw_capacity;
	cw_capacity = SOC_GetCapacity();
	if((cw_capacity >= -5) && (cw_capacity <= 100) && (socStr.Capacity != cw_capacity))
	{       
		socStr.Capacity = cw_capacity;
		cw_capacity += 5;				//优化电池电量充不满的问题
		if(cw_capacity>100)
			icarStr.Electricity = 100;
		else if(cw_capacity<0)
			icarStr.Electricity = 0;
		else
			icarStr.Electricity = cw_capacity;
	}
}


/**
* @brief				: SOC更新电池电压
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
uint8_t Index = 0;
uint8_t uSocErrorCnt= 0;
void SOC_UpdateVol(void)
{
	unsigned int cw_voltage;
	cw_voltage = SOC_GetVol();
	if(cw_voltage == 1){
		//read voltage error
		socStr.voltage = socStr.voltage;
	}else if(socStr.voltage != cw_voltage)
	{
		socStr.voltage = cw_voltage;
		icarStr.Voltage = (float)cw_voltage*3.f/1000.f;
	}
	
	if(icarStr.Voltage < 11.3f)	//电压<11.3v开始报警
	{
		uSocErrorCnt ++;
		if(uSocErrorCnt > 2)
		{
			GPIO_BuzzerEnable(BuzzerWarnning);
            uSocErrorCnt=0;
		}
	}
	else
	{
		uSocErrorCnt = 0;
	}
}


/**
* @brief				: USB插入状态检测
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
void SOC_UpdateUsbOnline(void)
{
	if(0) 
	//这里请注意，这里是客户需要自己配置修改的地方
	//请修改代码保证DC插入时配置soc_Struct.usb_online为 1，DC不在时配置soc_Struct.usb_online为0
	{
		socStr.UsbOnline = 1;
	}else{
		socStr.UsbOnline = 0;
	}
}


/**
* @brief				: SOC控制器
* @param        : None
* @date					: None
* @author       : None
* @note         : None
**/
void SOC_BatWork(void)
{
	SOC_UpdateUsbOnline();
	SOC_UpdateVol();
    SOC_UpdateCapacity();
}


/**
* @brief				: SOC初始化
* @param        : None
* @date					: None
* @author       : None
* @note         : return 1 : i2c读写错， return 2 : 芯片处于sleep模式 return 3 : 写入的profile信息读出后与代码中的不一致 return 4 : 芯片启动后30s内读电量值一直异常
**/
unsigned char SOC_Init(void)
{
	unsigned char ret;
	unsigned char loop = 0;
	//cw_bat_gpio_init();
	SOC_IIC_Init();
	
	ret = SOC_HardwareInit();
	
	while((loop++ < 200) && (ret != 0))
	{
		ret = SOC_HardwareInit();
	}	

	socStr.UsbOnline = 0;
	socStr.Capacity = 2;
	socStr.voltage = 0;
	
	return ret;	
}


/**
* @brief				: 电量计IC控制时序
* @param        : None
* @date					: None
* @author       : Leo
* @note         : None
**/
void SOC_Timer(void)
{
	socStr.Counter++;
	if(socStr.Counter>200000)
		socStr.Counter = 200000;
}


/**
* @brief				: 电量计IC控制逻辑
* @param        : None
* @date					: None
* @author       : Leo
* @note         : None
**/
void SOC_Handle(void)
{
	if(socStr.Counter>1000)//1s
	{
		SOC_BatWork();
		socStr.Counter = 0;
	}
}
//------------------------------------------------[END]-------------------------------------------------------------




