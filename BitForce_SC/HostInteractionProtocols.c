/*
 * OperationProtocols.c
 *
 * Created: 06/01/2013 17:25:01
 *  Author: NASSER GHOSEIRI
 */ 
#include "std_defs.h"
#include "MCU_Initialization.h"
#include "JobPipe_Module.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include "USBProtocol_Module.h"
#include "A2D_Module.h"
#include "ASIC_Engine.h"
#include <avr32/io.h>
#include "HostInteractionProtocols.h"
#include "AVR32X/AVR32_Module.h"
#include "AVR32_OptimizedTemplates.h"

#include <string.h>
#include <stdio.h>

// Declared somewhere else

PROTOCOL_RESULT Protocol_chain_forward(char iTarget, char* sz_cmd, unsigned int iCmdLen)
{
	// OK We've detected a ChainForward request. First Send 'OK' to the host
	char szRespData[2048];
	unsigned int iRespLen = 0;
	char  bDeviceNotResponded = 0;
	char  bTimeoutDetected = 0;
	
	for (unsigned int umx = 0; umx < 2048; umx++) szRespData[umx] = 0;
	
	volatile char iTotalAttempts = 0; // In case of an error, we'll retry several times. The error can be 3, 5 or 50
	
	while (iTotalAttempts < 3)
	{
		// Reset Watchdog
		WATCHDOG_RESET;
		
		// Proceed
		XLINK_MASTER_transact(iTarget,
						  sz_cmd,
						  iCmdLen,
						  szRespData,
						  &iRespLen,
						  2048,					// Maximum response length
						  __XLINK_TRANSACTION_TIMEOUT__,
						  &bDeviceNotResponded,
						  &bTimeoutDetected,
						  TRUE);
	
		// Check response errors
		if (bDeviceNotResponded)
		{
			USB_send_string("ERR:NO RESPONSE");
			return PROTOCOL_FAILED;
		}
	
		if (bTimeoutDetected)
		{
			if ((iTotalAttempts < 3) && ((bTimeoutDetected == 3) || (bTimeoutDetected == 5) || (bTimeoutDetected == 50)))
			{
				// Just retry
				iTotalAttempts++;
				continue;
			}
			else
			{
				sprintf(szRespData,"ERR:TIMEOUT %d\n", bTimeoutDetected);
				USB_send_string(szRespData);
				return PROTOCOL_FAILED;				
			}
		}		
	}
		
	// Send the response back to our host
	USB_write_data (szRespData, iRespLen);
	
	// We've been successful
	return PROTOCOL_SUCCESS;
}

