
/*
 * ASIC_Engine.c
 *
 * Created: 20/11/2012 23:17:15
 *  Author: NASSER
 */ 

#include "ASIC_Engine.h"
#include "std_defs.h"
#include "Generic_Module.h"

// Some useful macros
#define CHIP_EXISTS(x)					 ((__chip_existence_map[x] != 0))
#define IS_PROCESSOR_OK(xchip, yengine)  ((__chip_existence_map[xchip] & (1 << yengine)) != 0)

// We have to define this
#define DO_NOT_USE_ENGINE_ZERO

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

// Chip Existence Map
unsigned int __chip_existence_map[8] = {0,0,0,0,0,0,0,0}; // Bit 0 to Bit 16 in each word says if the engine is OK or not...
	
// Midstate for SHA1-Core, this is provided by the MCU (Which is in turn provided by the GetWork API)
void init_ASIC(void)
{
	MCU_SC_Initialize();
	
	// Here we must give 16 clock cycles with CS# inactive, to initialize the ASIC chips
	__MCU_ASIC_Deactivate_CS();
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);
	__ASIC_WriteEngine(0,0,0,0);
	
	// Proceed...
	ASIC_get_chip_count();
	
	// Also here we set the Oscillator Control Register
	ASIC_Bootup_Chips();
}

// This function checks to see if the processor in this engine is correctly operating. We must find the correct nonce here...
char ASIC_diagnose_processor(char iEngine, char iProcessor)
{
	
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

	__AVR32_SC_WriteData(CHIP, ENGINE, 0x00, DATAIN);
}


// Working...
void ASIC_Bootup_Chips()
{
	// Send initialization clocks
	__ASIC_WriteEngine(0,0,0,0);
	
	unsigned int iHover = 0;

	// Reset it...
	for (iHover = 0; iHover < 8; iHover++)
	{
		// We have data to write...
		__MCU_ASIC_Activate_CS();

		/*
		MCU_SC_WriteData(iHover,
						 0b0,// Issue to processor 0
						 ASIC_SPI_OSC_CONTROL,
						 0b0101010101010101); // Slowest mode	
		*/
		
		MCU_SC_WriteData(iHover,
						 0b0,// Issue to processor 0
						 ASIC_SPI_OSC_CONTROL,
						 0b1111111111111111); // Slowest mode		
					 

		MCU_SC_WriteData(iHover,
						 0, // Issue to processor 0
						 0,
						 (__ActualRegister0Value));

		// TEST: Disable clocks for all engines
		MCU_SC_WriteData(iHover, 0, ASIC_SPI_CLOCK_OUT_ENABLE, 0x0FFFF); // Activate clocks for all 16 engines on the chip
								 
						 
		/*MCU_SC_WriteData(iHover,
						 0b0,// Issue to processor 0
						 ASIC_SPI_OSC_CONTROL,
						 0b0101010101010101);*/
						 
					 
		// Enable clock
		// MCU_SC_WriteData(((iHover & 0b01110000) >> 4), (iHover & 0b01111), ASIC_SPI_CLOCK_OUT_ENABLE, 0x0FFFF); // Activate clocks for all 16 engines on the chip
			

		// Activate clocks for all engines
		__MCU_ASIC_Deactivate_CS();
	}	
	
	
	// Now we're done here, we need to execute the next command for all the 256 engines in system
	unsigned int iResetEngineCommand1Base = 0;
	unsigned int iResetEngineCommand2Base = (1<<12);
	unsigned int iResetEngineCommand3Base = 0;
	
	unsigned int iResetEngineCommand1BaseE0 = __ActualRegister0Value;
	unsigned int iResetEngineCommand2BaseE0 = (1<<12) | __ActualRegister0Value;
	unsigned int iResetEngineCommand3BaseE0 = __ActualRegister0Value;
	
	// Run through all the chips
	__MCU_ASIC_Activate_CS();
			
	for (iHover = 0; iHover <= 0x07F; iHover++)
	{
			unsigned int iResetEngineCommand1 = iResetEngineCommand1BaseE0 | (iHover << 24);
			unsigned int iResetEngineCommand2 = iResetEngineCommand2BaseE0 | (iHover << 24);
			unsigned int iResetEngineCommand3 = iResetEngineCommand3BaseE0 | (iHover << 24);
					
			MCU_SC_WriteData((unsigned char)((iResetEngineCommand1 >> 24) & 0x0FF),
							 (unsigned char)((iResetEngineCommand1 >> 16) & 0x0FF),
						 	 (unsigned char)((iResetEngineCommand1 >> 8) & 0x0FF),
							 (unsigned int)((iResetEngineCommand1 ) & 0x0FFFF));

			MCU_SC_WriteData((unsigned char)((iResetEngineCommand2 >> 24) & 0x0FF),
							 (unsigned char)((iResetEngineCommand2 >> 16) & 0x0FF),
							 (unsigned char)((iResetEngineCommand2 >> 8) & 0x0FF),
							 (unsigned int)((iResetEngineCommand2 ) & 0x0FFFF));
							 

			//#if defined(DO_NOT_USE_ENGINE_ZERO)
			//// ENGINE 0 MUST REMAIN IN RESET SINCE W'RE USING DIV2 OR DIV4
			//if ((iHover & 0b01111) == 0)
			//{
				//MCU_SC_WriteData((unsigned char)((iResetEngineCommand3 >> 24) & 0x0FF),
				//(unsigned char)((iResetEngineCommand3 >> 16) & 0x0FF),
				//(unsigned char)((iResetEngineCommand3 >> 8) & 0x0FF),
				//(unsigned int)((iResetEngineCommand3 ) & 0x0FFFF));
			//}
			//#else
			MCU_SC_WriteData((unsigned char)((iResetEngineCommand3 >> 24) & 0x0FF),
							 (unsigned char)((iResetEngineCommand3 >> 16) & 0x0FF),
							 (unsigned char)((iResetEngineCommand3 >> 8) & 0x0FF),
							 (unsigned int)((iResetEngineCommand3 ) & 0x0FFFF));
			
		/*
		__Reset_Engine(((iHover & 0b01110000) >> 4), (iHover & 0b01111));
		__ResetSPIErrFlags(((iHover & 0b01110000) >> 4), (iHover & 0b01111)); */

	}
	
			
	__MCU_ASIC_Deactivate_CS();
	
	/*
	for (iHover = 0; iHover < 8; iHover++)
	{
		__MCU_ASIC_Activate_CS();
		
		MCU_SC_WriteData(iHover, 0, ASIC_SPI_WRITE_REGISTER,iResetEngineCommand1Base);
		MCU_SC_WriteData(iHover, 0, ASIC_SPI_WRITE_REGISTER,iResetEngineCommand2Base);
		MCU_SC_WriteData(iHover, 0, ASIC_SPI_WRITE_REGISTER,iResetEngineCommand3Base);
		
		__MCU_ASIC_Deactivate_CS();
	}*/
	
	// Boot process finished...
}

