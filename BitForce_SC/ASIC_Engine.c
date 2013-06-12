
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
#define CHIP_EXISTS(x)     ((__chip_existence_map[x] == TRUE))

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
unsigned int __chip_existence_map[8] = {0,0,0,0,0,0,0,0};
	
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

void ASIC_Bootup_Chips()
{
	// Send initialization clocks
	__ASIC_WriteEngine(0,0,0,0);
	
	volatile unsigned int iHover = 0;
	
	// Setup Oscillator on all eight chips
	volatile unsigned int iOscillatorEnableCmdBase	  = (0b00000000000000000000000000000001); // Must be OR'ed with chip address
	volatile unsigned int iOscillatorFreqSelectCmdBase = (0b00000000011000001111111111010101); // Must be OR'ed with chip address
	
	for (iHover = 0; iHover < 8; iHover++)
	{
		// Get the correct version by OR'ing them with chip address
		volatile unsigned int iOscillatorEnableCmd     = iOscillatorEnableCmdBase     | ((iHover & 0x0FF) << 28);
		volatile unsigned int iOscillatorFreqSelectCmd = iOscillatorFreqSelectCmdBase | ((iHover & 0x0FF) << 28);
		
		// We have data to write...
		__MCU_ASIC_Activate_CS();
		
		MCU_SC_WriteData(((unsigned char)(iOscillatorEnableCmd >> 24) & 0x0FF),
						 ((unsigned char)(iOscillatorEnableCmd >> 16) & 0x0FF),
						 ((unsigned char)(iOscillatorEnableCmd >> 8) & 0x0FF),
						 ((unsigned char)(iOscillatorEnableCmd ) & 0x0FF));
				 
		MCU_SC_WriteData(((unsigned char)(iOscillatorFreqSelectCmd >> 24) & 0x0FF),
						 ((unsigned char)(iOscillatorFreqSelectCmd >> 16) & 0x0FF),
						 ((unsigned char)(iOscillatorFreqSelectCmd >> 8) & 0x0FF),
						 ((unsigned char)(iOscillatorFreqSelectCmd ) & 0x0FF));	
						 
		// Activate clocks for all engines
		MCU_SC_WriteData(iHover,0, ASIC_SPI_CLOCK_OUT_ENABLE, 0x0FFFF); // Activate clocks for all 16 engines on the chip
		
		__MCU_ASIC_Deactivate_CS();
	}	
	
	// Now we're done here, we need to execute the next command for all the 256 engines in system
	volatile unsigned int iResetEngineCommand1Base = 0;
	volatile unsigned int iResetEngineCommand2Base = (1<<12);
	volatile unsigned int iResetEngineCommand3Base = 0;
	
	// Run through all the chips
	for (iHover = 0; iHover <= 0x07F; iHover++)
	{
		unsigned int iResetEngineCommand1 = iResetEngineCommand1Base | (iHover << 24);
		unsigned int iResetEngineCommand2 = iResetEngineCommand2Base | (iHover << 24);
		unsigned int iResetEngineCommand3 = iResetEngineCommand3Base | (iHover << 24);
		
		__MCU_ASIC_Activate_CS();
		
		MCU_SC_WriteData(((unsigned char)(iResetEngineCommand1 >> 24) & 0x0FF),
				((unsigned char)(iResetEngineCommand1 >> 16) & 0x0FF),
				((unsigned char)(iResetEngineCommand1 >> 8) & 0x0FF),
				((unsigned char)(iResetEngineCommand1 ) & 0x0FF));
								
		MCU_SC_WriteData(((unsigned char)(iResetEngineCommand2 >> 24) & 0x0FF),
				((unsigned char)(iResetEngineCommand2 >> 16) & 0x0FF),
				((unsigned char)(iResetEngineCommand2 >> 8) & 0x0FF),
				((unsigned char)(iResetEngineCommand2 ) & 0x0FF));

		MCU_SC_WriteData(((unsigned char)(iResetEngineCommand3 >> 24) & 0x0FF),
				((unsigned char)(iResetEngineCommand3 >> 16) & 0x0FF),
				((unsigned char)(iResetEngineCommand3 >> 8) & 0x0FF),
				((unsigned char)(iResetEngineCommand3 ) & 0x0FF));	
									
		__MCU_ASIC_Deactivate_CS();
	}
	
	// Boot process finished...
}

int	ASIC_GetFrequencyFactor()
{
	
}

void ASIC_SetFrequencyFactor(int iFreqFactor)
{
	
}