PROTOCOL_RESULT Protocol_id(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
		USB_send_string(UNIT_ID_STRING);  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_respond_transact(UNIT_ID_STRING,
									 strlen(UNIT_ID_STRING),
									 __XLINK_TRANSACTION_TIMEOUT__,
									 &bTimeoutDetected,
									 FALSE);
	}
	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_info_request(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// What is our information request?
	char szInfoReq[8024];
	char szTemp[256];
	
	strcpy(szInfoReq,"DEVICE: BitFORCE SC\n");
	strcpy(szTemp,"");
	
	// Add Firmware Identification
	sprintf(szTemp,"FIRMWARE: %s\n", __FIRMWARE_VERSION);
	strcat(szInfoReq, szTemp);
	
	volatile UL32 uL1   = 0;    
	volatile UL32 uL2   = 0;
	volatile UL32 vL1	= 0;
	volatile UL32 vL2	= 0;    
	volatile UL32 xL1	= 0;
	volatile UL32 xL2	= 0;
	volatile UL32 uLRes = 0;    
	
	// Atomic Full-Asm Special CPLD Write latency
	unsigned int  iLM[16];
	unsigned char ilmCounter = 0;
	unsigned int  iNonceCount;
	
	
	unsigned int iTMTX = MACRO_GetTickCountRet;
	JobPipe__test_buffer_shifter();
	unsigned int iTMTX2 = MACRO_GetTickCountRet - iTMTX;
	sprintf(szTemp,"JobPipe__test_buffer_shifter takes %d us\n", iTMTX2);
	strcat(szInfoReq,szTemp);
	
	// Display mark1, mark2 and diff
	sprintf(szTemp,"Mark1: %d, Mark2: %d, Diff: %d\n", iMark1, iMark2, (unsigned int)(iMark2 - iMark1));
	strcat(szInfoReq, szTemp);
	
	#if defined(__REPORT_TEST_MINING_SPEED)
		// Our job-packet by default (Expected nonce is 8D9CB675 - Hence counter range is 8D9C670 to 8D9C67A)
		static job_packet jp;
		unsigned char index;
		char __stat_blockdata[] = {0xAC,0x84,0xF5,0x8B,0x4D,0x59,0xB7,0x4D,0x1B,0x2,0x85,0x52};
		char __stat_midstate[]  = {0x3C,0x42,0x49,0xA8,0xE1,0xC5,0x45,0x78,0xA5,0x2D,0x83,0xC1,0x1,0xE5,0xC5,0x8E,0xF5,0x2F,0x3,0xD,0xEE,0x2E,0x9D,0x29,0xB6,0x94,0x9A,0xDF,0xA6,0x95,0x97,0xAE};
		//char __stat_blockdata[] = {0x82,0xCC,0x07,0x16,0x51,0x9E,0x8B,0x95,0x1A,0x01,0x7F,0xE9};			
		//char __stat_midstate[]  = {0xE1,0x08,0xDA,0xE6,0xB6,0x0C,0xB5,0x0E,0x55,0xE8,0xC4,0x53,0xE3,0xCA,0x39,0x15,0x8B,0x5C,0x32,0xFB,0x41,0x34,0xC2,0xA2,0xD4,0x97,0xD9,0xDE,0xD5,0x35,0x67,0x64};
		
		for (index = 0; index < 12; index++) jp.block_data[index] = __stat_blockdata[index];
		for (index = 0; index < 32; index++) jp.midstate[index]   = __stat_midstate[index];
		jp.signature  = 0xAA;
	
		// By default it's set to true...
		unsigned char isProcessorOK = TRUE;
		
		// Now send the job to the engine
		volatile unsigned int UL_TIMEZERO = MACRO_GetTickCountRet;
		
		ASIC_job_issue(&jp, 0x0, 0xFFFFFFFF, FALSE, 0);
		
		volatile unsigned int UL_TIMESTART = MACRO_GetTickCountRet;
		volatile unsigned int UL_TOTALTIME = 0;		
	
		// Read back result. It should be DONE with FIFO zero of it being 0x8D9CB675
		while(ASIC_is_processing())
		{
			if (MACRO_GetTickCountRet - UL_TIMESTART > 2000000) break; // 2 Seconds is pretty much a timeout
			 WATCHDOG_RESET;
		}			 
		
		UL_TOTALTIME = MACRO_GetTickCountRet - UL_TIMESTART;
		
		/*unsigned int iResVals[16];
		unsigned int iResCount = 0;
		ASIC_get_job_status(iResVals, &iResCount, FALSE, 0);
		sprintf(szTemp,"--NONCES:%08X,%08X\n", iResVals[0], iResVals[1]);
		strcat(szInfoReq, szTemp);*/
		
		// Total time in microseconds is in UL_TOTALTIME
		float fTotalSpeed = ((4.2969 * 1000000) / (UL_TOTALTIME));
		float fTotalSpeedPostJob = ((4.2969 * 1000000) / (MACRO_GetTickCountRet - UL_TIMEZERO));
		sprintf(szTemp,"RAW MINIG SPEED: %.2f GH/s\n", fTotalSpeed);
		strcat(szInfoReq, szTemp);		
		sprintf(szTemp,"JOB SUBMISSION: %d us\n", UL_TIMESTART - UL_TIMEZERO);
		strcat(szInfoReq, szTemp);		
		sprintf(szTemp,"ZDX(SX) Mining Speed: %.2f GH/s\n", fTotalSpeedPostJob);
		strcat(szInfoReq, szTemp);		
		
		// Do we test pipe?
		#if defined(__TEST_PIPE_PERFORMANCE)
		
			JobPipe__pipe_set_buf_job_results_count(0);
			JobPipe__pipe_push_job(&jp);
			JobPipe__pipe_push_job(&jp);
			JobPipe__pipe_push_job(&jp);

			volatile unsigned int iActualTPP_Count = 0;		
			char	 bTimeoutDetected = FALSE;
			volatile unsigned int iActualTime = MACRO_GetTickCountRet;	
			
			volatile unsigned int iMicroKernelT1;
			volatile unsigned int iMicroKernelT2;
			volatile unsigned int iMicroKernelTMAX = 0;
			volatile unsigned char iPA = 0;
								
			while (JobPipe__pipe_get_buf_job_results_count() < 3)
			{
				// Do we have timeout?
				volatile unsigned int iVMR = MACRO_GetTickCountRet + 2;
				iVMR -= iActualTime;
				if ( iVMR >= 2000000)
				{
					bTimeoutDetected = TRUE;
					break;
				}
				
				// Proceed
				WATCHDOG_RESET;
				iMicroKernelT1 = MACRO_GetTickCountRet;
				Microkernel_Spin();
				iMicroKernelT2 = MACRO_GetTickCountRet - iMicroKernelT1 + 1;
				
				if (iMicroKernelT2 > iMicroKernelTMAX) 
				{
					iMicroKernelTMAX = iMicroKernelT2;
					iPA = JobPipe__pipe_get_buf_job_results_count();
				}					
				
				// If job count is one then set the thing
				if (JobPipe__pipe_get_buf_job_results_count() == 1)
				{
					if (iActualTPP_Count == 0)
					{
						iActualTPP_Count = MACRO_GetTickCountRet;
					}						
				}
				else if (JobPipe__pipe_get_buf_job_results_count() == 2)
				{
					iActualTPP_Count = MACRO_GetTickCountRet - iActualTPP_Count;
					break;
				}
			}
			
			sprintf(szTemp,"MicroKernel TMAX: %dus , iPA: %d\n", iMicroKernelT2,iPA);
			strcat(szInfoReq, szTemp);
		
			if (bTimeoutDetected == TRUE)
			{
				sprintf(szTemp,"TIMEOUT DETECTED @ %dus!\n", MACRO_GetTickCountRet - iActualTime );
				strcat(szInfoReq, szTemp);				
			}
			else
			{
				unsigned int iTotalPipeTimeItTook = iActualTPP_Count;
				float fTotalPipeSpeed = ((4.2949 * 1000000) / (iTotalPipeTimeItTook));
				sprintf(szTemp,"PIPE Mining Speed: %.2f GH/s\n", fTotalPipeSpeed);
				strcat(szInfoReq, szTemp);				
			}

		#endif 
				
	#endif
	
	
		
	
	#if defined(__CHIP_FREQUENCY_DETECT_AND_REPORT)
		for (char umx = 0; umx < TOTAL_CHIPS_INSTALLED; umx++)
		{		
			// Reset watchdog
			WATCHDOG_RESET;
				
			// Good engines number
			char iGoodEnginesIndex = 0xFF;
				
			// Check chip existance
			if (!CHIP_EXISTS(umx)) continue;
				
			// Report speed of this processor
			#if !defined(__LIVE_FREQ_DETECTION)
			
				// Report what we had detected before
				sprintf(szTemp,"PROCESSOR %d: %d engines @ %d MHz\n", umx, ASIC_get_chip_processor_count(umx) ,GLOBAL_CHIP_FREQUENCY_INFO[umx]);
				strcat(szInfoReq, szTemp);
			#else
			
				// Now detect a good engine
				for (char umz = 0; umz < 16; umz++)
				{
					// Should we ignore engine zero?
					#if defined(DO_NOT_USE_ENGINE_ZERO)
						if (umz == 0) continue;
					#endif
				
					// Is this engine ok?
					if (!IS_PROCESSOR_OK(umx, umz)) continue;
				
					// Is it a good engine?
					iGoodEnginesIndex = umz;
					break;
				}
			
				// Did we find any good engines?
				if (iGoodEnginesIndex == 0xFF)
				{
					sprintf(szTemp,"PROCESSOR %d: NO GOOD ENGINES!\n", umx);
					strcat(szInfoReq, szTemp);
				}
				else
				{
					// We have a good engine, test it
					int iDetectedFreq = 0;
					iDetectedFreq = ASIC_tune_chip_to_frequency(umx, iGoodEnginesIndex, TRUE);
					sprintf(szTemp,"PROCESSOR %d: %d engines @ %d MHz\n", umx, ASIC_get_chip_processor_count(umx) , (iDetectedFreq / 1000000));
					strcat(szInfoReq, szTemp);
				}
			#endif 
		}
	
	#endif
	
	#if defined(__REPORT_BALANCING_SPREADS)
		for (char vChip = 0; vChip < TOTAL_CHIPS_INSTALLED; vChip++)
		{
			if (!CHIP_EXISTS(vChip)) continue;
			
			for (char vEngine = 0; vEngine < 16; vEngine++)
			{
				if (!IS_PROCESSOR_OK(vChip, vEngine)) continue;
				
				// Report
				sprintf(szTemp,"CHIP %d PROCESSOR %d: %08X to %08X, TOTAL: %d\n", 
						vChip, vEngine, GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[vChip][vEngine], 
						GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][vEngine], 
						GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][vEngine] - GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[vChip][vEngine]);
				strcat(szInfoReq, szTemp);				
			}
		}
		
	
	#endif
	
	#if defined(__CHIP_DIAGNOSTICS_VERBOSE)
		// Turn LED OFF
		MCU_MainLED_Reset();
				
		// Working...
		volatile char bFailedEngine = FALSE;
		
		// Now run chip-by-chip diagnostics
		#if defined(__CHIP_BY_CHIP_DIAGNOSTICS)
		
					
			unsigned char umx = 0;
			for (umx = 0; umx < TOTAL_CHIPS_INSTALLED; umx++)
			{
				if (!CHIP_EXISTS(umx)) continue;

				// Show spreads?
				#if defined(__EXPORT_ENGINE_RANGE_SPREADS)
					sprintf(szTemp,"[CHIP %d] Spreads: %08X-%08X, %08X-%08X, %08X-%08X, %08X-%08X\n", umx, __ENGINE_LOWRANGE_SPREADS[umx][0], __ENGINE_HIGHRANGE_SPREADS[umx][0], __ENGINE_LOWRANGE_SPREADS[umx][1], __ENGINE_HIGHRANGE_SPREADS[umx][1], __ENGINE_LOWRANGE_SPREADS[umx][2], __ENGINE_HIGHRANGE_SPREADS[umx][2], __ENGINE_LOWRANGE_SPREADS[umx][3], __ENGINE_HIGHRANGE_SPREADS[umx][3]);
					strcat(szInfoReq, szTemp);
					sprintf(szTemp,"[CHIP %d] Spreads: %08X-%08X, %08X-%08X, %08X-%08X, %08X-%08X\n", umx, __ENGINE_LOWRANGE_SPREADS[umx][4], __ENGINE_HIGHRANGE_SPREADS[umx][4], __ENGINE_LOWRANGE_SPREADS[umx][5], __ENGINE_HIGHRANGE_SPREADS[umx][5], __ENGINE_LOWRANGE_SPREADS[umx][6], __ENGINE_HIGHRANGE_SPREADS[umx][6], __ENGINE_LOWRANGE_SPREADS[umx][7], __ENGINE_HIGHRANGE_SPREADS[umx][7]);
					strcat(szInfoReq, szTemp);
					sprintf(szTemp,"[CHIP %d] Spreads: %08X-%08X, %08X-%08X, %08X-%08X, %08X-%08X\n", umx, __ENGINE_LOWRANGE_SPREADS[umx][8], __ENGINE_HIGHRANGE_SPREADS[umx][8], __ENGINE_LOWRANGE_SPREADS[umx][9], __ENGINE_HIGHRANGE_SPREADS[umx][9], __ENGINE_LOWRANGE_SPREADS[umx][10], __ENGINE_HIGHRANGE_SPREADS[umx][10], __ENGINE_LOWRANGE_SPREADS[umx][11], __ENGINE_HIGHRANGE_SPREADS[umx][11]);
					strcat(szInfoReq, szTemp);					
					sprintf(szTemp,"[CHIP %d] Spreads: %08X-%08X, %08X-%08X, %08X-%08X, %08X-%08X\n", umx, __ENGINE_LOWRANGE_SPREADS[umx][12], __ENGINE_HIGHRANGE_SPREADS[umx][12], __ENGINE_LOWRANGE_SPREADS[umx][13], __ENGINE_HIGHRANGE_SPREADS[umx][13], __ENGINE_LOWRANGE_SPREADS[umx][14], __ENGINE_HIGHRANGE_SPREADS[umx][14], __ENGINE_LOWRANGE_SPREADS[umx][15], __ENGINE_HIGHRANGE_SPREADS[umx][15]);
					strcat(szInfoReq, szTemp);				
				#endif
				
				// Announce
				sprintf(szTemp,"[CHIP %d] Entry:\n", umx);
				strcat(szInfoReq, szTemp);
											
				// Are we running diagnostics on the engines too?
				#if defined(__ENGINE_BY_ENGINE_DIAGNOSTICS)
					unsigned char i_engine = 0;
					for (i_engine = 0; i_engine < 16; i_engine++)
					{
						// Reset Watchdog
						WATCHDOG_RESET;						
						bFailedEngine = FALSE;
						
						// STATIC RULE - Engine 0 not used
						#if defined(DO_NOT_USE_ENGINE_ZERO)
							if (i_engine == 0) continue; // Do not start engine 0
						#endif
						
						// Is engine ok?
						if (!IS_PROCESSOR_OK(umx, i_engine)) continue;
												
						// OK Proceed...
						uL1 = MACRO_GetTickCountRet;
						job_packet jp;
						ASIC_ReadComplete(umx,i_engine);
						ASIC_job_issue_to_specified_engine(umx, i_engine, &jp, FALSE, TRUE, TRUE, 0x0, 0x10000);
						xL2 = MACRO_GetTickCountRet;
						while (TRUE)
						{
							 __MCU_ASIC_Activate_CS();
						     if (ASIC_has_engine_finished_processing(umx, i_engine) == TRUE) break;
							 __MCU_ASIC_Deactivate_CS();
							 
							 WATCHDOG_RESET;
							 if (MACRO_GetTickCountRet - xL2 > 250000)
							 {
								 bFailedEngine = TRUE;
								 break;
							 }
						}							 
						uL2 = MACRO_GetTickCountRet;
						uLRes = (UL32)((UL32)uL2 - (UL32)uL1);
						if (bFailedEngine)
						{
							sprintf(szTemp,"\t[ENGINE %d] FAILED! us\n", i_engine, (unsigned int)uLRes);
							strcat(szInfoReq, szTemp);
						}						
						else
						{
							sprintf(szTemp,"\t[ENGINE %d] Single-job takes: %u us, Submission: %d\n", i_engine, (unsigned int)uLRes,(unsigned int)xL2 - uL1);
							strcat(szInfoReq, szTemp);							
						}
						
					}
				#endif

			}
			
		#endif
		
		// Turn it back on...
		MCU_MainLED_Set();
	#endif
	
	// Add Engine count
	sprintf(szTemp,"ENGINES: %d\n", ASIC_get_processor_count());
	strcat(szInfoReq, szTemp);
	
	// Add Engine freuqnecy
	sprintf(szTemp,"FREQUENCY: %d MHz\n", __ASIC_FREQUENCY_VALUES[__ASIC_FREQUENCY_ACTUAL_INDEX]);
	strcat(szInfoReq, szTemp);
		
	// Add Chain Status
	sprintf(szTemp,"XLINK MODE: %s\n", XLINK_ARE_WE_MASTER ? "MASTER" : "SLAVE" );
	strcat(szInfoReq, szTemp);
	
	// Critical Temperature
	sprintf(szTemp,"CRITICAL TEMPERATURE: %d\n", (GLOBAL_CRITICAL_TEMPERATURE == TRUE) ? 1 : 0);
	strcat(szInfoReq, szTemp);
		
	
	// Add XLINK chip installed status
	sprintf(szTemp,"XLINK PRESENT: %s\n", (XLINK_is_cpld_present() == TRUE) ? "YES" : "NO");
	strcat(szInfoReq, szTemp);	
	
	// If we are master, how many devices exist in chain?
	if ((XLINK_ARE_WE_MASTER == TRUE) && (XLINK_is_cpld_present() == TRUE))
	{
		// Show total devices
		sprintf(szTemp,"--DEVICES IN CHAIN: %d\n", XLINK_MASTER_getChainLength());
		strcat (szInfoReq, szTemp);
		
		// Show device bit-mask
		sprintf(szTemp,"--CHAIN PRESENCE MASK: %08X\n", GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK);
		strcat (szInfoReq, szTemp);
		
	}
	
	// Add the terminator
	strcat(szInfoReq, "OK\n");

	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szInfoReq);  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		char bTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(szInfoReq, strlen(szInfoReq), __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetected, FALSE);
	}
	
	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_fan_set(char iValue)
{
			
	// Requests...		
	volatile unsigned char iRequestedMod = iValue - 48;
	volatile unsigned char szResponse[64];
		
	strcpy(szResponse,"");
		
	// We have linear request, it should be 0 to 5 and '9'
	if ((iRequestedMod < 0) || ((iRequestedMod > 5) && (iRequestedMod != 9)))
	{	
		// We have an error
		strcpy(szResponse, "ERR:INVALID FAN STATE\n");
	}	
	else
	{	
		// Set the proper state
		// Note: iRequestedMod is now corresponds to FAN_STATE_<VALUE> modes, so we can used it directly
		FAN_SUBSYS_SetFanState(iRequestedMod);
	}	
		
		
	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
	{	
		USB_send_string(szResponse);  // Send it to USB
	}	
	else // We're a slave... send it by XLINK
	{	
		char bTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(szResponse, sizeof(szResponse), 
									 __XLINK_TRANSACTION_TIMEOUT__, 
									 &bTimeoutDetected, FALSE);
	}	
	
	return PROTOCOL_SUCCESS;
}		

