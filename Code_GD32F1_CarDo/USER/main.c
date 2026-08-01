/* Includes ------------------------------------------------------------------*/
#include "main.h"


/**@mainpage                【CarDO】智能汽车竞赛 - 开源智控板
* <table>
* <tr><th>Project       <td>IntelligentCar
* <tr><th>Author        <td>Leo
* <tr><th>copyright     <td>Copyright@2016-2022 BjssTech, Ltd.All Rights Reserved.
**********************************************************************************
* </table>
* @section      CarDo智控板嵌入式代码
*
* @section      [电机驱动][舵机驱动][USB通信][人机交互][自检]
*
* @section      北京赛曙科技有限公司：https://bjsstech.com/
* <table>
* <tr><th>Date          <th>Version     <th>Author      <th>Content         </tr>
* <tr><td>2022/02/09    <td>V1.0        <td>Leo         <td>InitialVersion  </tr>
* -#    Note:1
* -#    Note:2
* </tr>
* </table>
**********************************************************************************
*/


int main(void)
{
    Delay_Ms(100);                      //启动延时：等待系统稳定
    FLASH_LoadAllConfig();              //加载系统配置
    
    ICAR_Init();                        //智能车参数初始化
    ENCODER_Init();                     //编码器初始化
    GPIO_Initialize();                  //GPIO初始化
    PID_Init();                         //PID参数初始化
    MOTOR_Init();                       //电机初始化
    SERVO_Init();                       //舵机初始化
    USB_Edgeboard_Init();               //USB通信初始化
    SOC_Init();							//电量计初始化
    RGB_Init();                         //RGB灯通信初始化
    
    Delay_Ms(100);                      //启动延时：等待系统稳定
    
    TIM2_Init();                        //系统主线程定时器初始化
    GPIO_BuzzerEnable(BuzzerSysStart);  //开机音效
    
	while(1)
	{
        GPIO_Handle();                  //GPIO控制：LED/蜂鸣器
        SOC_Handle();                   //电量计采样
        ICAR_Handle();                  //智能车控制  
        FLASH_Handle();                 //Flash存储
        USB_Edgeboard_Handle();         //USB通信控制
	}
    return 0;
}










