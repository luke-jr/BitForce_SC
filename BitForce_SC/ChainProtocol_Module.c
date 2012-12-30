/*
 * ChainProtocol_Module.c
 *
 * Created: 08/10/2012 21:40:04
 *  Author: NASSER
 */ 

// Include standard definitions
#include "std_defs.h"
#include "Generic_Module.h"
#include "ChainProtocol_Module.h"
#include <string.h>

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


// Initialize
void init_XLINK()
{
	// Init CPLD XLINK Info
	MCU_CPLD_SetAccess();
	MCU_CPLD_Initialize();
	XLINK_set_cpld_passthrough(0); // Disable pass-through
	XLINK_set_cpld_id(0); // Initialize our ID
	XLINK_chain_device_count = 0;
}

// Detect if we are real master or not?
#define CHAIN_IN_BIT	0b0000010
#define CHAIN_OUT_BIT	0b0000001

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
	
}

// iAdrs = Target
// szData and iLen is the data stack
void XLINK_send_packet(char iAdrs, char* szData, unsigned short iLen, char LP, char BC)
{
	// Set CPLD Access
	MCU_CPLD_SetAccess();
	
	// Set the Target Address
	XLINK_set_target_address(iAdrs);
		
	// We send data 4byte by 4byte
	// First check TX Status
	volatile char iActualTXStatus = XLINK_get_TX_status();
	unsigned short iTimeoutHolder;
	
	// Are we in progress?
	iTimeoutHolder = GetTickCount();
	
	// Wait until actual buffer is sent...
	if ((iActualTXStatus & CPLD_TX_STATUS_TxInProg) != 0)
	{
		// Wait for 25uS
		while (GetTickCount() - iTimeoutHolder < 25); 
		
		// Did we timeout?
		if (GetTickCount() - iTimeoutHolder >= 25)
		{
			// Ok we've timed out....
			// TODO: We must do something important
		}		
		
		// This is not usual however, if we have a pending data
		// It means we're out of sync... Otherwise we would not have had any data pending...!!!!!
	}
	
	// We're ready to send...
	// Send data in 4byte packets. Each time we send, we wait for a reception of response package
	// Target device must respond with 'OK' and have its correct address in our packet.
	// Should we not receive the correct data, we'll retransmit the previous packet. This will be repeated
	// for 5 times until we realize that we're out of sync...
	// BitCorrect will handle the flip-flop correction indicator
	
	// Set the target device
	XLINK_set_target_address(iAdrs);
	
	// *** IREG - TX Control (8 Bit)
	//
	// MSB                                             LSB
	// +-----------+--------+----+----------------+------+
	// |BitCorrect | XXXXXX | LP | Length (3Bits) | SEND |
	// +-----------+--------+----+----------------+------+
	//      7         6..5     4        3 .. 1        0
	
	// Also bitStuff position
	unsigned short iTotalToSend = iLen;

	// Set the packet length
	char x_plen = (iTotalToSend > 4) ? 4 : iTotalToSend;
				
	// Set Data
	MCU_CPLD_Write(CPLD_ADDRESS_TX_BUF_BEGIN, szData[0]);
	if (x_plen > 1) MCU_CPLD_Write(CPLD_ADDRESS_TX_BUF_BEGIN+1, szData[1]);
	if (x_plen > 2) MCU_CPLD_Write(CPLD_ADDRESS_TX_BUF_BEGIN+2, szData[2]);
	if (x_plen > 3) MCU_CPLD_Write(CPLD_ADDRESS_TX_BUF_BEGIN+3, szData[3]);
		
	// What's the value to write to our TX Control
	char iTxControlVal = 0b00000001;	// Send bit is set by default
	iTxControlVal |= (x_plen << 1);		//
	if (LP) iTxControlVal |= CPLD_TX_CONTROL_LP;
		
	// Set BitStuff value
	iTxControlVal |= (BC != 0) ? CPLD_TX_CONTROL_BC : 0;
	
	// Send packet info
	MCU_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal);
			
	// Wait until send is completed
	volatile int iTimeoutGuard = 0; // Maximum overflow is 5000 increments
	
POINTX:	
	while (((MCU_CPLD_Read(CPLD_ADDRESS_TX_STATUS) & CPLD_TX_STATUS_TxInProg) != 0) && (iTimeoutGuard++ < 10000));
			
}

