/*
 * A2D_Module.h
 *
 * Created: 08/10/2012 21:38:25
 *  Author: NASSER GHOSEIRI
 * Company: Butterfly Labs
 */ 

#ifndef A2D_MODULE_H_
#define A2D_MODULE_H_

#include "std_defs.h"

// *********************************************************** A2D Converter

////////////////////////////////////////////////////////////////////////////
// Analog to digital functions  ////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define A2D_CHANNEL_1V			0
#define A2D_CHANNEL_3P3V		1
#define A2D_CHANNEL_PWR_MAIN	2

void  		 a2d_init(void);
volatile int a2d_get_temp(unsigned int iChannel); // Can be Channel 0 or Channel 1
volatile int a2d_get_voltage(unsigned int iVoltageChannel); // Choose between the available voltage channels

#endif /* A2D_MODULE_H_ */