int ASIC_are_all_engines_done(unsigned short iChip)
{
	if (MCU_SC_GetDone(iChip) == FALSE) return FALSE; // 0 = FALSE
		
	__MCU_ASIC_Activate_CS();
	
	// Check all engines
	for (unsigned short imx = 0; imx < 16; imx++)
	{
		if ((__ASIC_ReadEngine(iChip, imx, ASIC_SPI_READ_STATUS_REGISTER) & ASIC_SPI_READ_STATUS_DONE_BIT) == 0)
		{
			__MCU_ASIC_Deactivate_CS();
			return FALSE; // 0 = FALSE
		}			
	}	
	
	__MCU_ASIC_Deactivate_CS();	
		
	// We're ok
	return TRUE;
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
			unsigned int i_status_reg = __ASIC_ReadEngine(x_chip, y_engine, ASIC_SPI_READ_STATUS_REGISTER);
			
			// Check FIFO depths
			unsigned short iLocNonceCount = 0;
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
						
			// Clear the engine [ NOTE : CCCCCCCOOOOOOOORRRRRRRRRRRREEEEEEEEEECCCCCCCCCTTTTTTTTTT !!!!!!!!!!!! ]
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT );
			
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
	int iChipCount  = ASIC_get_chip_count();
	int iRangeSlice = ((_HighRange - _LowRange) / iChipCount / 16);
	int iRemainder = (_HighRange - _LowRange) - (iRangeSlice * iChipCount * 16); // This is reserved for the last engine in the last chip
		
	int iLowerRange = _LowRange;
	int iUpperRange = _LowRange + iRangeSlice;
	
	int iTotalChipsHovered = 0;
	pjob_packet pjp = (pjob_packet*)(pJobPacket);
	
	// Activate the SPI
	__MCU_ASIC_Activate_CS();
	
	// Send the jobs to the chips..
	// Try to read their results...
	for (int x_chip = 0; x_chip < iChipCount; x_chip++)
	{
		if (CHIP_EXISTS(x_chip) == 0) continue; // Skip it...
				
		// We assign the job to each engine in each chip
		for (int y_engine = 0; y_engine < 16; y_engine++)
		{
			// We have to correct the nonce-range if it's the last engine in the last chip
			if ((iTotalChipsHovered + 1 == iChipCount) && (y_engine == 15)) iUpperRange += iRemainder;
						
			// ***** Submit the job to the engine *****
			// Set dynamic midstates...
			unsigned int iMemAddress = ASIC_SPI_MAP_H0_A_LWORD;
			
			for (unsigned int inx = 0; inx < 8; inx++ , iMemAddress += 2) 
			{
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->midstate[inx] & 0x0FFFF));
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->midstate[inx] & 0x0FFFF0000) >> 16);
			}
			
			// Set static midstates...
			iMemAddress = ASIC_SPI_MAP_H1_A_LWORD;
			
			for (unsigned int inx = 0; inx < 8; inx++, iMemAddress += 2)
			{
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (STATIC_H1[inx] & 0x0FFFF));
				__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (STATIC_H1[inx] & 0x0FFFF0000) >> 16);
			}
			
			// Set Data
			iMemAddress = ASIC_SPI_MAP_W0_LWORD;
			
			for (unsigned int inx = 0; inx < 32; inx++, iMemAddress += 2)
			{
				if (inx < 3)
				{
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);
				}
				else if (inx == 4)
				{
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, 0);
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, 0);
				}
				else
				{
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 0, (pjp->block_data[inx] & 0x0FFFF));
					__ASIC_WriteEngine(x_chip, y_engine, iMemAddress + 1, (pjp->block_data[inx] & 0x0FFFF0000) >> 16);										
				}
			}
			
			// All data sent, now set range
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_LWORD,  (iLowerRange & 0x0FFFF));
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_LOW_HWORD,  (iLowerRange & 0x0FFFF0000) >> 16);					
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_LWORD, (iUpperRange & 0x0FFFF));
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_MAP_COUNTER_HIGH_HWORD, (iUpperRange & 0x0FFFF0000) >> 16);
			
			// We're all set, for this chips... Tell the engine to start
			__ASIC_WriteEngine(x_chip, y_engine, ASIC_SPI_WRITE_REGISTER, ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT);		
		}	
	}	
	
	// Deactivate the SPI
	__MCU_ASIC_Deactivate_CS();

}


void ASIC_job_issue_p2p(void* pJobPacketP2P)
{
	// Break job between chips... (Why would we do this anyway???)
	// Send the jobs to the chips..
	ASIC_job_issue(pJobPacketP2P, 
				  ((pjob_packet_p2p)pJobPacketP2P)->nonce_begin,
				  ((pjob_packet_p2p)pJobPacketP2P)->nonce_end);
}


///////////////////////////////////////////////////////////////
// A very useful macro, used to tailor code to some extent
///////////////////////////////////////////////////////////////
#define TAILOR_PROCESS2(x)  if (CHIP_EXISTS(x) && MCU_SC_GetDone(x)) \
							{ \
								iChipsDone++; \
								iChipDoneFlag[x] = TRUE; \
								if (ASIC_are_all_engines_done(x) == FALSE) return TRUE; \
							}

int ASIC_is_processing()
{
	// Check if any chips are done...
	int iChipsDone = 0;
	char iChipDoneFlag[8] = {0,0,0,0,0,0,0,0};
	
	TAILOR_PROCESS2(0);
	TAILOR_PROCESS2(1);
	TAILOR_PROCESS2(2);
	TAILOR_PROCESS2(3);
	TAILOR_PROCESS2(4);
	TAILOR_PROCESS2(5);
	TAILOR_PROCESS2(6);
	TAILOR_PROCESS2(7);
		
	// Since jobs are divided equally among the engines. they will all nearly finish the same time... 
	// (Almost that is...)
	if (iChipsDone != ASIC_get_chip_count())
	{
		// We're not done yet...
		return ASIC_CHIP_STATUS_WORKING;
	}
	
	// If we've reached here, it means we're not processing anymore
	return FALSE;	
}

int ASIC_get_chip_count()
{
	// Have we already detected this? If so, just return the previously known number
	static int iChipCount = 0;
	
	if (iChipCount != 0)
		return iChipCount;
		
	// Activate CS# of ASIC engines
	__MCU_ASIC_Activate_CS();
		
	// We haven't, so we need to read their register (one by one) to find which ones exist.
	int iActualChipCount = 0;
	
	for (int x_chip = 0; x_chip < 8; x_chip++)
	{
		__ASIC_WriteEngine(x_chip, 0, ASIC_SPI_READ_TEST_REGISTER,0x0AA);
		if (__ASIC_ReadEngine(x_chip,0, ASIC_SPI_READ_TEST_REGISTER) == 0x0AA)
		{
			iActualChipCount++;
			__chip_existence_map[x_chip] = TRUE;
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
	return TRUE;	
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


