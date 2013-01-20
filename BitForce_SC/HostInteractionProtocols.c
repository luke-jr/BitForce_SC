/*
 * OperationProtocols.c
 *
 * Created: 06/01/2013 17:25:01
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
#include <avr32/io.h>
#include "HostInteractionProtocols.h"
#include "AVR32X/AVR32_Module.h"
#include "AVR32_OptimizedTemplates.h"

#include <string.h>
#include <stdio.h>

// Information about the result we're holding
extern buf_job_result_packet  __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char 		   __buf_job_results_count;  // Total of results in our __buf_job_results

PROTOCOL_RESULT Protocol_chain_forward(char iTarget, char* sz_cmd, unsigned short iCmdLen)
{
	// OK We've detected a ChainForward request. First Send 'OK' to the host
	char szRespData[2048];
	unsigned short iRespLen = 0;
	char  bDeviceNotResponded = 0;
	char  bTimeoutDetected = 0;
	
	for (unsigned int umx = 0; umx < 2048; umx++) szRespData[umx] = 0;
	
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
		sprintf(szRespData,"ERR:TIMEOUT %d\n", bTimeoutDetected);
		USB_send_string(szRespData);
		return PROTOCOL_FAILED;
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



int iAssemblyTestFunction(int a, int b, int c, int d, int e);
int iAssemblyTestFunction(int a, int b, int c, int d, int e)
{
	register int retVal asm("r0");
	
	asm volatile
	(
		"mov r0, r11""\n"
	);
	
	return retVal;
}

PROTOCOL_RESULT Protocol_info_request(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;
	
	// What is our information request?
	char szInfoReq[1024];
	char szTemp[256];
	
	strcpy(szInfoReq,"DEVICE: BitFORCE SC\n");
	strcpy(szTemp,"");
	
	// Add Firmware Identification
	sprintf(szTemp,"FIRMWARE: %s\n", __FIRMWARE_VERSION);
	strcat(szInfoReq, szTemp);
	
	long uL1;
	long uL2;
	long uLRes;

	// Atomic func latency
	uL1 = MACRO_GetTickCountRet;
	MACRO_XLINK_send_packet(0, "ABCD", 4, 1, 0);
	uL2 = MACRO_GetTickCountRet;
	uLRes = uL2 - uL1;
	sprintf(szTemp,"ATOMIC FUNC LATENCY: %d us\n", uLRes);
	strcat(szInfoReq, szTemp);
	
	/*
	// Atomic IO Macro latency
	uL1 = MACRO_GetTickCount;
	MACRO_XLINK_send_packet(0, "ABCD", 4, 1, 0);
	uL2 = MACRO_GetTickCount;
	uLRes = (uL2 - uL1);
	sprintf(szTemp,"ATOMIC IO MACRO LATENCY: %d us\n", uLRes);
	strcat(szInfoReq, szTemp);
	
	// Atomic MARCO latency
	uL1 = MACRO_GetTickCount;
	uL2 = MACRO_GetTickCount;
	uLRes = uL2 - uL1;
	sprintf(szTemp,"ATOMIC FUNC MACRO LATENCY: %d us\n", uLRes);
	strcat(szInfoReq, szTemp);*/
	
	// Add Engine count
	sprintf(szTemp,"ASM Test Res: %d\n", iAssemblyTestFunction(10,20,30,40,50));
	strcat(szInfoReq, szTemp);
		
	// Add Engine count
	sprintf(szTemp,"ENGINES: %d\n", ASIC_get_chip_count());
	strcat(szInfoReq, szTemp);
	
	// Add Chain Status
	sprintf(szTemp,"XLINK MODE: %s\n", XLINK_ARE_WE_MASTER ? "MASTER" : "SLAVE" );
	strcat(szInfoReq, szTemp);
	
	// Add XLINK chip installed status
	sprintf(szTemp,"XLINK PRESENT: %s\n", (XLINK_is_cpld_present() == TRUE) ? "YES" : "NO");
	strcat(szInfoReq, szTemp);
	
	// If we are master, how many devices exist in chain?
	if ((XLINK_ARE_WE_MASTER == TRUE) && (XLINK_is_cpld_present() == TRUE))
	{
		sprintf(szTemp,"DEVICES IN CHAIN: %d\n", XLINK_chain_device_count);
		strcat (szInfoReq, szTemp);
	}
	
	// Add the terminator
	strcat(szInfoReq, "OK\n");

	// All is good... sent the identifier and get out of here...
	if (XLINK_ARE_WE_MASTER)
	USB_send_string(szInfoReq);  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		char bTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(szInfoReq, strlen(szInfoReq), __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetected, FALSE);
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
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	blink_medium();
	
	// Set the LED
	MCU_MainLED_Set();

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
	long iTurnaroundTime = 0;
	
	// First get our tick counter
	long iActualTick = GetTickCount();
	
	// Total timeout counters
	int iTotalTimeoutCounter = 0;
	int iTransactionTimes[30];

	char szResponseFromDev[395];
	int  iResponseLenFromDev = 0;
	char bDevNotResponded = FALSE;
	char bTimedOut = FALSE;
	
	///////////////////////////////////////////////////////////
	// CPLD TEST Write different values to CPLD and read back
	//////////////////////////////////////////////////////////
	
	long iActualTickTick = GetTickCount();
	
	int iTotalWrongReads = 0;
	unsigned int umi = 0;
	char burst_write[4] = {0,0,0,0};
	char burst_read[4]  = {0,0,0,0};
	
	/*for (unsigned int umm = 0; umm < 1000; umm++)
	{
		burst_write[0] = umm & 0x0FF;
		burst_write[1] = (umm+1) & 0x0FF;
		burst_write[2] = (umm+2) & 0x0FF;
		burst_write[3] = (umm+3) & 0x0FF;
		__AVR32_CPLD_BurstTxWrite(burst_write, CPLD_ADDRESS_TX_BUF_BEGIN);
		if (MCU_CPLD_Read(31) != iVal) iTotalWrongReads++;	
		
		// TO BE COMPLETED, We need to test burst-read and burst-write
																																		
	}
	
	sprintf(szReportResult, "Total wrong results: %d, operation took %d us\n", iTotalWrongReads, GetTickCount() - iActualTickTick);
	
	if (XLINK_ARE_WE_MASTER)
	{
		USB_send_string(szReportResult);  // Send it to USB
	}
	else // We're a slave... send it by XLINK
	{
		// We do nothing here...
	}
	
	
	//////////////////////
	/// WE RETURN HERE...
	//////////////////////
	return res; 
	*/
	
	// Now we send message to XLINK_GENERAL_DISPATCH_ADDRESS for an ECHO and count the successful iterations
	for (unsigned int x = 0; x < 100000; x++)
	{
		// Clear input
		MACRO_XLINK_clear_RX;

		
		// Now we wait for response
		char bTimedOut = FALSE;
		char szResponse[10] = {0,0,0,0,0,0,0,0,0,0};
		unsigned int iRespLen = 0;
		char iBC = 0;
		char iLP = 0;
		char iSendersAddress = 0;
		
		szResponse[0] = 0; szResponse[1] = 0; szResponse[2] = 0; szResponse[3] = 0;
		iRespLen = 0;	
		
		// Calculate single turn-around time
		long iActualTickTemp = MACRO_GetTickCountRet;
		long iSecondaryTickTemp = 0;
		
		// Send the command
		MACRO_XLINK_send_packet(1, "ZAX", 3, TRUE, FALSE);
		MACRO_XLINK_wait_packet(szResponse, iRespLen, 90, bTimedOut, iSendersAddress, iLP, iBC );
	
		// Update turnaround time
		iSecondaryTickTemp = MACRO_GetTickCountRet;

		
		if ((iRespLen == 4) &&
		(szResponse[0] == 'E') &&
		(szResponse[1] == 'C') &&
		(szResponse[2] == 'H') &&
		(szResponse[3] == 'O'))
		{

			
			if (x == 0) iTurnaroundTime = (iSecondaryTickTemp - iActualTickTemp);
			
			//  Update turnaround time
			iTurnaroundTime = (iTurnaroundTime + (iSecondaryTickTemp - iActualTickTemp)) / 2;
			
			// Increase our success counter			
			iSuccessCounter++;
		}

	}

	// Total time taken
	long iTotalTimeTaken = GetTickCount() - iActualTick;

	// Generate Report
	char szTempex[128];
	strcpy(szReportResult,"");
	
	/*for (int x = 0; x < 30; x++)
	{
		sprintf(szTempex,"%d,", iTransactionTimes[x]);
		strcat(szReportResult, szTempex);
		
	}*/
	
	sprintf(szTempex, "\nTotal success: %d / %d\nTotal duration: %d us\nAverage transact: %d us\n",
			iSuccessCounter, 100000, iTotalTimeTaken, iTurnaroundTime );

	strcat(szReportResult, szTempex);
	

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
		
	long iTickVal = GetTickCount();
		
	SHA256_Digest(0x0FFEE2E3A, iDigestValues);
	
	long iResultTick = GetTickCount() - iTickVal;
	
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
	
	/*unsigned long iActualTick = GetTickCount();
	volatile unsigned int iRetVals[8] = {0,0,0,0,0,0,0,0};
	SHA256_Digest("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiKK", 362, iRetVals);
	char szResponse[128];
	unsigned long iPostTick = GetTickCount();
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
PROTOCOL_RESULT Protocol_P2P_BUF_PUSH()
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// We've received the ZX command, can we take this job now?
	if (!__pipe_ok_to_push())
	{
		Flush_p2p_buffer_into_engines(); // Function is called before we return
		
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:BUFFER FULL\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		{
			unsigned int bXTimeoutDetected = 0;
			XLINK_SLAVE_respond_transact("ERR:BUFFER FULL\n",
									sizeof("ERR:BUFFER FULL\n"),
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
		USB_send_string("OK\n");
	else
	{
		unsigned int bYTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK\n", sizeof("OK\n"), __XLINK_TRANSACTION_TIMEOUT__, &bYTimeoutDetected, FALSE);
	}
	
	// Wait for job data (96 Bytes)
	char sz_buf[1024];
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout);
	else
	{
		char bTimeoutDetectedX = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 1024, 300, &bTimeoutDetectedX, FALSE);
		if (bTimeoutDetectedX == TRUE) return PROTOCOL_FAILED;
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
	if (i_read < sizeof(job_packet_p2p)) // Extra 16 bytes are preamble / postamble
	{
		sprintf(sz_buf, "ERR:INVALID DATA, i_read = %d\n", i_read);
		
		if (XLINK_ARE_WE_MASTER)
			USB_send_string(sz_buf);  // Send it to USB
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
	pjob_packet_p2p p_job = (pjob_packet_p2p)(sz_buf); // 8 Bytes are for the initial '>>>>>>>>'
	__pipe_push_P2P_job(p_job);

	// Before we return, we must call this function to get buffer loop going...
	Flush_p2p_buffer_into_engines();

	// Job was pushed into buffer, let the host know
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK:BUFFERED\n");  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK:BUFFERED\n",
									sizeof("OK:BUFFERED\n"),
									__XLINK_TRANSACTION_TIMEOUT__,
									&bXTimeoutDetected,
									FALSE);
	}

	// Increase the number of BUF-P2P jobs ever received
	__total_buf_p2p_jobs_ever_received++;

	// Return our result
	return res;
}

PROTOCOL_RESULT  Protocol_P2P_BUF_FLUSH(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Reset the FIFO
	__pipe_flush_buffer();

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact("OK\n",
				sizeof("OK\n"),
				__XLINK_TRANSACTION_TIMEOUT__,
				&bXTimeoutDetected,
				FALSE);
	}		

	// Return our result
	return res;
}

// Returns only the status of the last processed job
// from the buffer, and will not initiate the next job process
PROTOCOL_RESULT	Protocol_P2P_BUF_STATUS(void)
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// We can take the job (either we start processing or we put it in the buffer)
	char sz_rep[2048];
	char sz_temp[128];
	char sz_temp2[128];
	unsigned int istream_len;
	char i_cnt;

	// Should we do an engine start at the end of the function?
	char i_engine_start_req = 0;

	// How many jobs can we take in?
	strcpy(sz_rep,"");
	sprintf(sz_temp, "STORAGE:%d\n", PIPE_MAX_BUFFER_DEPTH-__total_jobs_in_buffer);
	strcat(sz_rep, sz_temp);

	// What is the actual
	// Are we processing something?
	unsigned int  found_nonce_list[16];
	char found_nonce_count;

	if (ASIC_get_job_status(found_nonce_list, &found_nonce_count) == ASIC_JOB_NONCE_PROCESSING)
	{
		// We are processing something...
		strcat(sz_rep, "ENGINE STATUS:PROCESSING\n");

		// Also say which midstate and nonce-range is in process
		stream_to_hex(__inprocess_midstate, sz_temp2, 32, &istream_len);
		sz_temp2[(istream_len * 2)] = 0;
		sprintf(sz_temp, "-MIDSTATE:%s\n", sz_temp2);
		strcat(sz_rep, sz_temp);

		// Nonce-Begin
		stream_to_hex(__inprocess_nonce_begin, sz_temp2, 4, &istream_len);
		sz_temp2[(istream_len * 2)] = 0;
		sprintf(sz_temp, "-BEGIN:%s\n", sz_temp2);
		strcat(sz_rep, sz_temp);

		// Nonce-End
		stream_to_hex(__inprocess_nonce_end, sz_temp2, 4, &istream_len);
		sz_temp2[(istream_len * 2)] = 0;
		sprintf(sz_temp, "-END:%s\n",sz_temp2);
		strcat(sz_rep, sz_temp);
	}
	else
	{
		// Engine is not active (i.e. not processing)
		strcat(sz_rep, "ENGINE STATUS:IDLE\n");

		// Enable an engine start if we have jobs in the queue
		if (__pipe_ok_to_pop()) // It means we have some job to do...
		{
			// Engine start requested
			i_engine_start_req = 1;
		}
	}

	// Ok, return the last result as well
	if (TRUE)
	{
		if (__pipe_get_buf_job_results_count() == 0)
		{
			strcat(sz_rep, "RECENT:NONE\n");
		}
		else
		{
			// Give out the count:
			sprintf(sz_temp, "RECENT:%d\n", __pipe_get_buf_job_results_count());

			// Now post them one by one
			for (i_cnt = 0; i_cnt < __pipe_get_buf_job_results_count(); i_cnt++)
			{
				// Also say which midstate and nonce-range is in process
				stream_to_hex( ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->midstate , sz_temp2, 32, &istream_len);
				sz_temp2[(istream_len * 2)] = 0;
				sprintf(sz_temp, "MIDSTATE:%s\n", sz_temp2);
				strcat(sz_rep, sz_temp);

				// Nonce-Begin
				stream_to_hex( ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->nonce_begin, sz_temp2, 4, &istream_len);
				sz_temp2[(istream_len * 2)] = 0;
				sprintf(sz_temp, "-BEGIN:%s\n", sz_temp2);
				strcat(sz_rep, sz_temp);

				// Nonce-End
				stream_to_hex( ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->nonce_end, sz_temp2, 4, &istream_len);
				sz_temp2[(istream_len * 2)] = 0;
				sprintf(sz_temp, "-END:%s\n",sz_temp2);
				strcat(sz_rep, sz_temp);

				// Nonces found...
				if ( ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->i_nonce_count > 0)
				{
					char ix;
					sprintf(sz_temp,"-NONCE:");

					for (ix = 0; ix < ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->i_nonce_count; ix++)
					{
						sprintf(sz_temp2, "%08X", ((pbuf_job_result_packet)(__pipe_get_buf_job_result(i_cnt)))->nonce_list[ix]);
						strcat(sz_temp, sz_temp2);

						// Add a comma if it's not the last nonce
						if (ix != found_nonce_count - 1)
						strcat(sz_temp,",");
					}

					// Add the 'Enter' character
					strcat(sz_temp, "\n");

					// Ok, we have the nonce
					strcat(sz_rep, sz_temp);
				}
				else
				{
					// No nonce was available
					strcat(sz_rep,"-NONCE:NONE\n");
				}
			}
		}

		// Add the OK to our response (finilizer)
		strcat(sz_rep, "OK\n");
	}

	// OK, reset the results count in results-buffer, since we've already read them
	__pipe_set_buf_job_results_count(0);
	
	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string(sz_rep);  // Send it to USB
	else // We're a slave... send it by XLINK
	{
		unsigned int bXTimeoutDetected = 0;
		XLINK_SLAVE_respond_transact(sz_rep,
				strlen(sz_rep),
				__XLINK_TRANSACTION_TIMEOUT__,
				&bXTimeoutDetected,
				FALSE);
	}		

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

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");	

	// Wait for job data (96 Bytes)
	char sz_buf[1024];
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;

	if (XLINK_ARE_WE_MASTER)
	    USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout);
	else
	{
		// Wait for incoming transactions
		char bTimeoutDetectedOnXLINK = 0;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 100, &bTimeoutDetectedOnXLINK, FALSE);
					
		// Check
		if (bTimeoutDetectedOnXLINK) return PROTOCOL_FAILED;	
	}

	// Timeout?
	if (i_timeout < 2)
	{
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:TIMEOUT\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("ERR:TIMEOUT\n");	
		}
				
		return PROTOCOL_TIMEOUT;
	}

	// Check integrity
	if (i_read < sizeof(job_packet)) // Extra 16 bytes are preamble / postamble
	{
		sprintf(sz_buf, "ERR:INVALID DATA\n", i_read);
		
		if (XLINK_ARE_WE_MASTER)
			USB_send_string(sz_buf);  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string(sz_buf);
						
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet p_job = (pjob_packet)(sz_buf);

	// We have all what we need, send it to ASIC-COMM interface
	ASIC_job_issue(p_job, 0, 0x0FFFFFFFF);

	// Send our OK
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");	

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

	// Send OK first
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");
	else
		XLINK_SLAVE_respond_string("OK\n");

	// Wait for job data (96 Bytes)
	char sz_buf[1024];
	unsigned int i_read;
	unsigned int i_timeout = 1000000000;

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout);
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE);
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

	// Check integrity
	if (i_read < sizeof(job_packet_p2p)) // Extra 16 bytes are preamble / postamble
	{
		sprintf(sz_buf, "ERR:INVALID DATA, i_read = %d\n", i_read);
		
		if (XLINK_ARE_WE_MASTER)
			USB_send_string(sz_buf);  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string(sz_buf);
		
		return PROTOCOL_INVALID_USB_DATA;
	}

	// All is ok, send data to ASIC for processing, and respond with OK
	pjob_packet_p2p p_job = (pjob_packet_p2p)(sz_buf); // 4 Bytes are for the initial '>>>>'

	// We have all what we need, send it to ASIC-COMM interface
	ASIC_job_issue_p2p(p_job);

	// Send our OK	
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");

	// Return our result...
	return res;
}

PROTOCOL_RESULT Protocol_get_status()
{
	// Our result
	PROTOCOL_RESULT res = PROTOCOL_SUCCESS;

	// Here, we must return our chip status
	unsigned int  nonce_list[16];
	char i;
	char nonce_count = 0;
	char stat;

	char sz_report[1024];
	char sz_tmp[48];

	stat = ASIC_get_job_status(nonce_list, &nonce_count);

	// What is our status?
	if (stat == ASIC_JOB_IDLE)
	{
		if (XLINK_ARE_WE_MASTER)
			 USB_send_string("IDLE\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			 XLINK_SLAVE_respond_string("IDLE\n");
	}		
	else if (stat == ASIC_JOB_NONCE_FOUND)
	{
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
			USB_send_string(sz_report);  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string(sz_report);
	}
	else if (stat == ASIC_JOB_NONCE_NO_NONCE)
	{
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("NO-NONCE\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("NO-NONCE\n");
	}		
	else if (stat == ASIC_JOB_NONCE_PROCESSING)
	{
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("BUSY\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("BUSY\n");
	}		
	else
	{
		// Send it to host
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:UKNOWN\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERR:UKNOWN\n");
	}

	// Return our result...
	return res;
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

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout);
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE);
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
	if (i_read != 4)
	{
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("ERR:INVALID DATA\n");  // Send it to USB
		else // We're a slave... send it by XLINK
			XLINK_SLAVE_respond_string("ERR:INVALID DATA\n");
		
		return PROTOCOL_FAILED;		
	}		
	
	// Get the Frequency word 
	int iFreqFactor = (sz_buf[0]) | (sz_buf[1] << 8) | (sz_buf[2] << 16) | (sz_buf[3] << 24);
	
	// Do whatever we want with this iFreqFactor
	ASIC_SetFrequencyFactor(iFreqFactor);
	
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

	// This packet contains Mid-State, Merkel-Data and Nonce Begin/End
	if (XLINK_ARE_WE_MASTER)
		USB_wait_stream(sz_buf, &i_read, 1024, &i_timeout);
	else
	{
		char bTimeoutDetected = FALSE;
		XLINK_SLAVE_wait_transact(sz_buf, &i_read, 256, 200, &bTimeoutDetected, FALSE);
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
	if (i_read != 4)
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
	
	// Check if it's not 0x1F
	if (iCPLDId == XLINK_GENERAL_DISPATCH_ADDRESS)
	{
		// Send OK first
		if (XLINK_ARE_WE_MASTER)
			USB_send_string("PRSN");  // Send it to USB
		else // We're a slave... send it by XLINK
		{
			// XLINK_SLAVE_respond_string("OK\n");
			char bTimeoutDetected = FALSE;
			XLINK_SLAVE_respond_transact("PRSN", 4, __XLINK_TRANSACTION_TIMEOUT__, &bTimeoutDetected, FALSE);
		}			
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

void Flush_p2p_buffer_into_engines()
{
	// Our flag which tells us where the previous job
	// was a P2P job processed or not :)
	static char _prev_job_was_p2p = 0;

	// Take the job from buffer and put it here...
	// (We take element 0, and push all the arrays back...)

	// Special case: if processing is finished, it's not ok to pop and
	// _prev_job_was_p2p is set to one, then we must take the result
	// and put it in our result list
	if ((!__pipe_ok_to_pop()) && (!ASIC_is_processing()) && _prev_job_was_p2p)
	{
		// Verify the result counter is correct
		if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
		{
			// Then do nothing in this case (for the moment)
		}

		// Read the result and put it here...
		unsigned int  i_found_nonce_list[16];
		char i_found_nonce_count;
		char i_result_index_to_put = 0;
		ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count);

		// Set the correct index
		i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ?  (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

		// Copy data...
		memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_midstate, 		32);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_begin, (void*)__inprocess_nonce_begin, 	4);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_end,	(void*)__inprocess_nonce_end, 		4);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)	 i_found_nonce_list, 		16*sizeof(int));
		__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

		// Increase the result count (if possible)
		if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH - 1) __buf_job_results_count++;

		// Also clear the _prev_job_was_p2p flag,
		// this prevents the loop from adding additional results
		// to the results list when they really don't exist...
		_prev_job_was_p2p = 0;

		// We return, since there is nothing left to do
		return;
	}

	// Do we have anything to pop? Are we processing?
	if ((!__pipe_ok_to_pop()) || (ASIC_is_processing())) 	return;

	// Ok, at this stage, we take the result of the previous operation
	// and put it in our result buffer
	if (TRUE)
	{
		// Verify the result counter is correct
		if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
		{
			// Then do nothing in this case (for the moment)
		}

		// Read the result and put it here...
		unsigned int  i_found_nonce_list[16];
		char i_found_nonce_count;
		char i_result_index_to_put = 0;
		ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count);

		// Set the correct index
		i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ?  (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

		// Copy data...
		memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_midstate, 		32);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_begin, (void*)__inprocess_nonce_begin, 	4);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_end,	(void*)__inprocess_nonce_end, 		4);
		memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)	 i_found_nonce_list, 		16*sizeof(int));
		__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

		// Increase the result count (if possible)
		if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH - 1) __buf_job_results_count++;
	}

	// -------------
	// We have something to pop, get it...
	job_packet_p2p job_p2p;
	if (__pipe_pop_P2P_job(&job_p2p) == PIPE_JOB_BUFFER_EMPTY)
	{
		// This is odd!!! Don't do anything...
		_prev_job_was_p2p = 0; // Obviously, this must be cleared...
		return;
	}

	// Before we issue the job, we must put the correct information
	memcpy((void*)__inprocess_midstate, 	(void*)job_p2p.midstate, 		32);
	memcpy((void*)__inprocess_nonce_begin,  (void*)job_p2p.nonce_begin, 	4);
	memcpy((void*)__inprocess_nonce_end, 	(void*)job_p2p.nonce_end, 		4);

	// Send it to processing...
	ASIC_job_issue_p2p(&job_p2p);

	// Our job is p2p now...
	_prev_job_was_p2p = 1;
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
}

int Delay_1()
{
	volatile int x;
	volatile int z;
	for (x = 0; x < 450; x++) z++;
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
	strcpy(sz_hex, "");
	*i_hex_len = 0;

	for (index = 0; index < i_stream_len; index++)
	{
		sprintf(sz_temp, "%02X", sz_stream[index]);
		strcat(sz_hex, sz_temp);

		if (index % 16 == 0)
		{
			strcat(sz_hex, " ");
			*i_hex_len += sizeof("") + 2;
		}
		else
		{
			strcat(sz_hex, " ");
			*i_hex_len += sizeof("") + 2;
		}
	}

	// We're done...
}
