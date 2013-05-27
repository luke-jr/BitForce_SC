
/*
 * ASIC_Engine.c
 *
 * Created: 20/11/2012 23:17:15
 *  Author: NASSER
 */ 

#include "ASIC_Engine.h"
#include "std_defs.h"
#include "Generic_Module.h"
#include "AVR32_OptimizedTemplates.h"
#include "AVR32X/AVR32_Module.h"
#include "avr32/io.h"


//static volatile unsigned int __ActualRegister0Value = (0);
  static volatile unsigned int __ActualRegister0Value = (1<<13);
//static volatile unsigned int __ActualRegister0Value = (1);

// Midstate for SHA2-Core, this is static and must be hard-coded in the chip
static const unsigned int STATIC_H1[8] = {
	0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
	0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static const unsigned int STATIC_W0[64] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x80000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000280,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000
};


// Midstate for SHA1-Core, this is provided by the MCU (Which is in turn provided by the GetWork API)
void init_ASIC(void)
{
	MCU_SC_Initialize();
	
	// Clear ASIC map
	__chip_existence_map[0] = 0;
	__chip_existence_map[1] = 0;
	__chip_existence_map[2] = 0;
	__chip_existence_map[3] = 0;
	__chip_existence_map[4] = 0;
	__chip_existence_map[5] = 0;
	__chip_existence_map[6] = 0;
	__chip_existence_map[7] = 0;
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) 
		__chip_existence_map[8] = 0;
		__chip_existence_map[9] = 0;
		__chip_existence_map[10] = 0;
		__chip_existence_map[11] = 0;
		__chip_existence_map[12] = 0;
		__chip_existence_map[13] = 0;
		__chip_existence_map[14] = 0;
		__chip_existence_map[15] = 0;	
	#endif
	
	// Here we must give 16 clock cycles with CS# inactive, to initialize the ASIC chips
	__MCU_ASIC_Deactivate_CS();
	
	// Reset first bank
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);

	// Reset second bank
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) 
		__ASIC_WriteEngine(8,0,0,0);
		__ASIC_WriteEngine(8,0,0,0);
		__ASIC_WriteEngine(8,0,0,0);
		__ASIC_WriteEngine(8,0,0,0);		
	#endif

	// Also here we set the Oscillator Control Register
	ASIC_Bootup_Chips();
		
	// Proceed...
	ASIC_get_chip_count(); // Also sets the '__chip_existence_map' table in it's first run
	
	// Ok now we diagnose the engines
	char iHoveringChip;
	char iHoveringEngine;
	
	for (iHoveringChip = 0; iHoveringChip < TOTAL_CHIPS_INSTALLED; iHoveringChip++)
	{
		// Reset WATCHDOG
		WATCHDOG_RESET;
		
		// Does the chip exist at all?
		if (!CHIP_EXISTS(iHoveringChip)) 
		{
			continue;
		}
					
		// Diagnose
		for (iHoveringEngine = 0; iHoveringEngine < 16; iHoveringEngine++)
		{
			// Reset watchdog
			WATCHDOG_RESET;
			
			// Is Engine 0 permitted?
			#if defined(DO_NOT_USE_ENGINE_ZERO)
				if (iHoveringEngine == 0) continue;			
			#endif
			
			// Is processor ok or does chip exist?
			if (!CHIP_EXISTS(iHoveringChip)) continue;
			if (!IS_PROCESSOR_OK(iHoveringChip, iHoveringEngine)) continue;
			
			// Proceed
			if (ASIC_diagnose_processor(iHoveringChip, iHoveringEngine) == FALSE)
			{
				__chip_existence_map[iHoveringChip] &= ~(1<<iHoveringEngine);
			}
			else
			{
				__chip_existence_map[iHoveringChip] |= (1<<iHoveringEngine);
			}
		}
		
		// Frequency tune the chips
		#if !defined(__DO_NOT_TUNE_CHIPS_FREQUENCY)
			for (iHoveringEngine = 0; iHoveringEngine < 16; iHoveringEngine++)		
			{
				// Is Engine 0 permitted?
				#if defined(DO_NOT_USE_ENGINE_ZERO)
					if (iHoveringEngine == 0) continue;
				#endif			
			
				// If this processor is bad, continue...
				if (!IS_PROCESSOR_OK(iHoveringChip, iHoveringEngine)) continue;
						
				// Use it to diagnose
				int iDetectedFreq = ASIC_tune_chip_to_frequency(iHoveringChip, iHoveringEngine, FALSE);
				GLOBAL_CHIP_FREQUENCY_INFO[iHoveringChip] = (iDetectedFreq / 1000000);
				break; // We break after here...
			}
		#else
			// Since job-distibution uses this information, if we're aksed not to tune chips then we must
			// use the predefined value by default (to avoid Div-By-Zero error)
			// (ASIC_calculate_engines_nonce_range function uses this information)
			GLOBAL_CHIP_FREQUENCY_INFO[iHoveringChip] = (__ASIC_FREQUENCY_VALUES[__ASIC_FREQUENCY_ACTUAL_INDEX]);			
		#endif
		
		// Ok, Now we have the __chip_existance_map for this chip, the value can be used to set clock-enable register in this chip
		ASIC_set_clock_mask(iHoveringChip, __chip_existence_map[iHoveringChip]);
	}	
	
	// Also, clear Timestamping for Engine supervision
	#if defined(__ENGINE_ACTIVITY_SUPERVISION)
		for (iHoveringChip = 0; iHoveringChip < TOTAL_CHIPS_INSTALLED; iHoveringChip++)
		{
			for (iHoveringEngine = 0; iHoveringEngine < TOTAL_CHIPS_INSTALLED; iHoveringEngine++)
			{
				GLOBAL_ENGINE_PROCESSING_STATUS[iHoveringChip][iHoveringEngine] = 0;
				GLOBAL_ENGINE_PROCESSING_START_TIMESTAMP[iHoveringChip][iHoveringEngine] = 0;
			}
		}
	#endif
	
	
	// Ok, now we calculate nonce-range for the engines
	#if defined(__ACTIVATE_JOB_LOAD_BALANCING)
		ASIC_calculate_engines_nonce_range();
	#endif
}

// Sets the clock mask for a deisng
void ASIC_set_clock_mask(char iChip, unsigned int iClockMaskValue)
{
	__MCU_ASIC_Activate_CS();
	#if defined(DISABLE_CLOCK_TO_ALL_ENGINES)
		__ASIC_WriteEngine(iChip, 0, ASIC_SPI_CLOCK_OUT_ENABLE, 0);	 // Disable all clocks
	#else
		__ASIC_WriteEngine(iChip, 0, ASIC_SPI_CLOCK_OUT_ENABLE, iClockMaskValue);
	#endif
	__MCU_ASIC_Deactivate_CS();
}

// This function checks to see if the processor in this engine is correctly operating. We must find the correct nonce here...
char ASIC_diagnose_processor(char iChip, char iEngine)
{
	#if defined(DO_NOT_USE_ENGINE_ZERO)
		if (iEngine == 0) return FALSE;
	#endif
	
	// Our job-packet by default (Expected nonce is 8D9CB675 - Hence counter range is 8D9C670 to 8D9C67A)
	static job_packet jp;
	unsigned char index;
	char __stat_blockdata[] = {0xAC,0x84,0xF5,0x8B,0x4D,0x59,0xB7,0x4D,0x1B,0x2,0x85,0x52};
	char __stat_midstate[]  = {0x3C,0x42,0x49,0xA8,0xE1,0xC5,0x45,0x78,0xA5,0x2D,0x83,0xC1,0x1,0xE5,0xC5,0x8E,0xF5,0x2F,0x3,0xD,0xEE,0x2E,0x9D,0x29,0xB6,0x94,0x9A,0xDF,0xA6,0x95,0x97,0xAE};
	for (index = 0; index < 12; index++) jp.block_data[index] = __stat_blockdata[index];
	for (index = 0; index < 32; index++) jp.midstate[index]   = __stat_midstate[index];
	jp.signature  = 0xAA;
	
	// By default it's set to true...
	unsigned char isProcessorOK = TRUE;
	
	// Run 20 Tests and check it every time...
	for (unsigned char imi = 0; imi < 40; imi++)
	{
		// Now send the job to the engine
		ASIC_job_issue_to_specified_engine(iChip,iEngine,&jp, TRUE, TRUE, 0x8D9CB670, 0x8D9CB67A);
		ASIC_job_start_processing(iChip, iEngine, TRUE);
	
		// Read back results immediately (with a little delay), it shouldv'e been finished since it takes about 40ns
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
	
		// Read back result. It should be DONE with FIFO zero of it being 0x8D9CB675
		unsigned int iReadbackNonce = 0;
		unsigned int iReadbackStatus = 0;

		__MCU_ASIC_Activate_CS();
	
		iReadbackStatus = __ASIC_ReadEngine(iChip,iEngine, ASIC_SPI_READ_STATUS_REGISTER);
		if ((iReadbackStatus & ASIC_SPI_READ_STATUS_DONE_BIT) != ASIC_SPI_READ_STATUS_DONE_BIT)
		{
			isProcessorOK = FALSE;
			break;
		}
	
		if ((iReadbackStatus & ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT) != 0) // Depth 2 should not have any nonces
		{
			isProcessorOK = FALSE;
			break;
		}
	
		iReadbackNonce = __ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_LWORD) | (__ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_HWORD) << 16);
		if (iReadbackNonce != 0x8D9CB675) 
		{
			isProcessorOK = FALSE;
			break;
		}
	
		__MCU_ASIC_Deactivate_CS();	
		
		// Set the second nonce
		// Now send the job to the engine
		ASIC_job_issue_to_specified_engine(iChip,iEngine,&jp, TRUE, TRUE, 0x38E94200, 0x38E94300);
		ASIC_job_start_processing(iChip, iEngine, TRUE);
	
		// Read back results immediately (with a little delay), it shouldv'e been finished since it takes about 40ns
		NOP_OPERATION; NOP_OPERATION; 
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
		NOP_OPERATION; NOP_OPERATION;
	
		__MCU_ASIC_Activate_CS();
	
		iReadbackStatus = __ASIC_ReadEngine(iChip,iEngine, ASIC_SPI_READ_STATUS_REGISTER);
		if ((iReadbackStatus & ASIC_SPI_READ_STATUS_DONE_BIT) != ASIC_SPI_READ_STATUS_DONE_BIT)
		{
			isProcessorOK = FALSE;
			break;
		}
	
		if ((iReadbackStatus & ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT) != 0) // Depth 2 should not have any nonces
		{
			isProcessorOK = FALSE;
			break;
		}
	
		iReadbackNonce = __ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_LWORD) | (__ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_HWORD) << 16);
		if (iReadbackNonce != 0x38E9425A) 
		{
			isProcessorOK = FALSE;
			break;
		}

		__MCU_ASIC_Deactivate_CS();	

		// Clear the fifo on engines
		ASIC_ReadComplete(iChip,iEngine);		
	}		
		
	// Deactivate the CS just to be sure
	__MCU_ASIC_Deactivate_CS();	
	
	// Clear the fifo on engines
	ASIC_ReadComplete(iChip,iEngine);
		
	// Return the result
	return isProcessorOK;
}

