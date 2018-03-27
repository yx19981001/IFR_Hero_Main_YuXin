#include "yun.h"

/*
整体结构：yaw轴暂定单独速度环//后期计划增加外接陀螺仪位置环进行选择
pitch轴位置环

经过控制信息处理，传入参数为
yaw速度？或者位置
pitch位置


*/
YUN_MOTOR_DATA 			yunMotorData=YUN_MOTOR_DATA_DEFAULT;
YUN_DATA          	yunData=YUN_DATA_DEFAULT;

PID_GENERAL          PID_PITCH_POSITION=PID_PITCH_POSITION_DEFAULT;
PID_GENERAL          PID_PITCH_SPEED=PID_PITCH_SPEED_DEFAULT;
PID_GENERAL          PID_YAW_POSITION=PID_YAW_POSITION_DEFAULT;
PID_GENERAL          PID_YAW_SPEED=PID_YAW_SPEED_DEFAULT;

extern  FIRST_ORDER_FILTER   FILTER_MOUSE_YAW;
extern  FIRST_ORDER_FILTER   FILTER_WAIST_YAW;
extern  MPU6050_REAL_DATA    MPU6050_Real_Data;
extern	RC_Ctl_t RC_Ctl;
extern GYRO_DATA Gyro_Data;

extern u32 time_1ms_count;

void Yun_Task(void)	//云台控制任务 
{
	Yun_Control_External_Solution();
}


void Yun_Control_External_Solution(void)	//外置反馈方案
{
	if(GetWorkState()==NORMAL_STATE)	//仅在正常模式下受控
	{
		{	//PC控制数据
			
		}
		
		{	//RC控制数据
			if(time_1ms_count%20==0)	//50hz
			{
				yunMotorData.yaw_tarP-=(int32_t)((RC_Ctl.rc.ch2-1024)*20.0/660.0);
				yunMotorData.yaw_tarP=yunMotorData.yaw_tarP>1800?yunMotorData.yaw_tarP-3600:yunMotorData.yaw_tarP;	//过零点
				yunMotorData.yaw_tarP=yunMotorData.yaw_tarP<-1800?yunMotorData.yaw_tarP+3600:yunMotorData.yaw_tarP;	//过零点
			}
			
			yunMotorData.pitch_tarP=(int32_t)(-(RC_Ctl.rc.ch3-1024)*430.0/660.0)+PITCH_INIT+50;	//-50是因为陀螺仪水平时云台上扬
		}
	}
	
	
	
	yunMotorData.pitch_tarV=-PID_General(yunMotorData.pitch_tarP,(Gyro_Data.angle[0]*8192/360.0+PITCH_INIT),&PID_PITCH_POSITION);
		
	if(yunMotorData.yaw_tarP-Gyro_Data.angle[2]*10>1800)	//过零点
	{
		yunMotorData.yaw_tarV=-PID_General(yunMotorData.yaw_tarP,Gyro_Data.angle[2]*10+3600,&PID_YAW_POSITION);
	}
	else if(yunMotorData.yaw_tarP-Gyro_Data.angle[2]*10<-1800)
	{
		yunMotorData.yaw_tarV=-PID_General(yunMotorData.yaw_tarP,Gyro_Data.angle[2]*10-3600,&PID_YAW_POSITION);
	}
	else
	{
		yunMotorData.yaw_tarV=-PID_General(yunMotorData.yaw_tarP,Gyro_Data.angle[2]*10,&PID_YAW_POSITION);
	}
	
	yunMotorData.pitch_output=PID_General(yunMotorData.pitch_tarV,(-Gyro_Data.angvel[1]/10.0),&PID_PITCH_SPEED);
	yunMotorData.yaw_output=PID_General(yunMotorData.yaw_tarV,(-Gyro_Data.angvel[2]/10.0),&PID_YAW_SPEED);	//采用外界陀螺仪做反馈
}


