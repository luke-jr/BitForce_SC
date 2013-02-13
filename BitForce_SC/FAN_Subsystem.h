/*
 * FAN_Subsystem.h
 *
 * Created: 11/02/2013 23:10:23
 *  Author: NASSER
 */ 


#ifndef FAN_SUBSYSTEM_H_
#define FAN_SUBSYSTEM_H_

// FAN sub speed
#define FAN_STATE_AUTO			9
#define FAN_STATE_VERY_SLOW		0
#define FAN_STATE_SLOW			1
#define FAN_STATE_MEDIUM		2
#define FAN_STATE_FAST			3
#define FAN_STATE_VERY_FAST		4

volatile char FAN_ActualState;
volatile UL32 FAN_ActualState_EnteredTick;

volatile void FAN_SUBSYS_Initialize(void);
volatile void FAN_SUBSYS_IntelligentFanSystem_Spin(void);
volatile void FAN_SUBSYS_SetFanState(char iState);


#endif /* FAN_SUBSYSTEM_H_ */