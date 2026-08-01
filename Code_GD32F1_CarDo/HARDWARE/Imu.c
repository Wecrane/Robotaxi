#include  "imu.h" 


IMU_STA ImuStructure;

void IMU_IIC_Delay(void);
bool IMU_IIC_Start(void);
void IMU_IIC_Stop(void);
void IMU_IIC_Ack(void);
void IMU_IIC_NoAck(void);
bool IMU_IIC_WaitAck(void);
void IMU_IIC_SendByte(u8 SendByte);
unsigned char IMU_IIC_RadeByte(void);
bool IMU_IIC_SingleWrite(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data);
unsigned char IMU_IIC_SingleRead(unsigned char SlaveAddress,unsigned char REG_Address);

static bool IMU_ReadReg(unsigned char SlaveAddress, unsigned char REG_Address, unsigned char *REG_data)
{
    if(REG_data == 0)
        return false;
    if(!IMU_IIC_Start())
        return false;
    IMU_IIC_SendByte(SlaveAddress);
    if(!IMU_IIC_WaitAck())
    {
        IMU_IIC_Stop();
        return false;
    }
    IMU_IIC_SendByte((u8)REG_Address);
    if(!IMU_IIC_WaitAck())
    {
        IMU_IIC_Stop();
        return false;
    }
    if(!IMU_IIC_Start())
    {
        IMU_IIC_Stop();
        return false;
    }
    IMU_IIC_SendByte(SlaveAddress + 1);
    if(!IMU_IIC_WaitAck())
    {
        IMU_IIC_Stop();
        return false;
    }

    *REG_data = IMU_IIC_RadeByte();
    IMU_IIC_NoAck();
    IMU_IIC_Stop();
    return true;
}

static bool IMU_DetectAddress(void)
{
    unsigned char who = 0;
    unsigned char addrs[2] = {MPU6050_Addr, MPU6050_Addr_ALT};
    int i;

    for(i = 0; i < 2; i++)
    {
        if(IMU_ReadReg(addrs[i], WHO_AM_I, &who) && (who == 0x68 || who == 0x69 || who == 0x70))
        {
            ImuStructure.Address = addrs[i];
            ImuStructure.WhoAmI = who;
            ImuStructure.Present = true;
            return true;
        }
    }

    ImuStructure.Address = MPU6050_Addr;
    ImuStructure.WhoAmI = who;
    ImuStructure.Present = false;
    return false;
}

static float IMU_NormalizeYaw(float yaw)
{
    while(yaw > 180.0f)
        yaw -= 360.0f;
    while(yaw < -180.0f)
        yaw += 360.0f;
    return yaw;
}


/*******************************************************************************
* Function Name  : IMU_IIC_Delay
* Description    : Simulation IIC Timing series delay
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void IMU_IIC_Delay(void)
{
    u8 i=30; //这里可以优化速度	，经测试最低到5还能写入
    while(i)
    {
        i--;
    }
}

/*******************************************************************************
* Function Name  : IMU_IIC_Start
* Description    : Master Start Simulation IIC Communication
* Input          : None
* Output         : None
* Return         : Wheather	 Start
****************************************************************************** */
bool IMU_IIC_Start(void)
{
    SDA_H;
    SCL_H;
    IMU_IIC_Delay();
    if(!SDA_read)return false;	//SDA线为低电平则总线忙,退出
    SDA_L;
    IMU_IIC_Delay();
    if(SDA_read) return false;	//SDA线为高电平则总线出错,退出
    SDA_L;
    IMU_IIC_Delay();
    return true;
}

