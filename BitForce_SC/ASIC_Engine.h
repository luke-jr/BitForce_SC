/*
 * ASIC_Engine.h
 *
 * Created: 20/11/2012 23:17:01
 *  Author: NASSER
 */ 

#ifndef ASIC_ENGINE_H_
#define ASIC_ENGINE_H_

#define ASIC_JOB_IDLE				0
#define ASIC_JOB_NONCE_PROCESSING	1
#define ASIC_JOB_NONCE_FOUND		2
#define ASIC_JOB_NONCE_NO_NONCE		3

// Counter low-dword and high-dword
#define ASIC_SPI_MAP_COUNTER_LOW_LWORD	0b01000000
#define ASIC_SPI_MAP_COUNTER_LOW_HWORD  0b01000001
#define ASIC_SPI_MAP_COUNTER_HIGH_LWORD	0b01000010
#define ASIC_SPI_MAP_COUNTER_HIGH_HWORD 0b01000011

#define ASIC_SPI_MAP_H0_A_LWORD   0b010000000
#define ASIC_SPI_MAP_H0_A_HWORD   0b010000001
#define ASIC_SPI_MAP_H0_B_LWORD   0b010000010
#define ASIC_SPI_MAP_H0_B_HWORD   0b010000011
#define ASIC_SPI_MAP_H0_C_LWORD   0b010000100
#define ASIC_SPI_MAP_H0_C_HWORD   0b010000101
#define ASIC_SPI_MAP_H0_D_LWORD   0b010000110
#define ASIC_SPI_MAP_H0_D_HWORD   0b010000111
#define ASIC_SPI_MAP_H0_E_LWORD   0b010001000
#define ASIC_SPI_MAP_H0_E_HWORD   0b010001001
#define ASIC_SPI_MAP_H0_F_LWORD   0b010001010
#define ASIC_SPI_MAP_H0_F_HWORD   0b010001011
#define ASIC_SPI_MAP_H0_G_LWORD   0b010001100
#define ASIC_SPI_MAP_H0_G_HWORD   0b010001101
#define ASIC_SPI_MAP_H0_H_LWORD   0b010001110
#define ASIC_SPI_MAP_H0_H_HWORD   0b010001111

#define ASIC_SPI_MAP_H1_A_LWORD   0b010010000
#define ASIC_SPI_MAP_H1_A_HWORD   0b010010001
#define ASIC_SPI_MAP_H1_B_LWORD   0b010010010
#define ASIC_SPI_MAP_H1_B_HWORD   0b010010011
#define ASIC_SPI_MAP_H1_C_LWORD   0b010010100
#define ASIC_SPI_MAP_H1_C_HWORD   0b010010101
#define ASIC_SPI_MAP_H1_D_LWORD   0b010010110
#define ASIC_SPI_MAP_H1_D_HWORD   0b010010111
#define ASIC_SPI_MAP_H1_E_LWORD   0b010011000
#define ASIC_SPI_MAP_H1_E_HWORD   0b010011001
#define ASIC_SPI_MAP_H1_F_LWORD   0b010011010
#define ASIC_SPI_MAP_H1_F_HWORD   0b010011011
#define ASIC_SPI_MAP_H1_G_LWORD   0b010011100
#define ASIC_SPI_MAP_H1_G_HWORD   0b010011101
#define ASIC_SPI_MAP_H1_H_LWORD   0b010011110
#define ASIC_SPI_MAP_H1_H_HWORD   0b010011111

#define ASIC_SPI_MAP_BARRIER_LWORD   0b01101110 // Must be set to 0x0FFFF FF7F (-129)
#define ASIC_SPI_MAP_BARRIER_HWORD   0b01101111

#define ASIC_SPI_MAP_W0_LWORD		  0b010100000 // Continues as W0_HWORD, W1_LWORD, W1_HWORD, ... Up to W31

#define ASIC_SPI_MAP_LIMITS_LWORD	  0b10100110 // Set to 0 by default
#define ASIC_SPI_MAP_LIMITS_HWORD	  0b10100111 // Set to 0 by default

#define ASIC_SPI_READ_STATUS_REGISTER	0b00000000
#define ASIC_SPI_WRITE_REGISTER			0b00000000

#define ASIC_SPI_CLOCK_OUT_ENABLE		0b01100001
#define ASIC_SPI_OSC_CONTROL			0b01100000

