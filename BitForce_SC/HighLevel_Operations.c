/*
 * HighLevel_Operations.c
 *
 * Created: 10/01/2013 00:13:49
 *  Author: NASSER
 */ 

#include "std_defs.h"
#include "MCU_Initialization.h"
#include "JobPipe_Module.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include "USBProtocol_Module.h"
#include "A2D_Module.h"
#include "ASIC_Engine.h"

#include "HostInteractionProtocols.h"
#include "HighLevel_Operations.h"
#include "FAN_Subsystem.h"

#include <string.h>
#include <stdio.h>

#include <avr32/io.h>




volatile void HighLevel_Operations_Spin()
{
	// Nothing for the moment
	// Reset Watchdog to prevent system reset. (Timeout for watchdog is 17ms)
	WATCHDOG_RESET;
	
	// Call our Fan-Spin
	FAN_SUBSYS_IntelligentFanSystem_Spin();
	
}

