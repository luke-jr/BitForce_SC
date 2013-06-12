/*
 * MainFile.C
 * -----------
 * Main MCU firmware code for A2GX and ST3 card series
 *
 *
 *  Created on: Nov 8, 2011
 *      Author: root (nasser)
 */

/* BUG FIX LOG
 * ------------------------------------------------------------------------------------------------
 * July 3rd 2012 - Fixed the P2P JOB problem (range_end - range_begin, which was inverse)
 * June 2nd 2012 - Realized the system must run a 16MHz to ensure compatibility with flash SPI
 *
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

#include <string.h>
#include <stdio.h>

#include "HostInteractionProtocols.h"
#include "HighLevel_Operations.h"
#include "AVR32_OptimizedTemplates.h"
#include "AVR32X/AVR32_Module.h"
// Information about the result we're holding
extern buf_job_result_packet  __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char 		   __buf_job_results_count;  // Total of results in our __buf_job_results

// -------------------------------------
// ========== META FEATURES ============
// -------------------------------------

void MCU_Main_Loop(void);

// -------------------------------------
// ============ END META ===============
// -------------------------------------

// This function initializes the XLINK Chain
static int	Management_MASTER_Initialize_XLINK_Chain(void);

/////////////////////////////////////////////////////////
/// PROTOCOL
/////////////////////////////////////////////////////////


/*!
 * brief Main function. Execution starts here.
 */
int main(void)
{
	// Initialize everything here...
	init_mcu();

	// Continue...
	init_mcu_led();
	init_pipe_job_system();
	init_XLINK(); // TEMPORARY!
	init_ASIC();
	init_USB();
	
	// Initialize A2D
	a2d_init();
	a2d_get_temp(0);    // This is to clear the first invalid conversion result...
	a2d_get_temp(1);    // This is to clear the first invalid conversion result...
	a2d_get_voltage(0); // This is to clear the first invalid conversion result...
	a2d_get_voltage(1); // This is to clear the first invalid conversion result...
	a2d_get_voltage(2); // This is to clear the first invalid conversion result...
	
	// Initialize timer
	MCU_Timer_Initialize();
	MCU_Timer_SetInterval(10);
	MCU_Timer_Start();
	
	// Turn on the front LED
	MCU_MainLED_Initialize();
	MCU_MainLED_Set();
	
	// Detect if we're chain master or not [MODIFY]
	XLINK_ARE_WE_MASTER = XLINK_detect_if_we_are_master(); // For the moment we're the chain master [MODIFY]
	if (XLINK_ARE_WE_MASTER) 
	{ 
		// Wait for a small time
		blink_medium(); 
		blink_medium(); 
		blink_medium(); 
		blink_medium(); 
		blink_medium(); 
		blink_medium(); 
		
		// Initialize the XLINK. Interrogate all devices in the chain and assign then addresses
		if (XLINK_is_cpld_present() == TRUE)
		{
			// We're the master, set proper configuration
			XLINK_set_cpld_id(0);
			XLINK_set_cpld_master(TRUE);
			XLINK_set_cpld_passthrough(FALSE);
			
			if (Management_MASTER_Initialize_XLINK_Chain() == FALSE)
			{
				// Ok this can be bad, we failed the chain initialization
			}
		}
	}
	else
	{
		if (XLINK_is_cpld_present() == TRUE)
		{
			// Disable pass-through and set our cpld-address = 255
			// We later will await enumeration
			XLINK_set_cpld_id(XLINK_GENERAL_DISPATCH_ADDRESS);
			XLINK_set_cpld_master(FALSE);
			XLINK_set_cpld_passthrough(FALSE);
		}
	}
	
	// Reset global values
	global_vals[0] = 0;
	global_vals[1] = 0;
	global_vals[2] = 0;
	global_vals[3] = 0;
	global_vals[4] = 0;
	global_vals[5] = 0;

	// Go to our protocol main loop
	MCU_Main_Loop();
	return(0);
}

//////////////////////////////////
//// PROTOCOL functions
//////////////////////////////////