PROTOCOL_RESULT Protocol_save_string(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// We can take the job (either we start processing or we put it in the buffer)
	// Send data to ASIC and wait for response...
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");	
	}	
	else
	{
		unsigned int bYTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK\n", sizeof("OK\n"), __XLINK_TRANSACTION_TIMEOUT__, &bYTimeoutDetected, FALSE);
	}
	
	// Wait for job data (96 Bytes)
	volatile char  sz_buf_tag[513];
	volatile char* sz_buf = sz_buf_tag;
	volatile unsigned int i_read;
	volatile unsigned int i_timeout = 1000000000;
	volatile unsigned char i_invaliddata = FALSE;
	

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
	{
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &i_invaliddata);
	}
	else
	{
		char bTimeoutDetectedX = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 1024, __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetectedX, FALSE, FALSE);
		
		// Error check
		if (bTimeoutDetectedX == TRUE) return PROTOCOL_FAILED;
		
		// Since XLINK will save the first character received, we have to advance the pointer
		sz_buf++;
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:TIMEOUT\n",
										 sizeof("ERR:TIMEOUT\n"),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}
		
		return PROTOCOL_TIMEOUT;
	}

	// Check integrity
	if (i_invaliddata == TRUE) // We should've received the correct byte count
	{
		sprintf(sz_buf, "ERR:INVALID DATA\n");
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact(sz_buf,
										 strlen(sz_buf),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}
		
		return PROTOCOL_INVALID_USB_DATA;
	}
	
	// Get string length and put 0 there
	volatile char string_to_save[512];
	volatile char iStringLen = i_read;
	sz_buf[i_read+1] = 0; // Null-Termination
	
	// Now copy it
	strcpy(string_to_save, (char*)(sz_buf));	
	
	// Add signature
	string_to_save[509] = 0x0AA;
	string_to_save[510] = 0x0BB;
	string_to_save[511] = 0x055;
	
	// Save it
	__AVR32_Flash_WriteUserPage((char*)(string_to_save));
	
	// We're ok
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("SUCCESS\n");  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("SUCCESS\n",
									 strlen("SUCCESS\n"),
									 __XLINK_TRANSACTION_TIMEOUT__,
									 &bXTimeoutDetected,
									 FALSE);
	}	
		
	// Return our result
	return res;
	
}

PROTOCOL_RESULT Protocol_load_string(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// The string buffer
	volatile char string_buffer[512];
	
	// Clear signature
	string_buffer[509] = 0x0;
	string_buffer[510] = 0x0;
	string_buffer[511] = 0x0;
	
	// Read
	__AVR32_Flash_ReadUserPage(string_buffer);
	
	// Check signature
	if ((string_buffer[509] == 0x0AA) &&
	    (string_buffer[510] == 0x0BB) &&
	    (string_buffer[511] == 0x055))
	{
		// String is correct, send it
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(string_buffer);  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			char bTimeoutDetected = FALSE;
			XLINK_SLAVE_respond_transact(string_buffer,
										strlen(string_buffer),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bTimeoutDetected,
										FALSE);
		}			   
	}
	else
	{
		// String is correct, send it
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("MEMORY EMPTY\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			char bTimeoutDetected = FALSE;
			XLINK_SLAVE_respond_transact("MEMORY EMPTY\n",
										 strlen("MEMORY EMPTY\n"),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bTimeoutDetected,
										 FALSE);
		}		
	}

	// Return our result...
	return res;	
}