/// This function tunes the CHIP to the requested frequency. If it's operating way below it,
/// then it will try to increase the frequency if needed. At the same time, it will see if the chip CAN
/// handle the frequency by checking the NONCE value. should it be wrong, then it will fall back
int ASIC_tune_chip_to_frequency(char iChip, char iEngineToUse, char bOnlyReturnOperatingFrequency)
{
	// Our job-packet by default (Expected nonce is 8D9CB675 - Hence counter range is 8D9C670 to 8D9C67A)
	static job_packet jp;
	unsigned char index;
	char __stat_blockdata[] = {0xAC,0x84,0xF5,0x8B,0x4D,0x59,0xB7,0x4D,0x1B,0x2,0x85,0x52};
	char __stat_midstate[]  = {0x3C,0x42,0x49,0xA8,0xE1,0xC5,0x45,0x78,0xA5,0x2D,0x83,0xC1,0x1,0xE5,0xC5,0x8E,0xF5,0x2F,0x3,0xD,0xEE,0x2E,0x9D,0x29,0xB6,0x94,0x9A,0xDF,0xA6,0x95,0x97,0xAE};
	for (index = 0; index < 12; index++) jp.block_data[index] = __stat_blockdata[index];
	for (index = 0; index < 32; index++) jp.midstate[index]   = __stat_midstate[index];
	jp.signature  = 0xAA;
	
	// Actual hovering indexes
	char iActualHoverIndex = __ASIC_FREQUENCY_ACTUAL_INDEX;
	char bInFrequencyReductionState = FALSE;
		
	// Repeat this process until tuned
	while (TRUE)
	{
		// Now send the job to the engine
		ASIC_job_issue_to_specified_engine(iChip, iEngineToUse, &jp, TRUE, TRUE, 0x8D98229A, 0x8D9CB67A); // 300,000 attempts will take 1.2ms per chip and gives us 1.7% precision
		ASIC_job_start_processing(iChip, iEngineToUse, TRUE);
		
		// Hold the time
		unsigned int ulBegin = MACRO_GetTickCountRet;
		unsigned int ulEnd = 0;
	
		// Read back results immediately (with a little delay), it shouldv'e been finished since it takes about 40ns
		NOP_OPERATION;
		NOP_OPERATION;
		NOP_OPERATION;
	
		// Read back result. It should be DONE with FIFO zero of it being 0x8D9CB675
		unsigned int iReadbackNonce = 0;
		unsigned int iReadbackStatus = 0;
		unsigned char isProcessorOK = TRUE;	
		
		__MCU_ASIC_Activate_CS();
	
		// Wait for the thing to finish...
		while (TRUE)
		{
			WATCHDOG_RESET;
			
			// Read back the status
			iReadbackStatus = __ASIC_ReadEngine(iChip, iEngineToUse, ASIC_SPI_READ_STATUS_REGISTER);
			if ((iReadbackStatus & ASIC_SPI_READ_STATUS_DONE_BIT) != ASIC_SPI_READ_STATUS_DONE_BIT)
			{
				if ((MACRO_GetTickCountRet - ulBegin) > 9000)
				{
					// This means failure
					isProcessorOK = FALSE;
					break;
				}
				else
				{
					// Do nothing...
					// Just continue...
					continue;
				}
			}	
			else
			{
				// We're out of here..
				ulEnd = MACRO_GetTickCountRet;
				break;
			}			
		}

		// Is our processor bad? If so, just revert
		if ((isProcessorOK == FALSE) || ((iReadbackStatus & ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT) != 0))
		{
			if (iActualHoverIndex == 0) // END OF LINE!!!
			{
				// We have a problem, abort...
				__MCU_ASIC_Deactivate_CS();	
				
				// Clear the fifo on engines
				ASIC_ReadComplete(iChip,iEngineToUse);
				
				return 0; // 0 means chip is DEAD!
			}
			else
			{
				// This is being modified by functions execute after...
				__MCU_ASIC_Deactivate_CS();	
				
				// Hover back one index, set frequency and exit
				iActualHoverIndex -= 1;
				
				// Set the new frequency and exit... This will reduce the board operating speed however :(
				ASIC_SetFrequencyFactor(iChip, __ASIC_FREQUENCY_WORDS[iActualHoverIndex]);
				
				// Clear the fifo on engines
				ASIC_ReadComplete(iChip,iEngineToUse);				
				
				// Unfortunately we have to try again, as the chip is not working properly...
				bInFrequencyReductionState = TRUE;
				continue;					
			}
		}

		iReadbackNonce = __ASIC_ReadEngine(iChip, iEngineToUse, ASIC_SPI_FIFO0_LWORD) | (__ASIC_ReadEngine(iChip, iEngineToUse, ASIC_SPI_FIFO0_HWORD) << 16);
		if (iReadbackNonce != 0x8D9CB675) 
		{
			if (iActualHoverIndex == 0)
			{
				// We have a problem, abort...
				__MCU_ASIC_Deactivate_CS();
				
				// Clear the fifo on engines
				ASIC_ReadComplete(iChip,iEngineToUse);
				
				// Return
				return 0;
			}
			else
			{
				// This is being modified by functions execute after...
				__MCU_ASIC_Deactivate_CS();
				
				// Clear the fifo on engines
				ASIC_ReadComplete(iChip,iEngineToUse);				
	
				// Hover back one index, set frequency and exit
				iActualHoverIndex -= 1;
	
				// Set the new frequency and exit... This will reduce the board operating speed however :(
				ASIC_SetFrequencyFactor(iChip, __ASIC_FREQUENCY_WORDS[iActualHoverIndex]);
				
				// Unfortunately we have to try again, as the chip is not working properly...
				bInFrequencyReductionState = TRUE;
				continue;
			}
		}
	
		// Finally deactivate the CS... we have processing to do...
		__MCU_ASIC_Deactivate_CS();		
		
		// Clear the fifo on engines
		ASIC_ReadComplete(iChip,iEngineToUse);
		
		// Ok, all was ok to this point
		// Calculate frequency
		float fDetectedFrequency = (1000000 / (ulEnd - ulBegin)) * 300000;
		unsigned int iDetectedFrequency = (unsigned int)fDetectedFrequency;
		
		// Ok, is it slower more than 5MHz ? If so, do a jump
		unsigned int iDiff = (iDetectedFrequency < __ASIC_FREQUENCY_WORDS[__ASIC_FREQUENCY_ACTUAL_INDEX]) ?  (__ASIC_FREQUENCY_WORDS[__ASIC_FREQUENCY_ACTUAL_INDEX] - iDetectedFrequency) : 0;
		
		// Now check, if we're in frequency reduction mode, it means that this is probably the highest frequency this chip can operate
		// without problem. If so, just return the actual value and get out, as there is no point in increasing frequency
		if (bInFrequencyReductionState == TRUE) return iDetectedFrequency;
		
		// Are we just checking frequency? if so, return it here...
		if (bOnlyReturnOperatingFrequency == TRUE) return iDetectedFrequency;
							 
		// Is it slower by more than 5MHz?
		if (iDiff > 5)
		{
			// Check frequency, if it's at 8 then there is nothing to do
			if (iActualHoverIndex < __MAXIMUM_FREQUENCY_INDEX)
			{
				// Go faster...
				iActualHoverIndex += 1;
				
				// Set the new frequency and exit... This will reduce the board operating speed however :(
				ASIC_SetFrequencyFactor(iChip, __ASIC_FREQUENCY_WORDS[iActualHoverIndex]);
				
				// We are in regression test... Make sure the engine does operate correct with the new frequency
				continue;
			}
			else
			{
				// We've run out of numbers, just return the actual frequency
				return iDetectedFrequency;
			}
		}		
		else // The number is within range, return it...
		{
			// All ok, just exit...
			return iDetectedFrequency;
		}
	}		
	
	// Ok, we have to return the actual frequency
	return 0; // Illegal value. This will cause the chip to be deactivated....
}

// Say Read-Complete
void ASIC_ReadComplete(char iChip, char iEngine)
{
	__MCU_ASIC_Activate_CS();
	if (iEngine == 0)
	{
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT | __ActualRegister0Value));	
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value));	
	}
	else
	{
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT));	
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (0));	
	}	
	__MCU_ASIC_Deactivate_CS();
}	


// ---------------------------------------------------------------------------
//   MASK Bits
// ---------------------------------------------------------------------------
#define RESET_BIT		12
#define WRITE_VALID_BIT	11
#define READ_COMP_BIT	10
#define RESET_ERR_BIT	 9
#define BUSY_BIT		01
#define DONE_BIT		00

#define MASK_RESET_BIT		(0x1 << RESET_BIT)
#define MASK_WRITE_VALID_BIT	(0x1 << WRITE_VALID_BIT)
#define MASK_READ_COMP_BIT	(0x1 << READ_COMP_BIT)
#define MASK_RESET_ERR_BIT	(0x1 << RESET_ERR_BIT)

#define MASK_BUSY_BIT		(0x1 << BUSY_BIT)
#define MASK_DONE_BIT		(0x1 << DONE_BIT)

// ---------------------------------------------------------------------
//
//   MASKS FOR CLOCK SETUP and ENGINE RESET
//

#define MASK_CLK		(0x3000)
#define MASK_RESET_ENGINE_0	(0x1000)