/*******************************************************************************
* Function Name  : IMU_IIC_Stop
* Description    : Master Stop Simulation IIC Communication
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void IMU_IIC_Stop(void)
{
    SCL_L;
    IMU_IIC_Delay();
    SDA_L;
    IMU_IIC_Delay();
    SCL_H;
    IMU_IIC_Delay();
    SDA_H;
    IMU_IIC_Delay();
}

/*******************************************************************************
* Function Name  : I2C_Ack
* Description    : Master Send Acknowledge Single
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void IMU_IIC_Ack(void)
{
    SCL_L;
    IMU_IIC_Delay();
    SDA_L;
    IMU_IIC_Delay();
    SCL_H;
    IMU_IIC_Delay();
    SCL_L;
    IMU_IIC_Delay();
}

/*******************************************************************************
* Function Name  : IMU_IIC_NoAck
* Description    : Master Send No Acknowledge Single
* Input          : None
* Output         : None
* Return         : None
****************************************************************************** */
void IMU_IIC_NoAck(void)
{
    SCL_L;
    IMU_IIC_Delay();
    SDA_H;
    IMU_IIC_Delay();
    SCL_H;
    IMU_IIC_Delay();
    SCL_L;
    IMU_IIC_Delay();
}

/*******************************************************************************
* Function Name  : IMU_IIC_WaitAck
* Description    : Master Reserive Slave Acknowledge Single
* Input          : None
* Output         : None
* Return         : Wheather	 Reserive Slave Acknowledge Single
****************************************************************************** */
bool IMU_IIC_WaitAck(void) 	 //返回为:=1有ACK,=0无ACK
{
    SCL_L;
    IMU_IIC_Delay();
    SDA_H;
    IMU_IIC_Delay();
    SCL_H;
    IMU_IIC_Delay();
    if(SDA_read)
    {
        SCL_L;
        IMU_IIC_Delay();
        return false;
    }
    SCL_L;
    IMU_IIC_Delay();
    return true;
}

/*******************************************************************************
* Function Name  : IMU_IIC_SendByte
* Description    : Master Send a Byte to Slave
* Input          : Will Send Date
* Output         : None
* Return         : None
****************************************************************************** */
void IMU_IIC_SendByte(u8 SendByte) //数据从高位到低位//
{
    u8 i=8;
    while(i--)
    {
        SCL_L;
        IMU_IIC_Delay();
        if(SendByte&0x80)
            SDA_H;
        else
            SDA_L;
        SendByte<<=1;
        IMU_IIC_Delay();
        SCL_H;
        IMU_IIC_Delay();
    }
    SCL_L;
}

/*******************************************************************************
* Function Name  : IMU_IIC_RadeByte
* Description    : Master Reserive a Byte From Slave
* Input          : None
* Output         : None
* Return         : Date From Slave
****************************************************************************** */
unsigned char IMU_IIC_RadeByte(void)  //数据从高位到低位
{
    u8 i=8;
    u8 ReceiveByte=0;

    SDA_H;
    while(i--)
    {
			ReceiveByte<<=1;
			SCL_L;
			IMU_IIC_Delay();
			SCL_H;
			IMU_IIC_Delay();
			if(SDA_read)
			{
					ReceiveByte|=0x01;
			}
    }
    SCL_L;
    return ReceiveByte;
}

/*******************************************************************************
* Function Name  : IMU_IIC_SingleWrite
* Description    : Write single byte
* Input          : SlaveAddress:slave address
									 REG_Address:register address
									 REG_data:the data to write
* Output         : None
* Return         : false:write failure; 
									 true:write success
****************************************************************************** */
bool IMU_IIC_SingleWrite(unsigned char SlaveAddress,unsigned char REG_Address,unsigned char REG_data)		     //void
{
    if(!IMU_IIC_Start())return false;
    IMU_IIC_SendByte(SlaveAddress);   //发送设备地址+写信号//IMU_IIC_SendByte(((REG_Address & 0x0700) >>7) | SlaveAddress & 0xFFFE);//设置高起始地址+器件地址
    if(!IMU_IIC_WaitAck()) {
        IMU_IIC_Stop();
        return false;
    }
    IMU_IIC_SendByte(REG_Address );   //设置低起始地址
    IMU_IIC_WaitAck();
    IMU_IIC_SendByte(REG_data);
    IMU_IIC_WaitAck();
    IMU_IIC_Stop();
    Delay_Ms(5);
    return true;
}

