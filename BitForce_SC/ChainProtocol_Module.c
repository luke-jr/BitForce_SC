/*
 * ChainProtocol_Module.c
 *
 * Created: 08/10/2012 21:40:04
 *  Author: NASSER GHOSEIRI
 */ 

// Include standard definitions
#include "std_defs.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include <string.h>
#include "AVR32X\AVR32_Module.h"
#include <avr32/io.h>
#include "AVR32_OptimizedTemplates.h"
#include "HostInteractionProtocols.h" // We use some constant here...

// *** IREG - RX Status (8 Bit)
//
// MSB                                              LSB
// +-------------------------+----------+-----+------+
// | BC | LP | Length (3Bit) | OVERFLOW | ERR | DATA |
// +----+----+---------------+----------+-----+------+
//   7    6       5..3            2        1      0


// *** IREG - RX Senders Address (8 Bit)
// 
// MSB                                              LSB
// +-------------------------+-----------------------+
// |            N/C          |    Senders Address    |
// +-------------------------+-----------------------+
//   7                     5   4                    0


// *** IREG - TX Status (8 Bit)
//
// MSB                                             LSB
// +-----------+----------+--------+-----------------+
// | N/C (5Bit)| TxInProg | TxDone |       N/C       |
// +-----------+----------+--------+-----------------+
//    7 .. 3        2          1            0


// *** IREG - RX Control (8 Bit)
//
// MSB                                             LSB
// +-------------------------------+-----------------+
// |  N/C (7 Bits)                 | Clear Registers |
// +-------------------------------+-----------------+
//              7 .. 1                      0


// *** IREG - TX Control (8 Bit)
//
// MSB                                             LSB
// +-----------+--------+----+----------------+------+
// |BitCorrect | XXXXXX | LP | Length (3Bits) | SEND |
// +-----------+--------+----+----------------+------+
//      7         6..5     4        3 .. 1        0


// *** IREG - TX TARGET ADRS (8 Bit)
//
// MSB                                             LSB
// +------------------------------+------------------+
// |  N/C (3 Bits)                | 5Bit Target ADRS |
// +------------------------------+------------------+
//              7 .. 5     	          4 .. 0


// *** IREG - Chain Status (8 Bit)
//
// MSB                                             LSB
// +--------------------------+----------+-----------+
// |  N/C (6 Bits)            | Chain IN | Chain OUT |
// +--------------------------+----------+-----------+
//              7 .. 2     	      1           0


// *** IREG - Master Control Register (8 Bit)
//
// MSB                                             LSB
// +-------------+------+---------------------+------+
// | PassThrough | NC   | CPLD ADDRESS (5Bit) | MSTR |
// +-------------+------+---------------------+------+
//      7           6           5..1             0

char __OUR_CPLD_ID = 0;
char __OUR_CHAIN_LENGTH = 0;
int  __internal_are_we_master = 0;

// This is the XLINK out box buffer
char XLINK_Outbox[4096]; 
char XLINK_Device_Status;
char XLINK_Outbox_Length;