PROTOCOL_RESULT Protocol_Blink(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// Send the OK back first
	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");
	
	// All is good... sent the identifier and get out of here...
	GLOBAL_BLINK_REQUEST = 30;

	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_Echo(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// Send the OK back first
	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("ECHO");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_send_packet(XLINK_get_cpld_id(), "ECHO", 4, TRUE, FALSE);
	
	// Return our result...
	return res;
}

/*
#define TESTMACRO_XLINK_get_TX_status(ret_value)  (OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_TX_STATUS, &ret_value))
#define TESTMACRO_XLINK_get_RX_status(ret_value)  (OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_RX_STATUS, &ret_value))
#define TESTMACRO_XLINK_set_target_address(x)	  (OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_TARGET_ADRS, x))

#define TESTMACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { TESTMACRO_XLINK_get_TX_status(read_tx_status);} \
		TESTMACRO_XLINK_set_target_address(iadrs); \
		unsigned char iTotalToSend = (ilen << 1); \
		char szMMR[4]; \
		OPTIMIZED__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		char iTxControlVal = 0b00000000; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal); \
		OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_START, CPLD_ADDRESS_TX_START_SEND); \
		})	
		
#define TESTMACRO_XLINK_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
while(TRUE) \
{ \
	BC = 0; \
	LP = 0; \
	timeout_detected = FALSE; \
	length = 0; \
	senders_address = 0; \
	volatile char iActualRXStatus = 0; \
	volatile unsigned char us1 = 0; \
	volatile unsigned char us2 = 0; \
	MACRO_XLINK_get_RX_status(iActualRXStatus); \
	UL32 iTimeoutHolder; \
	MACRO_GetTickCount(iTimeoutHolder); \
	UL32 iTickHolder; \
	if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0) \
	{ \
		while (TRUE) \
		{ \
			MACRO_XLINK_get_RX_status(iActualRXStatus); \
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break; \
			MACRO_GetTickCount(iTickHolder); \
			if ((iTickHolder - iTimeoutHolder) > time_out) \
			{ \
				timeout_detected = TRUE; \
				length = 0; \
				senders_address = 0; \
				break; \
			}  \
		} \
		if (timeout_detected == TRUE) break; \
	} \
	volatile char imrLen = 0; \
	imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
	length = imrLen; \
	LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
	BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
	MACRO__AVR32_CPLD_Read(senders_address, CPLD_ADDRESS_SENDERS_ADRS); \
	MACRO__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
	break; \
} \
	}) 		
*/

PROTOCOL_RESULT Protocol_Test_Command(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// Variables
	char szTempMsg[128];
	char szReportResult[4024];
	
	strcpy(szTempMsg,"");
	strcpy(szReportResult,"");
	
	// Success Counter
	int iSuccessCounter = 0;
	volatile UL32 iTurnaroundTime = 0;
	
	// First get our tick counter
	volatile UL32 iActualTick =  MACRO_GetTickCountRet;
	
	// Total timeout counters
	int iTotalTimeoutCounter = 0;
	int iTransactionTimes[30];

	char szResponseFromDev[395];
	int  iResponseLenFromDev = 0;
	char bDevNotResponded = FALSE;
	char bTimedOut = FALSE;
	
	
	// Activate all engines...
	job_packet jp; // We don't need to change anything in it. Some random data exists i'm sure
	unsigned int ilr_counter = 0;
	unsigned char iCurrentLEDState = 0;
	
	// Run for theoretical 7.5seconds
	//for (volatile unsigned int x = 0; x < 10; x++)
	//{
		WATCHDOG_RESET;
		//ASIC_job_issue_to_specified_engine(0,4,&jp, 0, 0x0FFFFFFFF);
		ASIC_job_issue(&jp,0,0x0FFFFFFFF, FALSE, 0);
		//Test_Sequence_2(3,1);
		
		/*__MCU_ASIC_Activate_CS();
		
		LoadBarrierAdder(0,1);
		LoadLimitRegisters(0,1);
		LoadRegistersValues_H0(0,1);
		LoadRegistersValues_H1(0,1);
		LoadRegistersValues_W(0,1);
		LoadCounterBounds(0,1,0,0x0FFFFFFFF);
		ASIC_WriteComplete(0,1);
		
		__MCU_ASIC_Deactivate_CS();*/
		
		// Wait for it to be finished
		/*while (ASIC_is_processing() == TRUE)
		{
			WATCHDOG_RESET;
			
			// Blink
			if ((ilr_counter % 100000) == 0)
			{
				if (iCurrentLEDState != 0)
				{
					 MCU_MainLED_Set();
					 iCurrentLEDState = 0;
				}
				else
				{
					MCU_MainLED_Reset();
					iCurrentLEDState = 1;
				}				
			}
			
			ilr_counter++;
		}*/
	//}
	
	MCU_MainLED_Set();	
	
	return PROTOCOL_SUCCESS; // We will not proceed with the code here
	
	///////////////////////////////////////////////////////////
	// CPLD TEST Write different values to CPLD and read back
	//////////////////////////////////////////////////////////
	
	volatile UL32 iActualTickTick = MACRO_GetTickCountRet;
	
	int iTotalWrongReads = 0;
	unsigned int umi = 0;
	char burst_write[4] = {0,0,0,0};
	char burst_read[4]  = {0,0,0,0};
		
	// Total time taken
	volatile UL32 iTotalTimeTaken = 0;
	volatile UL32 iTempX = 0;
	
	// Calculate single turn-around time
	volatile UL32 iSecondaryTickTemp = 0;
	volatile UL32 iActualTickTemp = 0;
	volatile UL32 iTotalGoodDelay = 0;
	
	iTurnaroundTime = 0x0FFFFFFFF;
	
	volatile char szFailures[12000];
	volatile char szFailTemp[128];
	strcpy(szFailTemp,"");
	strcpy(szFailures,"");	
	
	volatile char iTotal3Error = 0;
	volatile char iTotal5Error = 0;
	volatile char iTotal50Error = 0;
	volatile char iTotal7Error = 0;
	volatile char iTotal4Error = 0;
	volatile char iTotal51Error = 0;
	volatile char iTotalUTError = 0;
	volatile char iTotalXError = 0;
	
	volatile char szKeep[4] = {0,0,0,0};
	
	// Now we send message to XLINK_GENERAL_DISPATCH_ADDRESS for an ECHO and count the successful iterations
	for (unsigned int x = 0; x < 100000; x++)
	{
		// Reset WAtchdog
		WATCHDOG_RESET;
		
		// Clear input
		MACRO_XLINK_clear_RX;
		
		// Now we wait for response
		volatile char bTimedOut = FALSE;
		volatile char szResponse[10] = {0,0,0,0,0,0,0,0,0,0};
		volatile unsigned int iRespLen = 0;
		volatile char iBC = 0;
		volatile char iLP = 0;
		volatile char iSendersAddress = 0;
		
		szResponse[0] = 0; szResponse[1] = 0; szResponse[2] = 0; szResponse[3] = 0;
		iRespLen = 0;	
		
		iActualTickTemp = MACRO_GetTickCountRet;
	
		// Send the command
		//MACRO_XLINK_send_packet(1, "ZAX", 3, TRUE, FALSE);
		//MACRO_XLINK_wait_packet(szResponse, iRespLen, 90, bTimedOut, iSendersAddress, iLP, iBC );
		XLINK_MASTER_transact(1,"ZRX", 3, szResponse, &iRespLen, 128, __XLINK_TRANSACTION_TIMEOUT__, &bDevNotResponded, &bTimedOut, TRUE);
	
		// Update turnaround time
		iSecondaryTickTemp = MACRO_GetTickCountRet;
			
		// Increase total time
		iTempX = (iSecondaryTickTemp - iActualTickTemp);
		iTotalTimeTaken += iTempX;
		
		if ((iRespLen == 4) &&
			(szResponse[0] == 'P') &&
			(szResponse[1] == 'R') &&
			(szResponse[2] == 'S') &&
			(szResponse[3] == 'N'))
		{	
		
			// Increase good count
			iTotalGoodDelay += (iTempX);
			
			//  Update turnaround time
			if (iSecondaryTickTemp - iActualTickTemp < iTurnaroundTime) 
			{
				if (iSecondaryTickTemp - iActualTickTemp > 1) iTurnaroundTime = iSecondaryTickTemp - iActualTickTemp;
			}				
						
			// Increase our success counter			
			iSuccessCounter++;
		}
		else
		{
			// Report
			if ((bTimedOut == 3) || (bTimedOut == 77))
				iTotal3Error++;
			else if ((bTimedOut == 5))
				iTotal5Error++;
			else if ((bTimedOut == 50))		
			{		
				iTotal50Error++;
				szKeep[0] = GLOBAL_InterProcChars[0];
				szKeep[1] = GLOBAL_InterProcChars[1];
				szKeep[2] = GLOBAL_InterProcChars[2];
				szKeep[3] = GLOBAL_InterProcChars[3];
			}				
			else if ((bTimedOut == 51))
				iTotal50Error++;				
			else if (bTimedOut == 7)
				iTotal7Error++;
			else if (bTimedOut > 0)
				iTotalUTError++;
			else
				iTotalXError++;
			
		}
	}

	// Generate Report
	volatile char szTempex[128];
	strcpy(szReportResult,"");	
	/*for (int x = 0; x < 30; x++)
	{
		sprintf(szTempex,"%d,", iTransactionTimes[x]);
		strcat(szReportResult, szTempex);
		
	}*/	
	sprintf(szTempex, "\nTotal success: %d / %d\nTotal duration: %u us\nLeast transact: %u us, Total good delay: %u us\n"
			"3Err: %d, 4Err: %d, 5Err: %d, 50Err: %d, 51Err: %d, 7Err: %d, UTErr: %d, XErr: %d, KEEP: %02X%02X%02X%02X\n",
			iSuccessCounter, 100000, (unsigned int)iTotalTimeTaken, (unsigned int)iTurnaroundTime,(unsigned int)iTotalGoodDelay,
			iTotal3Error, iTotal4Error, iTotal5Error, iTotal50Error, iTotal51Error, iTotal7Error, iTotalUTError, iTotalXError,
			szKeep[0], szKeep[1], szKeep[2], szKeep[3]);


	/*sprintf(szTempex, "\nTotal success: %d / %d\nTotal duration: %u us\nLeast transact: %u us\n",
			iSuccessCounter, 100000, (unsigned int)iTotalTimeTaken, (unsigned int)iTurnaroundTime);
	*/
	strcat(szReportResult, szTempex);
	//strcat(szReportResult, szFailures);
	
	// Send the OK back first
	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szReportResult);  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		// We do nothing here...
	}	
	
	/*
	
	int iDigestValues[8] = {0,0,0,0,0,0,0,0};
		
	UL32 iTickVal = GetTickCount();
		
	SHA256_Digest(0x0FFEE2E3A, iDigestValues);
	
	UL32 iResultTick = GetTickCount() - iTickVal;
	
	char szReportResult[128];
	
	sprintf(szReportResult, "SHA2 Took %lu microseconds to complete\n", iResultTick);
	
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szReportResult);  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		// We do nothing here...
	}
	*/
	
	/*unsigned UL32 iActualTick = GetTickCount();
	volatile unsigned int iRetVals[8] = {0,0,0,0,0,0,0,0};
	SHA256_Digest("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiKK", 362, iRetVals);
	char szResponse[128];
	unsigned UL32 iPostTick = GetTickCount();
	unsigned int iDiff = (iPostTick - iActualTick);
	
	sprintf(szResponse, "HASH RESULT: %08X-%08X-%08X-%08X-%08X-%08X-%08X-%08X\nOperation took: %d us",
			iRetVals[0], iRetVals[1], iRetVals[2], iRetVals[3], iRetVals[4], iRetVals[5], iRetVals[6], iRetVals[7], iDiff);
	if (XLINK_ARE_WE_MASTER)
	{ 
		USB_send_string(szResponse);  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		// We do nothing here...
	}

	*/
	
	// Return our result...
	return res;
}

