
/*
 * ASIC_Engine.c
 *
 * Created: 20/11/2012 23:17:15
 *  Author: NASSER
 */ 

#include "ASIC_Engine.h"
#include "std_defs.h"
#include "Generic_Module.h"
#include "AVR32X/AVR32_Module.h"

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
unsigned int __chip_existence_map[TOTAL_CHIPS_INSTALLED] = {0,0,0,0,0,0,0,0}; // Bit 0 to Bit 16 in each word says if the engine is OK or not...
	
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

	// Also here we set the Oscillator Control Register
	ASIC_Bootup_Chips();
		
	// Proceed...
	ASIC_get_chip_count(); // Also sets the '__chip_existence_map' table in it's first run
	
	// Ok now we diagnose the engines
	char iHoveringChip;
	char iHoveringEngine;
	
	for (iHoveringChip = 0; iHoveringChip < TOTAL_CHIPS_INSTALLED; iHoveringChip++)
	{
		// Does the chip exist at all?
		if (!CHIP_EXISTS(iHoveringChip)) 
		{
			continue;
		}
					
		// Proceed
		for (iHoveringEngine = 0; iHoveringEngine < 16; iHoveringEngine++)
		{
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
		
		// Ok, Now we have the __chip_existance_map for this chip, the value can be used to set clock-enable register in this chip
		ASIC_set_clock_mask(iHoveringChip, __chip_existence_map[iHoveringChip]);
	}
}

// Sets the clock mask for a deisng
void ASIC_set_clock_mask(char iChip, unsigned int iClockMaskValue)
{
	__MCU_ASIC_Activate_CS();
	__ASIC_WriteEngine(iChip, 0, ASIC_SPI_CLOCK_OUT_ENABLE, iClockMaskValue);
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
		
	// Now send the job to the engine
	ASIC_job_issue_to_specified_engine(iChip,iEngine,&jp, 0x8D9CB670, 0x8D9CB67A);
	
	// Read back results immediately (with a little delay), it shouldv'e been finished since it takes about 40ns
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	
	// Read back result. It should be DONE with FIFO zero of it being 0x8D9CB675
	unsigned int iReadbackNonce = 0;
	unsigned int iReadbackStatus = 0;
	unsigned char isProcessorOK = TRUE;
	
	__MCU_ASIC_Activate_CS();
	
	iReadbackStatus = __ASIC_ReadEngine(iChip,iEngine, ASIC_SPI_READ_STATUS_REGISTER);
	if ((iReadbackStatus & ASIC_SPI_READ_STATUS_DONE_BIT) != ASIC_SPI_READ_STATUS_DONE_BIT)
	{
		isProcessorOK = FALSE;
	}
	
	if ((iReadbackStatus & ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT) != 0) // Depth 2 should not have any nonces
	{
		isProcessorOK = FALSE;
	}
	
	iReadbackNonce = __ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_LWORD) | (__ASIC_ReadEngine(iChip, iEngine, ASIC_SPI_FIFO0_HWORD) << 16);
	if (iReadbackNonce != 0x8D9CB675) 
	{
		isProcessorOK = FALSE;
	}
	
	__MCU_ASIC_Deactivate_CS();	
	
	// Return the result
	return isProcessorOK;
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
			//DATAIN=0xCD55; operates at 280
			DATAIN = 0xFFDF; // Operates 250MHz
			//DATAIN = 0xCDFF; // Operates 280MHz
			//DATAIN = 0xFFFF; // Operates Less than 250MHz
			__Write_SPI(CHIP,0,0x60,DATAIN);//int caddr, int engine, int reg, int data

			//Clear the Reset engine 1-15
			DATAIN=0x0000;
			for(c=1;c<16;c++){
				__Write_SPI(CHIP,c,0,DATAIN);//int caddr, int engine, int reg, int data
			}

			//Disable Clock Out, all engines
			DATAIN=0xFFFE; // Engine 0 clock is not enabled here
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
			//DATAREG0[0]='1'; //0=div 1
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
			DATAIN = 0xFFD5; // Operates 500MHz
			__Write_SPI(CHIP,0,0x60,DATAIN);//int caddr, int engine, int reg, int data

			//Clear the Reset engine 1-15
			DATAIN=0x0000;
			for(c=1;c<16;c++){
				__Write_SPI(CHIP,c,0,DATAIN);//int caddr, int engine, int reg, int data
			}

			//Disable Clock Out, all engines
			if (CHIP == 0) 
			{
				DATAIN=0xFFFE; // Engine 0 clock is not enabled here
			}
			else
			{
				DATAIN=0; // No engines is enabled for chips other than 0 
			}				
			__Write_SPI(CHIP,0,0x61,DATAIN);//int caddr, int engine, int reg, int data
		}
	
	#endif

	//CHIP STATE: Internal Clock, All Registers Reset,All BUT 0 Resets=0,
	//All clocks Disabled				
	return;				
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
		//if ((__ASIC_ReadEngine(iChip, imx, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_BUSY_BIT) == ASIC_SPI_READ_STATUS_BUSY_BIT) // Means it's busy
		if ((__AVR32_SC_ReadData(iChip, imx, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_BUSY_BIT) == ASIC_SPI_READ_STATUS_BUSY_BIT) // Means it's busy
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

int ASIC_get_job_status(unsigned int *iNonceList, unsigned int *iNonceCount)
{
	// Check if any chips are done...
	int iChipsDone = 0;
	int iChipCount;
	int iDetectedNonces = 0;
	
	char iChipDoneFlag[8] = {0,0,0,0,0,0,0,0};

	for (unsigned char index = 0; index < TOTAL_CHIPS_INSTALLED; index++)
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
	}
						
	// Since jobs are divided equally among the engines. they will all nearly finish the same time... 
	// (Almost that is...)
	if (iChipsDone != ASIC_get_chip_count())
	{
		// We're not done yet...
		return ASIC_JOB_NONCE_PROCESSING;
	}
	
	// Get the chip count and also check how many chips are IDLE
	iChipCount = ASIC_get_chip_count();
	
	unsigned char iTotalEngines = ASIC_get_processor_count();
	unsigned char iEnginesIDLE = 0;
		
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

#define MACRO__ASIC_WriteEngine(x,y,z,d)		__AVR32_SC_WriteData(x,y,z,d);


#define MACRO__AVR32_SPI0_SendWord_Express2(x) \
	({ \
		AVR32_SPI0_TDR = (x & 0x0FFFF); \
		while ((AVR32_SPI0_SR & (1 << 1)) == 0); \
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
	NOP_OPERATION;
	
	// Have we ever come here before? If so, don't program static data in the registers any longer
	static unsigned char bIsThisOurFirstTime = TRUE;

	//for (volatile int x_chip = 4; x_chip < 5; x_chip++)	
	for (volatile int x_chip = 0; x_chip < TOTAL_CHIPS_INSTALLED; x_chip++)
	{
		if (CHIP_EXISTS(x_chip) == 0) continue; // Skip it...
		
		// DEBUG
		// if ((x_chip != 3) && (x_chip != 4) && (x_chip != 5) && (x_chip != 6)) continue;
				
		// We assign the job to each engine in each chip
		for (volatile int y_engine = 1; y_engine < 16; y_engine++)
		{
			// Is this processor healthy?
			if (!IS_PROCESSOR_OK(x_chip, y_engine)) continue;
			
			// STATIC RULE - Engine 0 not used
			if (y_engine == 0) continue; // Do not start engine 0
			
			// Reset the engine
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, (1<<9) | (1<<12));
			MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, 0);

			// Load H1 (STATIC)
			if (bIsThisOurFirstTime)
			{
				// Set limit register
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
				MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);
				
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
			
			// Load H0 (MIDSTATE) [ FOR TEST ]
			//__ASIC_WriteEngine(x_chip,y_engine,0x80,0x423C);
			//__ASIC_WriteEngine(x_chip,y_engine,0x81,0xA849);
			//__ASIC_WriteEngine(x_chip,y_engine,0x82,0xC5E1);
			//__ASIC_WriteEngine(x_chip,y_engine,0x83,0x7845);
			//__ASIC_WriteEngine(x_chip,y_engine,0x84,0x2DA5);
			//__ASIC_WriteEngine(x_chip,y_engine,0x85,0xC183);
			//__ASIC_WriteEngine(x_chip,y_engine,0x86,0xE501);
			//__ASIC_WriteEngine(x_chip,y_engine,0x87,0x8EC5);
			//__ASIC_WriteEngine(x_chip,y_engine,0x88,0x2FF5);
			//__ASIC_WriteEngine(x_chip,y_engine,0x89,0x0D03);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8A,0x2EEE);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8B,0x299D);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8C,0x94B6);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8D,0xDF9A);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8E,0x95A6);
			//__ASIC_WriteEngine(x_chip,y_engine,0x8F,0xAE97);

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
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
						 
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0 & 0x0FFFF));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0 & 0x0FFFF0000) >> 16);
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (0x0FFFFFFFF & 0x0FFFF));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (0x0FFFFFFFF & 0x0FFFF0000) >> 16);
			
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0xB675));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0x8D9C));
						
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0xB675));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0x4D9C));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (0xB680));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (0x9D9C));
						
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (0x4250));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (0x38E9));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (0x4280));
			//__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (0x38E9));
	
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
			MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);	
						
			//
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
		}	
	}	
	
	// Ok It's no longer our fist time
	bIsThisOurFirstTime = FALSE;

	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();

}


