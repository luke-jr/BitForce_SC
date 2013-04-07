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
extern buf_job_result_packet __buf_job_results[PIPE_MAX_BUFFER_DEPTH];
extern char __buf_job_results_count;  // Total of results in our __buf_job_results
static char _prev_job_was_from_pipe = FALSE;

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
	char szInfoReq[4024];
	char szTemp[256];
	
	strcpy(szInfoReq,"DEVICE: BitFORCE SC\n");
	strcpy(szTemp,"");
	
	// Add Firmware Identification
	sprintf(szTemp,"FIRMWARE: %s\n", __FIRMWARE_VERSION);
	strcat(szInfoReq, szTemp);
	
	volatile UL32 uL1   = 0;    
	volatile UL32 uL2   = 0;    
	volatile UL32 uLRes = 0;    
	
	// Atomic Full-Asm Special CPLD Write latency
	unsigned int iLM[16];
	unsigned int iNonceCount;
	
	//ASIC_is_processing();
	//ASIC_job_issue(&jp,0,0xFFFFFFFF);
	//ASIC_get_job_status(iLM, &iNonceCount);
	//uL1 = MACRO_GetTickCountRet;
	//Flush_buffer_into_engines();
	//ASIC_job_issue(&jp,0,0x0FFFFFFFF);
	//uL2 = MACRO_GetTickCountRet;
	//uLRes = (UL32)((UL32)uL2 - (UL32)uL1);
	//sprintf(szTemp,"MACRO_GetTickCount round-time: %u us\n", (unsigned int)uLRes);
	//strcat(szInfoReq, szTemp);
	
	/*
	// FAN Process latency
	uL1 = MACRO_GetTickCountRet;
	FAN_SUBSYS_IntelligentFanSystem_Spin();
	uL2 = MACRO_GetTickCountRet;
	uLRes = (UL32)((UL32)uL2 - (UL32)uL1);
	sprintf(szTemp,"FAN_SUBSYS_Intelligent Spin roundtime: %u us\n", (unsigned int)uLRes);
	strcat(szInfoReq, szTemp);	

	// Atomic Full-Asm Special CPLD Write latency
	uL1 = MACRO_GetTickCountRet;
	MACRO_XLINK_send_packet(0,"ABCD",4,1,1);
	uL2 = MACRO_GetTickCountRet;
	uLRes = (UL32)((UL32)uL2 - (UL32)uL1);
	sprintf(szTemp,"ATOMIC MACRO SEND PACKET: %u us\n", (unsigned int)uLRes);
	strcat(szInfoReq, szTemp);*/
	
	/*volatile unsigned int iStats[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		
	__MCU_ASIC_Activate_CS();
	
	
	iStats[0]  = __ASIC_ReadEngine(0,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[1]  = __ASIC_ReadEngine(0,1,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[2]  = __ASIC_ReadEngine(0,2,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[3]  = __ASIC_ReadEngine(0,3,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[4]  = __ASIC_ReadEngine(0,4,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[5]  = __ASIC_ReadEngine(0,5,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[6]  = __ASIC_ReadEngine(0,6,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[7]  = __ASIC_ReadEngine(0,7,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[8]  = __ASIC_ReadEngine(0,8,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[9]  = __ASIC_ReadEngine(0,9,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[10] = __ASIC_ReadEngine(0,10,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[11] = __ASIC_ReadEngine(0,11,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[12] = __ASIC_ReadEngine(0,12,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[13] = __ASIC_ReadEngine(0,13,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[14] = __ASIC_ReadEngine(0,14,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[15] = __ASIC_ReadEngine(0,15,ASIC_SPI_READ_STATUS_REGISTER+0);
	
	__MCU_ASIC_Deactivate_CS();
	*/
	/*#define FIFO_ADDRESS_TO_READ 0x80
	#define EXA_CHIP CHIP_TO_TEST
			
	iStats[0]  = __ASIC_ReadEngine(EXA_CHIP,1, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,1,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[1]  = __ASIC_ReadEngine(EXA_CHIP,2, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,2,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[2]  = __ASIC_ReadEngine(EXA_CHIP,3, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,3,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[3]  = __ASIC_ReadEngine(EXA_CHIP,4, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,4,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[4]  = __ASIC_ReadEngine(EXA_CHIP,5, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,5,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[5]  = __ASIC_ReadEngine(EXA_CHIP,6, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,6,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[6]  = __ASIC_ReadEngine(EXA_CHIP,7, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,7,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[7]  = __ASIC_ReadEngine(EXA_CHIP,8, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,8,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[8]  = __ASIC_ReadEngine(EXA_CHIP,9, FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,9,FIFO_ADDRESS_TO_READ+1)  << 16);
	iStats[9]  = __ASIC_ReadEngine(EXA_CHIP,10,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,10,FIFO_ADDRESS_TO_READ+1) << 16);
	iStats[10] = __ASIC_ReadEngine(EXA_CHIP,11,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,11,FIFO_ADDRESS_TO_READ+1) << 16);
	iStats[11] = __ASIC_ReadEngine(EXA_CHIP,12,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,12,FIFO_ADDRESS_TO_READ+1) << 16);
	iStats[12] = __ASIC_ReadEngine(EXA_CHIP,13,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,13,FIFO_ADDRESS_TO_READ+1) << 16);
	iStats[13] = __ASIC_ReadEngine(EXA_CHIP,14,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,14,FIFO_ADDRESS_TO_READ+1) << 16);
	iStats[14] = __ASIC_ReadEngine(EXA_CHIP,15,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(EXA_CHIP,15,FIFO_ADDRESS_TO_READ+1) << 16);
	//iStats[15] = __ASIC_ReadEngine(CHIP_TO_TEST,16,FIFO_ADDRESS_TO_READ+0)   | (__ASIC_ReadEngine(CHIP_TO_TEST,16,FIFO_ADDRESS_TO_READ+1) << 16);
	*/
		
	// Read FSM Status Word 1
	/*
	iStats[0]  = __ASIC_ReadEngine(0,0,0b01100001);
	iStats[1]  = __ASIC_ReadEngine(1,0,0b01100001);
	iStats[2]  = __ASIC_ReadEngine(2,0,0b01100001);
	iStats[3]  = __ASIC_ReadEngine(3,0,0b01100001);
	iStats[4]  = __ASIC_ReadEngine(4,0,0b01100001);
	iStats[5]  = __ASIC_ReadEngine(5,0,0b01100001);
	iStats[6]  = __ASIC_ReadEngine(6,0,0b01100001);
	iStats[7]  = __ASIC_ReadEngine(7,0,0b01100001);*/
	

	
	// Ok Set an engine to reset mode

	/*__ASIC_WriteEngine(CHIP_TO_TEST,0,0,(1<<12) | (1<<13));
	__ASIC_WriteEngine(CHIP_TO_TEST,1,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,2,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,3,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,4,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,5,0,(1<<12));	
	__ASIC_WriteEngine(CHIP_TO_TEST,6,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,7,0,(1<<12));				
	__ASIC_WriteEngine(CHIP_TO_TEST,8,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,9,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,10,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,11,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,12,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,13,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,14,0,(1<<12));
	__ASIC_WriteEngine(CHIP_TO_TEST,15,0,(1<<12));			
								
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
		*/		
	//int fsm_word = 0 ;
	//fsm_word = __ASIC_ReadEngine(CHIP_TO_TEST,0,0b01100001);
	/*
	__ASIC_WriteEngine(CHIP_TO_TEST,0,0,(1<<13));
	__ASIC_WriteEngine(CHIP_TO_TEST,1,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,2,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,3,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,4,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,5,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,6,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,7,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,8,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,9,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,10,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,11,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,12,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,13,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,14,0,0);
	__ASIC_WriteEngine(CHIP_TO_TEST,15,0,0);*/

	/*
	iStats[0]  = __ASIC_ReadEngine(0,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[1]  = __ASIC_ReadEngine(1,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[2]  = __ASIC_ReadEngine(2,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[3]  = __ASIC_ReadEngine(3,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[4]  = __ASIC_ReadEngine(4,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[5]  = __ASIC_ReadEngine(5,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[6]  = __ASIC_ReadEngine(6,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	iStats[7]  = __ASIC_ReadEngine(7,0,ASIC_SPI_READ_STATUS_REGISTER+0);
	*/
	
	/*
	iStats[0]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000);
	iStats[1]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+1);
	iStats[2]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+2);
	iStats[3]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+3);
	iStats[4]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+4);
	iStats[5]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+5);
	iStats[6]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+6);
	iStats[7]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+7);
	iStats[8]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+8);
	iStats[9]  = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+9);
	iStats[10] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+10);
	iStats[11] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+11);
	iStats[12] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+12);
	iStats[13] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+13);
	iStats[14] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+14);
	iStats[15] = __ASIC_ReadEngine(CHIP_TO_TEST,13,0b010000000+15);*/
	
	//__MCU_ASIC_Deactivate_CS();	

	// Say Read-Complete
	
	
	//if ((iStats[0] & 0b01) == 0b01) { ASIC_ReadComplete(CHIP_TO_TEST,0); Reset_Engine(CHIP_TO_TEST,0)
	/*if ((iStats[1] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[1] = (__ASIC_ReadEngine(CHIP_TO_TEST, 1, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 1, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,1);}
	if ((iStats[2] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[2] = (__ASIC_ReadEngine(CHIP_TO_TEST, 2, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 2, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,2);}
	if ((iStats[3] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[3] = (__ASIC_ReadEngine(CHIP_TO_TEST, 3, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 3, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,3);}
	if ((iStats[4] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[4] = (__ASIC_ReadEngine(CHIP_TO_TEST, 4, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 4, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,4);}
	if ((iStats[5] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[5] = (__ASIC_ReadEngine(CHIP_TO_TEST, 5, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 5, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,5);}
	if ((iStats[6] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[6] = (__ASIC_ReadEngine(CHIP_TO_TEST, 6, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 6, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,6);}
	if ((iStats[7] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[7] = (__ASIC_ReadEngine(CHIP_TO_TEST, 7, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 7, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,7);}
	if ((iStats[8] & 0b01) == 0b01)  { __MCU_ASIC_Activate_CS(); iStats[8] = (__ASIC_ReadEngine(CHIP_TO_TEST, 8, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | ((__ASIC_ReadEngine(CHIP_TO_TEST, 8, FIFO_ADDRESS_TO_READ+1) & 0x0FFFF) << 16); __MCU_ASIC_Deactivate_CS();  ASIC_ReadComplete(CHIP_TO_TEST,8);}
	
	if ((iStats[9] & 0b01) == 0b01)  
	{ 
		__MCU_ASIC_Activate_CS(); 
		volatile unsigned int iVX1 =  __ASIC_ReadEngine(CHIP_TO_TEST, 9, FIFO_ADDRESS_TO_READ+1);
		volatile unsigned int iVX2 =  __ASIC_ReadEngine(CHIP_TO_TEST, 9, FIFO_ADDRESS_TO_READ);
		iStats[9] = (__ASIC_ReadEngine(CHIP_TO_TEST, 9,   FIFO_ADDRESS_TO_READ) & 0x0FFFF)  | (__ASIC_ReadEngine(CHIP_TO_TEST, 9, FIFO_ADDRESS_TO_READ+1) << 16); 
		__MCU_ASIC_Deactivate_CS(); 
		ASIC_ReadComplete(CHIP_TO_TEST,9); 
	}
	
	if ((iStats[10] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[10] = (__ASIC_ReadEngine(CHIP_TO_TEST, 10, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 10, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,10);}
	if ((iStats[11] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[11] = (__ASIC_ReadEngine(CHIP_TO_TEST, 11, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 11, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,11);}
	if ((iStats[12] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[12] = (__ASIC_ReadEngine(CHIP_TO_TEST, 12, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 12, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,12);}
	if ((iStats[13] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[13] = (__ASIC_ReadEngine(CHIP_TO_TEST, 13, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 13, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,13);}
	if ((iStats[14] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[14] = (__ASIC_ReadEngine(CHIP_TO_TEST, 14, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 14, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,14);}
	if ((iStats[15] & 0b01) == 0b01) { __MCU_ASIC_Activate_CS(); iStats[15] = (__ASIC_ReadEngine(CHIP_TO_TEST, 15, FIFO_ADDRESS_TO_READ) & 0x0FFFF) | (__ASIC_ReadEngine(CHIP_TO_TEST, 15, FIFO_ADDRESS_TO_READ+1) << 16); __MCU_ASIC_Deactivate_CS(); ASIC_ReadComplete(CHIP_TO_TEST,15);}
	*/
	
	/*
	ASIC_ReadComplete(CHIP_TO_TEST,1); 
	ASIC_ReadComplete(CHIP_TO_TEST,2); 
	ASIC_ReadComplete(CHIP_TO_TEST,3); 
	ASIC_ReadComplete(CHIP_TO_TEST,4);
	ASIC_ReadComplete(CHIP_TO_TEST,5);
	ASIC_ReadComplete(CHIP_TO_TEST,6);
	ASIC_ReadComplete(CHIP_TO_TEST,7);
	ASIC_ReadComplete(CHIP_TO_TEST,8);
	ASIC_ReadComplete(CHIP_TO_TEST,9);
	ASIC_ReadComplete(CHIP_TO_TEST,10);
	ASIC_ReadComplete(CHIP_TO_TEST,11);
	ASIC_ReadComplete(CHIP_TO_TEST,12);
	ASIC_ReadComplete(CHIP_TO_TEST,13);
	ASIC_ReadComplete(CHIP_TO_TEST,14);
	ASIC_ReadComplete(CHIP_TO_TEST,15);
	*/
	
	