// Initiate process for the next job from the buffer
// And returns previous popped job result
PROTOCOL_RESULT Protocol_PIPE_BUF_PUSH()
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// We've received the ZX command, can we take this job now?
	if (!JobPipe__pipe_ok_to_push())
	{
		PipeKernel_Spin(); // Function is called before we return
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:QUEUE FULL\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:QUEUE FULL\n",
									sizeof("ERR:QUEUE FULL\n"),
									__XLINK_TRANSACTION_TIMEOUT__,
									&bXTimeoutDetected,
									FALSE);
		}			
		
		return PROTOCOL_FAILED;
	}
	
	// Are we at high-temperature, waiting to cool down?
	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:HIGH TEMPERATURE RECOVERY\n",
										sizeof("ERR:HIGH TEMPERATURE RECOVERY\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_FAILED;		
	}

	// We can take the job (either we start processing or we put it in the buffer)
	// Send data to ASIC and wait for response...
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");
	}		
	else
	{
		unsigned int bYTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK\n", sizeof("OK\n"), __XLINK_TRANSACTION_TIMEOUT__, &bYTimeoutDetected, FALSE);
	}
	
	// Wait for job data (96 Bytes)
	volatile char sz_buf_tag[1024];
	volatile char* sz_buf = sz_buf_tag;
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData = FALSE;

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
	{
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
	}		
	else
	{
		char bTimeoutDetectedX = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 1024, __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetectedX, FALSE, FALSE);
		if (bTimeoutDetectedX == TRUE) return PROTOCOL_FAILED;
		
		// Advance pointer, as XLINK will intercept the first character which is not used here
		sz_buf++;
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:TIMEOUT\n",
										sizeof("ERR:TIMEOUT\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_TIMEOUT;
	}

	// Check integrity
	if ((bInvalidData) || (i_read < sizeof(job_packet))) // Extra 16 bytes are preamble / postamble
	{
		sprintf(sz_buf, "ERR:INVALID DATA\n");
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact(sz_buf,
										strlen(sz_buf),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
				
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet p_job = (pjob_packet)(sz_buf); 
	
	// Check signature
	if (p_job->signature != 0xAA)
	{
		sprintf(sz_buf, "ERR:SIGNATURE\n");

		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact(sz_buf,
										 strlen(sz_buf),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}

		return PROTOCOL_INVALID_USB_DATA;		
	}
	
	// Job is ok, push it to stack
	JobPipe__pipe_push_job(p_job);
	
	// SIGNATURE CHECK
	// Respond with 'ERR:SIGNATURE' if not matched

	// Before we return, we must call this function to get buffer loop going...
	PipeKernel_Spin();

	// Job was pushed into buffer, let the host know
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK:QUEUED\n");  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK:QUEUED\n",
									sizeof("OK:QUEUED"),
									__XLINK_TRANSACTION_TIMEOUT__,
									&bXTimeoutDetected,
									FALSE);
	}

	// Increase the number of BUF-P2P jobs ever received
	__total_buf_pipe_jobs_ever_received++;

	// Return our result
	return res;
}


