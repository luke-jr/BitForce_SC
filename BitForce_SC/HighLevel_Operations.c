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

#include "AVR32_OptimizedTemplates.h"

#include <string.h>
#include <stdio.h>

#include <avr32/io.h>


volatile void HighLevel_Operations_Spin()
{
	// Nothing for the moment
	// Reset Watchdog to prevent system reset. (Timeout for watchdog is 17ms)
	WATCHDOG_RESET;	
	
	// Job-Pipe Scheduling
	Flush_buffer_into_engines();
	
	// Scan XLINK Chain, to be executed every 1.2 seconds
	if (XLINK_ARE_WE_MASTER == TRUE)
	{
		// We refresh the chain every 1.2 seconds
		static volatile UL32 iInitialTimeHolder = 0;
		volatile UL32 iActualTickHolder = MACRO_GetTickCountRet;
		
		if (iActualTickHolder - iInitialTimeHolder > 1200000) // 1,200,000 us = 1.2sec
		{
			iInitialTimeHolder = iActualTickHolder;
			XLINK_MASTER_Refresh_Chain();
		}		
	}
	
	// Fan-Spin must be executed every 0.5 seconds
	{
		static volatile UL32 iInitialTimeHolder = 0;
		volatile UL32 iActualTickHolder = MACRO_GetTickCountRet;
		
		if (iActualTickHolder - iInitialTimeHolder > 500000) // 500,000 us = 0.5sec
		{
			iInitialTimeHolder = iActualTickHolder;
			
			// Call our Fan-Spin
			FAN_SUBSYS_IntelligentFanSystem_Spin();
		}
	}
	
	// Global Blink-Request subsystem
	{
		// Blink time holder
		static volatile UL32 iInitialTimeHolder = 0;
		
		if (GLOBAL_BLINK_REQUEST > 0)
		{
			static char actualBlinkValue = FALSE;
			volatile UL32 iActualTickHolder = MACRO_GetTickCountRet;
			
			if (iActualTickHolder - iInitialTimeHolder > 30000) // 30,000 us =  30 msec
			{
				iInitialTimeHolder = iActualTickHolder;
				
				// Reverse blink value, but set it to 1 if GLOBAL_BLINK_REQUEST is 1
				if (GLOBAL_BLINK_REQUEST == 1) 
					actualBlinkValue = TRUE;
				else
					actualBlinkValue = (actualBlinkValue == FALSE) ? TRUE : FALSE;
					
				// Now set the LED
				if (actualBlinkValue == TRUE)
					MCU_MainLED_Set();
				else
					MCU_MainLED_Reset();
					
				// BTW, Reduct he GLOBAL_BLINK_REQUEST
				GLOBAL_BLINK_REQUEST--;
			}
		}
		else
		{
			iInitialTimeHolder = MACRO_GetTickCountRet;
		}			
	}
	

	
}