volatile unsigned int DATAREG0 = 0;
volatile unsigned int DATAIN   = 0;
volatile unsigned int DATAOUT  = 0;

//==================================================================
void __initEngines(int CHIP);
void __ResetSPIErrFlags(int CHIP, int ENGINE);
void __Reset_Engine(int CHIP, int ENGINE);

void __ResetSPIErrFlags(int CHIP, int ENGINE)
{
	if(ENGINE == 0){
		__ActualRegister0Value |= (MASK_RESET_ERR_BIT);
		DATAIN    = __ActualRegister0Value;
	} else {
		DATAIN = (MASK_RESET_ERR_BIT);
	}
	__AVR32_SC_WriteData(CHIP,ENGINE,0x00, DATAIN);
	if(ENGINE == 0){
		__ActualRegister0Value &= ~(MASK_RESET_ERR_BIT);
		DATAIN = __ActualRegister0Value;
	} else {
		DATAIN &= ~(MASK_RESET_ERR_BIT);
	}
	__AVR32_SC_WriteData(CHIP,ENGINE,0x00,DATAIN);
}

void __initEngines(int CHIP)
{
	int i, engineID;

	DATAREG0 = MASK_CLK;
	DATAIN = DATAREG0;
	__AVR32_SC_WriteData(CHIP, 0, 0x00, DATAIN);	// --- Engine ID = 0

	// --- reseting all engines 1-15 ---
	DATAREG0 |= MASK_RESET_ENGINE_0;
	DATAIN = DATAREG0;
	__AVR32_SC_WriteData(CHIP, 0, 0x61, DATAIN);	// --- Engine ID = 0

	DATAIN = 0x1000;			// --- Enable Clock Out for all Engines
	for(engineID = 1; engineID < 16; engineID++){
		__AVR32_SC_WriteData(CHIP, engineID, 0x61, DATAIN);
	}

	DATAIN = 0x0000;			// --- Enable Clock Out for all Engines
	for(engineID = 1; engineID < 16; engineID++){
		__AVR32_SC_WriteData(CHIP, engineID, 0x0, DATAIN);
	}

	DATAIN = 0x0000;			// --- Enable Clock Out for all Engines
	__AVR32_SC_WriteData(CHIP, 0, 0x61, DATAIN);	// --- Engine ID = 0
}

void __Reset_Engine(int CHIP, int ENGINE)
{
	__ActualRegister0Value |= (MASK_RESET_ENGINE_0);
	DATAIN    = __ActualRegister0Value;

	__AVR32_SC_WriteData(CHIP, ENGINE, 0x00, DATAIN);
	__ActualRegister0Value &= ~(MASK_RESET_ENGINE_0);
	DATAIN    = __ActualRegister0Value;

	// For engine zero, do not clear the reset
	__AVR32_SC_WriteData(CHIP, ENGINE, 0x00, DATAIN);
}


void __Write_SPI(int iChip, int iEngine, int iAddress, int iData)
{
	__MCU_ASIC_Activate_CS();
	__ASIC_WriteEngine(iChip, iEngine, iAddress, iData);
	__MCU_ASIC_Deactivate_CS();
}

unsigned int __Read_SPI(int iChip, int iEngine, int iAddress)
{
	unsigned int iRetVal = 0;
	__MCU_ASIC_Activate_CS();
	iRetVal = __ASIC_ReadEngine(iChip, iEngine, iAddress);
	__MCU_ASIC_Deactivate_CS();
	return iRetVal;
}

// We are going for 500MHz test


// Working...
void ASIC_Bootup_Chips()
{

	unsigned int iHover = 0;
	unsigned int DATAIN;
	unsigned int CHIP = 0;
		
	int c=0;
	
	int iDisableFlag = 0;
	
	// -----------------------------
	#if defined(DECOMISSION_CHIP_0)
		iDisableFlag |= (1<<0);
	#endif
	
	#if defined(DECOMISSION_CHIP_1)
		iDisableFlag |= (1<<1);
	#endif
	
	#if defined(DECOMISSION_CHIP_2)
		iDisableFlag |= (1<<2);
	#endif
	
	#if defined(DECOMISSION_CHIP_3)
		iDisableFlag |= (1<<3);
	#endif
	
	#if defined(DECOMISSION_CHIP_4)
		iDisableFlag |= (1<<4);
	#endif
	
	#if defined(DECOMISSION_CHIP_5)
		iDisableFlag |= (1<<5);
	#endif
	
	#if defined(DECOMISSION_CHIP_6)
		iDisableFlag |= (1<<6);
	#endif
	
	#if defined(DECOMISSION_CHIP_7)
		iDisableFlag |= (1<<7);
	#endif
	
	#if defined(DECOMISSION_CHIP_8)
		iDisableFlag |= (1<<8);
	#endif
	
	#if defined(DECOMISSION_CHIP_9)
		iDisableFlag |= (1<<9);
	#endif
	
	#if defined(DECOMISSION_CHIP_10)
		iDisableFlag |= (1<<10);
	#endif
	
	#if defined(DECOMISSION_CHIP_11)
		iDisableFlag |= (1<<11);
	#endif
	
	#if defined(DECOMISSION_CHIP_12)
		iDisableFlag |= (1<<12);
	#endif
	
	#if defined(DECOMISSION_CHIP_13)
		iDisableFlag |= (1<<13);
	#endif
	
	#if defined(DECOMISSION_CHIP_14)
		iDisableFlag |= (1<<14);
	#endif
	
	#if defined(DECOMISSION_CHIP_15)
		iDisableFlag |= (1<<15);
	#endif
	// --------------------------------
	
	// Operates at 250MHz, all Engines ACTIVE
	#if !defined(FIVE_HUNDRED_MHZ_TEST)
	
		for (CHIP = 0; CHIP < TOTAL_CHIPS_INSTALLED; CHIP++)
		{
			//Select ExtClk mode div1
			//DATAREG0[15]='0';//0=INT_CLK
			//DATAREG0[14]='0';//0=div2
			//DATAREG0[13]='1';//0=div4
			//DATAREG0[12]='0';//1=RESET
			//DATAREG0[0]='1'; //0=div 1
			DATAIN=0x2001;//INT_CLK, div/2
			__Write_SPI(CHIP,0,0x00,DATAIN);//int caddr, int engine, int reg, int data

			//Reset all instances (0-15):
			//Reset engine 0:
			//DATAREG0[12]='1'; 
			DATAIN=0x3001;//INT_CLK, div/2, reset
			__Write_SPI(CHIP,0,0x00,DATAIN);//int caddr, int engine, int reg, int data
		
			//Reset instances 1-15:
			DATAIN=0x1000;// bit[12]==reset
			for(c=1;c<16;c++)
			{
				__Write_SPI(CHIP,c,0,DATAIN);//int caddr, int engine, int reg, int data
			}
		
			//Enable Clock Out, all instances:
			DATAIN=0xFFFF;//
			__Write_SPI(CHIP,0,0x61, DATAIN);//int caddr, int engine, int reg, int data

			//Set Osc Control to slowest frequency:0000 (highest=0xCD55)
			/*
			#if defined(ASICS_OPERATE_AT_264MHZ)
				DATAIN = 0xD555; // Operates 250MHz
			#elif defined(ASICS_OPERATE_AT_250MHZ)
				DATAIN = 0xFF55; // Operates at 227MHz or less (TEMPORARLY)
			#elif defined(ASICS_OPERATE_AT_242MHZ)
				DATAIN = 0xFFF5; // Operates at 242MHz or less (TEMPORARLY)
			#elif defined(ASICS_OPERATE_AT_236MHZ)
				DATAIN = 0xFFFD; // Operates at 236MHz or less (TEMPORARLY)
			#elif defined(ASICS_OPERATE_AT_228MHZ)
				DATAIN = 0xFFFF; // Operates at 232MHz or less (TEMPORARLY)				
			#elif defined(ASICS_OPERATE_AT_261MHZ)
				DATAIN = 0xFD55; // Operates at 261MHz or less (TEMPORARLY)				
			#elif defined(ASICS_OPERATE_AT_280MHZ)
				DATAIN = 0x5555; // 280MHz
			#elif defined(ASICS_OPERATE_AT_170MHZ)
				DATAIN = 0x0000;
			#else
				DATAIN = 0x5555; // INVALID STATE
			#endif
			*/
						
			//DATAIN = 0xFFDF; // Operates 250MHz
			//DATAIN = 0x0000; // Operates 170MHz
			//DATAIN = 0xCDFF; // Operates 280MHz
			//DATAIN = 0xFFFF; // Operates Less than 250MHz			
			DATAIN = __ASIC_FREQUENCY_WORDS[__ASIC_FREQUENCY_ACTUAL_INDEX];
			__Write_SPI(CHIP,0,0x60,DATAIN);

			//Clear the Reset engine 1-15
			DATAIN=0x0000;
			for(c=1;c<16;c++){
				__Write_SPI(CHIP,c,0,DATAIN);
			}

			//Disable Clock Out, all engines
			/*
			#if defined(DISABLE_CLOCK_TO_ALL_ENGINES)
				DATAIN = 0; // Disable all clocks
			#else
				DATAIN = 0x0FFFE;
			#endif
			*/
			
			// is this chip disabled?			
			DATAIN= ((iDisableFlag & (1<<CHIP)) == 0) ? 0xFFFE : 0x0; // Engine 0 clock is not enabled here			
			__Write_SPI(CHIP,0,0x61,DATAIN);//int caddr, int engine, int reg, int data
		}	
	
	#else
	
		// Operates at 500MHz, engine 0 is active
		for (CHIP = 0; CHIP < TOTAL_CHIPS_INSTALLED; CHIP++)
		{
			//Select ExtClk mode div1
			//DATAREG0[15]='0';//0=INT_CLK
			//DATAREG0[14]='0';//0=div2
			//DATAREG0[13]='1';//0=div4
			//DATAREG0[12]='0';//1=RESET
			//DATAREG0[0]='1' ;//0=div 1
			DATAIN=(1 << 13) | (1<<14);//INT_CLK, (div/2 & div/4)
			__Write_SPI(CHIP,0,0x00,DATAIN);//int caddr, int engine, int reg, int data

			//Reset all instances (0-15):
			//Reset engine 0:
			//DATAREG0[12]='1';
			DATAIN=(1 << 13) | (1<<14) | (1<<12) ;//INT_CLK, (div/2 & div/4), reset
			__Write_SPI(CHIP,0,0x00,DATAIN);//int caddr, int engine, int reg, int data
		
			//Reset instances 1-15:
			DATAIN=0x1000;// bit[12]==reset
			for(c=1;c<16;c++)
			{
				__Write_SPI(CHIP,c,0,DATAIN);//int caddr, int engine, int reg, int data
			}
		
			//Enable Clock Out, all instances:
			DATAIN=0xFFFF; 
			__Write_SPI(CHIP,0,0x61, DATAIN);//int caddr, int engine, int reg, int data

			//Set Osc Control to slowest frequency:0000 (highest=0xCD55)
			/*
			#if defined(ASICS_OPERATE_AT_250MHZ)
				DATAIN = 0xFFD5; // Operates 250MHz
			#elif defined(ASICS_OPERATE_AT_280MHZ)
				DATAIN = 0x5555; // 280MHz
			#elif defined(ASICS_OPERATE_AT_170MHZ)
				DATAIN = 0x0000;
			#else
				DATAIN = 0x5555; // INVALID STATE
			#endif 
			*/
					
			DATAIN = __ASIC_FREQUENCY_WORDS[__ASIC_FREQUENCY_ACTUAL_INDEX];						
			__Write_SPI(CHIP,0,0x60,DATAIN);

			//Clear the Reset engine 1-15
			DATAIN=0x0000;
			for(c=1;c<16;c++){
				__Write_SPI(CHIP,c,0,DATAIN);
			}

			DATAIN = ((iDisableFlag & (1<<CHIP)) == 0) ? 0xFFFE : 0x0; // Engine 0 clock is not enabled here
			
			// DATAIN = 0x0FFFE;
			__Write_SPI(CHIP,0,0x61,DATAIN);
		}
	
	#endif

	//CHIP STATE: Internal Clock, All Registers Reset,All BUT 0 Resets=0,
	//All clocks Disabled				
	return;				
}

