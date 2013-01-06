/*
 * A2D_Module.c
 *
 * Created: 08/10/2012 21:38:35
 *  Author: NASSER
 */ 

#include "A2D_Module.h"
#include "Generic_Module.h"
#include "std_defs.h"

/////////////////////////////////
//// A2D Sensing Subsystem
/////////////////////////////////

void  a2d_init()
{
	MCU_A2D_Initialize();
}

volatile int a2d_get_temp(unsigned int iChannel) // Can be Channel 0 or Channel 1
{
	volatile float umx = 0.0f;
	
	if (iChannel == 0) {
		 umx = MCU_A2D_GetTemp1();
	}
			 
	if (iChannel == 1) {
		 umx = MCU_A2D_GetTemp2();
	}	
		 
	return umx;
}

volatile int a2d_get_voltage(unsigned int iVoltageChannel) // Choose between the available voltage channels
{
	if (iVoltageChannel == A2D_CHANNEL_1V)		 return MCU_A2D_Get1V();
    if (iVoltageChannel == A2D_CHANNEL_3P3V)	 return MCU_A2D_Get3P3V();
	if (iVoltageChannel == A2D_CHANNEL_PWR_MAIN) return MCU_A2D_GetPWR_MAIN();
	return 0;
}