void Yun_Control_Inscribe_Solution(void)	//内接反馈方案
{
	//		yunMotorData.yaw_tarV=(int32_t)((RC_Ctl.rc.ch2-1024)*300.0/660.0);
//	yunMotorData.yaw_tarV=-PID_General(yunMotorData.yaw_tarP,Gyro_Data.angle[2]*10,&PID_YAW_POSITION);
//	yunMotorData.yaw_tarV=-PID_General(yunMotorData.yaw_tarP,yunMotorData.yaw_fdbP,&PID_YAW_POSITION);
	{	//采用板载陀螺仪的处理-速度
		//yunMotorData.yaw_output=PID_General(yunMotorData.yaw_tarV,MPU6050_Real_Data.Gyro_Z,&PID_YAW_SPEED);
		//yunMotorData.pitch_tarV=-PID_General(yunMotorData.pitch_tarP,yunMotorData.pitch_fdbP,&PID_PITCH_POSITION);
	}
	



//	yunMotorData.pitch_tarV=yun_pitch_tarV(yunMotorData.pitch_tarV);
}


//s16 __t_yaw_offset=0;
//	void __yun_yaw_offset(void)
//	{
//	static s32 Last_V_yaw=0;
//	static float a=0.05f;
//	__t_yaw_offset=(s32)(Last_V_yaw*(1-a)+yunMotorData.yaw_output*a);
//	Last_V_yaw=__t_yaw_offset;
//	}

s32 yun_pitch_tarV(s32 now_V)
{
	static s32 Last_V=0;
	static s32 now_acc_V=0;
	static float a=0.97f;
	now_acc_V=(s32)(Last_V*(1-a)+now_V*a);
	Last_V=now_acc_V;
	return now_acc_V;
}



/***************************************
函数名称：Yaw_Angle_Calculate
函数功能：通过当前机械角度与中值机械角度比较得到实际差角
函数参数：当前机械角度：src_angle
          中值机械角度：Init
函数返回值：实际差角：output
描述：无
****************************************/
float Yaw_Angle_Calculate(int16_t src_angle,int16_t Init)
{
    float output=-(float)(src_angle-Init)/8192*2*PI;	
	  return output;
}
//记录补偿值曲线
#define YAW_OFFSET_COUNT 11
const s32 YAW_OFFSET_VALUE[YAW_OFFSET_COUNT][2]=\
{\
	{5310,-620},\
	{5200,-530},\
	{5100,-475},\
	{5000,-390},\
	{4900,-110},\
	{4800,70},\
	{4700,280},\
	{4600,400},\
	{4500,470},\
	{4400,530},\
	{4300,573},\
};	//3.6测得

//const s32 YAW_OFFSET_VALUE[YAW_OFFSET_COUNT][2]=\
{\
	{5310,-1000},\
	{5180,-800},\
	{5000,-700},\
	{4910,-600},\
	{4800,-350},\
	{4710,0},\
	{4600,20},\
	{4500,240},\
	{4450,420},\
};	//旧

#define PITCH_OFFSET_COUNT 12
const s32 PITCH_OFFSET_VALUE[PITCH_OFFSET_COUNT][2]=\
{\
	{6600,-3500},\
	{6500,-2800},\
	{6400,-2500},\
	{6300,-2300},\
	{6200,-1920},\
	{6100,-1465},\
	{6000,-1300},\
	{5900,-1186},\
	{5800,-1200},\
	{5700,-1000},\
	{5600,-900},\
	{5500,-800},\
};	//原版，过大
//const s32 PITCH_OFFSET_VALUE[PITCH_OFFSET_COUNT][2]=\
{\
	{6600,-3000},\
	{6500,-2400},\
	{6400,-2100},\
	{6300,-1900},\
	{6200,-1750},\
	{6100,-1400},\
	{6000,-1200},\
	{5900,-1186},\
	{5800,-1100},\
	{5700,-1000},\
	{5600,-900},\
	{5500,-800},\
};