int	ASIC_GetFrequencyFactor()
{
	
}

void ASIC_SetFrequencyFactor(int iFreqFactor)
{
	
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
		if ((__ASIC_ReadEngine(iChip, imx, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_DONE_BIT) == 0)
		{
			// __MCU_ASIC_Deactivate_CS();
			// return FALSE; // 0 = FALSE
		}
		else
		{
			iTotalEnginesDone++;				
		}			

	}	
	
	__MCU_ASIC_Deactivate_CS();	
		
	// We're ok
	return (iExpectedEnginesToBeDone == iTotalEnginesDone);
}

// How many total processors are there?
int ASIC_get_processor_count()
{
	volatile int iTotalProcessorCount = 0;
	
	for (unsigned char xchip =0; xchip < 8; xchip++)
	{
		if (!CHIP_EXISTS(xchip)) continue;
		
		for (unsigned char yproc = 0; yproc < 16; yproc++)
		{
			if (IS_PROCESSOR_OK(xchip, yproc)) iTotalProcessorCount++;	
		}
	}
	
	return iTotalProcessorCount;
}

// A very useful macro, used to tailor code to some extent
#define IF_CHIP_DONE_INCREASE_COUNTER(x)	if (CHIP_EXISTS(x) && MCU_SC_GetDone(x)) \
											{ \
												iChipsDone++; \
												iChipDoneFlag[x] = TRUE; \
												if (ASIC_are_all_engines_done(x) == FALSE) return ASIC_JOB_NONCE_PROCESSING; \
											}

