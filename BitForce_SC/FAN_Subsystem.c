/*
 * FAN_Subsystem.c
 *
 * Created: 11/02/2013 23:10:36
 *  Author: NASSER
 */ 

// Include standard definitions
#include "std_defs.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include <string.h>
#include "AVR32X\AVR32_Module.h"
#include <avr32/io.h>
#include "AVR32_OptimizedTemplates.h"
#include "FAN_Subsystem.h"

// Now to our codes

volatile void FAN_SUBSYS_Initialize(void)
{
	// Initialize state to 0
	FAN_SUBSYS_SetFanState(FAN_STATE_AUTO);		
}

#define FAN_CONTROL_BYTE_VERY_SLOW	0
#define FAN_CONTROL_BYTE_SLOW		0
#define FAN_CONTROL_BYTE_MEDIUM		0
#define FAN_CONTROL_BYTE_FAST		0
#define FAN_CONTROL_BYTE_VERY_FAST	0

volatile void FAN_SUBSYS_IntelligentFanSystem_Spin(void)
{
	// We execute this function every 50th call
	static volatile char __attempt = 0;
	
	if (__attempt++ < 50) return;
	
	// It is the 50th call
	__attempt = 0;
	
	// Check temperature
	volatile int iTemp1 = __AVR32_A2D_GetTemp1();
	volatile int iTemp2 = __AVR32_A2D_GetTemp2();
	volatile int iTempAveraged = (iTemp2 + iTemp1) / 2;
	
	// Are we in critical situation? Override FAN if necessary
	if (iTempAveraged > 95)
	{
		// Holy jesus! We're in a critical situation...
		GLOBAL_IS_CRITICAL_TEMPERATURE = TRUE;
		
		// Override fan, set it to maximum
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_FAST);
		
		// We're done. The device will no longer process nonces
		return;
	}
	
	// If we're here, it means we're not critical anymore
	GLOBAL_IS_CRITICAL_TEMPERATURE = FALSE;
	
	// Ok, now set the FAN speed according to our setting
	if (FAN_ActualState == FAN_STATE_VERY_SLOW)
	{
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_SLOW);
		return;
	}
	else if (FAN_ActualState == FAN_STATE_SLOW)
	{
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_SLOW);
		return;
	}
	else if (FAN_ActualState == FAN_STATE_MEDIUM)
	{
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_MEDIUM);
		return;		
	}
	else if (FAN_ActualState == FAN_STATE_FAST)	
	{
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_FAST);
		return;		
	}
	else if (FAN_ActualState == FAN_STATE_VERY_FAST)
	{
		// Set the fan speed
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_FAST);
		return;		
	}
	
	// We're in AUTO mode... There are rules to respect form here...
	if (iTempAveraged <= 30)
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_SLOW);
	else if ((iTempAveraged > 30) && (iTempAveraged <= 40))
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_SLOW);
	else if ((iTempAveraged > 40) && (iTempAveraged <= 50))
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_MEDIUM);	
	else if ((iTempAveraged > 50) && (iTempAveraged <= 60))
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_FAST);		
	else 
		__AVR32_FAN_SetSpeed(FAN_CONTROL_BYTE_VERY_FAST);		
		
	// Ok, We're done...
}

volatile void FAN_SUBSYS_SetFanState(char iState)
{
	FAN_ActualState = iState;
	FAN_ActualState_EnteredTick = MACRO_GetTickCountRet;
}