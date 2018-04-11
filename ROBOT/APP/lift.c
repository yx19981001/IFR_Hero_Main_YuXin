#include "lift.h"
#include "control.h"

extern LIFT_DATA lift_Data;
extern u32 time_1ms_count;

#define AUTOCHASSIS_LIFT 10
void AutoChassisAttitude_Lift(float chassis_pitch_raw)	//自动调整姿态	//pitch正方向为前上	//注意放在lift_task前面
{
	static float chassis_pitch=0;
	static float ka=0.1f;
	static u16 steady_flat_count=0;

	chassis_pitch=chassis_pitch*(1-ka)+chassis_pitch_raw*ka;
	
	if(GetWorkState()==NORMAL_STATE)	//正常状态
	{
		
		if(abs(chassis_pitch)>8)	//8度阈值
		{
			steady_flat_count=0;
			
			lift_Data.lf_lift_tarP=-chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lf_lift_fdbP;
			lift_Data.rf_lift_tarP=-chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.rf_lift_fdbP;
			lift_Data.lb_lift_tarP=chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lb_lift_fdbP;
			lift_Data.rb_lift_tarP=chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.rb_lift_fdbP;
			
			if(lift_Data.lf_lift_fdbP!=lift_Data.rf_lift_fdbP)
			{
				s32 l_r_error=(lift_Data.lf_lift_fdbP-lift_Data.rf_lift_fdbP)/2;
				lift_Data.lf_lift_tarP-=l_r_error;
				lift_Data.rf_lift_tarP+=l_r_error;
			}
			
			if(lift_Data.lb_lift_fdbP!=lift_Data.rb_lift_fdbP)
			{
				s32 l_r_error=(lift_Data.lb_lift_fdbP-lift_Data.rb_lift_fdbP)/2;
				lift_Data.lb_lift_tarP-=l_r_error;
				lift_Data.rb_lift_tarP+=l_r_error;
			}
		}
		else
		{
			if(abs(lift_Data.lf_lift_tarP-lift_Data.lb_lift_tarP)<300)
			{
				if(steady_flat_count<0xFFFE)
				{
					steady_flat_count++;
				}
			}
				
			if(abs(lift_Data.lf_lift_fdbP+lift_Data.rf_lift_fdbP-2*FALL)>18&&abs(lift_Data.lb_lift_fdbP+lift_Data.rb_lift_fdbP-2*FALL)>18)
			{
				if(time_1ms_count%40==0)	//1ms加的太快10MS
				{
					if(lift_Data.lf_lift_fdbP!=lift_Data.rf_lift_fdbP)
					{
						lift_Data.lf_lift_tarP=lift_Data.lf_lift_fdbP<lift_Data.rf_lift_fdbP?lift_Data.rf_lift_fdbP:lift_Data.rf_lift_fdbP;
						lift_Data.rf_lift_tarP=lift_Data.lf_lift_tarP;
					}
					
					if(lift_Data.lb_lift_fdbP!=lift_Data.rb_lift_fdbP)
					{
						lift_Data.lb_lift_tarP=lift_Data.lb_lift_fdbP<lift_Data.rb_lift_fdbP?lift_Data.lb_lift_fdbP:lift_Data.rb_lift_fdbP;
						lift_Data.rb_lift_tarP=lift_Data.lb_lift_tarP;
					}
					lift_Data.lf_lift_tarP-=5;
					lift_Data.rf_lift_tarP-=5;
					lift_Data.lb_lift_tarP-=5;
					lift_Data.rb_lift_tarP-=5;
				}
			}
			
		}
		
		if(steady_flat_count>800)
		{
			lift_Data.lf_lift_tarP=FALL;
			lift_Data.rf_lift_tarP=FALL;
			lift_Data.lb_lift_tarP=FALL;
			lift_Data.rb_lift_tarP=FALL;
		}
//		if(lift_Data.lf_lift_fdbP!=lift_Data.rf_lift_fdbP)
//		{
//			lift_Data.lf_lift_tarP=lift_Data.lf_lift_fdbP<lift_Data.rf_lift_fdbP?lift_Data.rf_lift_fdbP:lift_Data.rf_lift_fdbP;
//			lift_Data.rf_lift_tarP=lift_Data.lf_lift_tarP;
//		}
//		
//		if(lift_Data.lb_lift_fdbP!=lift_Data.rb_lift_fdbP)
//		{
//			lift_Data.lb_lift_tarP=lift_Data.lb_lift_fdbP<lift_Data.rb_lift_fdbP?lift_Data.lb_lift_fdbP:lift_Data.rb_lift_fdbP;
//			lift_Data.rb_lift_tarP=lift_Data.lb_lift_tarP;
//		}
		
		
		lift_Data.lf_lift_tarP=lift_Data.lf_lift_tarP<FALL?FALL:lift_Data.lf_lift_tarP;	//限制行程
		lift_Data.lf_lift_tarP=lift_Data.lf_lift_tarP>ISLAND?ISLAND:lift_Data.lf_lift_tarP;
		
		lift_Data.rf_lift_tarP=lift_Data.rf_lift_tarP<FALL?FALL:lift_Data.rf_lift_tarP;	//限制行程
		lift_Data.rf_lift_tarP=lift_Data.rf_lift_tarP>ISLAND?ISLAND:lift_Data.rf_lift_tarP;
		
		lift_Data.lb_lift_tarP=lift_Data.lb_lift_tarP<FALL?FALL:lift_Data.lb_lift_tarP;	//限制行程
		lift_Data.lb_lift_tarP=lift_Data.lb_lift_tarP>ISLAND?ISLAND:lift_Data.lb_lift_tarP;
		
		lift_Data.rb_lift_tarP=lift_Data.rb_lift_tarP<FALL?FALL:lift_Data.rb_lift_tarP;	//限制行程
		lift_Data.rb_lift_tarP=lift_Data.rb_lift_tarP>ISLAND?ISLAND:lift_Data.rb_lift_tarP;
	}
}