int ASIC_get_job_status(unsigned int *iNonceList, char *iNonceCount)
{
	// Check if any chips are done...
	int iChipsDone = 0;
	int iChipCount;
	int iDetectedNonces = 0;
	
	char iChipDoneFlag[8] = {0,0,0,0,0,0,0,0};
	
	IF_CHIP_DONE_INCREASE_COUNTER(0);
	IF_CHIP_DONE_INCREASE_COUNTER(1);
	IF_CHIP_DONE_INCREASE_COUNTER(2);
	IF_CHIP_DONE_INCREASE_COUNTER(3);
	IF_CHIP_DONE_INCREASE_COUNTER(4);
	IF_CHIP_DONE_INCREASE_COUNTER(5);
	IF_CHIP_DONE_INCREASE_COUNTER(6);
	IF_CHIP_DONE_INCREASE_COUNTER(7);
			
	// Since jobs are divided equally among the engines. they will all nearly finish the same time... 
	// (Almost that is...)
	if (iChipsDone != ASIC_get_chip_count())
	{
		// We're not done yet...
		return ASIC_CHIP_STATUS_WORKING;
	}
	
	// Get the chip count
	iChipCount = ASIC_get_chip_count();
	
	// Activate the chips
	__MCU_ASIC_Activate_CS();
					
	// Try to read their results...
	for (int x_chip = 0; x_chip < iChipCount; x_chip++)
	{
		// Check for existence
		if (CHIP_EXISTS(x_chip) == FALSE) continue;
		
		// Check done 
		if (iChipDoneFlag[x_chip] == FALSE) continue; // Skip it...
		
		// Test all engines
		for (int y_engine = 0; y_engine < 16; y_engine++)
		{
			// Is this processor healthy?
			if (!IS_PROCESSOR_OK(x_chip,  y_engine)) continue;
			
			// Proceed
			unsigned int i_status_reg = __ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_READ_STATUS_REGISTER);
			
			// Check FIFO depths
			unsigned int iLocNonceCount = 0;
			unsigned int   iLocNonceFlag[8] = {0,0,0,0,0,0,0,0};
			
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
			if (iLocNonceCount == 0) continue;
				
			// Read them one by one
			unsigned int iNonceX = 0; 
			
			if (iLocNonceFlag[0] == TRUE) 
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO0_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO0_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
			if (iLocNonceFlag[1] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO1_LWORD)) |
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO1_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
			if (iLocNonceFlag[2] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO2_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO2_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
			if (iLocNonceFlag[3] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO3_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO3_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
			if (iLocNonceFlag[4] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO4_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO4_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
			if (iLocNonceFlag[5] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO5_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO5_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
		    if (iLocNonceFlag[6] == TRUE)
			{ 
				iNonceX = (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO6_LWORD)) | 
						  (__ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_FIFO6_HWORD) << 16); 
				iNonceList[iDetectedNonces++] = iNonceX; 
			}
				   
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
			if (iDetectedNonces >= 32) break;
		}
	}
	
	// Deactivate the CHIPs
	__MCU_ASIC_Deactivate_CS();
	
	// Set the read count
	*iNonceCount = iDetectedNonces;
	return ASIC_CHIP_STATUS_DONE;
}