// OK, a bit of explanation here:
// The only operation that can be in progress is Job-Handling.
// We can do one thing however, make sure that the job is not active...
// We must read the ASIC periodically until we have some response.
void MCU_Main_Loop()
{
	// Commands received from PC is 3 bytes in length
	// and will look like 'Z' + [COMMAND] + 'X'.
	//
	// Once we receive a packet like this, we must respond with 'OK'
	// and call the corresponding function.
	// The function will take it from there...
	// (PC Will send the rest of the data after that...)

	// First things first, we must clear the FTDI chip
	// Read all that you can..
	volatile int i = 10000;
	volatile unsigned int intercepted_command_length = 0;
	
	while (USB_inbound_USB_data() && i-- > 1) USB_read_byte();

	// OK, now the memory on FTDI is empty,
	// wait for standard packet size
	volatile char sz_cmd[1024];
	volatile unsigned short umx;
	volatile unsigned int  i_count = 0;
	
	volatile char bTimeoutDetectedOnXLINK = 0;
	volatile char bDeviceNotRespondedOnXLINK = 0;			
	
	for (umx = 0; umx < 1024; umx++) sz_cmd[umx] = 0;
	
	//////////////////////////////////////////
	// Turn the LED on
	//////////////////////////////////////////
	MCU_MainLED_Set();
	
	while (1)
	{
		// HighLevel Functions Spin
		HighLevel_Operations_Spin();

		//////////////////////////////////////////
		// If we are master, we'll listen to the USB
		//////////////////////////////////////////
		if (XLINK_ARE_WE_MASTER)
		{
			// We listen to USB
			i = 3000;
			while (!USB_inbound_USB_data() && i-- > 1);
			
			// Check, if 'i' equals zero, we discard the actual command buffer
			if (i <= 1)
			{
				// Clear buffer, something is wrong...
				i_count = 0;
				sz_cmd[0] = 0;
				sz_cmd[1] = 0;
				sz_cmd[2] = 0;
			}

			// We've reduced timeout counter to 5000, so we can run this function periodically
			// Flush the job (should they exist)
			// This must be called on a timer running 
			// Management_flush_p2p_buffer_into_engines();

			// Check if there is data, or we just had
			// an overflow?
			if (!USB_inbound_USB_data()) continue;
			
			// Was EndOfStream detected?
			volatile unsigned int bEOSDetected = FALSE;
			intercepted_command_length = 0;
			volatile unsigned int iExpectedPacketLength = 0;
			volatile unsigned int bInterceptingChainForwardReq = FALSE;

			// Read all the data that has arrived
			while (USB_inbound_USB_data() && i_count < 1024)
			{
				// Read byte
				sz_cmd[i_count] = USB_read_byte();
				
				// Are we expecting 
				bInterceptingChainForwardReq = (sz_cmd[0] == 64) ? TRUE : FALSE;
				if (bInterceptingChainForwardReq) {
					 intercepted_command_length = (i_count + 1) - 3; // First three characters are @XY (X = Packet Size, Y = Forware Number)
					 if (i_count > 1) iExpectedPacketLength = sz_cmd[1] & 0x0FF;
				}	
				
				// Increase Count				 
				i_count++;
				
				// Check if 3-byte packet is done
				if ((i_count == 3) && (bInterceptingChainForwardReq == FALSE))
				{
					 bEOSDetected = TRUE;
					 break;
				}					 
				
				if (bInterceptingChainForwardReq && (intercepted_command_length == iExpectedPacketLength))
				{
					bEOSDetected = TRUE;
					break;
				}
				
				// Check if we've overlapped
				if (i_count > 256)							
				{
					// Clear buffer, something is wrong...
					i_count = 0;
					sz_cmd[0] = 0;
					sz_cmd[1] = 0;
					sz_cmd[2] = 0;
					continue;
				}
				
			}	
			
			// Check for length and signature
			// If we've received less than three characters, continue waiting
			if (i_count < 3)
			{
				if (bEOSDetected == TRUE)
				{
					// Clear buffer, something is wrong...
					i_count = 0;
					sz_cmd[0] = 0;
					sz_cmd[1] = 0;
					sz_cmd[2] = 0;
					continue;
				}
				else 
					continue; // We'll continue...			
			}
		}
		else 
		{
			///////////////////////////////////////////////
			// We listen to XLINK, as we are a chain-slave
			///////////////////////////////////////////////
						
			// Wait for incoming transactions
			bTimeoutDetectedOnXLINK = FALSE;
			
			// Run the procedure
			XLINK_SLAVE_wait_transact(sz_cmd, 
									  &i_count, 
									  256, 
									  500,  // 500us, half a ms
									  &bTimeoutDetectedOnXLINK, FALSE);
			
			// Check for sz_cmd, if it's PUSH then we have an invalid command
			if ((sz_cmd[0] == 'P') && (sz_cmd[1] == 'U') && (sz_cmd[2] == 'S') && (sz_cmd[3] == 'H'))
			{
				MACRO_XLINK_send_packet(XLINK_get_cpld_id(), "INVA", 4, TRUE, FALSE);
				continue;
			}
			
			// Check for sz_cmd, AA BB C4 <ID>, then we set CPLD ID and enable pass-through
			if ((sz_cmd[0] == 0xAA) && (sz_cmd[1] == 0xBB) && (sz_cmd[2] == 0xC4) && (i_count == 4))
			{
				// We respond with 'ACK' and then change the address
				XLINK_SLAVE_respond_transact("ACK", 
											 3,
											 __XLINK_TRANSACTION_TIMEOUT__, 
											 &bTimeoutDetectedOnXLINK, 
											 FALSE); // Note: We'll used XLINK general dispatch address for this specific operation!

				// Set the new address
				XLINK_set_cpld_id(sz_cmd[3]);
				continue;
			}
			
			// Check 
			if (bTimeoutDetectedOnXLINK) continue;			
		}		

		// Check number of bytes received so far.
		// If they are 3, we may have a command here (4 for the AMUX Read)...
		if (TRUE)
		{
			// Reset the count anyway
			i_count = 0;

			// Check for packet integrity
			if ((sz_cmd[0] != 'Z' || sz_cmd[2] != 'X') && (sz_cmd[0] != '@')) // @XX means forward to XX (X must be between '0' and '9')
			{
				continue;
			}
			else
			{
				// We have a command. Check for validity
				if (sz_cmd[0] != '@' &&
					sz_cmd[1] != PROTOCOL_REQ_INFO_REQUEST &&
					sz_cmd[1] != PROTOCOL_REQ_HANDLE_JOB &&
					sz_cmd[1] != PROTOCOL_REQ_HANDLE_JOB_P2POOL &&
					sz_cmd[1] != PROTOCOL_REQ_ID &&
					sz_cmd[1] != PROTOCOL_REQ_GET_FIRMWARE_VERSION &&
					sz_cmd[1] != PROTOCOL_REQ_BLINK &&
					sz_cmd[1] != PROTOCOL_REQ_TEMPERATURE &&
					sz_cmd[1] != PROTOCOL_REQ_BUF_PUSH_JOB &&
					sz_cmd[1] != PROTOCOL_REQ_BUF_STATUS &&
					sz_cmd[1] != PROTOCOL_REQ_BUF_FLUSH &&
					sz_cmd[1] != PROTOCOL_REQ_GET_VOLTAGES &&
					sz_cmd[1] != PROTOCOL_REQ_GET_CHAIN_LENGTH &&
					sz_cmd[1] != PROTOCOL_REQ_SET_FREQ_FACTOR &&
					sz_cmd[1] != PROTOCOL_REQ_GET_FREQ_FACTOR &&
					sz_cmd[1] != PROTOCOL_REQ_SET_XLINK_ADDRESS	&&
					sz_cmd[1] != PROTOCOL_REQ_XLINK_ALLOW_PASS &&
					sz_cmd[1] != PROTOCOL_REQ_XLINK_DENY_PASS &&
					sz_cmd[1] != PROTOCOL_REQ_PRESENCE_DETECTION &&
					sz_cmd[1] != PROTOCOL_REQ_ECHO &&
					sz_cmd[1] != PROTOCOL_REQ_TEST_COMMAND &&
					sz_cmd[1] != PROTOCOL_REQ_GET_STATUS)
				{					
					if (XLINK_ARE_WE_MASTER)
						USB_send_string("ERR:UNKNOWN COMMAND\n");
					else
					{
						XLINK_SLAVE_respond_transact("ERR:UNKNOWN COMMAND\n", 
													 sizeof("ERR:UNKNOWN COMMAND\n"), 
													 __XLINK_TRANSACTION_TIMEOUT__, 
													 &bDeviceNotRespondedOnXLINK, 
													 FALSE);
					}
																		 
					// Continue the loop	
					continue;
				}
				
				// Do we have a Chain-Forward request?
				if ((sz_cmd[0] == '@') && (XLINK_ARE_WE_MASTER))
				{
					// Forward command to the device in chain...
					Protocol_chain_forward((char )sz_cmd[2], 
										   (char*)(sz_cmd+3), 
										   intercepted_command_length); // Length is always 3	
				}					
				else
				{
					// We have a valid command, go call its procedure...
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_PUSH_JOB)			Protocol_P2P_BUF_PUSH();
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_FLUSH)			Protocol_P2P_BUF_FLUSH();
					if (sz_cmd[1] == PROTOCOL_REQ_BUF_STATUS)			Protocol_P2P_BUF_STATUS();
					if (sz_cmd[1] == PROTOCOL_REQ_INFO_REQUEST)			Protocol_info_request();
					if (sz_cmd[1] == PROTOCOL_REQ_HANDLE_JOB)			Protocol_handle_job();
					if (sz_cmd[1] == PROTOCOL_REQ_ID)					Protocol_id();
					if (sz_cmd[1] == PROTOCOL_REQ_HANDLE_JOB_P2POOL)	Protocol_handle_job_p2p();
					if (sz_cmd[1] == PROTOCOL_REQ_BLINK)				Protocol_Blink();
					if (sz_cmd[1] == PROTOCOL_REQ_TEMPERATURE)			Protocol_temperature();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_STATUS)			Protocol_get_status();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_VOLTAGES)			Protocol_get_voltages();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_FIRMWARE_VERSION)	Protocol_get_firmware_version();
					if (sz_cmd[1] == PROTOCOL_REQ_SET_FREQ_FACTOR)		Protocol_set_freq_factor();
					if (sz_cmd[1] == PROTOCOL_REQ_GET_FREQ_FACTOR)		Protocol_get_freq_factor();
					if (sz_cmd[1] == PROTOCOL_REQ_SET_XLINK_ADDRESS)	Protocol_set_xlink_address();
					if (sz_cmd[1] == PROTOCOL_REQ_XLINK_ALLOW_PASS)		Protocol_xlink_allow_pass();
					if (sz_cmd[1] == PROTOCOL_REQ_XLINK_DENY_PASS)		Protocol_xlink_deny_pass();
					if (sz_cmd[1] == PROTOCOL_REQ_ECHO)					Protocol_Echo();
					if (sz_cmd[1] == PROTOCOL_REQ_TEST_COMMAND)			Protocol_Test_Command();
					if (sz_cmd[1] == PROTOCOL_REQ_PRESENCE_DETECTION)   Protocol_xlink_presence_detection();
				}	
				
				// Once we reach here, our procedure has run and we're back to standby...
			}
		}
		else
		{
			i_count = 0;
			continue;
		}		
	}
}