// Push a package of jobs, 5 to be exact
PROTOCOL_RESULT Protocol_PIPE_BUF_PUSH_PACK()
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// Our small buffer
	volatile char szBuffer64[64];
	volatile char iTotalAccepted = 0;
	
	// We've received the ZX command, can we take this job now?
	if (!JobPipe__pipe_ok_to_push())
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:QUEUE FULL\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:QUEUE FULL\n",
										sizeof("ERR:QUEUE FULL\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_FAILED;
	}
	
	// Are we at high-temperature, waiting to cool down?
	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB	
		}		
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:HIGH TEMPERATURE RECOVERY\n",
										sizeof("ERR:HIGH TEMPERATURE RECOVERY\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_FAILED;
	}

	// We can take the job (either we start processing or we put it in the buffer)
	// Send data to ASIC and wait for response...
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");
	}		
	else
	{
		unsigned int bYTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK\n", sizeof("OK\n"), __XLINK_TRANSACTION_TIMEOUT__, &bYTimeoutDetected, FALSE);
	}
	
	// Wait for job data (96 Bytes)
	volatile char sz_buf_tag[512];
	volatile char* sz_buf = sz_buf_tag;
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData = FALSE;
	unsigned char iDetectedPayloadSize = 0;
	
	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
	{
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
		iDetectedPayloadSize = (unsigned char)i_read;
	}		
	else
	{
		char bTimeoutDetectedX = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 512, __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetectedX, FALSE, FALSE);
		
		if (bTimeoutDetectedX == TRUE)
		{
			 return PROTOCOL_FAILED;
		}			 
		
		// Increment sz_buf since header byte does exist in XLINK and we don't need it
		iDetectedPayloadSize = sz_buf[0];
		i_read = i_read - 1; // Since XLINK does transfer the Payload Length to us, which is not considered as data in our case. Thus we have to ignore it.
		sz_buf = sz_buf + 1;
	}
	

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB	
		}		
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:TIMEOUT\n",
										sizeof("ERR:TIMEOUT\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_TIMEOUT;
	}
	
	// THIS IS THE STRUCTURE OF THE JOB PACK
	// --------------------------------------
	// unsigned __int8 payloadSize; // NOT SEEN IN OUR RESULT
	// unsigned __int8 signature;   == 0x0C1
	// unsigned __int8 jobsInArray;
	// job_packets[<jobsInArray>];
	// unsigned __int8 endOfWrapper == 0x0FE
	// --------------------------------------	
	
	// Received length should not be greater than 255. Check for it...
	if (i_read > 255)
	{
		// Some fault
		sprintf(sz_buf, "ERR:INVALID DATA\n");

		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact(sz_buf,
										 strlen(sz_buf),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}

		return PROTOCOL_INVALID_USB_DATA;
	}

	// Check integrity
	if ((bInvalidData) || (i_read < sizeof(job_packet) + 1 + 1 + 1 + 1)) // sizeof(job_packet) + payloadSize + signature + jobsInArray + endOfWrapper
	{
		sprintf(sz_buf, "ERR:INVALID DATA\n");
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB	
		}		
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact(sz_buf,
										strlen(sz_buf),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is OK, send data to ASIC for processing, and respond with OK
	volatile unsigned char stream__payloadSize  = iDetectedPayloadSize;
	volatile unsigned char stream__signature    = sz_buf[0];
	volatile unsigned char stream__jobsInArray  = sz_buf[1];
	volatile unsigned char stream__endOfWrapper = sz_buf[i_read - 1];
		
	// SIGNATURE CHECK
	// Respond with 'ERR:SIGNATURE' if not matched
	if ((stream__payloadSize  > 255+3)  ||
		(stream__signature    != 0x0C1) || // Initial Signature
		(stream__jobsInArray  > 5)		||
		(stream__jobsInArray  == 0)		|| // Terminating Signature
		(stream__endOfWrapper != 0x0FE))
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:SIGNATURE\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:SIGNATURE\n",
										 sizeof("ERR:SIGNATURE\n"),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}
	}
	
	// Now our famous JobPackets
	volatile pjob_packet pointer_to_jobs_0 = (pjob_packet)((void*)(sz_buf + 2 + (0*(sizeof(job_packet) + 1)) +1 ));
	volatile pjob_packet pointer_to_jobs_1 = (pjob_packet)((void*)(sz_buf + 2 + (1*(sizeof(job_packet) + 1)) +1 ));
	volatile pjob_packet pointer_to_jobs_2 = (pjob_packet)((void*)(sz_buf + 2 + (2*(sizeof(job_packet) + 1)) +1 ));
	volatile pjob_packet pointer_to_jobs_3 = (pjob_packet)((void*)(sz_buf + 2 + (3*(sizeof(job_packet) + 1)) +1 ));
	volatile pjob_packet pointer_to_jobs_4 = (pjob_packet)((void*)(sz_buf + 2 + (4*(sizeof(job_packet) + 1)) +1 )); // Last +1 is to skip the sigunature
	
	// Now check how many jobs we can take...
	volatile char total_space_available = JobPipe__available_space();
	volatile char total_jobs_to_take    = (total_space_available > stream__jobsInArray) ? stream__jobsInArray : total_space_available;
		
	// Push the job...
	if  (total_jobs_to_take >= 1) JobPipe__pipe_push_job((void*)(pointer_to_jobs_0));
	if ((total_jobs_to_take >= 2) && (stream__jobsInArray > 1)) JobPipe__pipe_push_job((void*)(pointer_to_jobs_1));
	if ((total_jobs_to_take >= 3) && (stream__jobsInArray > 2)) JobPipe__pipe_push_job((void*)(pointer_to_jobs_2));
	if ((total_jobs_to_take >= 4) && (stream__jobsInArray > 3)) JobPipe__pipe_push_job((void*)(pointer_to_jobs_3));
	if ((total_jobs_to_take == 5) && (stream__jobsInArray > 4)) JobPipe__pipe_push_job((void*)(pointer_to_jobs_4));
	
	// Job was pushed into buffer, let the host know
	// Generate the message
	sprintf(szBuffer64,"OK:QUEUED %d\n", total_jobs_to_take);
	
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szBuffer64);  // Send it to USB	
	}	
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(szBuffer64,
									strlen(szBuffer64),
									__XLINK_TRANSACTION_TIMEOUT__,
									&bXTimeoutDetected,
									FALSE);
	}

	// Increase the number of BUF-P2P jobs ever received
	__total_buf_pipe_jobs_ever_received++;
	
	// Set Mark1
	if (iMark1 == 0) iMark1 = MACRO_GetTickCountRet;

	// Return our result
	return res;
}

PROTOCOL_RESULT  Protocol_PIPE_BUF_FLUSH(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Reset the FIFO
	int iTotalJobsActuallyInBuffer = __total_jobs_in_buffer;
	JobPipe__pipe_flush_buffer();
	
	// Send OK first
	char szFlshString[32];
	sprintf(szFlshString, "OK:FLUSHED %02d\n", iTotalJobsActuallyInBuffer);
	
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szFlshString);  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(szFlshString, strlen(szFlshString),
									__XLINK_TRANSACTION_TIMEOUT__,
									&bXTimeoutDetected,
									FALSE);
	}		

	// Return our result
	return res;
}

// Returns only the status of the last processed job
// from the buffer, and will not initiate the next job process
PROTOCOL_RESULT	Protocol_PIPE_BUF_STATUS(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// We can take the job (either we start processing or we put it in the buffer)
	char sz_rep[3500];
	char sz_temp[256];
	char sz_temp2[256];
	char sz_temp3[32];
	unsigned int istream_len;
	char i_cnt;

	// Should we do an engine start at the end of the function?
	char i_engine_start_req = 0;

	// How many jobs can we take in?
	strcpy(sz_rep,"");
	/*sprintf(sz_temp, "STORAGE:%d\n", PIPE_MAX_BUFFER_DEPTH-__total_jobs_in_buffer);
	strcat(sz_rep, sz_temp);*/

	// What is the actual
	// Are we processing something?
	unsigned int  found_nonce_list[8];
	char found_nonce_count;

	// How many results?
	sprintf(sz_temp,"INPROCESS:%d\n", (PipeKernel_WasPreviousJobFromPipe() == TRUE) ? 1 : 0);
	strcat(sz_rep, sz_temp);
		
	sprintf(sz_temp,"COUNT:%d\n", JobPipe__pipe_get_buf_job_results_count());
	strcat(sz_rep, sz_temp);

	// How many jobs to take? The smallest between 15 and "total results in buffer"
	char iTotalResultsToTake = 0;	
	
	// Ok, return the last result as well
	if (JobPipe__pipe_get_buf_job_results_count() == 0)
	{
		// We simply do nothing... Just add the <LF> to the end of the line
	}
	else
	{
		if (JobPipe__pipe_get_buf_job_results_count() <= MAX_RESULTS_TO_SEND_AT_A_TIME_FROM_BUFFER)
		{
			iTotalResultsToTake = JobPipe__pipe_get_buf_job_results_count();
		}
		else
		{
			iTotalResultsToTake = MAX_RESULTS_TO_SEND_AT_A_TIME_FROM_BUFFER;
		}
		
		
		// Now post them one by one
		for (i_cnt = 0; i_cnt < iTotalResultsToTake; i_cnt++)
		{
			// Get our job
			pbuf_job_result_packet pjob_res = (pbuf_job_result_packet)(JobPipe__pipe_get_buf_job_result(i_cnt));
			
			// Also say which midstate and nonce-range is in process
			stream_to_hex(pjob_res->midstate , sz_temp2, 32, &istream_len);
			sz_temp2[istream_len] = 0;
			stream_to_hex(pjob_res->block_data, sz_temp3, 12, &istream_len);
			sz_temp3[istream_len] = 0;		
			
			// Add nonce-count...
			volatile unsigned char iNonceCount = pjob_res->i_nonce_count;
			
			#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
				//sprintf(sz_temp,"%s,%s,%X,%d", sz_temp2, sz_temp3, pjob_res->iProcessingChip, iNonceCount); // Add midstate, block-data, count and nonces...
				//strcat(sz_rep, sz_temp);
				sprintf(sz_temp,"%s,%s,%d", sz_temp2, sz_temp3, iNonceCount); // Add midstate, block-data, count and nonces...
				strcat(sz_rep, sz_temp);				
			#else
				sprintf(sz_temp,"%s,%s,%d", sz_temp2, sz_temp3, iNonceCount); // Add midstate, block-data, count and nonces...
				strcat(sz_rep, sz_temp);			
			#endif
			
			// If there are nonces, add a comma here
			if (iNonceCount > 0)
			{
				 strcat(sz_rep,",");
			}				 
					
			// Nonces found...
			if ( pjob_res->i_nonce_count > 0)
			{
				// Our loop counter
				unsigned char ix = 0;
				
				for (ix = 0; ix < pjob_res->i_nonce_count; ix++)
				{
					sprintf(sz_temp2, "%08X", pjob_res->nonce_list[ix]);
					strcat(sz_rep, sz_temp2);

					// Add a comma if it's not the last nonce
					if (ix != pjob_res->i_nonce_count - 1) strcat(sz_rep,",");
				}

				// Add the 'Enter' character
				strcat(sz_temp, "\n");
			}

			// In the end, add the <LF>
			strcat(sz_rep,"\n");
		}
	}

	// Add the OK to our response (finilizer)
	strcat(sz_rep, "OK\n");
	
	volatile int iStrLen = 0;
	iStrLen = strlen(sz_rep);
	

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(sz_rep);  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(sz_rep,
									 strlen(sz_rep),
									 __XLINK_TRANSACTION_TIMEOUT__,
									 &bXTimeoutDetected,
									 FALSE);
	}		

	// Also, we clear the results buffer
	JobPipe__pipe_skip_buf_job_results(iTotalResultsToTake);
	
	// Also Flush jobs into engines
	PipeKernel_Spin();
	
	// Return the result...
	return res;
}

