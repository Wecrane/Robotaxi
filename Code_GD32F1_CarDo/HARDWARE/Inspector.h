#ifndef __INSPECTOR_H__
#define __INSPECTOR_H__

/*-----------------------------------------  I N C L U D E S  -----------------------------------------*/
#include "main.h"

/**
* @brief    xxx
**/
typedef struct 
{
    bool enable;                //监测软件连接使能
    uint16_t counterDrop;       //掉线计数器    
    uint16_t counterSend;       //发送数据计数器
}InspectorStr;

extern InspectorStr inspectorStructure;

void USB_Inspector_Init(void);
void USB_Inspector_TransmitByte(uint8_t data);

void USB_Inspector_Handle(void);
void USB_Inspector_Timer(void);

void USB_SendToInspector_ServoThreshold(uint8_t chanel);
void USB_SendToInspector_KeyPress(uint16_t time);
void USB_SendToInspector_BatteryInfo(void);
void USB_SendToInspector_CarSpeed(void);
void USB_SendToInspector_Selfcheck(uint8_t step);

#endif

//===========================================  End Of File  ===========================================//