/*
	ASIC_reset_engine(CHIP_TO_TEST,0);
	ASIC_reset_engine(CHIP_TO_TEST,1);
	ASIC_reset_engine(CHIP_TO_TEST,2);
	ASIC_reset_engine(CHIP_TO_TEST,3);
	ASIC_reset_engine(CHIP_TO_TEST,4);
	ASIC_reset_engine(CHIP_TO_TEST,5);
	if ((iStats[9] & 0b01) == 0b01)  { ASIC_ReadComplete(CHIP_TO_TEST,9);  }
	if ((iStats[10] & 0b01) == 0b01) { ASIC_ReadComplete(CHIP_TO_TEST,10); }
	ASIC_reset_engine(CHIP_TO_TEST,6);
	ASIC_reset_engine(CHIP_TO_TEST,7);
	ASIC_reset_engine(CHIP_TO_TEST,8);
	ASIC_reset_engine(CHIP_TO_TEST,9);
	ASIC_reset_engine(CHIP_TO_TEST,10);
	ASIC_reset_engine(CHIP_TO_TEST,11);
	ASIC_reset_engine(CHIP_TO_TEST,12);
	ASIC_reset_engine(CHIP_TO_TEST,13);
	ASIC_reset_engine(CHIP_TO_TEST,14);*/
		
	/*
	sprintf(szTemp,"STATS:\n%08X %08X %08X %08X\n%08X %08X %08X %08X\n%08X %08X %08X %08X\n%08X %08X %08X %08X \n", 
		    iStats[0], iStats[1], iStats[2], iStats[3], iStats[4], iStats[5], iStats[6], iStats[7],
			iStats[8], iStats[9], iStats[10], iStats[11], iStats[12], iStats[13], iStats[14], iStats[15]);
			
	strcat(szInfoReq, szTemp);
	*/
	
	// Add Engine count
	sprintf(szTemp,"ENGINES: %d\n", ASIC_get_processor_count());
	strcat(szInfoReq, szTemp);
	
	// Add Engine freuqnecy
	sprintf(szTemp,"FREQUENCY: 280MHz\n");
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
		Flush_buffer_into_engines(); // Function is called before we return
		
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
			USB_send_string("ERR:HIGH TEMPERATURE RECOVERY\n");  // Send it to USB
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
	Flush_buffer_into_engines();

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
	sprintf(sz_temp,"INPROCESS:%d\n", (_prev_job_was_from_pipe == TRUE) ? 1 : 0);
	strcat(sz_rep, sz_temp);
		
	sprintf(sz_temp,"COUNT:%d\n", JobPipe__pipe_get_buf_job_results_count());
	strcat(sz_rep, sz_temp);
	
	
	// Ok, return the last result as well
	if (JobPipe__pipe_get_buf_job_results_count() == 0)
	{
		// We simply do nothing... Just add the <LF> to the end of the line
	}
	else
	{
		// Now post them one by one
		for (i_cnt = 0; i_cnt < JobPipe__pipe_get_buf_job_results_count(); i_cnt++)
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
				sprintf(sz_temp,"%s,%s,%X,%d", sz_temp2, sz_temp3, pjob_res->iProcessingChip, iNonceCount); // Add midstate, block-data, count and nonces...
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
	JobPipe__pipe_set_buf_job_results_count(0);
	
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

// =============================================================
// Buffer Management ===========================================
// =============================================================

#if !defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)

	void Flush_buffer_into_engines()
	{
		// Our flag which tells us where the previous job
		// was a P2P job processed or not :)

		// Take the job from buffer and put it here...
		// (We take element 0, and push all the arrays back...)
		if (ASIC_is_processing() == TRUE)
		{
			// Ok, since ASICs are processing, at this point we can start loading the next job in queue.
			// If the previous job is already loaded, then there isn't much we can do
			if (JobPipe__pipe_ok_to_pop() == FALSE)
			{
				// There is nothing to load, just return...
				return;
			}
		
			// Have we loaded something already and it has finished? If so, nothing to do as well
			if (JobPipe__get_interleaved_loading_progress_finished() == TRUE)
			{
				// Since the job is already loaded, there is nothing to do...
				return;
			}
		
			// Now we reach here, it means there is a job available for loading, which has not been 100% loaded.
			// Try to load one engine at a time...
			job_packet jp;
			char iRes = JobPipe__pipe_preview_next_job(&jp);
		
			// TODO: Take further action
			if (iRes == PIPE_JOB_BUFFER_EMPTY)
			{
				// This is a SERIOUS PROBLEM, it shouldn't be the case... (since we've already executed JobPipe__pipe_ok_to_pop)
			}
	
			// Ok, we have the JOB. We start looping...
			char iChipToProceed = JobPipe__get_interleaved_loading_progress_chip();
			char iTotalChipsInstalled = ASIC_get_chip_count();
			char iEngineToProceed = JobPipe__get_interleaved_loading_progress_engine();
			char iChipHover = 0;
			char iEngineHover = 0;
		
			unsigned int iProcessorCount = ASIC_get_processor_count();
			unsigned int iRangeSlice = (0xFFFFFFFF / iProcessorCount);
			unsigned int iRemainder  =  0xFFFFFFFF - (iRangeSlice * iProcessorCount); // This is reserved for the last engine in the last chip
		
			// Initial value is set here... (These are static, as they are only used in this function)
			static unsigned int iLowerRange = 0;
			static unsigned int iUpperRange = 0;
		
			// Did we perform any writes? If not, it means we're finished
			char bDidWeWriteAnyEngines = FALSE;
			char bEngineWasNotFinishedSoWeAborted = FALSE;
		
			// We reset lower-range and upper-range if we're dealing with chip-0 and engine-0
			if ((iChipToProceed == 0) && (iEngineToProceed == 0)) { iLowerRange = 0; iUpperRange = 0 + iRangeSlice; }
		
			// Ok, we continue until we get the good chip
			for (iChipHover = iChipToProceed; iChipHover < iTotalChipsInstalled; iChipHover++)		
			{
				// We reach continue to the end...
				if (!CHIP_EXISTS(iChipHover)) continue;
			
				// Hover the engines...
				for (iEngineHover = iEngineToProceed; iEngineHover < 16; iEngineHover++)
				{
					// Is this processor ok?
					if (!IS_PROCESSOR_OK(iChipHover, iEngineHover)) continue;
				
					// Is it engine 0 and we're restricted?
					#if defined(DO_NOT_USE_ENGINE_ZERO)
						if (iEngineHover == 0) continue;
					#endif
				
					// Is this engine finished? If yes then we'll write data to it. IF NOT THEN JUST FORGET IT! We abort this part altogether
					if (ASIC_has_engine_finished_processing(iChipHover, iEngineHover) == FALSE)
					{
						bEngineWasNotFinishedSoWeAborted = TRUE;
						break;
					}
				
					// Ok, now do Job-Issue to this engine on this chip
					ASIC_job_issue_to_specified_engine(iChipHover, iEngineHover, &jp, FALSE, FALSE, iLowerRange, iUpperRange);
				
					// Increment 
					iLowerRange += iRangeSlice;
					iUpperRange += iRangeSlice;
				
					// Ok, Set the next engine to be processed...
					if (iEngineHover == 15)
					{
						JobPipe__set_interleaved_loading_progress_chip(iChipHover+1);
						JobPipe__set_interleaved_loading_progress_engine(0);
					}
					else
					{
						JobPipe__set_interleaved_loading_progress_chip(iChipHover);
						JobPipe__set_interleaved_loading_progress_engine(iEngineHover+1);					
					}
				
					// Set the flag
					bDidWeWriteAnyEngines = TRUE;
				
					// Exit the loop
					break;			
				}
			
				// Did we perform any writes? If so, we're done
				if (bDidWeWriteAnyEngines == TRUE) break;
			
				// Was the designated engine busy? If so, we exit
				if (bEngineWasNotFinishedSoWeAborted == TRUE) break;
			}
				
			// Did we write any engines? If NOT then we're finished
			if ((bDidWeWriteAnyEngines == FALSE) && (bEngineWasNotFinishedSoWeAborted == FALSE))
			{
				// Set/Reset related settings
				JobPipe__set_interleaved_loading_progress_finished(TRUE);
				JobPipe__set_interleaved_loading_progress_chip(0);
				JobPipe__set_interleaved_loading_progress_engine(0);			
			
				// Also clear the bounds
				iLowerRange = 0;
				iUpperRange = 0;
			}
			else
			{
				// We're not finished
				JobPipe__set_interleaved_loading_progress_finished(FALSE);			
			}
			
			return; // We won't do anything, since there isn't anything we can do
		}		 
	
		// Now if we're not processing, we MAY need to save the results if the previous job was from the pipe
		if (_prev_job_was_from_pipe == TRUE)
		{
			// Verify the result counter is correct
			if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
			{
				// Then move all items one-index back (resulting in loss of the job-result at index 0)
				for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
				{
					memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
					memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
					memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
				}
			}

			// Read the result and put it here...
			unsigned int  i_found_nonce_list[16];
			volatile int i_found_nonce_count;
			char i_result_index_to_put = 0;
			ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count);

			// Set the correct index
			i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ?  (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

			// Copy data...
			memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_midstate, 		32);
			memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)__inprocess_blockdata, 		12);
			memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)	 i_found_nonce_list, 		8*sizeof(int));
			__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

			// Increase the result count (if possible)
			if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) __buf_job_results_count++;

			// We return, since there is nothing left to do
		}
	
		// Ok, now we have recovered any potential results that could've been useful to us
		// Now, Are there any jobs in our pipe system? If so, we need to start processing on that right away and set _prev_job_from_pipe.
		// If not, we just clear _prev_job_from_pipe and exit
		if (JobPipe__pipe_ok_to_pop() == FALSE)
		{
			// Last job was not from pipe...
			_prev_job_was_from_pipe = FALSE;
		
			// Reset interleaved settings as well...
			JobPipe__set_interleaved_loading_progress_finished(FALSE);
			JobPipe__set_was_last_job_loaded_in_engines(FALSE);
			JobPipe__set_interleaved_loading_progress_chip(0); // Reset them
			JobPipe__set_interleaved_loading_progress_engine(0); // Reset them
		
			// Exit the function. Nothing is left to do here...
			return;
		}

		// Ok so there are jobs that require processing. Suck them in and start processing
		// At this stage, we have know the ASICS have finished processing. We also know that there is a job to be sent
		// to the device for processing. However, we need to make a decision:
		//
		// 1) This job was interleaved-loaded into chips while they were processing. In this case just start the engines
		//
		// 2) No job was interleaved-loaded into chips. In this case, issue a full job. After that, ALSO perform a single interleaved-load action 
		//    if there are any more jobs to be loaded.
		//

		// ***********************************************************
		// We have something to pop, get it...
		if (JobPipe__get_interleaved_loading_progress_finished() == TRUE)
		{
			// We start the engines...
			char iEngineHover = 0;
			char iChipHover = 0;
		
			for (iChipHover = 0; iChipHover < TOTAL_CHIPS_INSTALLED; iChipHover++)
			{
				if (!CHIP_EXISTS(iChipHover)) continue;
			
				for (iEngineHover = 0; iEngineHover < 16; iEngineHover++)
				{
					if (!IS_PROCESSOR_OK(iChipHover, iEngineHover))	continue;
				
					// Start engine
					ASIC_job_start_processing(iChipHover, iEngineHover, FALSE);
				}
			}
		
			// Also POP the previous job, since it's already been fully transferred
			job_packet jpx_dummy;
			JobPipe__pipe_pop_job(&jpx_dummy);
		
			// Before we issue the job, we must put the correct information
			memcpy((void*)__inprocess_midstate, 	(void*)jpx_dummy.midstate, 	 32);
			memcpy((void*)__inprocess_blockdata, 	(void*)jpx_dummy.block_data, 12);
		
			// Ok, now no job is loaded in interleaved mode since the old was is now being processed
			JobPipe__set_interleaved_loading_progress_finished(FALSE);
			JobPipe__set_interleaved_loading_progress_chip(0);
			JobPipe__set_interleaved_loading_progress_engine(0);
		}
		else
		{
			// We have to issue a new job...
			job_packet job_from_pipe;
			if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
			{
				// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
				_prev_job_was_from_pipe = FALSE; // Obviously, this must be cleared...
				JobPipe__set_interleaved_loading_progress_finished(FALSE);
				JobPipe__set_interleaved_loading_progress_chip(0);
				JobPipe__set_interleaved_loading_progress_engine(0);
				return;
			}

			// Before we issue the job, we must put the correct information
			memcpy((void*)__inprocess_midstate, 	(void*)job_from_pipe.midstate, 	 32);
			memcpy((void*)__inprocess_blockdata, 	(void*)job_from_pipe.block_data, 12);

			// Send it to processing...
			ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF);	
		
			// DEBUG... Wait 150ms
			// volatile unsigned int iWaitVal = MACRO_GetTickCountRet;
			// while (MACRO_GetTickCountRet - iWaitVal < 150000);
		}
	
		// The job is coming from the PIPE...
		// Set we simply have to set the flag here
		_prev_job_was_from_pipe = TRUE;
	}
	
	// NOTE: Not Implemented
	void Flush_buffer_into_single_chip(char iChip)
	{
		// FUNCTION NOT IMPLEMENTED HERE IN SINGLE-JOB-PER-BOARD MODE
	}
	