// Our device map
volatile char XLINK_Internal_DeviceMap[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define XLINK_Internal_DeviceMap_RetryCount 2

// Initialize
void init_XLINK()
{
	// Init CPLD XLINK Info
	__AVR32_CPLD_SetAccess();
	__AVR32_CPLD_Initialize();
	XLINK_set_cpld_passthrough(0); // Disable pass-through
	XLINK_set_cpld_id(0); // Initialize our ID
	XLINK_chain_device_count = 0;
	XLINK_set_device_status(XLINK_DEVICE_STATUS_NO_TRANS);
	XLINK_set_outbox("",0);
	
	// Reset xlink address map
	GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK = 0; // All devices are off-line
}



int	XLINK_detect_if_we_are_master()
{
	// First check to see if CPLD is present. If it's not, then we're master for sure... (Either Jalapeno or Single)
	if (XLINK_is_cpld_present() == FALSE) return TRUE;
	
	// Check our CHAIN-IN activation. If it's active, then we're a slave. Otherwise we're a master.
	// We'll try this for 15,000 times. If for a period of 5000 times the state did not change, then we're ok
	volatile int v_counter = 0;
	volatile int total_runs = 0;
	volatile int bAreWeMaster = TRUE;
	
	while (v_counter < 15000)	
	{
		if (total_runs >= 5000)
		{
			bAreWeMaster = FALSE;
			break;
		}
		else
		{
			if ((XLINK_get_chain_status() & CHAIN_IN_BIT) == CHAIN_IN_BIT) // Meaning we have a CHAIN-IN Connected
			{
				total_runs++;
			}				
			else
			{
				if (total_runs > 2) total_runs -= 2;
			}				
		}					
		
		v_counter++;
	}	
	
	// If we've reached here, it means we're a master
	return bAreWeMaster;
}

// Send a string via XLINK (SLAVE calls this)
void XLINK_SLAVE_respond_string(char* szStringToSend)
{
	char bTimeoutDetected = FALSE;
	XLINK_SLAVE_respond_transact(szStringToSend, 
								 strlen(szStringToSend), 
								 __XLINK_TRANSACTION_TIMEOUT__, 
								 &bTimeoutDetected, FALSE);
}

// Called by the master, used to determine whether the chain exists or not
int XLINK_MASTER_chainDevicesExists()
{
	if ((XLINK_get_chain_status() & CHAIN_OUT_BIT) == CHAIN_OUT_BIT)
		return TRUE;
	else
		return FALSE;
}

// How many devices exist in chain?
int XLINK_MASTER_getChainLength()
{
	int iTotalDevicesInChain = 0;
	for (unsigned char x = 0; x < 31; x++) 
	{
		if ( (GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK & ((1<<x) & 0x0FFFFFFFF)) != 0) 
		{
			iTotalDevicesInChain++;
		}			
	}		
	
	return iTotalDevicesInChain;
}

// XLINK Is Device Present
char XLINK_MASTER_Is_Device_Present(char aID)
{
	// Our device detection counter...
	volatile int iDevDetectionCount = 0;
	
	// Is the device present?
	// set the proper command
	volatile char sz_cmd[3];
	sz_cmd[0] = 'Z';
	sz_cmd[2] = 'X';
	sz_cmd[1] = PROTOCOL_REQ_PRESENCE_DETECTION;
	
	// Send a message to general dispatch
	volatile char bTimedOut = FALSE;
	volatile char bDevNotResponded = FALSE;
	volatile char szResponse[10];
	volatile unsigned int iRespLen = 0;
	volatile char iBC = 0;
	volatile char iLP = 0;
	volatile char iSendersAddress = 0;
	
	iRespLen = 0;
	szResponse[0] = 0; 
	szResponse[1] = 0; 
	szResponse[2] = 0; 
	szResponse[3] = 0;
			
	XLINK_MASTER_transact(aID, sz_cmd, 3, szResponse, &iRespLen, 128, __XLINK_TRANSACTION_TIMEOUT__, &bDevNotResponded, &bTimedOut, TRUE);
	
	// Check response errors
	if (bTimedOut || bDevNotResponded)
	{
		return FALSE;
	}	
	
	// Device did respond, now was it correct?
	if ((szResponse[0] == 'P') && (szResponse[1] == 'R') && (szResponse[2] == 'S') && (szResponse[3] == 'N') && (iRespLen == 4))
	{
		// We're ok, Blink for a while....
		return TRUE;
	}
	else
	{
		// Then we have no more devices, exit the loop
		return FALSE;
	}	

}

// The Chain-Startup function
volatile int XLINK_MASTER_Start_Chain()
{
	
	// What we do here is we keep sending PROTOCOL_PRESENCE_DETECTION until we no UL32er receive
	// a response. For each response we receive, we send a SET-ID command. The responding device
	// we have an ID assigned to it and will no UL32er response to PROTOCOL_PRESENCE_DETECTION command
	// OK We've detected a ChainForward request. First Send 'OK' to the host
	
	// Check CPLD, is CHAIN_OUT connected?
	char iChainOutConnectValue = 0;
	MACRO__AVR32_CPLD_Read(iChainOutConnectValue, CPLD_ADDRESS_CHAIN_STATUS);
	
	if ((iChainOutConnectValue & CHAIN_OUT_BIT) == 0)
	{
		// Meaning there is not chain-out connection
		MAST_TICK_COUNTER += 5;
		return TRUE;
	}
	
	// Device Availability Bit
	// GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK
	// Each bit represents one device. Bit 1 means Device address 1...
	char  iActualID = 1; // The ID we have to assign
	
	// Main loop
	while (iActualID < 0x01F) // Maximum 30 devices supported. ID Starts from 1
	{
		// Reset Watchdog
		WATCHDOG_RESET;
		
		// Check if this thing already exists
		if (XLINK_MASTER_Is_Device_Present(iActualID) == TRUE)
		{
			GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK |= (1 << iActualID);
			XLINK_Internal_DeviceMap[iActualID] = XLINK_Internal_DeviceMap_RetryCount; // 2 Retries set...
			iActualID++;
			continue;
		}
		
		// Perform
		char bSucceeded = FALSE;
		XLINK_MASTER_Scan_And_Register_Device(iActualID, 10, 10, &bSucceeded);
		
		if (bSucceeded == FALSE)
		{
			// Abort, we're done
			break;
		}
		else
		{
			// ID successfully assigned
			iActualID++;
		}
	}
	
	// At this point, the iActualID shows the number of devices in chain
	XLINK_chain_device_count = iActualID - 1;
	
	// We've been successful
	return TRUE;
}


// XLINK Refresh chain
void XLINK_MASTER_Refresh_Chain()
{
	// What we do here is we check each device in the flag.
	// If they don't respond then we issue a chain-scan.
	// Should the device respond to chain-scan then we set the address
	// and register the device. If not, we reduce device count...
	// This operation could take a while to execute, so Watchdog Reset is important
	volatile char bNoResponseToEnumeration = FALSE;
	
	// First question, is the CPLD present at all?
	if (XLINK_is_cpld_present() == FALSE) return;
		
	// Main counter
	for (char aIDToAssign = 1; aIDToAssign < 31; aIDToAssign++)
	{
		// Reset Watchdog
		WATCHDOG_RESET;
		
		// It means the device should be present... Now is it?
		if (((GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK & (1 << aIDToAssign)) != 0))
		{
			if (XLINK_MASTER_Is_Device_Present(aIDToAssign))
			{
				// If present the we do nothing... All is ok!
				XLINK_Internal_DeviceMap[aIDToAssign] = XLINK_Internal_DeviceMap_RetryCount;				
				continue;	
			}
			else
			{
				if (XLINK_MASTER_Is_Device_Present(aIDToAssign))
				{
					// If present the we do nothing... All is ok!
					continue;
				}
				else
				{

					volatile char bSuccess = FALSE;
							
					// We've tried all we could, no luck unfortunately
					GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK &= (unsigned int)((~(1 << aIDToAssign)) & 0x0FFFFFFFF);	
							
					// In this case we do try...
					XLINK_MASTER_Scan_And_Register_Device(aIDToAssign, 5, 6, &bSuccess);
							
					// Did we succeed? Update the flag accordingly
					if (bSuccess)
					{
						volatile unsigned int iValueToSet = (1 << aIDToAssign);
						GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK |= iValueToSet;
						XLINK_Internal_DeviceMap[aIDToAssign] = XLINK_Internal_DeviceMap_RetryCount; // 
					}
					else // Clear
					{
						// No response to enumeration was detected
						bNoResponseToEnumeration = TRUE; // Meaning we shouldn't enumerate anymore, since no device responded	
						
						// Continue...
						if (XLINK_Internal_DeviceMap[aIDToAssign] == 0)
						{
							GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK &= (unsigned int)((~(1 << aIDToAssign)) & 0x0FFFFFFFF);
						}						
						else
						{
							XLINK_Internal_DeviceMap[aIDToAssign] -= 1;
						}
					}
				}
			}
			
		}
		else
		{
			
			// Then see if we can find a new device in chain
			volatile char bSucceeded = FALSE;
			
			if (bNoResponseToEnumeration == FALSE)
			{
				// In this case we do try...		
				XLINK_MASTER_Scan_And_Register_Device(aIDToAssign, 5, 6, &bSucceeded);
			
				// Did we succeed? Update the flag accordingly
				if (bSucceeded)
				{
					volatile unsigned int iValueToSet = (1 << aIDToAssign);
					GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK |= iValueToSet;
					XLINK_Internal_DeviceMap[aIDToAssign] = XLINK_Internal_DeviceMap_RetryCount;
				}					
				else // Clear
				{
					// Enumeration response null
					bNoResponseToEnumeration = TRUE;
					
					// Continue...
					if (XLINK_Internal_DeviceMap[aIDToAssign] == 0)
					{
						GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK &= (unsigned int)((~(1 << aIDToAssign)) & 0x0FFFFFFFF);
						bNoResponseToEnumeration = TRUE; // Meaning we shouldn't enumerate anymore, since no device responded
						XLINK_Internal_DeviceMap[aIDToAssign] = XLINK_Internal_DeviceMap_RetryCount;	
					}
					else
					{
						XLINK_Internal_DeviceMap[aIDToAssign] -= 1;
					}
					
				}									
			}
			else
			{
				if (XLINK_Internal_DeviceMap[aIDToAssign] == 0)
				{
					// Since in the past an enumeration has failed, there is no reason for us to re-enumerate now...
					GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK &= (unsigned int)((~(1 << aIDToAssign)) & 0x0FFFFFFFF);			
				}
				else
				{
					XLINK_Internal_DeviceMap[aIDToAssign] -= 1;					
				}					
			}
		}
	}	
}

// XLINK Scan and Register Device
void XLINK_MASTER_Scan_And_Register_Device(unsigned char  aIDToAssign,
										   unsigned char  aPassthroughRetryCounts,
										   unsigned char  aConnectRetryCounts,
										   unsigned char* aSucceeded)
{
	// Variables
	volatile char szRespData[32];
	volatile char sz_cmd[16];
	volatile unsigned int iRespLen = 0;
	volatile char bDeviceNotResponded = 0;
	volatile char bTimeoutDetected = 0;
	volatile unsigned int iTotalRetryCount = 0;
			
	// Our device detection counter...
	volatile int iDevDetectionCount = 0;
			
	// Did we succeed?
	*aSucceeded = FALSE;
	
	while (TRUE)
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
			// We will try for aConnectRetryCounts times here
			if (iDevDetectionCount++ > aConnectRetryCounts)
			{
				// Then we have no more devices, exit the loop
				*aSucceeded = FALSE;
				return;
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
			// We will try for 5 times here
			if (iDevDetectionCount++ > aConnectRetryCounts)
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
		sz_cmd[3] = aIDToAssign;
			
		// Clear the response message
		szRespData[0] = 0; szRespData[1] = 0; szRespData[2] = 0; szRespData[3] = 0; szRespData[4] = 0;
		szRespData[5] = 0; szRespData[6] = 0; szRespData[7] = 0; szRespData[8] = 0; szRespData[9] = 0;

			
		// Send a message to general dispatch
		XLINK_MASTER_transact(XLINK_GENERAL_DISPATCH_ADDRESS,//
							  sz_cmd,						 //
							  4,							 //
							  szRespData,					 //
							  &iRespLen,					 //
							  128,							 // Maximum response length
							  __XLINK_TRANSACTION_TIMEOUT__, // 400us timeout
							  &bDeviceNotResponded,			 //
							  &bTimeoutDetected,			 //
							  TRUE);						 // We're master
			

		// Check response errors
		if ( (bDeviceNotResponded) || (bTimeoutDetected) || (!((szRespData[0] == 'A') && (szRespData[1] == 'C') && (szRespData[2] == 'K') && (iRespLen == 3))) )
		{
			if (iTotalRetryCount++ >= aPassthroughRetryCounts)
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
					XLINK_MASTER_transact(aIDToAssign,
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
					aSucceeded = FALSE;				
					return;
				}

			}  else continue; // Otherwise we repeat
		}

		// Now ask the device to enable pass-through
		sz_cmd[0] = 'Z';
		sz_cmd[1] = PROTOCOL_REQ_XLINK_ALLOW_PASS;
		sz_cmd[2] = 'X';
				
		volatile unsigned int iPassthroughRetryCount = 0;
				
		while (iPassthroughRetryCount < aPassthroughRetryCounts)
		{
			// First clear the response buffer
			szRespData[0] = 0;
			szRespData[1] = 0;
			szRespData[2] = 0;
			szRespData[3] = 0;
			iRespLen = 0;
					
			// Get response
			XLINK_MASTER_transact(aIDToAssign, // We'll use the actual ID of the device
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
					
			// All ok, we exit the function
			*aSucceeded = TRUE;
			return;
		}
		
		// Check if we have succeeded or not by checking the retry count
		if (iPassthroughRetryCount >= aPassthroughRetryCounts)
		{
			// We have failed!
			*aSucceeded = FALSE;
			return;
		}				
	} // end of while(TRUE)
	

	// We're OK
	*aSucceeded = TRUE;
	return;
}

// iAdrs = Target
// szData and iLen is the data stack
void XLINK_send_packet(char iAdrs, char* szData, unsigned int iLen, char LP, char BC)
{
	// Set CPLD Access
	__AVR32_CPLD_SetAccess();

	// Wait until actual buffer is sent...
	while (TRUE)
	{
		char iTX_STATUS = 0;
		MACRO_XLINK_get_TX_status(iTX_STATUS);
		if ((iTX_STATUS & CPLD_TX_STATUS_TxInProg) == 0) break;
	}
	
	// We're ready to send...
	// Send data in 4byte packets. Each time we send, we wait for a reception of response package
	// Target device must respond with 'OK' and have its correct address in our packet.
	// Should we not receive the correct data, we'll retransmit the previous packet. This will be repeated
	// for 5 times until we realize that we're out of sync...
	// BitCorrect will handle the flip-flop correction indicator
	
	// Set the target device
	//while (XLINK_get_target_address() != iAdrs)
	MACRO_XLINK_set_target_address(iAdrs);
	
	// *** IREG - TX Control (8 Bit)
	//
	// MSB                                             LSB
	// +-----------+--------+----+----------------+------+
	// |BitCorrect | XXXXXX | LP | Length (3Bits) | SEND |
	// +-----------+--------+----+----------------+------+
	//      7         6..5     4        3 .. 1        0
	
	// Also bitStuff position
	unsigned char iTotalToSend = (iLen << 1);

	char szMMR[4];
				
	// Set Data
	MACRO__AVR32_CPLD_BurstTxWrite(szData, CPLD_ADDRESS_TX_BUF_BEGIN);

	// What's the value to write to our TX Control
	char iTxControlVal = 0b00000000;	
	iTxControlVal |= iTotalToSend;		//
	if (LP) iTxControlVal |= CPLD_TX_CONTROL_LP;
		
	// Set BitStuff value
	iTxControlVal |= (BC != 0) ? CPLD_TX_CONTROL_BC : 0;
	
	// Send packet info
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal);
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_START, CPLD_ADDRESS_TX_START_SEND);

}

