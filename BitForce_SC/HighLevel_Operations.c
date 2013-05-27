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
	/*
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
	*/		
	
	// Fan-Spin must be executed every 0.5 seconds
	{
		static volatile UL32 iInitialTimeHolder = 0;
		volatile UL32 iActualTickHolder = MACRO_GetTickCountRet;
		
		if (iActualTickHolder - iInitialTimeHolder > 5000000) // 5,000,000 us = 5sec
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
				{
					MCU_MainLED_Set();
				}					
				else
				{
					MCU_MainLED_Reset();
				}
									
				// BTW, Reduct he GLOBAL_BLINK_REQUEST
				GLOBAL_BLINK_REQUEST--;
			}
		}
		else
		{
			iInitialTimeHolder = MACRO_GetTickCountRet;
		}			
	}
	
	// Blink Chip LEDs if needed
	{
		volatile unsigned char index_hover = 0;
		
		for (index_hover = 0; index_hover < 8; index_hover++)
		{
			if (CHIP_EXISTS(index_hover))
			{
				if (GLOBAL_ChipActivityLEDCounter[index_hover] != 0) 
				{
					MCU_LED_Reset(index_hover+1);  // LEDs are 1based
					GLOBAL_ChipActivityLEDCounter[index_hover]--; 
				} 
				else 
				{ 
					MCU_LED_Set(index_hover+1);   // LEDs are 1based
				}
			}				
		}			
	}
	
	// Should we Pulse?
	{
		if (GLOBAL_PULSE_BLINK_REQUEST != 0)
		{
			// Ok, we turn the LED OFF if 200ms if left until the Pulse Request Expires
			// First, is GlobalPulseRequest already expired?
			unsigned int iDiffVal = MACRO_GetTickCountRet - GLOBAL_PULSE_BLINK_REQUEST;
			
			// if iDiffVal > 1,000,000 it means it's over, and we'll reset it...			
			// if iDiffVal <   900,000 we'll keep the LED ON
			// if iDiffVal >   900,000 we'll keep the LED OFF
			if (iDiffVal > 1000000)
			{
				// Turn the LED on and Reset the GLOBAL_PULSE_BLINK_REQUEST
				MCU_MainLED_Set();				
				GLOBAL_PULSE_BLINK_REQUEST = 0;
			}
			else
			{
				if (iDiffVal < 900000)
				{
					MCU_MainLED_Set(); // We leave it on...
				}	
				else
				{
					MCU_MainLED_Reset(); // We'll turn it off...					
				}
			}		
		}
	}
	
	
	// Engine monitoring Authority
	#if defined(__ENGINE_ACTIVITY_SUPERVISION)
	{
		// We perform this run every 200th attempt
		static int iActualAttempt = 0;
		iActualAttempt++;
		
		if (iActualAttempt == 200)	
		{
			// Reset
			iActualAttempt = 0;
			
			// If anything was modified, we have to recalculate nonce-range
			char bWasAnyEngineModified = FALSE;
			
			// Now we check the array. Is any active engine taking ENGINE_MAXIMUM_BUSY_TIME to complete? If so, cut it off
			for (char xChip = 0; xChip < TOTAL_CHIPS_INSTALLED; xChip++)
			{
				// Does the chip exist?
				if (!CHIP_EXISTS(xChip)) continue;
				
				// Now check the engines
				for (char yEngine = 0; yEngine < 16; yEngine++)
				{
					#if defined(DO_NOT_USE_ENGINE_ZERO)
						if (yEngine == 0) continue;
					#endif
					
					// Is engine in use?
					if (!IS_PROCESSOR_OK(xChip, yEngine)) continue; 
					
					// Ok, now check the information. If this engine
					// is active mining and it has been running for more than ENGINE_MAXIMUM_BUSY_TIME then cut it off
					if (GLOBAL_ENGINE_PROCESSING_STATUS[xChip][yEngine] == FALSE) continue; // No need to check, as this engine is IDLE
					
					// Check operating time
					if ((unsigned int)(MACRO_GetTickCountRet - GLOBAL_ENGINE_PROCESSING_START_TIMESTAMP[xChip][yEngine]) > ENGINE_MAXIMUM_BUSY_TIME)
					{
						// Deactivate it
						DECOMMISSION_PROCESSOR(xChip, yEngine);
						bWasAnyEngineModified = TRUE;
					}
					
				}
			}
			
			// Did we deactivate any engine? If so, we need to recalculate nonce-range for engines (Only if we are NOT running one engine per chip
			#if !defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
				if (bWasAnyEngineModified == TRUE)
				{
					ASIC_calculate_engines_nonce_range();
				}
			#endif
		}
	}
	#endif
	
}