int	ASIC_GetFrequencyFactor()
{
	// NOT IMPLEMENTED
	return 0;
}

void ASIC_SetFrequencyFactor(char iChip, int iFreqFactor)
{
	// if iChip is FF, then we set frequency for all
	__MCU_ASIC_Activate_CS();
	if (iChip == 0xFF)
	{
		__Write_SPI(iChip,0,0x60,iFreqFactor);	
		__Write_SPI(iChip,1,0x60,iFreqFactor);	
		__Write_SPI(iChip,2,0x60,iFreqFactor);	
		__Write_SPI(iChip,3,0x60,iFreqFactor);									
		__Write_SPI(iChip,4,0x60,iFreqFactor);
		__Write_SPI(iChip,5,0x60,iFreqFactor);
		__Write_SPI(iChip,6,0x60,iFreqFactor);
		__Write_SPI(iChip,7,0x60,iFreqFactor);
		
		#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
			__Write_SPI(iChip,8,0x60 ,iFreqFactor);
			__Write_SPI(iChip,9,0x60 ,iFreqFactor);
			__Write_SPI(iChip,10,0x60,iFreqFactor);
			__Write_SPI(iChip,11,0x60,iFreqFactor);
			__Write_SPI(iChip,12,0x60,iFreqFactor);
			__Write_SPI(iChip,13,0x60,iFreqFactor);
			__Write_SPI(iChip,14,0x60,iFreqFactor);
			__Write_SPI(iChip,15,0x60,iFreqFactor);	
		#endif
	}
	else
	{
		__Write_SPI(iChip,0,0x60,iFreqFactor); //int caddr, int engine, int reg, int data	
	}	
	__MCU_ASIC_Deactivate_CS();
}

void ASIC_WriteComplete(char iChip, char iEngine)
{
	// We're all set, for this chips... Tell the engine to start
	if (iEngine == 0)
	{
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT | __ActualRegister0Value));
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value)); // Clear Write-Register Valid Bit
	}
	else
	{
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT));
		__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_WRITE_REGISTER, 0); // Clear Write-Register Valid Bit
	}	
}

int ASIC_are_all_engines_done(unsigned int iChip)
{
	__MCU_ASIC_Activate_CS();
	
	volatile unsigned char iExpectedEnginesToBeDone = 0;
	volatile unsigned char iTotalEnginesDone = 0;
	
	// Check all engines
	for (unsigned int imx = 0; imx < 16; imx++)
	{
		// Is this processor ok? Because if it isn't, we shouldn't check it
		if (!IS_PROCESSOR_OK(iChip, imx)) continue;
		iExpectedEnginesToBeDone++;
		
		// Proceed...
		if ((__AVR32_SC_ReadData(iChip, imx, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_BUSY_BIT) == ASIC_SPI_READ_STATUS_BUSY_BIT) // Means it's busy
		{
			// __MCU_ASIC_Deactivate_CS();
			// return FALSE; // 0 = FALSE
		}
		else
		{
			// We set the flag here
			iTotalEnginesDone++;	
			
			// Also we update the flag for global supervision of engine activity, because THIS FUNCTION IS REPETITIVELY USED AND CALLED			
			#if defined(__ENGINE_ACTIVITY_SUPERVISION)
				GLOBAL_ENGINE_PROCESSING_STATUS[iChip][imx] = FALSE; // Meaning no longer processing			
			#endif
		}			

	}	
	
	__MCU_ASIC_Deactivate_CS();	
		
	// We're ok
	return (iExpectedEnginesToBeDone == iTotalEnginesDone);
}

char ASIC_has_engine_finished_processing(char iChip, char iEngine)
{
	return ((__AVR32_SC_ReadData(iChip, iEngine, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_BUSY_BIT) != ASIC_SPI_READ_STATUS_BUSY_BIT);
}

// How many total processors are there?
int ASIC_get_processor_count()
{
	volatile int iTotalProcessorCount = 0;
	static int iTotalProcessorCountDetected = 0xFFFFFFFF;
	
	if (iTotalProcessorCountDetected != 0xFFFFFFFF) return iTotalProcessorCountDetected;
	
	for (unsigned char xchip =0; xchip < TOTAL_CHIPS_INSTALLED; xchip++)
	{
		if (!CHIP_EXISTS(xchip)) continue;
		
		for (unsigned char yproc = 0; yproc < 16; yproc++)
		{
			if (IS_PROCESSOR_OK(xchip, yproc)) iTotalProcessorCount++;	
		}
	}
	
	iTotalProcessorCountDetected = iTotalProcessorCount;
	return iTotalProcessorCount;
}

// Calculate the lower-range and upper-range of each engine on the board
void ASIC_calculate_engines_nonce_range()
{	
	// First find total processing power = Sigma(total engines per chip x speed of the chip)
	unsigned int iTotalProcessingPower = 0;
	for (char vChip = 0; vChip < TOTAL_CHIPS_INSTALLED; vChip++)
	{
		if (!CHIP_EXISTS(vChip)) continue;
		iTotalProcessingPower += (GLOBAL_CHIP_FREQUENCY_INFO[vChip] * ASIC_get_chip_processor_count(vChip));
	}
	
	// Now we have total processing power, see the TotalAttempts / TotalProcessingPower
	float fAttempt_Per_Unit_In_Total = (0xFFFFFFFF) / iTotalProcessingPower;
	
	// Now Calculate SharePerChip (equaling fAttempt_Per_Unit_In_Total x (total-engines-in-chip x chip-freq)
	float fChipShare = 0;
	
	// Our low and high bound values
	unsigned int  iActualLowBound = 0;
	unsigned int  iChipInitialBound = 0;
	unsigned int  iChipShare = 0;
	unsigned int  iEngineShare = 0;
	unsigned char iLastEnginesIndex = 0;
	unsigned char iLastChipIndex = 0;
		
	// Calculate per chip...
	for (char vChip = 0; vChip < TOTAL_CHIPS_INSTALLED; vChip++)
	{
		// Does chip exist?
		if (!CHIP_EXISTS(vChip)) continue;
		
		// Calculate how much should be given to the chip
		fChipShare = fAttempt_Per_Unit_In_Total * (ASIC_get_chip_processor_count(vChip) * GLOBAL_CHIP_FREQUENCY_INFO[vChip]);
		
		// Now we know the chip share
		iChipShare = (unsigned int)fChipShare;
		iChipInitialBound = iActualLowBound;
		
		// What is the engine share here?
		iEngineShare = iChipShare / ASIC_get_chip_processor_count(vChip);
		
		// Anyway..., give engines their share...
		for (char vEngine = 0; vEngine < 16; vEngine++)
		{
			// Allowed to use engine 0?
			#if defined(DO_NOT_USE_ENGINE_ZERO)
				if (vEngine == 0) continue;
			#endif
			
			// Is the engine OK?
			if (!IS_PROCESSOR_OK(vChip, vEngine)) continue;
			
			// Give it the bound
			GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[vChip][vEngine] =	iActualLowBound;
			GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][vEngine] = iActualLowBound + iEngineShare + ((iEngineShare > 1) ? (-1) : 0);
			iActualLowBound += iEngineShare;
			
			// Remember the last engines index
			iLastEnginesIndex = vEngine;
		}
		
		// Set last engines upper bound to the correct value
		GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][iLastEnginesIndex] = iChipInitialBound + iChipShare + ((iChipShare > 1) ? (-1) : 0);
		if (GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][iLastEnginesIndex] != 0) GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[vChip][iLastEnginesIndex]--; 
		
		// Now, set the actual low bound to chip initial low bound + 
		iActualLowBound = iChipInitialBound + iChipShare;
		
		// Remember the last chip we visited
		iLastChipIndex = vChip;
	}
	
	// Correct the last chips last engines upper board
	GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[iLastChipIndex][iLastEnginesIndex] = 0xFFFFFFFF;
		
	// We're done...
}

// How many total processors are there?
int ASIC_get_chip_processor_count(char iChip)
{
	int iTotalProcessorCount = 0;
	if (!CHIP_EXISTS(iChip)) return 0;	
	for (unsigned char yproc = 0; yproc < 16; yproc++) { if (IS_PROCESSOR_OK(iChip, yproc)) iTotalProcessorCount++;	}
	return iTotalProcessorCount;
}