PROTOCOL_RESULT Protocol_handle_job(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Send data to ASIC and wait for response...
	// BTW, our actual timeout is 16 seconds... (1GH/s)
	// Also, we do expect a packet which is 64 Bytes data, 16 Bytes Midstate, 16 Bytes difficulty (totaling 96 Bytes)
	
	// Are we at high-temperature, waiting to cool down?
	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB	
		}		
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:HIGH TEMPERATURE RECOVERY\n",
										 sizeof("ERR:HIGH TEMPERATURE RECOVERY\n"),
										 __XLINK_TRANSACTION_TIMEOUT__,
										 &bXTimeoutDetected,
										 FALSE);
		}
			
		return PROTOCOL_FAILED;
	}


	// Send OK first
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		XLINK_SLAVE_respond_string("OK\n");	
	}		

	// Wait for job data (96 Bytes)
	volatile char sz_buf_tag[1024];
	volatile char* sz_buf = sz_buf_tag;
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData = FALSE;

	if (XLINK_ARE_WE_MASTER)
	{
	    USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
	}		
	else
	{
		// Wait for incoming transactions
		char bTimeoutDetectedOnXLINK = 0;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 100, &bTimeoutDetectedOnXLINK, FALSE, FALSE);
					
		// Check
		if (bTimeoutDetectedOnXLINK) return PROTOCOL_FAILED;	
		
		// Since XLINK will save the first character received, we have to advance the pointer
		sz_buf++;
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:TIMEOUT\n");	
		}
				
		return PROTOCOL_TIMEOUT;
	}

	// Check integrity
	if ((bInvalidData) || (i_read < sizeof(job_packet))) // Extra 16 bytes are preamble / postamble 
														   // We write i_read because the byte-header is trimmed out when length is returned by the function
	{
		sprintf(sz_buf, "ERR:INVALID DATA\n");
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string(sz_buf);
		}
								
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet p_job = (pjob_packet)(sz_buf);
	
	// Check signature
	if (p_job->signature != 0xAA)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:SIGNATURE<\n");  // Send it to USB
		}		
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:SIGNATURE<\n");	
		}
		return PROTOCOL_FAILED;
	}

	// We have all what we need, send it to ASIC-COMM interface
	ASIC_job_issue(p_job, 0, 0x0FFFFFFFF, FALSE, 0);

	// Send our OK
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		XLINK_SLAVE_respond_string("OK\n");	
	}
	
	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_handle_job_single_stage(char* szStream)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Send data to ASIC and wait for response...
	// BTW, our actual timeout is 16 seconds... (1GH/s)
	// Also, we do expect a packet which is 64 Bytes data, 16 Bytes Midstate, 16 Bytes difficulty (totaling 96 Bytes)
	
	// Are we at high-temperature, waiting to cool down?
	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:HIGH TEMPERATURE RECOVERY\n",
										sizeof("ERR:HIGH TEMPERATURE RECOVERY\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_FAILED;
	}

	// Since XLINK will save the first character received, we have to advance the pointer
	char* sz_buf = (szStream + 3); // We should have the job here...
	
	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet p_job = (pjob_packet)(sz_buf);
	
	// Check signature
	if (p_job->signature != 0xAA)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:SIGNATURE<\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:SIGNATURE<\n");
		}
		return PROTOCOL_FAILED;
	}

	// We have all what we need, send it to ASIC-COMM interface
	ASIC_job_issue(p_job, 0, 0x0FFFFFFFF, FALSE, 0);

	// Send our OK
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		XLINK_SLAVE_respond_string("OK\n");
	}
	
	// Return our result...
	return res;
}



PROTOCOL_RESULT Protocol_handle_job_p2p(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Send data to ASIC and wait for response...
	// BTW, our actual timeout is 16 seconds... (1GH/s)
	// Also, we do expect a packet which is 64 Bytes data, 16 Bytes Midstate, 16 Bytes difficulty (totaling 96 Bytes)

	// Are we at high-temperature, waiting to cool down?
	if (GLOBAL_CRITICAL_TEMPERATURE == TRUE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:HIGH TEMPERATURE RECOVERY\n",
										sizeof("ERR:HIGH TEMPERATURE RECOVERY\n"),
										__XLINK_TRANSACTION_TIMEOUT__,
										&bXTimeoutDetected,
										FALSE);
		}
		
		return PROTOCOL_FAILED;
	}

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");
	else
		XLINK_SLAVE_respond_string("OK\n");

	// Wait for job data (96 Bytes)
	volatile char sz_buf_tag[1024];
	volatile char* sz_buf = sz_buf_tag;
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData = FALSE;
	
	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE, FALSE);
		if (bTimeoutDetected) return PROTOCOL_FAILED;
		
		// Since XLINK will save the first character received, we have to advance the pointer
		sz_buf++;		
	}		

	// Timeout?
	if (i_timeout < 2)
	{		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:TIMEOUT\n");
		}				
		return PROTOCOL_TIMEOUT;
	}

	// Check integrity
	if ((bInvalidData) || (i_read < sizeof(job_packet_p2p))) // Extra 16 bytes are preamble / postamble
	{
		strcpy(sz_buf, "ERR:INVALID DATA\n");
		
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_buf);  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string(sz_buf);
		}			
		
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet_p2p p_job = (pjob_packet_p2p)(sz_buf); // 4 Bytes are for the initial '>>>>'

	// Check signature
	if (p_job->signature != 0xAA)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:SIGNATURE<\n");  // Send it to USB
		}
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:SIGNATURE<\n");
		}
		return PROTOCOL_FAILED;
	}
	
	// We have all what we need, send it to ASIC-COMM interface
	ASIC_job_issue(p_job, 
				   p_job->nonce_begin, 
				   p_job->nonce_end,
				   FALSE, 0);				   
				   
	// Send our OK	
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string("OK\n");  // Send it to USB
	}		
	else // We're a slave... send it by XLINK
	{
		XLINK_SLAVE_respond_string("OK\n");
	}		

	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_get_status()
{
	// Here, we must return our chip status
	volatile unsigned int nonce_list[16];
	volatile char i;
	volatile int nonce_count = 0;
	volatile char stat;

	volatile char sz_report[1024];
	volatile char sz_tmp[48];

	stat = ASIC_get_job_status(nonce_list, &nonce_count, FALSE, 0);
	
	// What is our status?
	if (stat == ASIC_JOB_IDLE)
	{
		if (XLINK_ARE_WE_MASTER)
		{
			 USB_send_string("IDLE\n");  // Send it to USB
		}			 
		else // We're a slave... send it by XLINK
		{
			 XLINK_SLAVE_respond_string("IDLE\n");
		}
	}		
	else if (stat == ASIC_JOB_NONCE_FOUND)
	{
		// Job-Finish timestamp
		GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet;
				
		// Report the found NONCEs
		strcpy(sz_report, "NONCE-FOUND:");

		for (i = 0; i < nonce_count; i++)
		{
			sprintf(sz_tmp, "%08X", nonce_list[i]);
			strcat(sz_report, sz_tmp);
			if (i < nonce_count -1) strcat(sz_report,",");
		}

		// Add the 'Carriage return'
		strcat(sz_report, "\n");

		// Send it to host
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string(sz_report);  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{		
			XLINK_SLAVE_respond_string(sz_report);
		}
	}
	else if (stat == ASIC_JOB_NONCE_NO_NONCE)
	{
		// Job-Finish timestamp
		GLOBAL_LastJobResultProduced = MACRO_GetTickCountRet;
		
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("NO-NONCE\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{		
			XLINK_SLAVE_respond_string("NO-NONCE\n");
		}			
	}		
	else if (stat == ASIC_JOB_NONCE_PROCESSING)
	{
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("BUSY\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("BUSY\n");
		}			
	}		
	else
	{
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
		{
			USB_send_string("ERR:UKNOWN\n");  // Send it to USB
		}			
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:UKNOWN\n");
		}			
	}

	// Return our result...
	return PROTOCOL_SUCCESS;
}