s32 Yaw_output_offset(s32 yaw_fbdP)	//克服云台yaw轴非线性力及非对称性的补偿 //虽然yaw云台阻尼曲线满足收敛，但因为参照物并非为云台电机，故应采用当前反馈位置做参照
{
	s32 offset=0;
	int i=0;
	
	yaw_fbdP=yaw_fbdP>YAW_OFFSET_VALUE[0][0]?YAW_OFFSET_VALUE[0][0]:yaw_fbdP;
	yaw_fbdP=yaw_fbdP<YAW_OFFSET_VALUE[YAW_OFFSET_COUNT-1][0]?YAW_OFFSET_VALUE[YAW_OFFSET_COUNT-1][0]:yaw_fbdP;
	
	for(i=0;i<YAW_OFFSET_COUNT;i++)	//遍历数组寻找位置
	{
		if(yaw_fbdP>=YAW_OFFSET_VALUE[i][0]) break;
	}
	
	i=i>YAW_OFFSET_COUNT-2?YAW_OFFSET_COUNT-2:i;	//限制到倒数第二个元素的位置，以免下一步运算数组越界
	
	offset=YAW_OFFSET_VALUE[i][1]+(YAW_OFFSET_VALUE[i+1][1]-YAW_OFFSET_VALUE[i][1])*(YAW_OFFSET_VALUE[i][0]-yaw_fbdP)/(YAW_OFFSET_VALUE[i][0]-YAW_OFFSET_VALUE[i+1][0]);
	return offset;
}

s16 Pitch_output_offset(s32 pitch_tarP)	//克服云台pitch轴非线性力及非对称性的补偿	//因为云台pitch阻尼曲线满足收敛（在外部激励情况下只存在一个最小值），故采用tarP作为补偿参照可以提高间接反应速度
{
	s16 offset=0;
	int i=0;
	
	pitch_tarP=pitch_tarP>PITCH_OFFSET_VALUE[0][0]?PITCH_OFFSET_VALUE[0][0]:pitch_tarP;
	pitch_tarP=pitch_tarP<PITCH_OFFSET_VALUE[PITCH_OFFSET_COUNT-1][0]?PITCH_OFFSET_VALUE[PITCH_OFFSET_COUNT-1][0]:pitch_tarP;
	
	for(i=0;i<PITCH_OFFSET_COUNT;i++)	//遍历数组寻找位置
	{
		if(pitch_tarP>=PITCH_OFFSET_VALUE[i][0]) break;
	}
	
	i=i>PITCH_OFFSET_COUNT-2?PITCH_OFFSET_COUNT-2:i;	//限制到倒数第二个元素的位置，以免下一步运算数组越界
	
	offset=PITCH_OFFSET_VALUE[i][1]+(PITCH_OFFSET_VALUE[i+1][1]-PITCH_OFFSET_VALUE[i][1])*(PITCH_OFFSET_VALUE[i][0]-pitch_tarP)/(PITCH_OFFSET_VALUE[i][0]-PITCH_OFFSET_VALUE[i+1][0]);
	return offset;
}



//extern char g_Yun_State_Change_Flag;
//extern char g_frictionWheelState;

//float Yaw_move_save[3]={0.0,0.0,0.0};
//volatile float Pitch_Tp_V=0;
//volatile float Yaw_Tp_V=0;
//volatile char  g_Yun_State = 0;
//volatile char  yunPID_Choose = 0;

///************************************
//函数名称：Yun_Move_Mouse
//函数功能：鼠标键盘控制云台得到YAW轴和PITCH轴的目标位置
//函数参数：无
//函数返回值：无
//描述：
//*************************************/
//void Yun_Move_Mouse(void)
//{
//		yunData.yaw_move = yunData.value_mouse_X*12;
//		yunData.pitch_move = yunData.value_mouse_Y*2;       
//	  Yaw_move_save[0]=Yaw_move_save[1];
//	  Yaw_move_save[1]=yunData.yaw_move ;
//	  Yaw_move_save[2]=abs(Yaw_move_save[1]- Yaw_move_save[0]);
//	
//		yunMotorData.yaw_Tp=YAW_INIT;		//9170						
//		Pitch_Tp_V=-yunData.pitch_move;
//	  yunMotorData.pitch_Tp-=Pitch_Tp_V;					
//		yunMotorData.yaw_Tp=Filter_firstOrder(yunMotorData.yaw_Tp,&FILTER_MOUSE_YAW);			
//}	
