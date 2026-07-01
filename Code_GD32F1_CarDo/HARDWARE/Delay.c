#include "delay.h"
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
* @brief        机械延时函数
* @param        us：微秒
* @ref          2021年11月20日 11:27:46
* @author       Leo
* @note         基于72MHz主频
**/
void Delay_Us(u32 us)
{
    int s,t = 0;
    for(s=0;s<us;s++)
    {
        for(t=0;t<10;t++)
        {
            __ASM("NOP");
        }
    }
}



/**
* @brief        机械延时函数
* @param        ms：毫秒
* @ref          2021年11月20日 11:27:43
* @author       Leo
* @note         基于72MHz主频
**/
void Delay_Ms(u16 ms)
{
    int t = 0;
    for(t=0;t<ms;t++)
    {
        Delay_Us(1000);		
    }
}



