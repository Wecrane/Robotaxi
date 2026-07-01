#ifndef __FLASH_H__
#define __FLASH_H__

#include "main.h"

#define FLASH_SIZE				12   
#define FLASH_ADDR_START 		0x08000000+1024*63                              //最后一个扇区用于存储用户数据                                                                                                                                                                                            
#define FLASH_DATA_OK			0x5F											//Flash存储标志

extern bool flashSaveEnable;

void FLASH_WriteBuffToFlash(int WriteAddress,uint8_t * pbuff,int num);
void FLASH_ReadFlashNBtye(int ReadAddress, uint8_t *pbuff, int ReadNum);
void FLASH_SaveAllConfig(void);
void FLASH_LoadAllConfig(void);
void FLASH_Handle(void);


#endif

//===========================================  End Of File  ===========================================//