int ASIC_get_job_status(unsigned int *iNonceList, unsigned int *iNonceCount, const char iCheckOnlyOneChip, const char iChipToCheck)
{
	// Check if any chips are done...
	int iChipsDone = 0;
	int iChipCount;
	int iDetectedNonces = 0;
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) 
		char iChipDoneFlag[TOTAL_CHIPS_INSTALLED] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	#else		
		char iChipDoneFlag[TOTAL_CHIPS_INSTALLED] = {0,0,0,0,0,0,0,0};
	#endif
	
	for (unsigned char index = ((iCheckOnlyOneChip == TRUE) ? iChipToCheck : 0);  
		 index < TOTAL_CHIPS_INSTALLED; 
		 index++)
	{
		if (CHIP_EXISTS(index))
		{			
			if (ASIC_are_all_engines_done(index) == FALSE) 
			{	
				return ASIC_JOB_NONCE_PROCESSING;
			}	
			else
			{
				iChipsDone++;
				iChipDoneFlag[index] = TRUE;
			}			
		}
		
		// Are we checking a single chip only? If so, exit here			
		if (iCheckOnlyOneChip == TRUE) break;
	}
						
	// Since jobs are divided equally among the engines. they will all nearly finish the same time... 
	// (Almost that is...)
	if (iChipsDone != ((iCheckOnlyOneChip == TRUE) ? 1 : (ASIC_get_chip_count())))
	{
		// We're not done yet...
		return ASIC_JOB_NONCE_PROCESSING;
	}
	
	// Get the chip count and also check how many chips are IDLE
	iChipCount = ASIC_get_chip_count();
	
	// Get the number of engines
	unsigned char iTotalEngines = 0; 
	if (iCheckOnlyOneChip == FALSE) 
	{ 
		iTotalEngines = ASIC_get_processor_count(); 
	}		
	else 
	{
		iTotalEngines = ASIC_get_chip_processor_count(iChipToCheck);
	}		
	
	unsigned char iEnginesIDLE = 0;
		
	// Activate the chips
	__MCU_ASIC_Activate_CS();
					
	// Try to read their results...
	for (int x_chip = ((iCheckOnlyOneChip == TRUE) ? iChipToCheck : 0) ; x_chip < TOTAL_CHIPS_INSTALLED; x_chip++)
	{
		// Check for existence
		if (CHIP_EXISTS(x_chip) == FALSE)
		{
			if (iCheckOnlyOneChip == TRUE)
			{
				break;
			}				
			else
			{
				continue;
			}				
		}			 
		
		// Check done 
		if (iChipDoneFlag[x_chip] == FALSE)
		{
			if (iCheckOnlyOneChip == TRUE)
			{
				break;
			}
			else
			{				
				continue; // Skip it...
			}				
		}			 
		
		// Test all engines
		for (int y_engine = 0; y_engine < 16; y_engine++)
		{
			// Is this processor healthy?
			if (!IS_PROCESSOR_OK(x_chip,  y_engine)) continue;
			
			// Proceed
			volatile unsigned int i_status_reg = __ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_READ_STATUS_REGISTER);
			
			// Is this engine IDLE?
			if ((i_status_reg & (ASIC_SPI_READ_STATUS_DONE_BIT | ASIC_SPI_READ_STATUS_BUSY_BIT)) == 0)
			{
				// This engine is IDLE
				iEnginesIDLE++;
				continue; 
			}

			// Is DONE bit set for this engine?
			if ((i_status_reg & ASIC_SPI_READ_STATUS_DONE_BIT) != ASIC_SPI_READ_STATUS_DONE_BIT)
			{
				 continue;
			}				 
			
			// Check FIFO depths
			unsigned int iLocNonceCount = 0;
			unsigned int iLocNonceFlag[8] = {0,0,0,0,0,0,0,0};
					
			// Check nonce existence
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH1_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[0] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT) != 0) { iLocNonceCount++;	iLocNonceFlag[1] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH3_BIT) != 0) { iLocNonceCount++;	iLocNonceFlag[2] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH4_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[3] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH5_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[4] = 1; } 
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH6_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[5] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH7_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[6] = 1; }
			if ((i_status_reg & ASIC_SPI_READ_STATUS_FIFO_DEPTH8_BIT) != 0) { iLocNonceCount++; iLocNonceFlag[7] = 1; }
				
			// Do any nonces exist?
			if (iLocNonceCount == 0)
			{
				// Clear FIFO for this engine
				if (y_engine == 0)
				{
					__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT | __ActualRegister0Value));
					__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value));
				}
				else
				{
					__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT));
					__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (0));
				}		 
				
				// Proceed
				continue;
			}				 
				
			// Read them one by one
			volatile unsigned int iNonceX = 0; 

			// Check total nonces found
			if (iDetectedNonces >= 16) break;
					
			// Check
			if (iLocNonceFlag[0] == TRUE) 
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO0_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO0_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;			
				
			// Check				   
			if (iLocNonceFlag[1] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO1_LWORD)) |
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO1_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;				   
			
			// Check
			if (iLocNonceFlag[2] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO2_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO2_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
			
			// Check
			if (iLocNonceFlag[3] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO3_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO3_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
			
			// Check
			if (iLocNonceFlag[4] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO4_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO4_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
			
			// Check
			if (iLocNonceFlag[5] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO5_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO5_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
			
			// Check
		    if (iLocNonceFlag[6] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO6_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO6_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
			
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
			
			// Check
		    if (iLocNonceFlag[7] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO7_LWORD)) |
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO7_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}	
					
			// Clear the engine [ NOTE : CORRECT !!!!!!!!!!!! ]
			if (y_engine == 0)
			{
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT | __ActualRegister0Value));
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value));
			}
			else
			{
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT));
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (0));				
			}
						
			// Check total nonces found
			if (iDetectedNonces >= 16) break;
		}
		
		// Are we checking only one chip? If so, we need to exit
		if (iCheckOnlyOneChip == TRUE) break;
		
	}
	
	// Deactivate the CHIPs
	__MCU_ASIC_Deactivate_CS();
	
	// Set the read count
	*iNonceCount = iDetectedNonces;
	
	// What to return...
	if (iEnginesIDLE > (iTotalEngines >> 1))
	{
		return ASIC_JOB_IDLE;
	}
	else
	{
		return (iDetectedNonces > 0) ? ASIC_JOB_NONCE_FOUND : ASIC_JOB_NONCE_NO_NONCE;
	}
	
}

#define MACRO__ASIC_WriteEngine(x,y,z,d)				__AVR32_SC_WriteData(x,y,z,d);
#define MACRO__ASIC_WriteEngine_SecondBank(x,y,z,d)		__AVR32_SC_WriteData(x,y,z,d);


#define MACRO__AVR32_SPI0_SendWord_Express2(x) \
	({ \
		AVR32_SPI0_TDR = (x & 0x0FFFF); \
		while ((AVR32_SPI0_SR & (1 << 1)) == 0); \
	})
	
#define MACRO__AVR32_SPI1_SendWord_Express2(x) \
	({ \
	AVR32_SPI1_TDR = (x & 0x0FFFF); \
	while ((AVR32_SPI1_SR & (1 << 1)) == 0); \
	})	

#define MACRO__ASIC_WriteEngineExpress(x,y,z,d) \
	{ \
		unsigned int iCommand = 0; \
		iCommand = (((unsigned int)(x   & 0x0FF) << 12 ) &  0b0111000000000000) | \
				   (((unsigned int)(y   & 0x0FF) << 8  ) &  0b0000111100000000) | \
				   (((unsigned int)(z   & 0x0FF)       ) &  0b0000000011111111); \		
		MACRO__AVR32_SPI0_SendWord_Express2(iCommand); \
		MACRO__AVR32_SPI0_SendWord_Express2(d & 0x0FFFF); \
	}
	
#define MACRO__ASIC_WriteEngineExpress_SecondBank(x,y,z,d) \
	{ \
		unsigned int iCommand = 0; \
		iCommand = (((unsigned int)(x   & 0x0FF) << 12 ) &  0b0111000000000000) | \
					(((unsigned int)(y   & 0x0FF) << 8  ) &  0b0000111100000000) | \
					(((unsigned int)(z   & 0x0FF)       ) &  0b0000000011111111); \
		MACRO__AVR32_SPI1_SendWord_Express2(iCommand); \
		MACRO__AVR32_SPI1_SendWord_Express2(d & 0x0FFFF); \
	}	

