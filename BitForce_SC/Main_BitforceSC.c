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
#ifdef __COMPILING_FOR_AVR32__
	#include <avr32/io.h>
	#include <avr32/isi_005.h>
#else
	// #include <stm32.h>
#endif 

#include "std_defs.h"
#include "MCU_Initialization.h"
#include "JobPipe_Module.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include "USBProtocol_Module.h"
#include "A2D_Module.h"
#include "ASIC_Engine.h"

#include <string.h>
#include <stdio.h>

// Information about the result we're holding
extern buf_job_result_packet  __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char 		   __buf_job_results_count;  // Total of results in our __buf_job_results

// -------------------------------------
// ========== META FEATURES ============
// -------------------------------------

// Unit Identification String
#define UNIT_FIRMWARE_ID_STRING	">>>>ID: BitFORCE SC SHA256 Version 1.0>>>>\n"
#define UNIT_FIRMWARE_REVISION	">>>>REVISION 1.0>>>>"
#define UNIT_FIRMWARE_TYPE		">>>>JALAPENO>>>>" // OR ">>>>MINIRIG>>>>" OR ">>>>SINGLE>>>>" OR ">>>>LITTLE-SINGLE>>>>"
#define UNIT_FIRMWARE_SPEED		">>>>32>>>>"

// -------------------------------------
// ============ END META ===============
// -------------------------------------

/// ************************ DEBUG
void dbg_printf(const char* sz_message);
void dbg_to_hex_string(const char* szInput, int input_len, char* szoutput);

/// ************************ Main  definition
#define MIN(x,y) (((x < y) ? x : y))

unsigned int tickcount;

void clear_buffer(char* sz_stream, unsigned int ilen);
void stream_to_hex(char* sz_stream, char* sz_hex, unsigned int i_stream_len, unsigned int *i_hex_len);

/// ************************ Protocols
#define PROTOCOL_RESULT int

// ***** Global results
const PROTOCOL_RESULT PROTOCOL_SUCCESS 			= 0;
const PROTOCOL_RESULT PROTOCOL_FAILED 			= 1;
const PROTOCOL_RESULT PROTOCOL_TIMEOUT			= 2;

// ***** ASIC configuration
const PROTOCOL_RESULT PROTOCOL_CONFIG_FAILED 	= 3;
const PROTOCOL_RESULT PROTOCOL_WAIT_CONFIG		= 4;
const PROTOCOL_RESULT PROTOCOL_CONFIG_DONE		= 5;

// ***** Computer connection
const PROTOCOL_RESULT PROTOCOL_INVALID_REQUEST 	= 6;
const PROTOCOL_RESULT PROTOCOL_INVALID_USB_DATA = 7;

// ***** JOB Process
const PROTOCOL_RESULT PROTOCOL_NONCE_FOUND 		= 8;
const PROTOCOL_RESULT PROTOCOL_NONCE_NOT_FOUND 	= 9;
const PROTOCOL_RESULT PROTOCOL_PROCESSING_NONCE = 10;

// ***** Flash programming
const PROTOCOL_RESULT PROTOCOL_FLASH_OK			= 11;
const PROTOCOL_RESULT PROTOCOL_FLASH_FAILED		= 12;

// ***** Req numbers
#define PROTOCOL_REQ_INFO_REQUEST			2+65  // C
#define PROTOCOL_REQ_HANDLE_JOB				3+65  // D
#define PROTOCOL_REQ_HANDLE_JOB_P2POOL		15+65 // P
#define PROTOCOL_REQ_GET_STATUS				5+65  // F
#define PROTOCOL_REQ_ID						6+65  // G
#define PROTOCOL_REQ_GET_FIRMWARE_VERSION	9+65  // J
#define PROTOCOL_REQ_TEMPERATURE  			11+65 // L  (ZLX)
#define PROTOCOL_REQ_BLINK					12+65 // M
#define	PROTOCOL_REQ_BUF_PUSH_JOB			13+65 // N
#define	PROTOCOL_REQ_BUF_STATUS			  	14+65 // O
#define PROTOCOL_REQ_BUF_FLUSH				16+65 // Q ZQX
#define PROTOCOL_REQ_GET_VOLTAGES			19+65 // T
#define PROTOCOL_REQ_PRESENCE_DETECTION		17+65 // R // Respond with something if we don't have an ID attached to us...
#define PROTOCOL_REQ_GET_CHAIN_LENGTH		23+65 // X
#define PROTOCOL_REQ_SET_FREQ_FACTOR		21+65 // V
#define PROTOCOL_REQ_GET_FREQ_FACTOR		10+65 // K
#define PROTOCOL_REQ_SET_XLINK_ADDRESS		4+65  // E 
#define PROTOCOL_REQ_XLINK_ALLOW_PASS		7+65  // H 
#define PROTOCOL_REQ_XLINK_DENY_PASS		8+65  // I