void ASIC_job_issue(void* pJobPacket, 
					unsigned int _LowRange, 
					unsigned int _HighRange)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	if (_HighRange - _LowRange < 256) return;
	
	// Break job between chips...
	volatile int iChipCount  = ASIC_get_chip_count();
	volatile int iProcessorCount = ASIC_get_processor_count();
	// volatile int iProcessorCount = 1; // TEST
	volatile unsigned int iRangeSlice = ((_HighRange - _LowRange) / iProcessorCount);
	volatile unsigned int iRemainder = (_HighRange - _LowRange) - (iRangeSlice * iProcessorCount); // This is reserved for the last engine in the last chip
		
	volatile unsigned int iLowerRange = _LowRange;
	volatile unsigned int iUpperRange = _LowRange + iRangeSlice;
	
	volatile int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	
	// Send the jobs to the chips..
	// Try to read their results...
	for (volatile int x_chip = 0; x_chip < 8; x_chip++)
	{
		if (CHIP_EXISTS(x_chip) == 0) continue; // Skip it...
				
		// We assign the job to each engine in each chip
		for (volatile int y_engine = 0; y_engine < 16; y_engine++)
		{
			// Is this processor healthy?
			if (!IS_PROCESSOR_OK(x_chip, y_engine)) continue;			
			
			// Some things to do...
			//__Reset_Engine(x_chip, y_engine);
			//__ResetSPIErrFlags(x_chip, y_engine);
			
			// We have to correct the nonce-range if it's the last engine in the last chip
			// if ((iTotalChipsHovered + 1 == iChipCount) && (y_engine == 15)) iUpperRange += ;
						
			// ***** Submit the job to the engine *****
			// Set dynamic midstates...
			unsigned int test_midstates[] = {0x423CA849,0xC5E17845,0x2DA5C183,0xE5018EC5,0x2FF50D03,0x2EEE299D,0x94B6DF9A,0x95A6AE97};
			unsigned int test_blockdata[] = {0x84AC,0x8BF5,0x594D,0x4DB7,0x021B,0x5285,0x8000, 0x84AC, 0x8BF5,0x594D,0x4DB7,0x021B,0x5285,0x8000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 0x0000, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0280, 0x0000, 0x0000,0x8000,0x0000,0x0000,0x0000,0x0000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 0x0000, 0x0100,0x0000};

			
			unsigned int iMemAddress = ASIC_SPI_MAP_H0_A_LWORD;
			//for (unsigned int inx = 0; inx < 8; inx++ , iMemAddress += 2) 
			//{
				//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->midstate[inx] & 0x0FFFF));
				//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->midstate[inx] & 0x0FFFF0000) >> 16);
			//}
			//
			
			// TEST
			for (unsigned int inx = 0; inx < 8; inx++ , iMemAddress += 2)
			{
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (test_midstates[inx] & 0x0FFFF));
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (test_midstates[inx] & 0x0FFFF0000) >> 16);
			}
						
			// Set static midstates...
			iMemAddress = ASIC_SPI_MAP_H1_A_LWORD;
			
			for (unsigned int inx = 0; inx < 8; inx++, iMemAddress += 2)
			{
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (STATIC_H1[inx] & 0x0FFFF));
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (STATIC_H1[inx] & 0x0FFFF0000) >> 16);
			}
			
			// Set barrier offset to -129
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);		
			
			// Reset limit (W3)
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x0082);
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x0081);

			// Set Data
			iMemAddress = ASIC_SPI_MAP_W0_LWORD;
			
			//
			//for (unsigned int inx = 0; inx < 32; inx++, iMemAddress += 2)
			//{
				//if (inx < 3)
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);
				//}
				//else if (inx == 3)
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, 0);
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, 0);
				//}
				//else
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);										
				//}
			//}
			
			//// TEST
			for (unsigned int inx = 0; inx < 52; inx++)
			{
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + inx, (test_blockdata[inx] & 0x0FFFF));
			}			
			
			// All data sent, now set range
			
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
			
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (0x0FFFFFF));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (0x0FFFFFF));
			
			// We're all set, for this chips... Tell the engine to start
			//if (y_engine == 0)
			//{
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT | __ActualRegister0Value));			
				__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (__ActualRegister0Value)); // Clear Write-Register Valid Bit
			//}
			//else
			//{
			//	__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, (ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT));			
			//	__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, 0); // Clear Write-Register Valid Bit
			//}
						
			
			// Increase our range
			iLowerRange += iRangeSlice;
			iUpperRange += iRangeSlice;
		}	
	}	
	
	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();

}