//////////////////////////////////////////////////////////////////////////////
// AUXILIARY FUNCTIONS
//////////////////////////////////////////////////////////////////////////////
void XLINK_wait_packet (char  *data,
						unsigned int *length,
						long  time_out,
						char  *timeout_detected,
						char  *senders_address,
						char  *LP,
						char  *BC)
{
	// Wait to something...
	MCU_CPLD_SetAccess();
	
	// Reset all variables
	*BC = 0;
	*LP = 0;
	*timeout_detected = FALSE;
	*length = 0;
	*senders_address = 0;
	
	// Wait until RX Data is 1
	// We send data 4byte by 4byte
	// First check TX Status
	volatile char  iActualRXStatus = XLINK_get_RX_status();
	long iTimeoutHolder;

	// Are we in progress?
	iTimeoutHolder = GetTickCount();

	// Wait until actual buffer is sent...
	if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0)
	{
		
		// Wait for 25uS
		while (GetTickCount() - iTimeoutHolder < time_out)
		{
			iActualRXStatus = XLINK_get_RX_status();
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break;
		}
		
		// Did we timeout?
		if (GetTickCount() - iTimeoutHolder >= time_out)
		{
			// Ok we've timed out....
			// TODO: We must do something important
			*timeout_detected = 1; // Means we've timed out
			*length		= 0;
			*senders_address = 0;
			return;
		}
		
		// This is not usual however, if we have a pending data
		// It means we're out of sync... Otherwise we would not have had any data pending...!!!!!
	}
	
	// We've received data
	volatile char imrLen = ((iActualRXStatus & 0b0111000) >> 3);
	*length = imrLen;
	*LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0;
	*BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0;
	*senders_address = MCU_CPLD_Read(CPLD_ADDRESS_SENDERS_ADRS);
	
	for (char ux = 0; ux < imrLen; ux++)
	{
		data[ux] = MCU_CPLD_Read(CPLD_ADDRESS_RX_BUF_BEGIN + ux);
	}
	
	// Clear READ buffer
	MCU_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);
	
	// OK we have all we need
}

// This is used by master only...
void XLINK_MASTER_transact(char   iAdrs, 
					       char*  szData, 
					       unsigned short  iLen,
						   char*  szResp,
						   unsigned short* response_length,
						   unsigned short  iMaxRespLen,
						   long    transaction_timeout, // Master timeout
						   char   *bDeviceNotResponded, // Device did not respond, even to the first packet
						   char   *bTimeoutDetected, // Was a timeout detected?
					       char   bWeAreMaster)
{
	// First check if we are master
	if (bWeAreMaster == 0) return;
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	volatile long iActualTickcount = GetTickCount();
	volatile unsigned short iTotalSent = 0;
	volatile char  iBytesToSend = 0;
	volatile char  iLP = 0; // LastPacket
	volatile char  iBC = 0; // BitCorrector
	volatile char  iTotalRetryCount = 0; 
		
	// Wait for OK packet for 20us
	volatile char iTimeoutDetected = 0;
	volatile char szDevResponse[4];
	volatile unsigned int  __iRespLen = 0;
	volatile char __senders_address = 0;
	volatile char __lp = 0;
	volatile char __bc = 0;
	
	while (iTotalSent < iLen)
	{
		// Preliminary calculations
		iBytesToSend = ((iLen - iTotalSent) > 4) ? 4 : (iLen - iTotalSent);
		iLP = ((iTotalSent + iBytesToSend >= iLen) ? 1 : 0);
		iTotalRetryCount = 0;

RETRY_POINT_1:
		// Before doing anything, clear the CPLD
		XLINK_clear_RX();
		
		// Send these bytes
		XLINK_send_packet(iAdrs, (char*)(szData + iTotalSent), iBytesToSend, iLP, iBC);
			
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
		XLINK_wait_packet(szDevResponse, 
						  &__iRespLen, 
						  __XLINK_WAIT_PACKET_TIMEOUT__ + ((iTotalSent == 0) ? 100 : 0),  // For the first packet (only), we allow 120us delay 
						  &iTimeoutDetected, 
						  &__senders_address, 
						  &__lp, 
						  &__bc);
						  
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout) 
		{
			*bTimeoutDetected = 1;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			XLINK_clear_RX();			
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 1;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();
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
			(__iRespLen != 4) ||
			(__senders_address != iAdrs)) // Check both for address-match and 'OK' response
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 2;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();				
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
	volatile unsigned short iTotalReceived = 0;
	iTotalRetryCount = 0;
	
	// We have to reset the BitCorrector, as it will start from 0 for the PUSH part
	iBC = FALSE;
		
	// Now we have to wait for data
	while (iTotalReceived < iMaxRespLen)
	{
		// Send push command
		
RETRY_POINT_2:
		// Before doing anything, clear the CPLD
		XLINK_clear_RX();
		
		// Proceed
		XLINK_send_packet(iAdrs,"PUSH", 4, iLP, iBC);
		
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
		XLINK_wait_packet(szDevResponse, 
						  &__iRespLen,
						  __XLINK_WAIT_PACKET_TIMEOUT__ + ((iTotalReceived == 0) ? 100 : 0),  // For the first packet (only), we allow 120us delay
						  &iTimeoutDetected,
						  &__senders_address,
						  &__lp,
						  &__bc);
		
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout)
		{
			*bTimeoutDetected = 3;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			XLINK_clear_RX();
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected || (__iRespLen == 0))
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 4;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();				
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_2;
			}
		}
		
		// No timeout was detected, check resp. If it was not ok, try again
		if (__senders_address != iAdrs || __iRespLen == 0) // Check both for address-match and response-length
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 5;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();				
				return;
			}
			else
			{
				iTotalRetryCount++;
				goto RETRY_POINT_2;
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
		
		// Was it the last packet? if so, exit the loop
		if (__lp == 1) break;				
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
		XLINK_clear_RX();
		
		// Proceed
		XLINK_send_packet(iAdrs,"TERM", 4, iLP, 0);
			
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
		XLINK_wait_packet(szDevResponse,
						  &__iRespLen,
						  __XLINK_WAIT_PACKET_TIMEOUT__,  // For the first packet (only), we allow 120us delay
						  &iTimeoutDetected,
						  &__senders_address,
						  &__lp,
						  &__bc);
						  
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout)
		{
			*bTimeoutDetected = 6;
			if (iTotalSent == 0) *bDeviceNotResponded = 1;
			
			// Before doing anything, clear the CPLD
			XLINK_clear_RX();
			return;
		}

		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 7;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();			
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
			if (iTotalRetryCount == __XLINK_ATTEMPT_RETRY_MAXIMUM__)
			{
				// We've failed
				*bTimeoutDetected = 8;
				*bDeviceNotResponded = (iTotalSent == 0) ? 1 : 0;
				
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();				
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
	XLINK_clear_RX();

	// Reset errors and exit
	*bTimeoutDetected = 0;
	*bDeviceNotResponded = 0;
	return;	
}