#else

	#define SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS	1
	#define SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE		2
	#define SINGLE_CHIP_QUEUE__CHIP_BUSY			3
	
	void Flush_buffer_into_engines()
	{
		// Run the process for all available chips on board
		char iTotalChips = ASIC_get_chip_count();
		char iTotalTookJob = 0;
		char iTotalNoJobsAvailable = 0;
		char iTotalProcessing = 0;
		char iRetStat = 0;
		char iHover = 0;
	
		for (iHover = 0; iHover < TOTAL_CHIPS_INSTALLED; iHover++)
		{
			if (!CHIP_EXISTS(iHover)) continue;
			iRetStat = Flush_buffer_into_single_chip(iHover);
			iTotalTookJob += (iRetStat == SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS) ? 1 : 0;
			iTotalNoJobsAvailable += (iRetStat == SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE) ? 1 : 0;
			iTotalProcessing += (iRetStat == SINGLE_CHIP_QUEUE__CHIP_BUSY) ? 1 : 0;			
		}		
		
		// We can determine if the _prev_job_was_from_queue is TRUE or FALSE
		if (iTotalNoJobsAvailable == iTotalChips)	
		{
			// Meaning no job was available. We're done now...
			_prev_job_was_from_pipe = FALSE;
		}
		
		// If anyone (even only a single chip) took a job, then we can consider that previous job was from pipe
		if (iTotalTookJob > 0)
		{
			// A queue job is being processed...
			_prev_job_was_from_pipe = TRUE;	
		}
	}	
	
	char Flush_buffer_into_single_chip(char iChip)
	{
		// Our flag which tells us where the previous job
		// was a P2P job processed or not :)
		// Take the job from buffer and put it here...
		// (We take element 0, and push all the arrays back...)
		if (ASIC_is_chip_processing(iChip) == TRUE)
		{
			return SINGLE_CHIP_QUEUE__CHIP_BUSY; // We won't do anything, since there isn't anything we can do
		}

		// Now if we're not processing, we MAY need to save the results if the previous job was from the pipe
		// This is conditioned on whether this chip was processing a single-job or not...
		if ((_prev_job_was_from_pipe == TRUE) && (__inprocess_SCQ_chip_processing[iChip] == TRUE)) 
		{
			// Verify the result counter is correct
			if (__buf_job_results_count == PIPE_MAX_BUFFER_DEPTH)
			{
				// Then move all items one-index back (resulting in loss of the job-result at index 0)
				for (char pIndex = 0; pIndex < PIPE_MAX_BUFFER_DEPTH - 1; pIndex += 1) // PIPE_MAX_BUFFER_DEPTH - 1 because we don't touch the last item in queue
				{
					__buf_job_results[pIndex].iProcessingChip = __buf_job_results[pIndex+1].iProcessingChip;
					
					memcpy((void*)__buf_job_results[pIndex].midstate, 	(void*)__buf_job_results[pIndex+1].midstate, 	32);
					memcpy((void*)__buf_job_results[pIndex].block_data, (void*)__buf_job_results[pIndex+1].block_data, 	12);
					memcpy((void*)__buf_job_results[pIndex].nonce_list,	(void*)__buf_job_results[pIndex+1].nonce_list,  8*sizeof(UL32)); // 8 nonces maximum
				}
			}

			// Read the result and put it here...
			unsigned int  i_found_nonce_list[16];
			volatile int i_found_nonce_count;
			char i_result_index_to_put = 0;
			ASIC_get_job_status(i_found_nonce_list, &i_found_nonce_count, TRUE, iChip);

			// Set the correct index
			i_result_index_to_put = (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH) ? (__buf_job_results_count) : (PIPE_MAX_BUFFER_DEPTH - 1);

			// Copy data...
			memcpy((void*)__buf_job_results[i_result_index_to_put].midstate, 	(void*)__inprocess_SCQ_midstate[iChip],		32);
			memcpy((void*)__buf_job_results[i_result_index_to_put].block_data, 	(void*)__inprocess_SCQ_blockdata[iChip],	12);
			memcpy((void*)__buf_job_results[i_result_index_to_put].nonce_list,	(void*)i_found_nonce_list,		8*sizeof(int));
			__buf_job_results[i_result_index_to_put].iProcessingChip = iChip;
			__buf_job_results[i_result_index_to_put].i_nonce_count = i_found_nonce_count;

			// Increase the result count (if possible)
			if (__buf_job_results_count < PIPE_MAX_BUFFER_DEPTH)
			{
				 __buf_job_results_count++;
			}				 

			// We return, since there is nothing left to do
		}

		// Ok, now we have recovered any potential results that could've been useful to us
		// Now, Are there any jobs in our pipe system? If so, we need to start processing on that right away and set _prev_job_from_pipe.
		// If not, we just clear _prev_job_from_pipe and exit
		if (JobPipe__pipe_ok_to_pop() == FALSE)
		{
			// Exit the function. Nothing is left to do here...
			__inprocess_SCQ_chip_processing[iChip] = FALSE;
			return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
		}

		// We have to issue a new job...
		job_packet job_from_pipe;
		if (JobPipe__pipe_pop_job(&job_from_pipe) == PIPE_JOB_BUFFER_EMPTY)
		{
			// This is odd!!! THIS SHOULD NEVER HAPPEN, however we do set things here by default...
			__inprocess_SCQ_chip_processing[iChip] = FALSE;
			return SINGLE_CHIP_QUEUE__NO_JOB_TO_TAKE;
		}

		// Before we issue the job, we must put the correct information
		memcpy((void*)__inprocess_SCQ_midstate[iChip], 	(void*)job_from_pipe.midstate, 	 32);
		memcpy((void*)__inprocess_SCQ_blockdata[iChip], (void*)job_from_pipe.block_data, 12);
	
		// Send it to processing...
		ASIC_job_issue(&job_from_pipe, 0x0, 0xFFFFFFFF, TRUE, iChip);
		
		// This chip is processing now
		__inprocess_SCQ_chip_processing[iChip] = TRUE;
		
		// The engine took a job
		return SINGLE_CHIP_QUEUE__TOOK_JOB_TO_PROCESS;	
	}

#endif

// ======================================================================================
// ======================================================================================
// ======================================================================================




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
	sz_hex[0] = 0x0; // Clear the hex string

	for (index = 0; index < i_stream_len; index++)
	{
		sprintf(sz_temp, "%02X", sz_stream[index]);
		strcat(sz_hex, sz_temp);
	}
	
	*i_hex_len = i_stream_len * 2;

	// We're done...
}