void ASIC_job_issue_to_specified_engine(char  iChip, 
										char  iEngine,
										void* pJobPacket,
										unsigned int _LowRange,
										unsigned int _HighRange)
{
	// Error Check: _HighRange - _LowRange must be at least 256
	char x_chip = iChip;
	char y_engine = iEngine;
		
	if (!CHIP_EXISTS(iChip)) return;
	if (!IS_PROCESSOR_OK(x_chip, y_engine)) return;
	
	volatile int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	NOP_OPERATION;
	NOP_OPERATION;

	// Reset the engine
	MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, (1<<9) | (1<<12));
	MACRO__ASIC_WriteEngineExpress(x_chip,y_engine, 0, 0);

	// Set limit register
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_LWORD, 0x82);
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_LIMITS_HWORD, 0x81);
				
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

	// All data sent, now set range
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (_LowRange & 0x0FFFF));
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (_LowRange & 0x0FFFF0000) >> 16);
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (_HighRange & 0x0FFFF));
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (_HighRange & 0x0FFFF0000) >> 16);
	
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_LWORD, 0x0FF7F);
	MACRO__ASIC_WriteEngineExpress(x_chip, y_engine, ASIC_SPI_MAP_BARRIER_HWORD, 0x0FFFF);	
						
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
	volatile int iChipsDone = 0;
	volatile int iTotalChips = ASIC_get_chip_count();
	volatile int iTotalChipsDone = 0;
	
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
		// SPI Sync Clear
		// __ResetSPIErrFlags(0,0);
		
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