//////////////////////////////////////////////////////////////////////////////
// AUXILIARY FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void XLINK_wait_packet (char  *data,
						unsigned int *length,
						UL32  time_out,
						char  *timeout_detected,
						char  *senders_address,
						char  *LP,
						char  *BC)
{
	// Reset all variables
	*BC = 0;
	*LP = 0;
	*timeout_detected = FALSE;
	*length = 0;
	*senders_address = 0;
	
	volatile char  iActualRXStatus = 0;
	volatile unsigned char us1 = 0;
	volatile unsigned char us2 = 0;

	iActualRXStatus = XLINK_get_RX_status();
	UL32 iTimeoutHolder = MACRO_GetTickCountRet;

	// Wait until actual buffer is sent...
	if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0)
	{
		
		// Wait for 25uS
		while (MACRO_GetTickCountRet - iTimeoutHolder < time_out)
		{
			iActualRXStatus = XLINK_get_RX_status();
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break;
		}
		
		// Did we timeout?
		if (MACRO_GetTickCountRet - iTimeoutHolder >= time_out)
		{
			// Ok we've timed out....
			// TODO: We must do something important
			*timeout_detected = 1; // Means we've timed out
			*length		= 0;
			*senders_address = 0;
			return;
		}
	}
	
	// We've received data
	volatile char imrLen = 0;
	imrLen = ((iActualRXStatus & 0b0111000) >> 3);
	*length = imrLen;
	*LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0;
	*BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0;
	*senders_address = __AVR32_CPLD_Read(CPLD_ADDRESS_SENDERS_ADRS);
	__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN);
	__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);

}