// This is used by slave...
void XLINK_SLAVE_wait_transact (char  *data,
							    unsigned int *length,
							    unsigned int  max_len,
							    long transaction_timeout,
							    char  *bTimeoutDetected,
							    char  bWeAreMaster)
{
	// This is used by slave only, make sure we are slave..
	if (bWeAreMaster == 1) return;	
	
	// At this stage, the data has been sent, now we must ask Device for results...
	volatile unsigned short iTotalReceived = 0;
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	volatile long  iActualTickcount = GetTickCount();
	volatile char  iBC = 0; // BitCorrector
	volatile char  iTotalRetryCount = 0;
	
	char sziMX[16] = {0,0,0,0,0};
		
	// What is our address?
	volatile char  our_address = __OUR_CPLD_ID;
	volatile short iLoopCounter = 0;
	
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
			
		XLINK_wait_packet(szResp,
						  &__iRespLen,
						  __XLINK_WAIT_PACKET_TIMEOUT__,  // For the first packet (only), we allow 120us delay
						  &iTimeoutDetected,
						  &__senders_address,
						  &__lp,
						  &__bc);
			
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout)
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
		if (iTotalReceived == 0) // Meaning that this is the first packet received...
		{
			if ((__iRespLen == 4) &&
				(szResp[0] == 'T') &&
				(szResp[1] == 'E') &&
				(szResp[2] == 'R') &&
				(szResp[3] == 'M'))
			{
				// Clear the Input buffer first
				XLINK_clear_RX();				
				
				// Reply with TERM and repeat the loop
				XLINK_send_packet(__OUR_CPLD_ID,"TERM",4, 1, 0);
				continue;
			}
		}
			
		// Resp was OK... Take the data
		if ((__iRespLen >= 1) && (iTotalReceived + 1 <= max_len))
		{
			 data[iTotalReceived] = szResp[0];
			 iTotalReceived += 1;
		}			 
		
		if ((__iRespLen >= 2) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[1];
			iTotalReceived += 1;
		}
					
		if ((__iRespLen >= 3) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[2];
			iTotalReceived += 1;
		}			
		
		if ((__iRespLen >= 4) && (iTotalReceived + 1 <= max_len)) 
		{
			data[iTotalReceived] = szResp[3];
			iTotalReceived += 1;
		}			
		// We've sent 4 more bytes
		iBC = (iBC == 0) ? 1 : 0; // Flip BitCorrector
		
		// Clear the input buffer first
		XLINK_clear_RX();
		
		// At this point, sent an OK back to sender
		sziMX[0] = 'A';
		sziMX[1] = 'R';
		sprintf(sziMX+2, "%02X", iLoopCounter++);
		
		XLINK_send_packet(__OUR_CPLD_ID,sziMX, 4, 1, 0);
		
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
									long transaction_timeout,
									char  *bTimeoutDetected,
									char  bWeAreMaster)
{
	// Wait for PUSH Command. Also remember, the first PUSH must have BitCorrector 0.
	// This is used by slave only, make sure we are slave..
	if (bWeAreMaster == 1) return;
	
	// This is how we do it, we start sending packets and we wait for response.
	// Each time we wait for 20us for reply. Should the device not respond, we abort the transaction
	unsigned int iActualTickcount = GetTickCount();
	unsigned short iTotalSent = 0;
	char  iBytesToSend = 0;
	char  iBC = 0; // BitCorrector
	char  iTotalRetryCount = 0;
	
	char  szPrevData[4];
	char  iPrevDataLen = 0;
	char  iPrevLP = 0;
	char  iPrevBC = 0;
	
	// Now we have to wait for data
	while (1)
	{
		// Wait for OK packet for 20us
		char iTimeoutDetected = 0;
		char szResp[4];
		unsigned int  __iRespLen = 0;
		char __senders_address = 0;
		char __lp = 0;
		char __bc = 0;
		
RETRY_POINT_1:

		// Clear szResp
		szResp[0] = 0;
		szResp[1] = 0;
		szResp[2] = 0;
		szResp[3] = 0;
		
		XLINK_wait_packet(szResp,
						  &__iRespLen,
						  __XLINK_WAIT_PACKET_TIMEOUT__,  // We wait for 200us
						  &iTimeoutDetected,
						  &__senders_address,
						  &__lp,
						  &__bc);
		
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout)
		{
			*bTimeoutDetected = 1;
			XLINK_clear_RX();			
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			*bTimeoutDetected = 1;
			XLINK_clear_RX();
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
				*bTimeoutDetected = 1;
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
				*bTimeoutDetected = 1;
				XLINK_clear_RX();				
				return;		
			}
			else
			{
				// Before doing anything, clear the CPLD
				XLINK_clear_RX();
				
				// We will respond with 'OK' (perhaps the host has not received the OK from the time the first command was executed)
				XLINK_send_packet(__OUR_CPLD_ID, "AR99", 4, TRUE, 0);				
				
				// We'll try again as well
				iTotalRetryCount++;
				goto RETRY_POINT_1;
			}
		}
		
		// Reset retry count
		iTotalRetryCount = 0;
		
		// OK, we either have to send new data, or old data (if BitCorrector doesn't match actual)
		if (__bc != iBC)
		{
			// Before doing anything, clear the CPLD
			XLINK_clear_RX();
			
			// This means we have to send old data and continue the loop
			XLINK_send_packet(__OUR_CPLD_ID, szPrevData, iPrevDataLen, iPrevLP, iPrevBC);
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
			XLINK_clear_RX();	
			
			// Now send the data
			XLINK_send_packet(__OUR_CPLD_ID,szPrevData, iBytesToSend, iPrevLP, 0);				
			
			// Flip the BitCorrector
			iBC = (iBC == 1) ? 0 : 1;
			iTotalSent += iBytesToSend;
			
			// Was it the last packet? If so, break the loop
			if (iPrevLP == 1) break;
		}
	}
	
	// OK, we now here wait for TERM signal
	// Now we have to wait for data
	while (1)
	{
		// Wait for OK packet for 20us
		char iTimeoutDetected = 0;
		char szResp[4];
		unsigned int  __iRespLen = 0;
		char __senders_address = 0;
		char __lp = 0;
		char __bc = 0;
		
RETRY_POINT_2:
		
		XLINK_wait_packet(szResp,
						  &__iRespLen,
						  __XLINK_WAIT_PACKET_TIMEOUT__,  // 200us Timeout
						  &iTimeoutDetected,
						  &__senders_address,
						  &__lp,
						  &__bc);
		
		// Check master timeout
		if (GetTickCount() - iActualTickcount > transaction_timeout)
		{
			*bTimeoutDetected = 1;
			XLINK_clear_RX();
			return;
		}
		
		// Check for issues
		if (iTimeoutDetected)
		{
			// Ok we've timed out, try for 3 times
			*bTimeoutDetected = 1;
			XLINK_clear_RX();
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
				*bTimeoutDetected = 1;
				XLINK_clear_RX();				
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
					*bTimeoutDetected = 1;
					XLINK_clear_RX();					
					return;
				}
				else
				{
					// Resend the packet
					XLINK_clear_RX();					
					XLINK_send_packet(__OUR_CPLD_ID, szPrevData, iPrevDataLen, iPrevLP, iPrevBC);
					
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
					*bTimeoutDetected = 1;
					XLINK_clear_RX();						
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
		
		// Reset retry count
		iTotalRetryCount = 0;

		// Before doing anything, clear the CPLD
		XLINK_clear_RX();		
		
		// OK we've received our 
		XLINK_send_packet(__OUR_CPLD_ID, "TERM", 4, 1, 0);
		break; 
				
		// Now here is the catch: If the host won't received the TERM signal, it will resend it. The next TERM packet
		// will be received by our SLAVE_wait_transact. That function simply responds with 'TERM'. So everybody will be happy...
	}
	
	// OK, We've come out
	// there is nothing special to do...
	XLINK_clear_RX();
	*bTimeoutDetected = 0;
	return;										
}



