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

#include <string.h>
#include <stdio.h>


void HighLevel_Operations_Spin()
{
	// We have to execute Tasks here
	
	// This functions sends the next job in the queue
	// should the engines have finished the previously issue job
	/*Flush_p2p_buffer_into_engines(); 
		
	// Switch the LED
	static char iVal = 0;
	if (iVal == 0) MCU_MainLED_Set(); else MCU_MainLED_Reset();
	iVal = (iVal == 0) ? 1 : 0;	*/
}