void ASIC_job_issue(void* pJobPacket, 
					unsigned int _LowRange, 
					unsigned int _HighRange,
					const char bIssueToSingleChip,
					const char iChipToIssue)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	if (_HighRange - _LowRange < 256) return;
	
	// Does chip exist? If not, return
	if ((bIssueToSingleChip == TRUE) && (CHIP_EXISTS(iChipToIssue) == FALSE)) return; // We won't do anything as the chip doesn't exist
	
	// Ok, since we're issuing a job, Activate pulsing if it has been requested
	#if defined(ENABLE_JOB_PULSING_SYSTEM)
		System_Request_Pulse_Blink();
	#endif
	
	// Break job between chips...
	volatile int iChipCount  = (bIssueToSingleChip == FALSE) ? (ASIC_get_chip_count()) : 1;
	volatile int iProcessorCount = (bIssueToSingleChip == FALSE) ? (ASIC_get_processor_count()) : (ASIC_get_chip_processor_count(iChipToIssue));
	// volatile int iProcessorCount = 1; // TEST
	
	volatile unsigned int iRangeSlice = ((_HighRange - _LowRange) / iProcessorCount);
	volatile unsigned int iRemainder = (_HighRange - _LowRange) - (iRangeSlice * iProcessorCount); // This is reserved for the last engine in the last chip
	volatile unsigned int iLowerRange = _LowRange;
	volatile unsigned int iUpperRange = _LowRange + iRangeSlice;
	
	volatile int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	NOP_OPERATION;
	
	// Have we ever come here before? If so, don't program static data in the registers any longer
	static unsigned char bIsThisOurFirstTime = TRUE;

	for (volatile int x_chip = (bIssueToSingleChip == TRUE) ? (iChipToIssue) : (0); 
		 x_chip < TOTAL_CHIPS_INSTALLED; x_chip++)
	{
		// Check chip existence
		if (CHIP_EXISTS(x_chip) == FALSE) continue; // Skip it...
		
		if (x_chip < 8)
		{
			// We assign the job to each engine in each chip
			for (volatile int y_engine = 0; y_engine < 16; y_engine++)
			{
				// Is this processor healthy?
				if (!IS_PROCESSOR_OK(x_chip, y_engine)) continue;
			
				// STATIC RULE - Engine 0 not used
				#if defined(DO_NOT_USE_ENGINE_ZERO)
					if (y_engine == 0) continue; // Do not start engine 0
				#endif
			
				// Reset the engine
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, (1<<9) | (1<<12));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, 0);
			
				// Set limit register
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);			

				// Load H1 (STATIC)
				if (bIsThisOurFirstTime)
				{
					// Set static H values [0..7]
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x90,0xE667);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x91,0x6A09);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x92,0xAE85);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x93,0xBB67);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x94,0xF372);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x95,0x3C6E);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x96,0xF53A);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x97,0xA54F);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x98,0x527F);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x99,0x510E);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9A,0x688C);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9B,0x9B05);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9C,0xD9AB);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9D,0x1F83);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9E,0xCD19);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9F,0x5BE0);				
				}
			
				// Load H0 (MIDSTATE)
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x80,((pjob_packet)(pJobPacket))->midstate[0]  | (((pjob_packet)(pJobPacket))->midstate[1]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x81,((pjob_packet)(pJobPacket))->midstate[2]  | (((pjob_packet)(pJobPacket))->midstate[3]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x82,((pjob_packet)(pJobPacket))->midstate[4]  | (((pjob_packet)(pJobPacket))->midstate[5]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x83,((pjob_packet)(pJobPacket))->midstate[6]  | (((pjob_packet)(pJobPacket))->midstate[7]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x84,((pjob_packet)(pJobPacket))->midstate[8]  | (((pjob_packet)(pJobPacket))->midstate[9]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x85,((pjob_packet)(pJobPacket))->midstate[10] | (((pjob_packet)(pJobPacket))->midstate[11] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x86,((pjob_packet)(pJobPacket))->midstate[12] | (((pjob_packet)(pJobPacket))->midstate[13] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x87,((pjob_packet)(pJobPacket))->midstate[14] | (((pjob_packet)(pJobPacket))->midstate[15] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x88,((pjob_packet)(pJobPacket))->midstate[16] | (((pjob_packet)(pJobPacket))->midstate[17] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x89,((pjob_packet)(pJobPacket))->midstate[18] | (((pjob_packet)(pJobPacket))->midstate[19] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8A,((pjob_packet)(pJobPacket))->midstate[20] | (((pjob_packet)(pJobPacket))->midstate[21] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8B,((pjob_packet)(pJobPacket))->midstate[22] | (((pjob_packet)(pJobPacket))->midstate[23] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8C,((pjob_packet)(pJobPacket))->midstate[24] | (((pjob_packet)(pJobPacket))->midstate[25] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8D,((pjob_packet)(pJobPacket))->midstate[26] | (((pjob_packet)(pJobPacket))->midstate[27] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8E,((pjob_packet)(pJobPacket))->midstate[28] | (((pjob_packet)(pJobPacket))->midstate[29] << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8F,((pjob_packet)(pJobPacket))->midstate[30] | (((pjob_packet)(pJobPacket))->midstate[31] << 8));			

				// Load W (From JOBs)
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA0,((pjob_packet)(pJobPacket))->block_data[0]  | (((pjob_packet)(pJobPacket))->block_data[1]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA1,((pjob_packet)(pJobPacket))->block_data[2]  | (((pjob_packet)(pJobPacket))->block_data[3]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA2,((pjob_packet)(pJobPacket))->block_data[4]  | (((pjob_packet)(pJobPacket))->block_data[5]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA3,((pjob_packet)(pJobPacket))->block_data[6]  | (((pjob_packet)(pJobPacket))->block_data[7]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA4,((pjob_packet)(pJobPacket))->block_data[8]  | (((pjob_packet)(pJobPacket))->block_data[9]  << 8));
				MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA5,((pjob_packet)(pJobPacket))->block_data[10] | (((pjob_packet)(pJobPacket))->block_data[11] << 8));
			
				if (bIsThisOurFirstTime)
				{
					// STATIC [W0]
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA8,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA9,0x8000);
			
					// STATIC [W1]
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAA,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAB,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAC,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAD,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAE,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAF,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB0,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB1,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB2,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB3,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB4,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB5,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB6,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB7,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB8,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB9,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBA,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBB,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBC,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBD,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBE,0x0280);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBF,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC0,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC1,0x8000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC2,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC3,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC4,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC5,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC6,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC7,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC8,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC9,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCA,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCB,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCC,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCD,0x0000);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCE,0x0100);
					MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCF,0x0000);			
				}			
	
				// All data sent, now set range
				if (bIssueToSingleChip == TRUE)
				{
					MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
					MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
					MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
					MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);					
				}
				else
				{
					#if defined(__ACTIVATE_JOB_LOAD_BALANCING)
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine]  & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine]  & 0x0FFFF0000) >> 16);
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine] & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine] & 0x0FFFF0000) >> 16);
						
						iUpperRange = GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine];
						iLowerRange = GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine];
					#else
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
					#endif					
				}


	
				//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0x4250));
				//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0x38E9));
				//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (0x4280));
				//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (0x38E9));
	
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);	
			
				// Should we export spreads?
				#if defined(__EXPORT_ENGINE_RANGE_SPREADS)			
					__ENGINE_LOWRANGE_SPREADS[x_chip][y_engine] = iLowerRange;
					__ENGINE_HIGHRANGE_SPREADS[x_chip][y_engine] = iUpperRange;
				#endif
			
				// We're all set, for this chips... Tell the engine to start
				if ((y_engine == 0))
				{
					MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT | __ActualRegister0Value));			
					MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value)); // Clear Write-Register Valid Bit
				}
				else
				{
					MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT ));			
					MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, 0); // Clear Write-Register Valid Bit
				}
								
			
				// Increase our range
				iLowerRange += iRangeSlice;
				iUpperRange += iRangeSlice;
				if (iUpperRange < iLowerRange) iUpperRange = 0xFFFFFFFF; // Last Number
				if (iUpperRange > 0x0FFFFFF00) iUpperRange = 0xFFFFFFFF;
			}	
		}
		#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
			else // We are from chip number 8 to 15
			{
				// Correct chip identification
				char x_chip_b2 = x_chip - 8;
			
				// We assign the job to each engine in each chip
				for (volatile int y_engine = 0; y_engine < 16; y_engine++)
				{
					// Is this processor healthy?
					if (!IS_PROCESSOR_OK(x_chip_b2, y_engine)) continue;
					
					// STATIC RULE - Engine 0 not used
					#if defined(DO_NOT_USE_ENGINE_ZERO)
						if (y_engine == 0) continue; // Do not start engine 0
					#endif
					
					// Reset the engine
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine, 0, (1<<9) | (1<<12));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine, 0, 0);
					
					// Set limit register
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);

					// Load H1 (STATIC)
					if (bIsThisOurFirstTime)
					{
						// Set static H values [0..7]
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x90,0xE667);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x91,0x6A09);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x92,0xAE85);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x93,0xBB67);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x94,0xF372);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x95,0x3C6E);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x96,0xF53A);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x97,0xA54F);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x98,0x527F);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x99,0x510E);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9A,0x688C);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9B,0x9B05);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9C,0xD9AB);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9D,0x1F83);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9E,0xCD19);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9F,0x5BE0);
					}
					
					// Load H0 (MIDSTATE)
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x80,((pjob_packet)(pJobPacket))->midstate[0]  | (((pjob_packet)(pJobPacket))->midstate[1]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x81,((pjob_packet)(pJobPacket))->midstate[2]  | (((pjob_packet)(pJobPacket))->midstate[3]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x82,((pjob_packet)(pJobPacket))->midstate[4]  | (((pjob_packet)(pJobPacket))->midstate[5]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x83,((pjob_packet)(pJobPacket))->midstate[6]  | (((pjob_packet)(pJobPacket))->midstate[7]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x84,((pjob_packet)(pJobPacket))->midstate[8]  | (((pjob_packet)(pJobPacket))->midstate[9]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x85,((pjob_packet)(pJobPacket))->midstate[10] | (((pjob_packet)(pJobPacket))->midstate[11] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x86,((pjob_packet)(pJobPacket))->midstate[12] | (((pjob_packet)(pJobPacket))->midstate[13] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x87,((pjob_packet)(pJobPacket))->midstate[14] | (((pjob_packet)(pJobPacket))->midstate[15] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x88,((pjob_packet)(pJobPacket))->midstate[16] | (((pjob_packet)(pJobPacket))->midstate[17] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x89,((pjob_packet)(pJobPacket))->midstate[18] | (((pjob_packet)(pJobPacket))->midstate[19] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8A,((pjob_packet)(pJobPacket))->midstate[20] | (((pjob_packet)(pJobPacket))->midstate[21] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8B,((pjob_packet)(pJobPacket))->midstate[22] | (((pjob_packet)(pJobPacket))->midstate[23] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8C,((pjob_packet)(pJobPacket))->midstate[24] | (((pjob_packet)(pJobPacket))->midstate[25] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8D,((pjob_packet)(pJobPacket))->midstate[26] | (((pjob_packet)(pJobPacket))->midstate[27] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8E,((pjob_packet)(pJobPacket))->midstate[28] | (((pjob_packet)(pJobPacket))->midstate[29] << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8F,((pjob_packet)(pJobPacket))->midstate[30] | (((pjob_packet)(pJobPacket))->midstate[31] << 8));
											   
					// Load W (From JOBs)		   
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA0,((pjob_packet)(pJobPacket))->block_data[0]  | (((pjob_packet)(pJobPacket))->block_data[1]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA1,((pjob_packet)(pJobPacket))->block_data[2]  | (((pjob_packet)(pJobPacket))->block_data[3]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA2,((pjob_packet)(pJobPacket))->block_data[4]  | (((pjob_packet)(pJobPacket))->block_data[5]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA3,((pjob_packet)(pJobPacket))->block_data[6]  | (((pjob_packet)(pJobPacket))->block_data[7]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA4,((pjob_packet)(pJobPacket))->block_data[8]  | (((pjob_packet)(pJobPacket))->block_data[9]  << 8));
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA5,((pjob_packet)(pJobPacket))->block_data[10] | (((pjob_packet)(pJobPacket))->block_data[11] << 8));
					
					if (bIsThisOurFirstTime)
					{
						// STATIC [W0]
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA8,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA9,0x8000);
						
						// STATIC [W1]
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAA,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAB,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAC,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAD,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAE,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAF,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB0,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB1,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB2,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB3,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB4,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB5,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB6,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB7,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB8,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB9,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBA,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBB,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBC,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBD,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBE,0x0280);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBF,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC0,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC1,0x8000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC2,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC3,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC4,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC5,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC6,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC7,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC8,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC9,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCA,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCB,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCC,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCD,0x0000);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCE,0x0100);
						MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCF,0x0000);
					}
					
					// All data sent, now set range
					if (bIssueToSingleChip == TRUE)
					{
						MACRO__ASIC_WriteEngineExpress(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
						MACRO__ASIC_WriteEngineExpress(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
						MACRO__ASIC_WriteEngineExpress(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
					}
					else
					{
						#if defined(__ACTIVATE_JOB_LOAD_BALANCING)
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine]  & 0x0FFFF));
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine]  & 0x0FFFF0000) >> 16);
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine] & 0x0FFFF));
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine] & 0x0FFFF0000) >> 16);
							iUpperRange = GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[x_chip][y_engine];
							iLowerRange = GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[x_chip][y_engine];
						#else
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
							MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
						#endif
					}
					
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
					MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);
					
					// Should we export spreads?
					#if defined(__EXPORT_ENGINE_RANGE_SPREADS)
						__ENGINE_LOWRANGE_SPREADS[x_chip][y_engine] = iLowerRange;
						__ENGINE_HIGHRANGE_SPREADS[x_chip][y_engine] = iUpperRange;
					#endif
					
					// We're all set, for this chips... Tell the engine to start
					if ((y_engine == 0))
					{
						MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT | __ActualRegister0Value));
						MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value)); // Clear Write-Register Valid Bit
					}
					else
					{
						MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT ));
						MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, 0); // Clear Write-Register Valid Bit
					}
					
					
					// Increase our range
					iLowerRange += iRangeSlice;
					iUpperRange += iRangeSlice;
					if (iUpperRange < iLowerRange) iUpperRange = 0xFFFFFFFF; // Last Number
					if (iUpperRange > 0x0FFFFFF00) iUpperRange = 0xFFFFFFFF;
				}	
			}
		#endif		
	
		// Chip Started, so make it blink
		if (GLOBAL_ChipActivityLEDCounter[x_chip] == 0) { GLOBAL_ChipActivityLEDCounter[x_chip] = 10; }
		
		// Are we issuing to one chip only? If so, we need to exit here as we've already issued the job to the desired chip
		if (bIssueToSingleChip == TRUE) break;	
	}	
	
	// Ok It's no longer our fist time
	// WARNING: TO BE CORRECTED. THIS CAN CAUSE PROBLEM IF WE'RE ISSUEING TO A SINGLE ENGINE AND OTHERS ARENT INITIALIZED
	// THIS WILL NOT BE THE CASE TODAY AS WE DIAGNOSE THE ENGINES IN STARTUP. BUT IT COULD BE PROBLEMATIC IF WE DON'T DIAGNOSE
	// ON STARTUP (I.E. CHIPS WILL NOT HAVE THEIR STATIC VALUES SET)
	// ORIGINAL> if (bIssueToSingleChip == FALSE) bIsThisOurFirstTime = FALSE; // We only set it to false if we're sending the job to all engines on the BOARD
	bIsThisOurFirstTime = FALSE; // We only set it to false if we're sending the job to all engines on the BOARD

	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();
}