/*******************************************************************************
* Function Name  : IMU_IIC_SingleRead
* Description    : Read single byte
* Input          : SlaveAddress:slave address
									 REG_Address:register address
* Output         : None
* Return         : false:read failure; 
									 REG_data :read success and return the data
****************************************************************************** */
unsigned char IMU_IIC_SingleRead(unsigned char SlaveAddress,unsigned char REG_Address)
{   unsigned char REG_data;
    if(!IMU_IIC_Start())return false;
    IMU_IIC_SendByte(SlaveAddress); 
    if(!IMU_IIC_WaitAck()) {
        IMU_IIC_Stop();
        return false;
    }
    IMU_IIC_SendByte((u8) REG_Address);   //设置低起始地址
    IMU_IIC_WaitAck();
    IMU_IIC_Start();
    IMU_IIC_SendByte(SlaveAddress+1);
    IMU_IIC_WaitAck();

    REG_data= IMU_IIC_RadeByte();
    IMU_IIC_NoAck();
    IMU_IIC_Stop();
    return REG_data;

}

/**
* @Description  : IMU初始化（MPU6050）
* @params       : 
* @Date         : 
* @author       : Leo
* @notes        : 
**/
void IMU_Init(void)
{
    //IMU-GPIO初始化
    GPIO_InitTypeDef  GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_13 | GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
		
    Delay_Ms(5);
    /*
       IMU_IIC_SingleWrite(MPU6050_Addr,PWR_M, 0x80);   //
       IMU_IIC_SingleWrite(MPU6050_Addr,SMPL, 0x07);    //
       IMU_IIC_SingleWrite(MPU6050_Addr,DLPF, 0x1E);    //±2000°
       IMU_IIC_SingleWrite(MPU6050_Addr,INT_C, 0x00 );  //
       IMU_IIC_SingleWrite(MPU6050_Addr,PWR_M, 0x00);   //
    */
    ImuStructure.Counter = 0;
    ImuStructure.AacX = 0;
    ImuStructure.AacY = 0;
    ImuStructure.AacZ = 0;
    ImuStructure.GyroX = 0;
    ImuStructure.GyroY = 0;
    ImuStructure.GyroZ = 0;
    ImuStructure.GyroRawZ = 0;
    ImuStructure.GyroZDps = 0.0f;
    ImuStructure.GyroZOffset = 0.0f;
    ImuStructure.YawDeg = 0.0f;
    ImuStructure.CalibSum = 0.0f;
    ImuStructure.CalibCount = 0;
    ImuStructure.Address = MPU6050_Addr;
    ImuStructure.WhoAmI = 0;
    ImuStructure.Calibrated = false;
    ImuStructure.Present = false;
    ImuStructure.ReadOk = false;
    ImuStructure.Valid = false;

#if !IMU_SOFT_IIC_ENABLE
    return;
#endif

    Delay_Ms(5);
    if(!IMU_DetectAddress())
        return;

    IMU_IIC_SingleWrite(ImuStructure.Address,PWR_MGMT_1, 	0x00);	//解除休眠状态
    Delay_Ms(20);
    IMU_IIC_SingleWrite(ImuStructure.Address,SMPLRT_DIV, 	0x07);
    IMU_IIC_SingleWrite(ImuStructure.Address,CONFIG, 			0x06);
    IMU_IIC_SingleWrite(ImuStructure.Address,GYRO_CONFIG, 	0x18);
    IMU_IIC_SingleWrite(ImuStructure.Address,ACCEL_CONFIG, 0x01);
    Delay_Ms(20);
    IMU_DetectAddress();
}