PROTOCOL_RESULT Protocol_temperature()
{
	 // Our result
	 PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	 // Get temperature
	 volatile int temp_val1 = a2d_get_temp(0);
	 volatile int temp_val2 = a2d_get_temp(1);

	 // All is good... sent the identifier and get out of here...
	 char sz_resp[128];
	 sprintf(sz_resp,"Temp1: %d, Temp2: %d\n", temp_val1, temp_val2); // .1f means 1 digit after decimal point, format=floating
	 
	 if (XLINK_ARE_WE_MASTER)
		 USB_send_string(sz_resp);  // Send it to USB
	 else // We're a slave... send it by XLINK
	 	 XLINK_SLAVE_respond_string(sz_resp);
	 
	 // Return our result...
	 return res;
}

PROTOCOL_RESULT Protocol_get_voltages()
{
	 // Our result
	 PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	 // Get temperature
	 volatile int temp_val1 = a2d_get_voltage(A2D_CHANNEL_3P3V) * 2; // x2 because the 3.3 is received via a divide-by-2 ladder
	 volatile int temp_val2 = a2d_get_voltage(A2D_CHANNEL_1V);
	 volatile int temp_val3 = a2d_get_voltage(A2D_CHANNEL_PWR_MAIN) * 10; // x10 because the PWR_MAIN is received via a divide-by-10 ladder

	 // All is good... sent the identifier and get out of here...
	 char sz_resp[128];
	 sprintf(sz_resp,"%d,%d,%d\n", temp_val1, temp_val2, temp_val3); // .1f means 1 digit after decimal point, format=floating
	 
	 if (XLINK_ARE_WE_MASTER)
		 USB_send_string(sz_resp);  // Send it to USB
	 else // We're a slave... send it by XLINK
	 	 XLINK_SLAVE_respond_string(sz_resp);
	 
	 // Return our result...
	 return res;
}

PROTOCOL_RESULT Protocol_get_firmware_version()
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string(__FIRMWARE_VERSION);  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string(__FIRMWARE_VERSION);

	// Return our result
	return res;
}

// This sets our ASICs frequency
PROTOCOL_RESULT  Protocol_get_freq_factor()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;

	// Get the frequency factor from engines...
	char szResp[128];
	sprintf(szResp,"FREQ:%d\n", ASIC_GetFrequencyFactor());
	
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string(szResp);  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string(szResp);	
		
	// We have our frequency factor sent, exit 
	return result;	
}

// This sets our ASICs frequency
PROTOCOL_RESULT  Protocol_set_freq_factor()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");
		
	// Wait for 4bytes of frequency factor
	char sz_buf[1024];
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData  = FALSE;

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
	{
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
	}		
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE, FALSE);
		if (bTimeoutDetected) return PROTOCOL_FAILED;
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERR:TIMEOUT\n");
			
		return PROTOCOL_TIMEOUT;
	}
	
	// If i_read is not 4, we've got something wrong...
	if ((bInvalidData) || (i_read != 4))
	{
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:INVALID DATA\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERR:INVALID DATA\n");
		
		return PROTOCOL_FAILED;		
	}		
	
	// Get the Frequency word 
	volatile int iFreqFactor = (sz_buf[0]) | (sz_buf[1] << 8) | (sz_buf[2] << 16) | (sz_buf[3] << 24);
	iFreqFactor &= 0x0FFFF; // Only 16 bits counts
		
	// Do whatever we want with this iFreqFactor
	ASIC_SetFrequencyFactor(0xFF, iFreqFactor); // 0xFF for chip address means ALL CHIPS!
	
	// Say we're ok
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");	
	
	// We have our frequency factor sent, exit
	return result;
}

// Our XLINK Support...
PROTOCOL_RESULT  Protocol_set_xlink_address()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;

	// Do we have CPLD installed at all?
	if (XLINK_is_cpld_present() == FALSE)
	{
		// Send OK first
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERROR: XLINK NOT PRESENT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERROR: XLINK NOT PRESENT\n");
	}
	
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");
		
	// Wait for 4bytes of frequency factor
	char sz_buf[1024];
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;
	unsigned char bInvalidData = FALSE;
	
	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout, &bInvalidData);
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE, FALSE);
		if (bTimeoutDetected) return PROTOCOL_FAILED;
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
		USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("ERR:TIMEOUT\n");
	
		return PROTOCOL_TIMEOUT;
	}

	// If i_read is not 4, we've got something wrong...
	if ((bInvalidData) || (i_read != 4))
	{
		if (XLINK_ARE_WE_MASTER)
		USB_send_string("ERR:INVALID DATA\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("ERR:INVALID DATA\n");
	
		return PROTOCOL_FAILED;
	}
	
	// Say we're ok
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");	
		
	// Set the CPLD address (we'll do it after the transaction is over)
	XLINK_set_cpld_id(sz_buf[0]);		
		
	// We have our frequency factor sent, exit
	return result;
	
}

PROTOCOL_RESULT Protocol_xlink_presence_detection()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;

	// Do we have CPLD installed at all?
	if (XLINK_is_cpld_present() == FALSE)
	{
		// Nothing to send
		return PROTOCOL_SUCCESS;
	}
	
	// Get our ID
	volatile unsigned char iCPLDId = XLINK_get_cpld_id();
	
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("PRSN");  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		// XLINK_SLAVE_respond_string("OK\n");
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_respond_transact("PRSN", 4, __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetected, FALSE);
	}			

	// We're ok
	return PROTOCOL_SUCCESS;
}

PROTOCOL_RESULT  Protocol_xlink_allow_pass()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;

	// Do we have CPLD installed at all?
	if (XLINK_is_cpld_present() == FALSE)
	{
		// Send OK first
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERROR: XLINK NOT PRESENT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERROR: XLINK NOT PRESENT\n");		
	}
	
	// Allow pass through
	XLINK_set_cpld_passthrough(TRUE);

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");
	
	// We have our frequency factor sent, exit
	return result;	
}

PROTOCOL_RESULT  Protocol_xlink_deny_pass()
{
	// Our result
	PROTOCOL_RESULT result = PROTOCOL_SUCCESS;
	
	// Do we have CPLD installed at all?
	if (XLINK_is_cpld_present() == FALSE)
	{
		// Send OK first
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERROR: XLINK NOT PRESENT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERROR: XLINK NOT PRESENT\n");
	}
	
	// Allow passthrough
	XLINK_set_cpld_passthrough(FALSE);
	
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");

	// We have our frequency factor sent, exit
	return result;	
}


/////////////////////////////////////////
// AUXILIARY FUNCTIONS
/////////////////////////////////////////
void init_mcu_led()
{
	MCU_LED_Initialize();
}

void blink_slow()
{
	MCU_MainLED_Set();
	Delay_2();
	MCU_MainLED_Reset();
	Delay_2();
}

void blink_medium()
{
	MCU_MainLED_Set();
	Delay_3();
	MCU_MainLED_Reset();
	Delay_3();
}

void blink_fast()
{
	MCU_MainLED_Set();
	Delay_1();
	MCU_MainLED_Reset();
	Delay_1();
	MCU_MainLED_Set();
}

int Delay_1()
{
	volatile int x;
	volatile int z;
	for (x = 0; x < 950; x++) z++;
	return z;
}

int Delay_2()
{
	volatile int x;
	volatile int z;
	for (x = 0; x < 12800; x++) z++;
	return z;
}

int Delay_3()
{
	volatile int x;
	volatile int z;
	for (x = 0; x < 162800; x++) z++;
	return z;
}

void clear_buffer(char* sz_stream, unsigned int ilen)
{
	unsigned int u = 0;
	for (u = 0; u < ilen; u++) sz_stream[u] = 0;
}

void stream_to_hex(char* sz_stream, char* sz_hex, unsigned int i_stream_len, unsigned int  *i_hex_len)
{
	// OK, convert sz_stream
	char sz_temp[2];
	unsigned int index;
	sz_hex[0] = 0x0; // Clear the hex string

	for (index = 0; index < i_stream_len; index++)
	{
		sprintf(sz_temp, "%02X", sz_stream[index]);
		strcat(sz_hex, sz_temp);
	}
	
	*i_hex_len = i_stream_len * 2;

	// We're done...
}