char XLINK_data_inbound(void)
{
	return ((XLINK_get_RX_status() & CPLD_RX_STATUS_DATA) == CPLD_RX_STATUS_DATA);
}	
							  
void  XLINK_set_cpld_id(char iID)
{
	MCU_CPLD_SetAccess();
	char iValueToSet = MCU_CPLD_Read(CPLD_ADDRESS_MASTER_CONTROL) & ~(0b0111110);
	iValueToSet |= ((iID & 0b011111) << 1);	
	MCU_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);	
	__OUR_CPLD_ID = iID;
}

char XLINK_get_cpld_id(void)
{
	return __OUR_CPLD_ID;
}

void  XLINK_set_cpld_master(char bMaster)
{
	MCU_CPLD_SetAccess();	
	char iValueToSet = MCU_CPLD_Read(CPLD_ADDRESS_MASTER_CONTROL) & ~(CPLD_MASTER_CONTROL_MSTR);
	if (bMaster) iValueToSet |= CPLD_MASTER_CONTROL_MSTR;
	MCU_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);
}

void  XLINK_set_cpld_passthrough(char bPassthrough)
{
	MCU_CPLD_SetAccess();
	char iValueToSet = MCU_CPLD_Read(CPLD_ADDRESS_MASTER_CONTROL) & ~(CPLD_MASTER_CONTROL_PASSTHROUGH);
	iValueToSet |= CPLD_MASTER_CONTROL_PASSTHROUGH;	
	MCU_CPLD_Write(CPLD_ADDRESS_MASTER_CONTROL, iValueToSet);	
}