//////////////////////////////////
//// Management functions
//////////////////////////////////

static int Management_MASTER_Initialize_XLINK_Chain()
{
	// What we do here is we keep sending PROTOCOL_PRESENCE_DETECTION until we no UL32er receive
	// a response. For each response we receive, we send a SET-ID command. The responding device
	// we have an ID assigned to it and will no UL32er response to PROTOCOL_PRESENCE_DETECTION command
	// OK We've detected a ChainForward request. First Send 'OK' to the host
		
	char szRespData[32];
	char sz_cmd[16];
	unsigned short iRespLen = 0;
	char  bDeviceNotResponded = 0;
	char  bTimeoutDetected = 0;
	char  iActualID = 1; // The ID we have to assign
	unsigned int iTotalRetryCount = 0;
	
	// Our device detection counter...
	int iDevDetectionCount = 0;
	
	// Check CPLD, is CHAIN_OUT connected?
	char iChainOutConnectValue = 0;
	MACRO__AVR32_CPLD_Read(iChainOutConnectValue, CPLD_ADDRESS_CHAIN_STATUS);
	
	if ((iChainOutConnectValue & CHAIN_OUT_BIT) == 0)
	{
		// Meaning there is not chain-out connection
		MAST_TICK_COUNTER += 5;
		return TRUE;	
	}		
		
	// Main loop
	while (iActualID < 0x01F) // Maximum 31 devices supported
	{
		// set the proper command
		sz_cmd[0] = 'Z';
		sz_cmd[2] = 'X';
		sz_cmd[1] = PROTOCOL_REQ_PRESENCE_DETECTION;

		// Clear the response message
		szRespData[0] = 0; szRespData[1] = 0; szRespData[2] = 0; szRespData[3] = 0; szRespData[4] = 0;
		szRespData[5] = 0; szRespData[6] = 0; szRespData[7] = 0; szRespData[8] = 0; szRespData[9] = 0;

		// Send a message to general dispatch	
		XLINK_MASTER_transact(XLINK_GENERAL_DISPATCH_ADDRESS,
							  sz_cmd,
							  3,
							  szRespData,
							  &iRespLen,
							  128,					// Maximum response length
							  __XLINK_TRANSACTION_TIMEOUT__,			// 400us timeout
							  &bDeviceNotResponded,
							  &bTimeoutDetected,
							  TRUE);				// We're master

		// Check response errors
		if (bDeviceNotResponded || bTimeoutDetected)
		{
			// We will try for 80 times here
			if (iDevDetectionCount++ > 10) 
			{
				// Then we have no more devices, exit the loop
				break;
			}
			else continue;
		}
		
		// reset iDevDetectionCount since we've succeeded in getting a response
		iDevDetectionCount = 0;

		// Check response, is it 'Present'?
		if ((szRespData[0] == 'P') && (szRespData[1] == 'R') && (szRespData[2] == 'S') && (szRespData[3] == 'N') && (iRespLen == 4))
		{
			// We're ok, Blink for a while....
		}
		else
		{
			// We will try for 80 times here
			if (iDevDetectionCount++ > 80)
			{
				// Then we have no more devices, exit the loop
				break;
			}
			else continue; // Otherwise we try again
			
		}
		
		// OK, We have 'Present', set the device ID
		// set the proper command (This is valid in XLINK: AA BB C4 <ID>, device will then activate pass-through
		sz_cmd[0] = 0xAA;
		sz_cmd[1] = 0xBB;
		sz_cmd[2] = 0xC4;
		sz_cmd[3] = iActualID;
		
		// Clear the response message
		szRespData[0] = 0; szRespData[1] = 0; szRespData[2] = 0; szRespData[3] = 0; szRespData[4] = 0;
		szRespData[5] = 0; szRespData[6] = 0; szRespData[7] = 0; szRespData[8] = 0; szRespData[9] = 0;
		
		// Send a message to general dispatch
		XLINK_MASTER_transact(XLINK_GENERAL_DISPATCH_ADDRESS,
							  sz_cmd,
							  4,
							  szRespData,
							  &iRespLen,
							  128,					// Maximum response length
							  __XLINK_TRANSACTION_TIMEOUT__,					// 400us timeout
							  &bDeviceNotResponded,
							  &bTimeoutDetected,
							  TRUE);				// We're master
		
		
		// Check response errors
		if ( (bDeviceNotResponded) || (bTimeoutDetected) || (!((szRespData[0] == 'A') && (szRespData[1] == 'C') && (szRespData[2] == 'K') && (iRespLen == 3))) )
		{
			if (iTotalRetryCount++ >= 200)
			{
				// Device did not respond to our message for 200 attempts. 
				// Either it doesn't exist or has already  taken the ID but we failed to received the
				// confirmation from it. Should this be the case, we can discover it by trying to execute
				// presence detection on it's new address. If it responded, then we're ok.
				
				int iPresenceDetectionRetryCount = 0;
				int iDeviceIsPresent = FALSE;
				
				// Note that we will execute this command for 40 times maximum
				// set the proper command
				while (iPresenceDetectionRetryCount++ < 40)
				{
					sz_cmd[0] = 'Z';
					sz_cmd[2] = 'X';
					sz_cmd[1] = PROTOCOL_REQ_PRESENCE_DETECTION;
					
					// Clear the response message
					szRespData[0] = 0; szRespData[1] = 0; szRespData[2] = 0; szRespData[3] = 0; szRespData[4] = 0;
					szRespData[5] = 0; szRespData[6] = 0; szRespData[7] = 0; szRespData[8] = 0; szRespData[9] = 0;
					
					// Send a message to general dispatch
					XLINK_MASTER_transact(iActualID,
										  sz_cmd,
										  3,
										  szRespData,
										  &iRespLen,
										  128,					// Maximum response length
										  __XLINK_TRANSACTION_TIMEOUT__,			// 400us timeout
										  &bDeviceNotResponded,
										  &bTimeoutDetected,
										  TRUE);				// We're master

					// Check response errors
					if (bDeviceNotResponded || bTimeoutDetected)
					{
						// We have no more devices...
						continue;
					}
					
					// Check response
					if ((szRespData[0] == 'P') && (szRespData[1] == 'R') && (szRespData[2] == 'S') && (szRespData[3] == 'N') && (iRespLen == 4))
					{
						// We're ok
						iDeviceIsPresent = TRUE;
						break;
					}					
				}
				
				// Did we fail?
				if (iDeviceIsPresent == FALSE)				
				{
					// Chain Device INIT has failed! This is bad..
					return FALSE;					
				}

			}  else continue; // Otherwise we repeat
		}

		// Since we've reached here, it means the ACK was received and we're OK to allow 'PassThrough'
		if (TRUE)
		{
			// Now ask the device to enable pass-through
			sz_cmd[0] = 'Z';
			sz_cmd[1] = PROTOCOL_REQ_XLINK_ALLOW_PASS;
			sz_cmd[2] = 'X';
			
			unsigned int iPassthroughRetryCount = 0;
			
			while (iPassthroughRetryCount < 200)
			{
				 // First clear the response buffer
				 szRespData[0] = 0; 
				 szRespData[1] = 0;
				 szRespData[2] = 0;
				 szRespData[3] = 0;
				 iRespLen = 0;
				 
				 // Get response
				 XLINK_MASTER_transact(iActualID, // We'll use the actual ID of the device
									   sz_cmd,
									   3,
									   szRespData,
									   &iRespLen,
									   128,					
									   __XLINK_TRANSACTION_TIMEOUT__,	
									   &bDeviceNotResponded,
									   &bTimeoutDetected,
									   TRUE);	
			
				// Check for response
				if (bDeviceNotResponded || bTimeoutDetected)					   
				{
					// We continue...
					iPassthroughRetryCount++;
					continue;
				}
				
				// If Response was not 'OK' we try again as well
				if (szRespData[0] != 'O' || szRespData[1] != 'K')
				{
					// We continue...
					iPassthroughRetryCount++;
					continue;
				}
				
				// All ok, we break
				break;
			}						
			
			// Check if we have succeeded or not by checking the retry count
			if (iPassthroughRetryCount >= 200)
			{
				// We have failed!	
				return FALSE;
			}
			
			// We're OK
			iActualID++;
		}
	
	}	
	
	// At this point, the iActualID shows the number of devices in chain
	XLINK_chain_device_count = iActualID - 1;
	
	// We've been successful
	return TRUE;	
}