#define ASIC_SPI_FIFO0_LWORD	  0b010000000 // This is the fifo0 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H0...)
#define ASIC_SPI_FIFO0_HWORD	  0b010000001 // This is the fifo0 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H0...)
#define ASIC_SPI_FIFO1_LWORD	  0b010000010 // This is the fifo1 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H1...)
#define ASIC_SPI_FIFO1_HWORD	  0b010000011 // This is the fifo1 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H1...)
#define ASIC_SPI_FIFO2_LWORD	  0b010000100 // This is the fifo2 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H2...)
#define ASIC_SPI_FIFO2_HWORD	  0b010000101 // This is the fifo2 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H2...)
#define ASIC_SPI_FIFO3_LWORD	  0b010000110 // This is the fifo3 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H3...)
#define ASIC_SPI_FIFO3_HWORD	  0b010000111 // This is the fifo3 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H3...)
#define ASIC_SPI_FIFO4_LWORD	  0b010001000 // This is the fifo4 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H4...)
#define ASIC_SPI_FIFO4_HWORD	  0b010001001 // This is the fifo4 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H4...)
#define ASIC_SPI_FIFO5_LWORD	  0b010001010 // This is the fifo5 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H5...)
#define ASIC_SPI_FIFO5_HWORD	  0b010001011 // This is the fifo5 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H5...)
#define ASIC_SPI_FIFO6_LWORD	  0b010001100 // This is the fifo6 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H6...)
#define ASIC_SPI_FIFO6_HWORD	  0b010001101 // This is the fifo6 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H6...)
#define ASIC_SPI_FIFO7_LWORD	  0b010001110 // This is the fifo7 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H7...)
#define ASIC_SPI_FIFO7_HWORD	  0b010001111 // This is the fifo7 address (Lower 16 Bit) (it applies when reading, so no conflict with MAP_H7...)

#define ASIC_SPI_GLOBAL_QUERY_LISTEN	0b011111111 // This tells us which engine has finished...

// Read Status Register
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH8_BIT		0b01000000000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH7_BIT		0b00100000000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH6_BIT		0b00010000000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH5_BIT		0b00001000000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH4_BIT		0b00000100000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH3_BIT		0b00000010000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH2_BIT		0b00000001000000000
#define ASIC_SPI_READ_STATUS_FIFO_DEPTH1_BIT		0b00000000100000000
#define ASIC_SPI_READ_STATUS_DONE_BIT				0b00000000000000001
#define ASIC_SPI_READ_STATUS_BUSY_BIT				0b00000000000000010

// Write Control Register
#define ASIC_SPI_WRITE_CONTROL_EXTERNAL_CLOCK_BIT	0b01000000000000000
#define ASIC_SPI_WRITE_CONTROL_DIV2_BIT				0b00100000000000000
#define ASIC_SPI_WRITE_CONTROL_DIV4_BIT				0b00010000000000000
#define ASIC_SPI_WRITE_WRITE_REGISTERS_VALID_BIT	0b00000100000000000
#define ASIC_SPI_WRITE_READ_REGISTERS_DONE_BIT		0b00000010000000000
#define ASIC_SPI_WRITE_OSC_CTRL6_BIT				0b00000000000100000
#define ASIC_SPI_WRITE_OSC_CTRL5_BIT				0b00000000000010000
#define ASIC_SPI_WRITE_OSC_CTRL4_BIT				0b00000000000001000
#define ASIC_SPI_WRITE_OSC_CTRL3_BIT				0b00000000000000100
#define ASIC_SPI_WRITE_OSC_CTRL2_BIT				0b00000000000000010
#define ASIC_SPI_WRITE_OSC_CTRL1_BIT				0b00000000000000001

#define ASIC_SPI_WRITE_TEST_REGISTER				0b00000000000000001 // Address
#define ASIC_SPI_READ_TEST_REGISTER					0b00000000000000001 // Address

#define ASIC_CHIP_STATUS_DONE		0
#define ASIC_CHIP_STATUS_WORKING	1

void    init_ASIC(void);

// Maximum 32 nonces supported
int			 ASIC_get_job_status(unsigned int *iNonceList, unsigned int *iNonceCount, const char iCheckOnlyOneChip, const char iChipToCheck);
void		 ASIC_job_issue(void* pJobPacket, unsigned int _LowRange,unsigned int _HighRange, const char bIssueToSingleChip, const char iChipToIssue);
void		 ASIC_job_issue_to_specified_engine(char  iChip, char  iEngine,	void* pJobPacket, char bLoadStaticData, char  bResetBeforStart, unsigned int _LowRange, unsigned int _HighRange);
void		 ASIC_job_start_processing(char iChip, char iEngine, char bForcedStart);
int  		 ASIC_get_chip_count(void);
int			 ASIC_get_chip_processor_count(char iChip);
int  		 ASIC_get_processor_count(void);
char		 ASIC_has_engine_finished_processing(char iChip, char iEngine);
char		 ASIC_diagnose_processor(char iEngine, char iProcessor);
int			 ASIC_tune_chip_to_frequency(char iChip, char iEngineToUse, char bOnlyReturnOperatingFrequency);
int			 ASIC_are_all_engines_done(unsigned int iChip);
void		 ASIC_reset_engine(char iChip, char iEngine);
int			 ASIC_does_chip_exist(unsigned int iChipIndex);
int			 ASIC_is_processing(void);
int			 ASIC_is_chip_processing(char iChip); // Chip based function
void		 ASIC_set_clock_mask(char iChip, unsigned int iClockMaskValue);
void		 ASIC_Bootup_Chips(void);
void		 ASIC_ReadComplete(char iChip, char iEngine);
void		 ASIC_WriteComplete(char iChip, char iEngine);
int			 ASIC_GetFrequencyFactor(void);
void		 ASIC_SetFrequencyFactor(char iChip, int iFreqFactor);

void		 __ASIC_WriteEngine(char iChip, char iEngine, unsigned int iAddress, unsigned int iData16Bit);
unsigned int __ASIC_ReadEngine (char iChip, char iEngine, unsigned int iAddress);

#endif /* ASIC_ENGINE_H_ */