void ASIC_job_issue_to_specified_engine(char iChip, char iEngine,
										void* pJobPacket,
										unsigned int _LowRange,
										unsigned int _HighRange)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	if (_HighRange - _LowRange < 256) return;
	
	// Break job between chips...
	volatile int iChipCount  = 1;
	volatile int iProcessorCount = 1;
	// volatile int iProcessorCount = 1; // TEST
	volatile unsigned int iRangeSlice = ((_HighRange - _LowRange) / iProcessorCount);
	volatile unsigned int iRemainder = (_HighRange - _LowRange) - (iRangeSlice * iProcessorCount); // This is reserved for the last engine in the last chip
	
	volatile unsigned int iLowerRange = _LowRange;
	volatile unsigned int iUpperRange = _LowRange + iRangeSlice;
	
	volatile int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	
	// Send the jobs to the chips..
	// Try to read their results...
			
			// Enable clock for this engine
			// MCU_SC_WriteData(iChip, 0, ASIC_SPI_CLOCK_OUT_ENABLE, (1<<iEngine) | (0b01)); // Activate clocks for all 16 engines on the chip
		
			// Some things to do...
			__Reset_Engine(iChip, iEngine);
			//__ResetSPIErrFlags(x_chip, y_engine);
			
			// We have to correct the nonce-range if it's the last engine in the last chip
			// if ((iTotalChipsHovered + 1 == iChipCount) && (y_engine == 15)) iUpperRange += ;
			
			// ***** Submit the job to the engine *****
			// Set dynamic midstates...
			unsigned int test_midstates[] = {0x423CA849,0xC5E17845,0x2DA5C183,0xE5018EC5,0x2FF50D03,0x2EEE299D,0x94B6DF9A,0x95A6AE97};
			unsigned int test_blockdata[] = {0x84AC,0x8BF5,0x594D,0x4DB7,0x021B,0x5285,0x8000, 0x84AC, 0x8BF5,0x594D,0x4DB7,0x021B,0x5285,0x8000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 0x0000, 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0280, 0x0000, 0x0000,0x8000,0x0000,0x0000,0x0000,0x0000,
											 0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 0x0000, 0x0100,0x0000};

	
			unsigned int iMemAddress = ASIC_SPI_MAP_H0_A_LWORD;
			//for (unsigned int inx = 0; inx < 8; inx++ , iMemAddress += 2)
			//{
				//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->midstate[inx] & 0x0FFFF));
				//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->midstate[inx] & 0x0FFFF0000) >> 16);
			//}
			//
	
			// TEST
			for (unsigned int inx = 0; inx < 8; inx++ , iMemAddress += 2)
			{
				__ASIC_WriteEngine(iChip, iEngine, iMemAddress + 1, (test_midstates[inx] & 0x0FFFF));
				__ASIC_WriteEngine(iChip, iEngine, iMemAddress + 0, (test_midstates[inx] & 0x0FFFF0000) >> 16);
			}
	
			// Set static midstates...
			iMemAddress = ASIC_SPI_MAP_H1_A_LWORD;
	
			for (unsigned int inx = 0; inx < 8; inx++, iMemAddress += 2)
			{
				__ASIC_WriteEngine(iChip, iEngine, iMemAddress + 0, (STATIC_H1[inx] & 0x0FFFF));
				__ASIC_WriteEngine(iChip, iEngine, iMemAddress + 1, (STATIC_H1[inx] & 0x0FFFF0000) >> 16);
			}
	
			// Set barrier offset to -129
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);
	
			// Reset limit (W3)
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_LIMITS_LWORD, 0x0082);
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_LIMITS_HWORD, 0x0081);

			// Set Data
			iMemAddress = ASIC_SPI_MAP_W0_LWORD;
	
			//
			//for (unsigned int inx = 0; inx < 32; inx++, iMemAddress += 2)
			//{
				//if (inx < 3)
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);
				//}
				//else if (inx == 3)
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, 0);
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, 0);
				//}
				//else
				//{
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					//__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);
				//}
			//}
	
			//// TEST
			for (unsigned int inx = 0; inx < 52; inx++)
			{
				__ASIC_WriteEngine(iChip, iEngine, iMemAddress + inx, (test_blockdata[inx] & 0x0FFFF));
			}
	
			// All data sent, now set range
	
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
			__ASIC_WriteEngine(iChip, iEngine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
	
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
		
	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();

}

// Reset the engines...
void ASIC_reset_engine(char iChip, char iEngine)
{
	__MCU_ASIC_Activate_CS();
	__Reset_Engine(iChip, iEngine);
	__MCU_ASIC_Activate_CS();
}

///////////////////////////////////////////////////////////////
// A very useful macro, used to tailor code to some extent
///////////////////////////////////////////////////////////////
int ASIC_is_processing()
{
	// Check if any chips are done...
	volatile int iChipsDone = 0;
	volatile int iTotalChips = ASIC_get_chip_count();
	volatile int iTotalChipsDone = 0;
	
	for (unsigned char iChip = 0; iChip < 8; iChip++)
	{
		if (!CHIP_EXISTS(iChip)) continue;
		if (MCU_SC_GetDone(iChip) != TRUE) continue;
		
		// Are all processors done in this engine?
		if (ASIC_are_all_engines_done(iChip) == TRUE)
		{
			iTotalChipsDone++;
		}
		else
		{
			// Still processing something
			// return TRUE;
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
	
	for (int x_chip = 0; x_chip < 8; x_chip++)
	{
		__ASIC_WriteEngine(x_chip, 0, ASIC_SPI_READ_TEST_REGISTER,0x0F81F);
		if (__ASIC_ReadEngine(x_chip,0, ASIC_SPI_READ_TEST_REGISTER) == 0x0F81F)
		{
			iActualChipCount++;
			//#if defined(DO_NOT_USE_ENGINE_ZERO)
			//	__chip_existence_map[x_chip] = 0x0FFFFFFFF & (~(0b01));
			//#else
				__chip_existence_map[x_chip] = 0x0FFFFFFFF;
			//#endif
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


void __ASIC_WriteEngine(char iChip, char iEngine, unsigned int iAddress, unsigned int iData16Bit)
{
	// We issue the SPI command
	MCU_SC_SetAccess();
	MCU_SC_WriteData(iChip, iEngine, iAddress, iData16Bit);
}


unsigned int __ASIC_ReadEngine(char iChip, char iEngine, unsigned int iAddress)
{
	// We issue the SPI command
	MCU_SC_SetAccess();
	return MCU_SC_ReadData(iChip, iEngine, (unsigned char)iAddress);
}