// This is used by master only...
void XLINK_MASTER_transact(char   iAdrs, 
					       char*  szData, 
					       unsigned int  iLen,
						   char*  szResp,
						   unsigned int* response_length,
						   unsigned int  iMaxRespLen,
						   UL32    transaction_timeout, // Master timeout
						   char   *bDeviceNotResponded, // Device did not respond, even to the first packet
						   char   *bTimeoutDetected, // Was a timeout detected?
					       char   bWeAreMaster)
{
	// Reset this value
	*bDeviceNotResponded = 0;
	*bTimeoutDetected = 0;
	
	// First check if we are master
	if (bWeAreMaster == 0) return;
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	volatile UL32 iTransactionStartTickcount = MACRO_GetTickCountRet;
	volatile unsigned int iTotalSent = 0;
	volatile char  iBytesToSend = 0;
	volatile char  iLP = 0; // LastPacket
	volatile char  iBC = 0; // BitCorrector ((Note: BitCorrector starts with 0))
	volatile char  iTotalRetryCount = 0; 
		
	// Wait for OK packet for 20us
	volatile char iTimeoutDetected = 0;
	volatile char szDevResponse[4];
	volatile unsigned int  __iRespLen = 0;
	volatile char __senders_address = 0;
	volatile char __lp = 0;
	volatile char __bc = 0;
	
	volatile UL32 vTemp1 = 0;
	volatile UL32 vTemp2 = 0;
	volatile UL32 vTemp3 = 0;
	
	volatile unsigned int iExpectedChecksum = 0;
		
	while (iTotalSent < iLen)
	{
		// Preliminary calculations
		iBytesToSend = ((iLen - iTotalSent) > 4) ? 4 : (iLen - iTotalSent);
		iLP = ((iTotalSent + iBytesToSend >= iLen) ? 1 : 0);
		iTotalRetryCount = 0;

RETRY_POINT_1:
		// Before doing anything, clear the CPLD
		MACRO_XLINK_clear_RX;
		
		// Send these bytes
		char* szDest = (char*)(szData + iTotalSent);
		MACRO_XLINK_send_packet(iAdrs, szDest, iBytesToSend, iLP, iBC);
		
		
		// Calculate iExpectedChecksum
		iExpectedChecksum = 0;
		iExpectedChecksum += ((iBytesToSend >= 1) ? szDest[0] : 0);
		iExpectedChecksum += ((iBytesToSend >= 2) ? szDest[1] : 0);
		iExpectedChecksum += ((iBytesToSend >= 3) ? szDest[2] : 0);
		iExpectedChecksum += ((iBytesToSend == 4) ? szDest[3] : 0);
	
		// Reset variables
		iTimeoutDetected = 0;
		__iRespLen = 0;
		__lp = 0;
		__bc = 0;
		
		// Clear szDevResponse
		szDevResponse[0] = 0;
		szDevResponse[1] = 0;
		szDevResponse[2] = 0;
		szDevResponse[3] = 0;
		
		// Wait for response
		MACRO_XLINK_wait_packet(szDevResponse, 
							    __iRespLen, 
							    (__XLINK_WAIT_PACKET_TIMEOUT__ >> 3),  // We operate on reduced timeout... This is to make sure we try more than the slave waits...
								iTimeoutDetected, 
								__senders_address, 
								__lp, 
								__bc);
						  
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iTransactionStartTickcount;
		
		if (vTemp3 > transaction_timeout) 
		{
			*bTimeoutDetected = 1;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			MACRO_XLINK_clear_RX;			
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount ==  (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 3))
			{
				// We've failed
				*bTimeoutDetected = 1;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_1;
			}
		}
		
		// No timeout was detected, check resp. If it was not ok, try again
		if ((szDevResponse[0] != 'A') || 
			(szDevResponse[1] != 'R') ||
			(szDevResponse[2] != (((iExpectedChecksum & 0x0FF00) >> 8) & 0x0FF)) ||
			(szDevResponse[3] !=  (iExpectedChecksum & 0x0FF)) ||
			(__iRespLen != 4) ||
			(__senders_address != iAdrs)) // Check both for address-match and 'OK' response
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount ==  (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 3))
			{
				// We've failed
				*bTimeoutDetected = 2;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;				
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_1;
			}			
		}			
		
		// Resp was OK... Now we continue...
		// Reset retry count
		iTotalRetryCount = 0;
						
		// We've sent 4 more bytes
		iBC = (iBC == 0) ? 1 : 0; // Flip BitCorrector
		iTotalSent += iBytesToSend; // Update total sent
	}
		
				
	// At this stage, the data has been sent, now we must ask Device for results...
	volatile unsigned int iTotalReceived = 0;
	iTotalRetryCount = 0;
	
	volatile UL32 imrActualHolder = 0;
	volatile UL32 imrActualTime = 0;
	
	// We have to reset the BitCorrector, as it will start from 0 for the PUSH part
	iBC = TRUE; // Push must be set to one
	char iExpectedBC = TRUE; // This is what we expect from the slave as BitCorrector
		
	// Now we have to wait for data
	while (iTotalReceived < iMaxRespLen)
	{
		// Before doing anything, clear the CPLD
		MACRO_XLINK_clear_RX;
		
		// Proceed
		MACRO_XLINK_send_packet(iAdrs,"PUSH", 4, iLP, iBC);
		
		// Reset variables
		iTimeoutDetected = 0;
		__iRespLen = 0;
		__lp = 0;
		__bc = 0;
		
		// Clear szDevResponse
		szDevResponse[0] = 0;
		szDevResponse[1] = 0;
		szDevResponse[2] = 0;
		szDevResponse[3] = 0;
		
		// We would want to measure something here...
		imrActualHolder = MACRO_GetTickCountRet;
		
		// Wait for response		
		MACRO_XLINK_wait_packet(szDevResponse, 
							    __iRespLen,
								(__XLINK_WAIT_PACKET_TIMEOUT__ >> 2),  // For the first packet (only) we wait UL32 time. For the rest, we'll be fast...
								iTimeoutDetected,
								__senders_address,
								__lp,
								__bc);
		
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iTransactionStartTickcount;
		
		if (vTemp3 > transaction_timeout)
		{
			*bTimeoutDetected = (iTotalReceived == 0) ? 3 : 77;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			MACRO_XLINK_clear_RX;
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 2))
			{
				// We've failed
				*bTimeoutDetected = 4;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;				
				return;
			}
			else
			{
				iTotalRetryCount++;
				continue;
			}
		}
		
		// No timeout was detected, check resp. If it was not ok, try again
		if (__senders_address != iAdrs || __iRespLen == 0 || iExpectedBC != __bc) // Check both for address-match, response-length and bit-corrector
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 2))
			{
				// We've failed
				if (__senders_address != iAdrs)
						*bTimeoutDetected = 5;
				else if (iExpectedBC != __bc)
				{
						*bTimeoutDetected = 50;
						GLOBAL_InterProcChars[0] = szDevResponse[0];
						GLOBAL_InterProcChars[1] = szDevResponse[1];
						GLOBAL_InterProcChars[2] = szDevResponse[2];
						GLOBAL_InterProcChars[3] = szDevResponse[3];
				}						
				else *bTimeoutDetected = 51;
						
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;				
				return;
			}
			else
			{
				iTotalRetryCount++;
				continue;
			}
		}
		
		
		// Reset retry count
		iTotalRetryCount = 0;
		
		// Resp was OK... Take the data
		if (__iRespLen >= 1 && (iTotalReceived + 1 <= iMaxRespLen)) szResp[iTotalReceived++] = szDevResponse[0];
		if (__iRespLen >= 2 && (iTotalReceived + 1 <= iMaxRespLen)) szResp[iTotalReceived++] = szDevResponse[1];
		if (__iRespLen >= 3 && (iTotalReceived + 1 <= iMaxRespLen)) szResp[iTotalReceived++] = szDevResponse[2];
		if (__iRespLen >= 4 && (iTotalReceived + 1 <= iMaxRespLen)) szResp[iTotalReceived++] = szDevResponse[3];
		
		// We've sent 4 more bytes
		iBC = (iBC == 0) ? 1 : 0; // Flip BitCorrector
		iExpectedBC = (iExpectedBC == 0) ? 1 : 0; // Flip the expected BitCorrector
		
		// Was it the last packet? if so, exit the loop
		if (__lp == 1)
		{
			 imrActualTime++;
			 break;		
		}			 
		
		// Update imrTime
		vTemp1 = MACRO_GetTickCountRet;
		vTemp2 = vTemp1 - imrActualHolder;
		imrActualTime = vTemp2;		
		imrActualTime++;
	}		
	
	// Set the response size
	*response_length = iTotalReceived;
	
	// Reset TotalRetryCount
	iTotalRetryCount = 0;
						
	// We've received data, LP was detected. So now we send the TERM signal until we receive it, and then we're done...
	// Send TERM signal
	while (1)
	{
		// .................. ..... ...... .. .... ....
RETRY_POINT_3:
		// Before doing anything, clear the CPLD
		MACRO_XLINK_clear_RX;
		
		// Proceed
		MACRO_XLINK_send_packet(iAdrs,"TERM", 4, iLP, 0);
			
		// Reset variables
		iTimeoutDetected = 0;
		__iRespLen = 0;
		__lp = 0;
		__bc = 0;
		
		// Clear szDevResponse
		szDevResponse[0] = 0;
		szDevResponse[1] = 0;
		szDevResponse[2] = 0;
		szDevResponse[3] = 0;		
		
		// Wait for response
		MACRO_XLINK_wait_packet(szDevResponse,
								__iRespLen,
								(__XLINK_WAIT_PACKET_TIMEOUT__ >> 3),  // We wait an 4th of the maximum allowed, since we're master
								iTimeoutDetected,
								__senders_address,
								__lp,
								__bc);
						  
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iTransactionStartTickcount;
		
		if (vTemp3 > transaction_timeout)
		{
			*bTimeoutDetected = 6;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			MACRO_XLINK_clear_RX;
			return;
		}

		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 3))
			{
				// We've failed
				*bTimeoutDetected = 7;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;			
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_3;
			}
		}

		// No timeout was detected, check resp. If it was not ok, try again
		if ((__senders_address != iAdrs) || 
			(__lp == 0)			||
			(__iRespLen != 4)	||
			(szDevResponse[0] != 'T')	||
			(szDevResponse[1] != 'E')	||
			(szDevResponse[2] != 'R')	||
			(szDevResponse[3] != 'M')) // Check both for address-match and response-length
		{ 
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount ==  (__XLINK_ATTEMPT_RETRY_MAXIMUM__ << 3))
			{
				// We've failed
				*bTimeoutDetected = 8;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;				
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_3;
			}
		}
		
		// If we've reached here it means we've successfully received 'TERM' response, so we're ok
		break;						  
	}

	// Clear RX
	MACRO_XLINK_clear_RX;

	// Reset errors and exit
	*bTimeoutDetected = 0;
	*bDeviceNotResponded = 0;
	return;	
}