void ASIC_job_start_processing(char iChip, char iEngine, char bForcedStart)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	char x_chip = iChip;
	char y_engine = iEngine;

	if (bForcedStart == FALSE)
	{
		if (!CHIP_EXISTS(iChip)) return;
		if (!IS_PROCESSOR_OK(x_chip, y_engine)) return;
	}
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	NOP_OPERATION;
	NOP_OPERATION;	
	
	// Set Write-Register valid
	// We're all set, for this chips... Tell the engine to start
	if ((y_engine == 0))
	{
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT | __ActualRegister0Value));
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value)); // Clear Write-Register Valid Bit
	}
	else
	{
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT ));
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, 0); // Clear Write-Register Valid Bit
	}
				
	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();
}


void ASIC_job_issue_to_specified_engine(char  iChip, 
										char  iEngine,
										void* pJobPacket,
										char  bLoadStaticData,
										char  bResetBeforStart,
										unsigned int _LowRange,
										unsigned int _HighRange)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	char x_chip = iChip;
	char y_engine = iEngine;
		
	if (!CHIP_EXISTS(iChip)) return;
	if (!IS_PROCESSOR_OK(x_chip, y_engine)) return;
	
	// STATIC RULE - Engine 0 not used
	#if defined(DO_NOT_USE_ENGINE_ZERO)
		if (y_engine == 0) return; // Do not start engine 0
	#endif
	
	volatile int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	NOP_OPERATION;
	NOP_OPERATION;
	
	if (x_chip < 8)
	{
		// Reset the engine
		if (bResetBeforStart == TRUE)
		{
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, (1<<9) | (1<<12));
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, 0);
		}
	
		// Set static H values [0..7]
		if (bLoadStaticData == TRUE)
		{
			// Set limit register
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);
			
			// Proceed	
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x90,0xE667);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x91,0x6A09);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x92,0xAE85);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x93,0xBB67);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x94,0xF372);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x95,0x3C6E);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x96,0xF53A);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x97,0xA54F);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x98,0x527F);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x99,0x510E);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9A,0x688C);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9B,0x9B05);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9C,0xD9AB);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9D,0x1F83);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9E,0xCD19);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x9F,0x5BE0);		
		}
	
				
		// Load H0 (MIDSTATE)
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x80,((pjob_packet)(pJobPacket))->midstate[0]  | (((pjob_packet)(pJobPacket))->midstate[1]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x81,((pjob_packet)(pJobPacket))->midstate[2]  | (((pjob_packet)(pJobPacket))->midstate[3]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x82,((pjob_packet)(pJobPacket))->midstate[4]  | (((pjob_packet)(pJobPacket))->midstate[5]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x83,((pjob_packet)(pJobPacket))->midstate[6]  | (((pjob_packet)(pJobPacket))->midstate[7]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x84,((pjob_packet)(pJobPacket))->midstate[8]  | (((pjob_packet)(pJobPacket))->midstate[9]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x85,((pjob_packet)(pJobPacket))->midstate[10] | (((pjob_packet)(pJobPacket))->midstate[11] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x86,((pjob_packet)(pJobPacket))->midstate[12] | (((pjob_packet)(pJobPacket))->midstate[13] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x87,((pjob_packet)(pJobPacket))->midstate[14] | (((pjob_packet)(pJobPacket))->midstate[15] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x88,((pjob_packet)(pJobPacket))->midstate[16] | (((pjob_packet)(pJobPacket))->midstate[17] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x89,((pjob_packet)(pJobPacket))->midstate[18] | (((pjob_packet)(pJobPacket))->midstate[19] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8A,((pjob_packet)(pJobPacket))->midstate[20] | (((pjob_packet)(pJobPacket))->midstate[21] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8B,((pjob_packet)(pJobPacket))->midstate[22] | (((pjob_packet)(pJobPacket))->midstate[23] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8C,((pjob_packet)(pJobPacket))->midstate[24] | (((pjob_packet)(pJobPacket))->midstate[25] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8D,((pjob_packet)(pJobPacket))->midstate[26] | (((pjob_packet)(pJobPacket))->midstate[27] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8E,((pjob_packet)(pJobPacket))->midstate[28] | (((pjob_packet)(pJobPacket))->midstate[29] << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0x8F,((pjob_packet)(pJobPacket))->midstate[30] | (((pjob_packet)(pJobPacket))->midstate[31] << 8));			
			
		// Load W (From JOBs)
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA0,((pjob_packet)(pJobPacket))->block_data[0]  | (((pjob_packet)(pJobPacket))->block_data[1]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA1,((pjob_packet)(pJobPacket))->block_data[2]  | (((pjob_packet)(pJobPacket))->block_data[3]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA2,((pjob_packet)(pJobPacket))->block_data[4]  | (((pjob_packet)(pJobPacket))->block_data[5]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA3,((pjob_packet)(pJobPacket))->block_data[6]  | (((pjob_packet)(pJobPacket))->block_data[7]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA4,((pjob_packet)(pJobPacket))->block_data[8]  | (((pjob_packet)(pJobPacket))->block_data[9]  << 8));
		MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA5,((pjob_packet)(pJobPacket))->block_data[10] | (((pjob_packet)(pJobPacket))->block_data[11] << 8));

		if (bLoadStaticData == TRUE)
		{
			// STATIC [W0]
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA8,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xA9,0x8000);
			
			// STATIC [W1]
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAA,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAB,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAC,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAD,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAE,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xAF,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB0,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB1,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB2,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB3,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB4,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB5,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB6,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB7,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB8,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xB9,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBA,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBB,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBC,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBD,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBE,0x0280);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xBF,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC0,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC1,0x8000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC2,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC3,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC4,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC5,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC6,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC7,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC8,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xC9,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCA,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCB,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCC,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCD,0x0000);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCE,0x0100);
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine,0xCF,0x0000);		
		
			// Load barrier offset
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);
		}
			
		// All data sent, now set range
		MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (_LowRange & 0x0FFFF));
		MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (_LowRange & 0x0FFFF0000) >> 16);
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (_HighRange & 0x0FFFF));
		MACRO__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (_HighRange & 0x0FFFF0000) >> 16);
		
	}
	else
	{
		// Second bank chip real number
		volatile char x_chip_b2 = x_chip - 8;
		
		// Reset the engine
		if (bResetBeforStart == TRUE)
		{
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine, 0, (1<<9) | (1<<12));
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine, 0, 0);
		}
		
		// Set static H values [0..7]
		if (bLoadStaticData == TRUE)
		{
			// Set limit register
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);
			
			// Proceed
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x90,0xE667);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x91,0x6A09);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x92,0xAE85);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x93,0xBB67);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x94,0xF372);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x95,0x3C6E);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x96,0xF53A);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x97,0xA54F);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x98,0x527F);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x99,0x510E);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9A,0x688C);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9B,0x9B05);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9C,0xD9AB);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9D,0x1F83);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9E,0xCD19);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x9F,0x5BE0);
		}
		
		
		// Load H0 (MIDSTATE)
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x80,((pjob_packet)(pJobPacket))->midstate[0]  | (((pjob_packet)(pJobPacket))->midstate[1]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x81,((pjob_packet)(pJobPacket))->midstate[2]  | (((pjob_packet)(pJobPacket))->midstate[3]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x82,((pjob_packet)(pJobPacket))->midstate[4]  | (((pjob_packet)(pJobPacket))->midstate[5]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x83,((pjob_packet)(pJobPacket))->midstate[6]  | (((pjob_packet)(pJobPacket))->midstate[7]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x84,((pjob_packet)(pJobPacket))->midstate[8]  | (((pjob_packet)(pJobPacket))->midstate[9]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x85,((pjob_packet)(pJobPacket))->midstate[10] | (((pjob_packet)(pJobPacket))->midstate[11] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x86,((pjob_packet)(pJobPacket))->midstate[12] | (((pjob_packet)(pJobPacket))->midstate[13] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x87,((pjob_packet)(pJobPacket))->midstate[14] | (((pjob_packet)(pJobPacket))->midstate[15] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x88,((pjob_packet)(pJobPacket))->midstate[16] | (((pjob_packet)(pJobPacket))->midstate[17] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x89,((pjob_packet)(pJobPacket))->midstate[18] | (((pjob_packet)(pJobPacket))->midstate[19] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8A,((pjob_packet)(pJobPacket))->midstate[20] | (((pjob_packet)(pJobPacket))->midstate[21] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8B,((pjob_packet)(pJobPacket))->midstate[22] | (((pjob_packet)(pJobPacket))->midstate[23] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8C,((pjob_packet)(pJobPacket))->midstate[24] | (((pjob_packet)(pJobPacket))->midstate[25] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8D,((pjob_packet)(pJobPacket))->midstate[26] | (((pjob_packet)(pJobPacket))->midstate[27] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8E,((pjob_packet)(pJobPacket))->midstate[28] | (((pjob_packet)(pJobPacket))->midstate[29] << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0x8F,((pjob_packet)(pJobPacket))->midstate[30] | (((pjob_packet)(pJobPacket))->midstate[31] << 8));
		
		// Load W (From JOBs)
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA0,((pjob_packet)(pJobPacket))->block_data[0]  | (((pjob_packet)(pJobPacket))->block_data[1]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA1,((pjob_packet)(pJobPacket))->block_data[2]  | (((pjob_packet)(pJobPacket))->block_data[3]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA2,((pjob_packet)(pJobPacket))->block_data[4]  | (((pjob_packet)(pJobPacket))->block_data[5]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA3,((pjob_packet)(pJobPacket))->block_data[6]  | (((pjob_packet)(pJobPacket))->block_data[7]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA4,((pjob_packet)(pJobPacket))->block_data[8]  | (((pjob_packet)(pJobPacket))->block_data[9]  << 8));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA5,((pjob_packet)(pJobPacket))->block_data[10] | (((pjob_packet)(pJobPacket))->block_data[11] << 8));

		if (bLoadStaticData == TRUE)
		{
			// STATIC [W0]
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA8,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xA9,0x8000);
			
			// STATIC [W1]
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAA,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAB,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAC,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAD,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAE,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xAF,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB0,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB1,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB2,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB3,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB4,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB5,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB6,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB7,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB8,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xB9,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBA,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBB,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBC,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBD,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBE,0x0280);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xBF,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC0,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC1,0x8000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC2,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC3,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC4,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC5,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC6,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC7,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC8,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xC9,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCA,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCB,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCC,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCD,0x0000);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCE,0x0100);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2,y_engine,0xCF,0x0000);
			
			// Load barrier offset
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
			MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);
		}
		
		// All data sent, now set range
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (_LowRange & 0x0FFFF));
		MACRO__ASIC_WriteEngineExpress_SecondBank(x_chip_b2, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (_LowRange & 0x0FFFF0000) >> 16);
		
		MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (_HighRange & 0x0FFFF)); // We use original chip number here, not the x_chip_b2
		MACRO__ASIC_WriteEngine_SecondBank(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (_HighRange & 0x0FFFF0000) >> 16);  // We use original chip number here, not the x_chip_b2
	}
			
	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();
}

// Reset the engines...
void ASIC_reset_engine(char iChip, char iEngine)
{
	__MCU_ASIC_Activate_CS();
	__Reset_Engine(iChip, iEngine);
	__MCU_ASIC_Deactivate_CS();
}

///////////////////////////////////////////////////////////////
// A very useful macro, used to tailor code to some extent
///////////////////////////////////////////////////////////////
int ASIC_is_processing()
{
	// Check if any chips are done...
	int iChipsDone = 0;
	int iTotalChips = ASIC_get_chip_count();
	int iTotalChipsDone = 0;
	
	for (unsigned char iChip = 0; iChip < TOTAL_CHIPS_INSTALLED; iChip++)
	{
		if (!CHIP_EXISTS(iChip)) continue;
		
		// Are all processors done in this engine?
		if (ASIC_are_all_engines_done(iChip) == TRUE)
		{
			iTotalChipsDone++;
		}
		else
		{
			// Still processing something
			return TRUE;
		}		
	}
		
	// Since jobs are divided equally among the engines. they will all nearly finish the same time... 
	// (Almost that is...)
	if (iTotalChips != iTotalChipsDone)
	{
		// We're not done yet...
		return TRUE;
	}
	
	// If we've reached here, it means we're not processing anymore
	return FALSE;	
}

int ASIC_is_chip_processing(char iChip)
{
	// Check if any chips are done...
	int iChipsDone = 0;
	int iTotalChips = ASIC_get_chip_count();
	int iTotalChipsDone = 0;
	
	if (!CHIP_EXISTS(iChip)) return FALSE;
		
	// Are all processors done in this engine?
	if (ASIC_are_all_engines_done(iChip) == TRUE)
	{
		// We're no longer processing
		return FALSE;
	}
	else
	{
		// Still processing something
		return TRUE;
	}		
}

int ASIC_get_chip_count()
{
	// Have we already detected this? If so, just return the previously known number
	static int iChipCount = 0;
	
	// This function WILL NOT CHANGE THE MAP IF THE ENUMERATION HAS BEEN PERFORMED BEFORE!
	if (iChipCount != 0)
		return iChipCount;
				
	// Activate CS# of ASIC engines
	__MCU_ASIC_Activate_CS();
		
	// We haven't, so we need to read their register (one by one) to find which ones exist.
	int iActualChipCount = 0;
	
	for (int x_chip = 0; x_chip < TOTAL_CHIPS_INSTALLED; x_chip++)
	{
		#if defined(DECOMISSION_CHIP_15)
			if (x_chip == 15) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_14)
			if (x_chip == 14) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_13)
			if (x_chip == 13) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_12)
			if (x_chip == 12) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_11)
			if (x_chip == 11) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_10)
			if (x_chip == 10) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_9)
			if (x_chip == 9) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_8)
			if (x_chip == 8) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
			
		#if defined(DECOMISSION_CHIP_7)
			if (x_chip == 7) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
		
		#if defined(DECOMISSION_CHIP_6)
			if (x_chip == 6) { __chip_existence_map[x_chip] = 0; continue; }
		#endif		
		
		#if defined(DECOMISSION_CHIP_5)
			if (x_chip == 5) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
		
		#if defined(DECOMISSION_CHIP_3)
			if (x_chip == 3) { __chip_existence_map[x_chip] = 0; continue; }		
		#endif
		
		#if defined(DECOMISSION_CHIP_4)
			if (x_chip == 4) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
			
		#if defined(DECOMISSION_CHIP_2)
			if (x_chip == 2) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
		
		#if defined(DECOMISSION_CHIP_1)
			if (x_chip == 1) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
		
		#if defined(DECOMISSION_CHIP_0)
			if (x_chip == 0) { __chip_existence_map[x_chip] = 0; continue; }
		#endif
		
		// Proceed
		for (int iRepeating = 0; iRepeating < 100; iRepeating++)
		{
			__ASIC_WriteEngine(x_chip, 0, ASIC_SPI_READ_TEST_REGISTER,0x0F81F + x_chip);
			if (__ASIC_ReadEngine(x_chip,0, ASIC_SPI_READ_TEST_REGISTER) == 0x0F81F + x_chip)
			{
				iActualChipCount++;
				
				#if defined(DO_NOT_USE_ENGINE_ZERO)
					__chip_existence_map[x_chip] = 0x0FFFFFFFF & (~(0b01));
				#else
					__chip_existence_map[x_chip] = 0x0FFFFFFFF;
				#endif
				
				break;
			}						
		}
	}
	
	// Deactivate CS# of ASIC engines
	__MCU_ASIC_Deactivate_CS();
	
	iChipCount = iActualChipCount;
	return iActualChipCount;	
}


int ASIC_does_chip_exist(unsigned int iChipIndex)
{
	// Does this chip exist?
	return CHIP_EXISTS(iChipIndex);
}


inline void __ASIC_WriteEngine(char iChip, char iEngine, unsigned int iAddress, unsigned int iData16Bit)
{
	// We issue the SPI command
	//MCU_SC_SetAccess();
	//MCU_SC_WriteData(iChip, iEngine, iAddress, iData16Bit);
	__AVR32_SC_WriteData(iChip, iEngine, iAddress, iData16Bit);
}

inline unsigned int __ASIC_ReadEngine(char iChip, char iEngine, unsigned int iAddress)
{
	// We issue the SPI command
	//MCU_SC_SetAccess();
	// return MCU_SC_ReadData(iChip, iEngine, (unsigned char)iAddress);
	return __AVR32_SC_ReadData(iChip, iEngine, iAddress);
}