// ***** Functions
static void 		   Protocol_Main			 (void);
static PROTOCOL_RESULT Protocol_handle_job		 (void);
static PROTOCOL_RESULT Protocol_handle_job_p2p	 (void);
static PROTOCOL_RESULT Protocol_info_request	 (void);
static PROTOCOL_RESULT Protocol_get_status		 (void);
static PROTOCOL_RESULT Protocol_get_voltages	 (void);
static PROTOCOL_RESULT Protocol_get_firmware_version(void);
static PROTOCOL_RESULT Protocol_id				 (void);
static PROTOCOL_RESULT Protocol_Blink			 (void);
static PROTOCOL_RESULT Protocol_temperature		 (void);
static PROTOCOL_RESULT Protocol_chain_forward    (char iTarget, char* sz_cmd, unsigned short iCmdLen);

// Initiate process for the next job from the buffer
// And returns previous popped job result
static PROTOCOL_RESULT  Protocol_P2P_BUF_PUSH(void);

// Returns only the status of the last processed job
// from the buffer, and will not initiate the next job process
static PROTOCOL_RESULT	Protocol_P2P_BUF_STATUS (void);

// This function flushes the P2P FIFO
static PROTOCOL_RESULT  Protocol_P2P_BUF_FLUSH(void);

// This sets/gets our ASICs frequency
static PROTOCOL_RESULT  Protocol_get_freq_factor(void);
static PROTOCOL_RESULT  Protocol_set_freq_factor(void);

// Our XLINK Support...
static PROTOCOL_RESULT  Protocol_set_xlink_address(void);
static PROTOCOL_RESULT  Protocol_xlink_allow_pass(void);
static PROTOCOL_RESULT  Protocol_xlink_deny_pass(void);
static PROTOCOL_RESULT  Protocol_xlink_presence_detection(void);

// This function initializes the XLINK Chain
static void	Management_MASTER_Initialize_XLINK_Chain(void);
static void Management_flush_p2p_buffer_into_engines(void);

void 	init_mcu_led(void);
void	blink_fast(void);
void	blink_slow(void);
void 	blink_medium(void);
void 	blink_high_freq(void);

int		Delay_1(void);
int 	Delay_2(void);
int 	Delay_3(void);

/////////////////////////////////////////////////////////
/// PROTOCOL
/////////////////////////////////////////////////////////

/*!
 * brief Main function. Execution starts here.
 */
int main(void)
{
	// Initialize tick count
	tickcount = 0;

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
			Management_MASTER_Initialize_XLINK_Chain();	
		}
	}
	else
	{
		if (XLINK_is_cpld_present() == TRUE)
		{
			// Disable pass-through and set our cpld-address = 255
			// We later will await enumeration
			XLINK_set_cpld_id(XLINK_GENERAL_DISPATCH_ADDRESS);
			// XLINK_set_cpld_id(0x01);
			XLINK_set_cpld_master(FALSE);
			XLINK_set_cpld_passthrough(FALSE);
		}
	}

	// Go to our protocol main loop
	Protocol_Main();
	return(0);
}

//////////////////////////////////
//// PROTOCOL functions
//////////////////////////////////

// OK, a bit of explanation here:
// The only operation that can be in progress is Job-Handling.
// We can do one thing however, make sure that the job is not active...
// We must read the ASIC periodically until we have some response.
static void Protocol_Main(void)
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
	unsigned int intercepted_command_length = 0;
	
	while (USB_inbound_USB_data() && i-- > 1) USB_read_byte();

	// OK, now the memory on FTDI is empty,
	// wait for standard packet size
	char sz_cmd[1024];
	unsigned short umx;
	unsigned int  i_count = 0;
	
	unsigned int bTimeoutDetectedOnXLINK = 0;
	unsigned int bDeviceNotRespondedOnXLINK = 0;			
	
	for (umx = 0; umx < 1024; umx++) sz_cmd[umx] = 0;
	
	while (1)
	{
		//////////////////////////////////////////
		// Turn the LED on
		//////////////////////////////////////////
		MCU_MainLED_Set();

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
			unsigned int bEOSDetected = FALSE;
			intercepted_command_length = 0;
			unsigned int iExpectedPacketLength = 0;
			unsigned int bInterceptingChainForwardReq = FALSE;

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
		else // We listen to XLINK, as we are a chain-slave
		{
			// Wait for incoming transactions
			XLINK_SLAVE_wait_transact(sz_cmd, &i_count, 256, 100, &bTimeoutDetectedOnXLINK, FALSE);
			
			// Check 
			if (bTimeoutDetectedOnXLINK) continue;			
		}		

		// Check number of bytes received so far.
		// If they are 3, we may have a command here (4 for the AMUX Read)...
		if (1)
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
										   
					// Read the leftover
					/*	if (TRUE)
						{
							char szUMX[100];
							char bTimeout = 0;
							char bLength = 0;
							char bSender = 0;
							char bLP = 0;
							char bBC = 0;
							XLINK_wait_packet(szUMX, &bLength, 100, &bTimeout, &bSender, &bLP, &bBC);
						}		*/	
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

PROTOCOL_RESULT Protocol_chain_forward(char iTarget, char* sz_cmd, unsigned short iCmdLen)
{
	// OK We've detected a ChainForward request. First Send 'OK' to the host
	char szRespData[2048];
	unsigned short iRespLen = 0;
	char  bDeviceNotResponded = 0;
	char  bTimeoutDetected = 0;
	
	XLINK_MASTER_transact(iTarget, 
						  sz_cmd, 
						  iCmdLen,
						  szRespData,
						  &iRespLen,
						  2048,					// Maximum response length
						  __XLINK_TRANSACTION_TIMEOUT__,					// 20000us timeout
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
		USB_send_string("ERR:TIMEOUT");
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
		XLINK_SLAVE_respond_string(szInfoReq);

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
		Management_flush_p2p_buffer_into_engines(); // Function is called before we return
		
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
	Management_flush_p2p_buffer_into_engines();

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
	if (1)
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
static PROTOCOL_RESULT  Protocol_get_freq_factor()
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
static PROTOCOL_RESULT  Protocol_set_freq_factor()
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
static PROTOCOL_RESULT  Protocol_set_xlink_address()
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
	
	// Set the CPLD address
	XLINK_set_cpld_id(sz_buf[0]);
	
	// Also allow pass-through
	XLINK_set_cpld_passthrough(TRUE);
			
	// Say we're ok
	if (XLINK_ARE_WE_MASTER)
		USB_send_string("OK\n");  // Send it to USB
	else // We're a slave... send it by XLINK
		XLINK_SLAVE_respond_string("OK\n");	
		
	// We have our frequency factor sent, exit
	return result;
	
}

static PROTOCOL_RESULT Protocol_xlink_presence_detection()
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
			USB_send_string("OK\n");  // Send it to USB
		else // We're a slave... send it by XLINK
		{
			XLINK_SLAVE_respond_string("OK\n");
		}			
	}

	// We're ok
	return PROTOCOL_SUCCESS;
}