char XLINK_get_chain_status(void)
{
	MCU_CPLD_SetAccess();	
	return MCU_CPLD_Read(CPLD_ADDRESS_CHAIN_STATUS);
}

char XLINK_get_TX_status(void)
{
	MCU_CPLD_SetAccess();		
	return MCU_CPLD_Read(CPLD_ADDRESS_TX_STATUS);
}

char XLINK_get_RX_status(void)
{
	MCU_CPLD_SetAccess();	
	return MCU_CPLD_Read(CPLD_ADDRESS_RX_STATUS);		
}

void  XLINK_set_target_address(char uAdrs)
{
	MCU_CPLD_SetAccess();	
	MCU_CPLD_Write(CPLD_ADDRESS_TX_TARGET_ADRS, uAdrs & 0b011111);
}

void  XLINK_clear_RX(void)
{
	MCU_CPLD_SetAccess();											
    while ((MCU_CPLD_Read(CPLD_ADDRESS_RX_STATUS) & CPLD_RX_STATUS_DATA) == CPLD_RX_STATUS_DATA) 
		MCU_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);
}

int	  XLINK_is_cpld_present(void)
{
	MCU_CPLD_SetAccess();
	return (MCU_CPLD_Read(CPLD_ADDRESS_IDENTIFICATION) == 0x0A4);		
}