// This is used by slave...
void XLINK_SLAVE_wait_transact (char  *data,
							    unsigned int *length,
							    unsigned int  max_len,
							    UL32 transaction_timeout,
							    char  *bTimeoutDetected,
							    char  bWeAreMaster,
								char  bWaitingForCommand)
{
	// This is used by slave only, make sure we are slave..
	if (bWeAreMaster == 1) return;	
	
	// At this stage, the data has been sent, now we must ask Device for results...
	volatile unsigned int iTotalReceived = 0;
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	volatile UL32  iActualTickcount;
	iActualTickcount = MACRO_GetTickCountRet;
	
	volatile char  iBC = 0; // BitCorrector
	volatile char  iTotalRetryCount = 0;
	
	volatile char sziMX[16] = {0,0,0,0,0};
		
	// What is our address?
	volatile char  our_address = __OUR_CPLD_ID;
	volatile short iLoopCounter = 0;
	volatile UL32 vTemp1 = 0;
	volatile UL32 vTemp2 = 0;
	volatile UL32 vTemp3 = 0;
	
	// Respond with checksum
	volatile unsigned int iGeneratedChecksum = 0;
	
	
	// Now we have to wait for data
	while (iTotalReceived < max_len)
	{
		// Wait for OK packet for 20us
		volatile unsigned char iTimeoutDetected = 0;
		volatile unsigned char szResp[4];
		volatile unsigned int  __iRespLen = 0;
		volatile unsigned char __senders_address = 0;
		volatile unsigned char __lp = 0;
		volatile unsigned char __bc = 0;
		
		// Clear the buffer
		szResp[0] = 0;
		szResp[1] = 0;
		szResp[2] = 0;
		szResp[3] = 0;
			
		MACRO_XLINK_wait_packet(szResp,
								__iRespLen,
								__XLINK_WAIT_PACKET_TIMEOUT__,  // For the first packet (only), we allow 120us delay
								iTimeoutDetected,
								__senders_address,
								__lp,
								__bc);
			
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iActualTickcount;
		if (vTemp3 > transaction_timeout)
		{
			*bTimeoutDetected = 1;
			return;
		}
			
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			*bTimeoutDetected = 1;
			return;
		}
			
		// No timeout was detected, check resp. If it was not ok, try again
		if (__iRespLen == 0) // Check both for address-match and response-length
		{
			// Ok we've timed out, try for several times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 1;
				return;
			}
			else
			{
				iTotalRetryCount++;
				continue;
			}
		}
		
		// Note that address 255 is always accepted! (it's a general dispatch)
		if (__senders_address == our_address)
		{
			// Ok, if not, we ignore...
		}
		else
		{
			// Ignore the packet!
			iTotalRetryCount++;			
			continue; 
		}
		
		// SPECIAL CASE: Is it a 'TERM' signal? If so, simply reply with TERM and ignore the whole story and continue the loop
		if ((iTotalReceived == 0) && (bWaitingForCommand == TRUE)) // Meaning that this is the first packet received...
		{
			if ((__iRespLen == 4) &&
				(szResp[0] == 'T') &&
				(szResp[1] == 'E') &&
				(szResp[2] == 'R') &&
				(szResp[3] == 'M'))
			{
				// Clear the Input buffer first
				MACRO_XLINK_clear_RX;				
				
				// Reply with TERM and repeat the loop
				MACRO_XLINK_send_packet(__OUR_CPLD_ID,"TERM",4, 1, 0);
				continue;
			}
			
			if ((__iRespLen == 3) &&
				(szResp[0] == 'Z') &&
				(szResp[1] == 'A') &&
				(szResp[2] == 'X'))
			{
				// Clear the Input buffer first
				MACRO_XLINK_clear_RX;
							
				// Reply with TERM and repeat the loop
				MACRO_XLINK_send_packet(__OUR_CPLD_ID,"ECHO",4, TRUE, FALSE);
				continue;
			}
		}
		
		// Check the BitCorrector:
		// If it's not what we expected it to be, then we need to go back and rewrite the bytes. We won't ignore it however
		/*if (__bc != iBC) // Bit corrector not matched. If we can, we need to roll-back the address
		{
			if (iTotalReceived > 3) iTotalReceived -= 4;
		}*/	
		
		// Reset the Checksum
		iGeneratedChecksum = 0;		
			
		// Resp was OK... Take the data
		if ((__iRespLen >= 1) && (iTotalReceived + 1 <= max_len))
		{
			 data[iTotalReceived] = szResp[0];
			 iGeneratedChecksum += szResp[0];			 
			 iTotalReceived += 1;
		}			 
		
		if ((__iRespLen >= 2) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[1];
			iGeneratedChecksum  += szResp[1];			
			iTotalReceived += 1;
		}
					
		if ((__iRespLen >= 3) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[2];
			iGeneratedChecksum  += szResp[2];			
			iTotalReceived += 1;
		}			
		
		if ((__iRespLen >= 4) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[3];
			iGeneratedChecksum  += szResp[3];			
			iTotalReceived += 1;
		}			
		// We've sent 4 more bytes
		iBC = (iBC == 0) ? 1 : 0; // Flip BitCorrector
		
		// Clear the input buffer first
		MACRO_XLINK_clear_RX;
		
		// At this point, sent an OK back to sender
		sziMX[0] = 'A';
		sziMX[1] = 'R';
		sziMX[2] = (((iGeneratedChecksum & 0x0FF00) >> 8) & 0x0FF);
		sziMX[3] = (iGeneratedChecksum & 0x0FF);
		//sziMX[2] = 'T';
		//sziMX[3] = 'X';
		
		MACRO_XLINK_send_packet(__OUR_CPLD_ID,sziMX, 4, 1, 0);
		
		// Was it the last packet? if so, exit the loop
		if (__lp == 1) break;
	}		
		
	// OK, We've come out
	// there is nothing special to do...
	*bTimeoutDetected = 0;
	*length = iTotalReceived;
	return;								
}