/**
* @Description  : IMU读取惯性数据
* @params       : 
* @Date         : 
* @author       : Leo
* @notes        : 
**/
void IMU_Handle(void)
{
#if !IMU_SOFT_IIC_ENABLE
    ImuStructure.Counter = 0;
    ImuStructure.Present = false;
    ImuStructure.ReadOk = false;
    ImuStructure.Valid = false;
    return;
#endif

    if(ImuStructure.Counter >= 10)	//10ms
    {
        unsigned char buff[12];     //接收数据缓存区
        uint16_t elapsedMs = ImuStructure.Counter;
        int16_t gyroRawZ;
        float gyroZDps;
        float dt;
        bool readOk;
        if(elapsedMs > 100)
            elapsedMs = 100;
        dt = (float)elapsedMs * 0.001f;

        if(!ImuStructure.Present)
        {
            IMU_DetectAddress();
            ImuStructure.ReadOk = false;
            ImuStructure.Valid = false;
            ImuStructure.Counter = 0;
            return;
        }
        
        readOk = true;
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_XOUT_L,&buff[0]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_XOUT_H,&buff[1]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_YOUT_L,&buff[2]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_YOUT_H,&buff[3]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_ZOUT_L,&buff[4]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,GYRO_ZOUT_H,&buff[5]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_XOUT_L,&buff[6]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_XOUT_H,&buff[7]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_YOUT_L,&buff[8]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_YOUT_H,&buff[9]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_ZOUT_L,&buff[10]);
        readOk = readOk && IMU_ReadReg(ImuStructure.Address,ACCEL_ZOUT_H,&buff[11]);
        ImuStructure.ReadOk = readOk;
        if(!readOk)
        {
            ImuStructure.Present = false;
            ImuStructure.Valid = false;
            ImuStructure.Calibrated = false;
            ImuStructure.CalibSum = 0.0f;
            ImuStructure.CalibCount = 0;
            ImuStructure.Counter = 0;
            return;
        }

        ImuStructure.GyroX =	(buff[1]<<8)|buff[0];
        ImuStructure.GyroX /= 16.4; 						   //读取计算X轴数据

        ImuStructure.GyroY =	(buff[3]<<8)|buff[2];
        ImuStructure.GyroY /=16.4; 						   	//读取计算Y轴数据
    
        gyroRawZ =	(buff[5]<<8)|buff[4];
        ImuStructure.GyroRawZ = gyroRawZ;
        gyroZDps = (float)gyroRawZ / 16.4f; 	           //读取计算Z轴数据
        ImuStructure.GyroZ = (short)gyroZDps;
        ImuStructure.GyroZDps = gyroZDps - ImuStructure.GyroZOffset;

        ImuStructure.AacX =	(buff[7]<<8)|buff[6];
        ImuStructure.AacY =	(buff[9]<<8)|buff[8];
        ImuStructure.AacZ =	(buff[11]<<8)|buff[10];
        if(ImuStructure.AacX == 0 && ImuStructure.AacY == 0 && ImuStructure.AacZ == 0)
        {
            ImuStructure.Valid = false;
            ImuStructure.Calibrated = false;
            ImuStructure.CalibSum = 0.0f;
            ImuStructure.CalibCount = 0;
            ImuStructure.Counter = 0;
            return;
        }

        if(!ImuStructure.Calibrated)
        {
            ImuStructure.CalibSum += gyroZDps;
            ImuStructure.CalibCount++;
            if(ImuStructure.CalibCount >= 200)             //静止约2s完成零偏标定
            {
                ImuStructure.GyroZOffset = ImuStructure.CalibSum / (float)ImuStructure.CalibCount;
                ImuStructure.YawDeg = 0.0f;
                ImuStructure.Calibrated = true;
                ImuStructure.Valid = true;
            }
        }
        else
        {
            if(ImuStructure.GyroZDps > -0.5f && ImuStructure.GyroZDps < 0.5f)
                ImuStructure.GyroZDps = 0.0f;
            ImuStructure.YawDeg = IMU_NormalizeYaw(ImuStructure.YawDeg + ImuStructure.GyroZDps * dt);
            ImuStructure.Valid = true;
        }
        
        ImuStructure.Counter = 0;
    }
}

/**
* @Description  : IMU线程控制器
* @params       : 
* @Date         : 
* @author       : Leo
* @notes        : 
**/
void IMU_Timer(void)
{
    ImuStructure.Counter++;
    if(ImuStructure.Counter > 1000)
        ImuStructure.Counter = 1000;
}
