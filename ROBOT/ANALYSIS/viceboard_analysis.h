#ifndef __VICEBOARD_ANALYSIS_H
#define __VICEBOARD_ANALYSIS_H

#include "bsp.h"

#define VALVE_ISLAND 0		//电磁阀控制位定义
#define VALVE_BULLET_PROTRACT 1
#define VALVE_BULLET_CLAMP 2



void ViceBoard_SendDataRun(void);
void ViceBoard_SendDataRefresh(void);
void ViceData_Receive(u8 data);	//从主板传过来的数据解析（主副板通用）
void SensorData_Deal(volatile u8 *pData);

typedef struct
{
	u8 valve[6];
	u8 servo[2];
}ViceControlDataTypeDef;	//控制副板


typedef struct
{
	u8 statu;
	u8 data[5];
	u8 count;
}ViceBoardSendTypeDef;	//发送给副板的数据结构体

#define VICEBOARD_SENDDATA_DEFAULT \
{\
	0,\
	{0x5A,0,0,0,0xA5},\
	0,\
}\

typedef struct
{
	volatile u8 headOK_state;
	volatile u8 valid_state;	//数据帧有效标志位
	volatile u8 databuffer[5];
	volatile u8 count;
}ReceiveDataTypeDef;	//副板数据处理数据结构体

typedef struct
{
	u8 Infrare[4];
	u8 Limit[4];
}SensorDataTypeDef;	//经解析得到的传感器数据

extern ViceBoardSendTypeDef SendData;
extern ReceiveDataTypeDef ReceiveData;


#endif