void XLINK_SLAVE_respond_transact  (char  *data,
									unsigned int length,
									UL32 transaction_timeout,
									char  *bTimeoutDetected,
									char  bWeAreMaster)
{
	// Wait for PUSH Command. Also remember, the first PUSH must have BitCorrector 0.
	// This is used by slave only, make sure we are slave..
	if (bWeAreMaster == 1) return;
	
	// What address do we used?
	volatile char iAddressToUseForTransactions = XLINK_get_cpld_id();
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	volatile unsigned int iActualTickcount = MACRO_GetTickCountRet;
	volatile unsigned int iTotalSent = 0;
	volatile unsigned int iBytesToSend = 0;
	volatile char iBC = 1;	 // BitCorrector is ONE, since the previous part has already set it to one!
	volatile char iTotalRetryCount = 0;
	
	volatile char szPrevData[4];
	volatile char iPrevDataLen = 0;
	volatile char iPrevLP = 0;
	volatile char iPrevBC = 0;
	
	volatile UL32 vTemp1;
	volatile UL32 vTemp2;
	volatile UL32 vTemp3;
	
	// Now we have to wait for data
	while (1)
	{
		// Wait for OK packet for 20us
		volatile char iTimeoutDetected = 0;
		volatile char szResp[4];
		volatile unsigned int __iRespLen = 0;
		volatile char __senders_address = 0;
		volatile char __lp = 0;
		volatile char __bc = 0;
		
RETRY_POINT_1:

		// Clear szResp
		szResp[0] = 0;
		szResp[1] = 0;
		szResp[2] = 0;
		szResp[3] = 0;
		
	   MACRO_XLINK_wait_packet(szResp,
							  __iRespLen,
							  __XLINK_WAIT_PACKET_TIMEOUT__,  // We wait for 200us
							  iTimeoutDetected,
							  __senders_address,
							  __lp,
							  __bc);
		
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iActualTickcount;
		if (vTemp3 > transaction_timeout)
		{
			*bTimeoutDetected = 1;
			MACRO_XLINK_clear_RX;			
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected == TRUE)
		{
			// Did we time out?
			if (iTotalRetryCount > __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// Ok we've timed out, try for 3 times
				*bTimeoutDetected = 1;
				MACRO_XLINK_clear_RX;
				return;	
			}
			else
			{
				iTotalRetryCount++;
				continue;
			}
		}
		
		// No timeout was detected, check resp. If it was not ok, try again
		if (__iRespLen == 0) // Check both for address-match and response-length
		{
			// Invalid command!...
			// Probably we have to abort...
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 2;
				MACRO_XLINK_clear_RX;				
				return;			
			}
			else
			{
				// We will try again
				iTotalRetryCount++;
				goto RETRY_POINT_1;
			}
		}
		
		// Check for Host Response Data
		if ((szResp[0] != 'P') || 
			(szResp[1] != 'U') || 
			(szResp[2] != 'S') || 
			(szResp[3] != 'H') ||
		    (__iRespLen != 4))
		{
			// Invalid command!...
			// Probably we have to abort...
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 3;
				MACRO_XLINK_clear_RX;				
				return;		
			}
			else
			{
				// Before doing anything, clear the CPLD
				MACRO_XLINK_clear_RX;
				
				// We will respond with 'OK' (perhaps the host has not received the OK from the time the first command was executed)
				MACRO_XLINK_send_packet(iAddressToUseForTransactions, "AR99", 4, TRUE, !iBC); // BitCorrector must never be correct for this				
				
				// We'll try again as well
				iTotalRetryCount++;
				goto RETRY_POINT_1;
			}
		}
		
		// OK, we either have to send new data, or old data (if BitCorrector doesn't match actual)
		if (__bc != iBC)
		{
			// Before doing anything, clear the CPLD
			MACRO_XLINK_clear_RX;
			
			// This means we have to send old data and continue the loop
			MACRO_XLINK_send_packet(iAddressToUseForTransactions, szPrevData, iPrevDataLen, iPrevLP, iPrevBC);
		}
		else // We send new data
		{
			// OK we send new data
			iBytesToSend = length - iTotalSent;
			if (iBytesToSend > 4) iBytesToSend = 4;
			iPrevLP = (iBytesToSend + iTotalSent >= length) ? 1 : 0;
			iPrevBC = iBC;
			iPrevDataLen = iBytesToSend;
			
			// Set previous data
			if (iBytesToSend >= 1) szPrevData[0] = data[iTotalSent+0];
			if (iBytesToSend >= 2) szPrevData[1] = data[iTotalSent+1];
			if (iBytesToSend >= 3) szPrevData[2] = data[iTotalSent+2];
			if (iBytesToSend == 4) szPrevData[3] = data[iTotalSent+3];
		
			// Before doing anything, clear the CPLD
			MACRO_XLINK_clear_RX;	
			
			// Now send the data
			MACRO_XLINK_send_packet(iAddressToUseForTransactions,szPrevData, iBytesToSend, iPrevLP, iBC);				
			
			// Flip the BitCorrector
			iBC = (iBC == 1) ? 0 : 1;
			iTotalSent += iBytesToSend;
			
			// Was it the last packet? If so, break the loop
			if (iPrevLP == 1) break;
		}
	}
	
	// Reset the retry count
	iTotalRetryCount = 0;
	
	// OK, we now here wait for TERM signal
	// Now we have to wait for data
	while (1)
	{
		// Wait for OK packet for 20us
		volatile char iTimeoutDetected = 0;
		volatile char szResp[4];
		volatile unsigned int  __iRespLen = 0;
		volatile char __senders_address = 0;
		volatile char __lp = 0;
		volatile char __bc = 0;
		
RETRY_POINT_2:
		
		// Response clearing
		szResp[0] = 0; szResp[1] = 0; szResp[2] = 0;szResp[3] = 0;
		
		// Wait for the packet to arrive
		MACRO_XLINK_wait_packet(szResp,
							  __iRespLen,
							  __XLINK_WAIT_PACKET_TIMEOUT__,  // 200us Timeout
							  iTimeoutDetected,
							  __senders_address,
							  __lp,
							  __bc);
		
		// Check master timeout
		vTemp1 = MACRO_GetTickCountRet;
		vTemp3 = vTemp1 - iActualTickcount;
		if (vTemp3 > transaction_timeout)
		{
			*bTimeoutDetected = 4;
			MACRO_XLINK_clear_RX;
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			*bTimeoutDetected = 5;
			MACRO_XLINK_clear_RX;
			return;
		}
		
		// No timeout was detected, check resp. If it was not ok, try again
		if (__iRespLen == 0) // Check both for address-match and response-length
		{
			// Invalid command!...
			// Probably we have to abort...
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 6;
				MACRO_XLINK_clear_RX;				
				return;
			}
			else
			{
				// We will try again
				iTotalRetryCount++;
			}
		}
		
		// Check for Host Response Data
		if ((szResp[0] != 'T') ||
			(szResp[1] != 'E') ||
			(szResp[2] != 'R') ||
			(szResp[3] != 'M'))
		{
			// Is it PUSH? It must be regarding previous transaction. Probably the host didn't receive the packet
			// correctly. Send the last packet again. Of course, we do apply 20 attempt limit here
			if ((szResp[0] == 'P') &&
				(szResp[1] == 'U') &&
				(szResp[2] == 'S') &&
				(szResp[3] == 'H'))
			{
				if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__) // for TERM it's 20 retries
				{
					// We've failed
					*bTimeoutDetected = 7;
					MACRO_XLINK_clear_RX;					
					return;
				}
				else
				{
					// Resend the packet
					MACRO_XLINK_clear_RX;					
					MACRO_XLINK_send_packet(iAddressToUseForTransactions, szPrevData, iPrevDataLen, iPrevLP, iPrevBC);
					
					// We will try again
					iTotalRetryCount++;
					goto RETRY_POINT_2;
				}
			}
			else // It was not push either... OK never mind, we'll give it another try
			{
				if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__) // for TERM it's 20 retries
				{
					// We've failed
					*bTimeoutDetected = 8;
					MACRO_XLINK_clear_RX;						
					return;
				}
				else
				{
					// We will try again
					iTotalRetryCount++;
					goto RETRY_POINT_2;
				}
			}
		}
		
		// Before doing anything, clear the CPLD
		MACRO_XLINK_clear_RX;	
		
		// OK we've received our 
		MACRO_XLINK_send_packet(iAddressToUseForTransactions, "TERM", 4, 1, 0);
		break; 
				
		// Now here is the catch: If the host won't received the TERM signal, it will resend it. The next TERM packet
		// will be received by our SLAVE_wait_transact. That function simply responds with 'TERM'. So everybody will be happy...
	}
	
	// OK, We've come out
	// there is nothing special to do...
	// XLINK_clear_RX();
	*bTimeoutDetected = 0;
	return;										
}

