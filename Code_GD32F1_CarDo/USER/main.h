/*
********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2019; SaiShu.Lcc.; Leo
*
*               The code is for internal use only, not for commercial transactions.
*               The code ADAPTS the corresponding hardware circuit board, 
*               the specific details consult the professional.
*********************************************************************************************************
*/

#ifndef __MAIN_H__
#define __MAIN_H__


typedef enum { false = 0,true = 1} bool;


/************************ System *****************************/
#include "stm32f10x.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/*************************USER_MODULES***********************************************/

#include "Delay.h"
#include "Encoder.h"
#include "Gpio.h"
#include "Pid.h"
#include "Motor.h"
#include "Flash.h"
#include "Soc.h"
#include "Timer.h"
#include "Icar.h"
#include "Servo.h"
#include "Rgb.h" 
#include "Usb.h"


#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 

//IO口地址映射
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 

//IO口操作,只对单一的IO口!
//确保n的值小于16!
#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //输出 
#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //输入 
#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //输出 
#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //输入 
#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //输出 
#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //输入 

typedef union 
{
	uint8_t U8_Buff[2];
	uint16_t U16;
	int16_t S16;
}Bint16_Union;
	
typedef union 
{
	uint8_t U8_Buff[4];
	float Float;
    unsigned long U32;
}Bint32_Union;



#endif