static PROTOCOL_RESULT  Protocol_xlink_allow_pass()
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

static PROTOCOL_RESULT  Protocol_xlink_deny_pass()
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


//////////////////////////////////
//// Management functions
//////////////////////////////////

static void Management_MASTER_Initialize_XLINK_Chain()
{
	// What we do here is we keep sending PROTOCOL_PRESENCE_DETECTION until we no longer receive
	// a response. For each response we receive, we send a SET-ID command. The responding device
	// we have an ID assigned to it and will no longer response to PROTOCOL_PRESENCE_DETECTION command
	// OK We've detected a ChainForward request. First Send 'OK' to the host
		
	char szRespData[32];
	char sz_cmd[16];
	unsigned short iRespLen = 0;
	char  bDeviceNotResponded = 0;
	char  bTimeoutDetected = 0;
	char  iActualID = 1; // The ID we have to assign
	unsigned int iTotalRetryCount = 0;

	
	while (TRUE)
	{
		// set the proper command
		sz_cmd[0] = 'Z';
		sz_cmd[2] = 'X';
		sz_cmd[1] = PROTOCOL_REQ_PRESENCE_DETECTION;
		
		// Clear the response messag
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
			// We have no more devices...
			break;
		}

		// Check response, is it 'Present'?
		if ((szRespData[0] == 'O') && (szRespData[1] == 'K')) // && (szRespData[2] == 'S') && (szRespData[3] == 'N'))
		{
			// We're ok, Blink for a while....
			// blink_medium();blink_medium();blink_medium();blink_medium();blink_medium();blink_medium();blink_medium();blink_medium();blink_medium();
		}
		else
		{
			// Exit the loop, we're done with our devices here...
			break;
		}
		
		// OK, We have 'Present', set the device ID
		// set the proper command
		sz_cmd[0] = iActualID;
		sz_cmd[1] = 0;
		sz_cmd[2] = 0;
		sz_cmd[3] = 0;
		
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
		if (bDeviceNotResponded || bTimeoutDetected)
		{
			// We have no more devices...
			break;
		}

		// Check response, is it 'Present'?
		if ((szRespData[0] == 'O') && (szRespData[1] == 'K'))
		{
			// We're not OK, try again (unless failure tells us not too)
			iActualID++;
		}
		else
		{
			// Exit the loop, we're done with our devices here...
			iTotalRetryCount++;
			
			if (iTotalRetryCount == 5)
			{
				// This is a major failure. Blink 50 times and give up
				for (unsigned int umx = 0; umx < 50; umx++) blink_medium();
				break;
			}
		}		
	}	
		
	// We've been successful
	return PROTOCOL_SUCCESS;	
}

static void Management_flush_p2p_buffer_into_engines()
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
	if (1)
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

///////////////////////////////////////
//// Debug functions
///////////////////////////////////////
void dbg_printf(const char* sz_message)
{
	USB_send_string((const char*)sz_message);
}

void dbg_to_hex_string(const char* szInput, int input_len, char* szoutput)
{
	char szHld[10];
	strcpy(szoutput,"");

	int x;
	int y;
	for (x = 0; x < input_len; x += 16)
	{
		for (y = x; y < x + 16 && y < input_len; y++)
		{
			sprintf(szHld, "%02X ", szInput[y]);
			strcat(szoutput, szHld);
		}
		strcat(szoutput, "\n");
	}
}

/////////////////////////////////
//// MCU LED
/////////////////////////////////
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