char XLINK_data_inbound(void)
{
	return ((XLINK_get_RX_status() & CPLD_RX_STATUS_DATA) == CPLD_RX_STATUS_DATA);
}	
							  
void  XLINK_set_cpld_id(char iID)
{
	__AVR32_CPLD_SetAccess();
	char iValueToSet = 0;
	MACRO__AVR32_CPLD_Read(iValueToSet,CPLD_ADDRESS_MASTER_CONTROL);
	iValueToSet &= ~(0b0111110);
	iValueToSet |= ((iID & 0b011111) << 1);	
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);	
	__OUR_CPLD_ID = iID;
}

char XLINK_get_cpld_id(void)
{
	return __OUR_CPLD_ID;
}

void  XLINK_set_cpld_master(char bMaster)
{
	__AVR32_CPLD_SetAccess();	
	char iValueToSet = 0;
	MACRO__AVR32_CPLD_Read(iValueToSet, CPLD_ADDRESS_MASTER_CONTROL);
	iValueToSet &= ~(CPLD_MASTER_CONTROL_MSTR);
	if (bMaster) iValueToSet |= CPLD_MASTER_CONTROL_MSTR;
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);
}

void  XLINK_set_cpld_passthrough(char bPassthrough)
{
	__AVR32_CPLD_SetAccess();
	char iValueToSet = 0;
	MACRO__AVR32_CPLD_Read(iValueToSet, CPLD_ADDRESS_MASTER_CONTROL);
	iValueToSet &= ~(CPLD_MASTER_CONTROL_PASSTHROUGH);
	if (bPassthrough == TRUE) iValueToSet |= CPLD_MASTER_CONTROL_PASSTHROUGH;	
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);	
}