#define TILT 1	//倾斜状态
#define STAEDY_REAL 0	//平稳状态
#define STAEDY_ADJUST 2	//经调整平稳状态
u8 Adjust_Statu=STAEDY_REAL;
void AutoChassisAttitude_Lift_V2(float chassis_pitch_raw)	//自动调整姿态	//pitch正方向为前上	//注意放在lift_task前面
{
	static float chassis_pitch=0;
	static float ka=0.05f;
	
	chassis_pitch=chassis_pitch*(1-ka)+chassis_pitch_raw*ka;
	
	if(GetWorkState()==NORMAL_STATE)
	{
		switch(Adjust_Statu)
		{
			case STAEDY_REAL:
			{
				static u16 tilt_change_count=0;	//若阈值检测效果不好，则使用消抖检测
				lift_Data.lf_lift_tarP=FALL;
				lift_Data.rf_lift_tarP=FALL;
				lift_Data.lb_lift_tarP=FALL;
				lift_Data.rb_lift_tarP=FALL;
				if(abs(chassis_pitch)>8)	//触发阈值	为7时有意外触发现象
				{
					Adjust_Statu=TILT;
				}
				break;
			}
			case TILT:
			{
				static u16 staedy_adjust_count=0;
				if(chassis_pitch>0)	//前仰，可以通过下前解决，当下前无法解决，采用上后
				{
					if(lift_Data.lf_lift_tarP<=FALL)	//若下前至极限
					{
						lift_Data.lb_lift_tarP=chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lb_lift_fdbP;
						lift_Data.rb_lift_tarP=lift_Data.lb_lift_tarP;
					}
					else
					{
						lift_Data.lf_lift_tarP=-chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lf_lift_fdbP;
						lift_Data.rf_lift_tarP=lift_Data.lf_lift_tarP;	//以左前电机为基准
					}
					
				
				}
				else	//前俯，可以通过下后解决，若下后无法解决，采用上前
				{
					if(lift_Data.lb_lift_tarP<=FALL)	//下后
					{
						lift_Data.lf_lift_tarP=-chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lf_lift_fdbP;
						lift_Data.rf_lift_tarP=lift_Data.lf_lift_tarP;	//以左前电机为基准
					}
					else
					{
						lift_Data.lb_lift_tarP=chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lb_lift_fdbP;
						lift_Data.rb_lift_tarP=lift_Data.lb_lift_tarP;
					}
					
					
				}
				
//				lift_Data.lf_lift_tarP=-chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lf_lift_fdbP;
//				lift_Data.rf_lift_tarP=lift_Data.lf_lift_tarP;	//以左前电机为基准
//				lift_Data.lb_lift_tarP=chassis_pitch*AUTOCHASSIS_LIFT+lift_Data.lb_lift_fdbP;
//				lift_Data.rb_lift_tarP=lift_Data.lb_lift_tarP;
				
////////				if(chassis_pitch>0)	//前仰，可以通过下前解决，当下前无法解决，采用上后	待在此处写强制下降下前下后
////////				{
////////					if(abs(lift_Data.lf_lift_tarP-FALL)>)
////////				}
////////				else	//前俯，可以通过下后解决，若下后无法解决，采用上前
////////				{
////////					
////////				}
				
//				if(abs(lift_Data.lf_lift_tarP-FALL)>10&&abs(lift_Data.lb_lift_tarP-FALL)>10)	//未在重心最高点	无效
//				{
//					if(time_1ms_count%20==0)
//					{
//						lift_Data.lf_lift_tarP-=5;
//						lift_Data.rf_lift_tarP-=5;
//						lift_Data.lb_lift_tarP-=5;
//						lift_Data.rb_lift_tarP-=5;
//					}
//				}
				
				if(abs(chassis_pitch)<2&&staedy_adjust_count<0xFFFE)
				{
					staedy_adjust_count++;
				}
				else
				{
					staedy_adjust_count=0;
				}
				
				if(staedy_adjust_count>500)	//稳定了	//待调整
				{
					Adjust_Statu=STAEDY_ADJUST;
					staedy_adjust_count=0;
				}
				break;
			}
			case STAEDY_ADJUST:
			{
				static u16 staedy_real_count=0;
				if(abs(chassis_pitch)>4)
				{
					Adjust_Statu=TILT;
				}
				else
				{
					if(abs(lift_Data.lf_lift_tarP-lift_Data.lb_lift_tarP)<300&&staedy_real_count<0xFFFE)
					{
						staedy_real_count++;
					}
				}
				
				if(staedy_real_count>200)	//0.3s
				{
					Adjust_Statu=STAEDY_REAL;
					staedy_real_count=0;
				}
				break;
			}
		}	
	}
	
	
	lift_Data.lf_lift_tarP=lift_Data.lf_lift_tarP<FALL?FALL:lift_Data.lf_lift_tarP;	//限制行程
	lift_Data.lf_lift_tarP=lift_Data.lf_lift_tarP>ISLAND?ISLAND:lift_Data.lf_lift_tarP;
	
	lift_Data.rf_lift_tarP=lift_Data.rf_lift_tarP<FALL?FALL:lift_Data.rf_lift_tarP;	//限制行程
	lift_Data.rf_lift_tarP=lift_Data.rf_lift_tarP>ISLAND?ISLAND:lift_Data.rf_lift_tarP;
	
	lift_Data.lb_lift_tarP=lift_Data.lb_lift_tarP<FALL?FALL:lift_Data.lb_lift_tarP;	//限制行程
	lift_Data.lb_lift_tarP=lift_Data.lb_lift_tarP>ISLAND?ISLAND:lift_Data.lb_lift_tarP;
	
	lift_Data.rb_lift_tarP=lift_Data.rb_lift_tarP<FALL?FALL:lift_Data.rb_lift_tarP;	//限制行程
	lift_Data.rb_lift_tarP=lift_Data.rb_lift_tarP>ISLAND?ISLAND:lift_Data.rb_lift_tarP;
}


/***********************************************************
对于V2版本，出现回中时重心升高问题解决思想
1.发生后及时处理，在检测到轮平面低于一阈值时及时降到最低平面（方案阈值检测鲁棒性延迟问题）
2.在发生阶段避免，当pitch>0,车身前仰，在TITL调整阶段
	无需上升的不上升，可通过下降解决的全通过下降解决
	
***********************************************************/