char XLINK_get_chain_status(void)
{
	__AVR32_CPLD_SetAccess();
	char retval = 0;	
	MACRO__AVR32_CPLD_Read(retval,CPLD_ADDRESS_CHAIN_STATUS);
	return retval;
}

char XLINK_get_TX_status(void)
{
	__AVR32_CPLD_SetAccess();		
	char retval = 0;	
	MACRO__AVR32_CPLD_Read(retval, CPLD_ADDRESS_TX_STATUS);
	return retval;
}

char XLINK_get_RX_status(void)
{
	__AVR32_CPLD_SetAccess();
	char retval = 0;		
	MACRO__AVR32_CPLD_Read(retval, CPLD_ADDRESS_RX_STATUS);		
	return retval;
}

void XLINK_set_target_address(char uAdrs)
{
	__AVR32_CPLD_SetAccess();	
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_TARGET_ADRS, uAdrs & 0b011111);
}

char XLINK_get_target_address(void)
{
	__AVR32_CPLD_SetAccess();
	char retVal = 0;
	MACRO__AVR32_CPLD_Read(retVal, CPLD_ADDRESS_TX_TARGET_ADRS);
	return retVal;
}

void XLINK_clear_RX(void)
{
	__AVR32_CPLD_SetAccess();											
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);
}

int	XLINK_is_cpld_present(void)
{
	__AVR32_CPLD_SetAccess();
	char retVal = 0;
	MACRO__AVR32_CPLD_Read(retVal,CPLD_ADDRESS_IDENTIFICATION);
	return (retVal == 0x0A4);
}

char XLINK_get_device_status()
{
	return XLINK_Device_Status;
}

void XLINK_set_device_status(char iDevState)
{
	XLINK_Device_Status = iDevState;
}

void XLINK_set_outbox(char* szData, short iLen)
{
	XLINK_Outbox_Length = iLen;
	for (unsigned int x = 0; x < iLen; x++) XLINK_Outbox[x] = szData[x];
}
