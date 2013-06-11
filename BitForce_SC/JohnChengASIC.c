/*
 * JohnChengASIC.c
 *
 * Created: 24/03/2013 17:28:45
 */ 

#include "JohnChengASIC.h"
#include "ASIC_Engine.h"
#include "Generic_Module.h"
#include "std_defs.h"
#include <avr32/io.h>

#define RESET_BIT			12
#define WRITE_VALID_BIT		11
#define READ_COMP_BIT		10
#define RESET_ERR_BIT		 9

#define BUSY_BIT		01
#define DONE_BIT		00

// ---------------------------------------------------------------------------
//   MASK Bits
// ---------------------------------------------------------------------------

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

#define MASK_CLK		(0x4000)
#define MASK_RESET_ENGINE_0	(0x1000)

// ---------------------------------------------------------------------
//   GLOBAL VARIABLES
// ---------------------------------------------------------------------

int DATAREG0;
int DATAIN;
int DATAOUT;

void SynchronizeSPI_Module(void)
{
	__MCU_ASIC_Deactivate_CS(1);
	__MCU_ASIC_Deactivate_CS(2);
	
	__ASIC_WriteEngine(0, 0, 0, 0);
	__ASIC_WriteEngine(0, 0, 0, 0);
	__ASIC_WriteEngine(0, 0, 0, 0);
	__ASIC_WriteEngine(0, 0, 0, 0);
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		__ASIC_WriteEngine(8, 0, 0, 0);
		__ASIC_WriteEngine(8, 0, 0, 0);
		__ASIC_WriteEngine(8, 0, 0, 0);
		__ASIC_WriteEngine(8, 0, 0, 0);
	#endif
}

//==================================================================
void Write_SPI(int chip, int engine, int reg, int value)
{
	__MCU_ASIC_Activate_CS((chip < 8) ? (1) : (2));
	__ASIC_WriteEngine(chip, engine, reg, value);
	__MCU_ASIC_Deactivate_CS((chip < 8) ? (1) : (2));
}

//==================================================================
int Read_SPI(int chip, int engine, int reg){
	int res;
	__MCU_ASIC_Activate_CS((chip < 8) ? (1) : (2));
	res = __ASIC_ReadEngine(chip, engine, reg);
	__MCU_ASIC_Deactivate_CS((chip < 8) ? (1) : (2));
	return res;
}

//==================================================================
void initEngines(int CHIP){
	int i, engineID;
	
	SynchronizeSPI_Module();

	DATAREG0 = MASK_CLK;
	DATAIN = DATAREG0;
	Write_SPI(CHIP, 0, 0x00, DATAIN);	// --- Engine ID = 0

	// --- reseting all engines 1-15 ---
	DATAREG0 |= MASK_RESET_ENGINE_0;
	DATAIN = DATAREG0;
	Write_SPI(CHIP, 0, 0, DATAIN);	// --- Engine ID = 0
	
	
	DATAREG0 &= ~(MASK_RESET_ENGINE_0);
	DATAIN = DATAREG0;
	Write_SPI(CHIP, 0, 0, DATAIN);	// --- Engine ID = 0

	DATAIN = 0x1000;			// --- Reset Engines ( Set RESET )
	for(engineID = 1; engineID < 16; engineID++){
		Write_SPI(CHIP, engineID, 0x0, DATAIN);
	}

	DATAIN = 0x0000;			// --- Reset Engines ( Clear RESET )
	for(engineID = 1; engineID < 16; engineID++){
		Write_SPI(CHIP, engineID, 0x0, DATAIN);
	}
	
	Write_SPI(CHIP,
			  0b0,// Issue to processor 0
			  0b01100000,
			  0b0101010101010101); // Slowest mode	

	DATAIN = 0x0000;			// --- Enable Clock Out for all Engines
	Write_SPI(CHIP, 0, 0x61, DATAIN);	// --- Engine ID = 0
} /* --- end of initEngines --- */

//==================================================================
void Enable_HE_Clock(int CHIP, int ENGINE){

	DATAIN = ((0x1) << ENGINE);
	Write_SPI(CHIP, 0, 0x61, DATAIN);
}

//==================================================================
void Reset_Engine(int CHIP, int ENGINE){

	if(ENGINE == 0){
		DATAREG0 |= (MASK_RESET_ENGINE_0);
		DATAIN    = DATAREG0;
	} else {
		DATAIN    = 0x0000;
		DATAIN   |= (MASK_RESET_ENGINE_0);
	}
	Write_SPI(CHIP, 0, 0x00, DATAIN);
	if(ENGINE == 0){
		DATAREG0 &= ~(MASK_RESET_ENGINE_0);
		DATAIN    = DATAREG0;
	} else {
		DATAIN   &= ~(MASK_RESET_ENGINE_0);
	}
	Write_SPI(CHIP, 0, 0x00, DATAIN);

}

//=========================================================================
int Wait_Busy(int CHIP, int ENGINE, int max_num_trys){
	int v;
	int num_trys;

	num_trys = 0;
	while(num_trys < max_num_trys){
		v = Read_SPI(CHIP, ENGINE, 0x00);
		if(v & MASK_BUSY_BIT){
			break;
		}
		num_trys++;
	}
	if(num_trys < max_num_trys)
	return 0;
	else
	return 1;
}

//=========================================================================
int Wait_Done(int CHIP, int ENGINE, int max_num_trys){
	int v;
	int num_trys;

	num_trys = 0;
	while(num_trys < max_num_trys){
		WATCHDOG_RESET;
		v = Read_SPI(CHIP, ENGINE, 0x00);
		if(v & MASK_DONE_BIT){
			break;
		}
		num_trys++;
	}
	if(num_trys < max_num_trys)
	return 0;
	else
	return 1;
}

//=========================================================================
void Set_Regs_Valid(int CHIP, int ENGINE){
	if(ENGINE == 0){
		DATAREG0 |= (MASK_WRITE_VALID_BIT);
		DATAIN = DATAREG0;
	} else {
		DATAIN = (MASK_WRITE_VALID_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00, DATAIN);
	if(ENGINE == 0){
		DATAREG0 &= ~(MASK_WRITE_VALID_BIT);
		DATAIN = DATAREG0;
	} else {
		DATAIN &= ~(MASK_WRITE_VALID_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00,DATAIN);
}

//=========================================================================
void Set_Rd_Complete(int CHIP, int ENGINE){
	if(ENGINE == 0){
		DATAREG0 |= (MASK_READ_COMP_BIT);
		DATAIN = DATAREG0;
	} else {
		DATAIN = (MASK_READ_COMP_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00, DATAIN);
	if(ENGINE == 0){
		DATAREG0 &= ~(MASK_READ_COMP_BIT);
		DATAIN = DATAREG0;
	} else {
		DATAIN &= ~(MASK_READ_COMP_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00,DATAIN);
}

//==================================================================
void Reset_Err_Flags(int CHIP, int ENGINE){
	if(ENGINE == 0){
		DATAREG0 |= (MASK_RESET_ERR_BIT);
		DATAIN    = DATAREG0;
	} else {
		DATAIN = (MASK_RESET_ERR_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00, DATAIN);
	if(ENGINE == 0){
		DATAREG0 &= ~(MASK_RESET_ERR_BIT);
		DATAIN = DATAREG0;
	} else {
		DATAIN &= ~(MASK_RESET_ERR_BIT);
	}
	Write_SPI(CHIP,ENGINE,0x00,DATAIN);
}

//==================================================================
void LoadBarrierAdder(int CHIP, int ENGINE){
	Write_SPI(CHIP,ENGINE,0x6E,0xFF7F);
	Write_SPI(CHIP,ENGINE,0x6F,0xFFFF);
}

//==================================================================
void LoadLimitRegisters(int CHIP, int ENGINE){
	Write_SPI(CHIP,ENGINE,0xA6,0x0082);
	Write_SPI(CHIP,ENGINE,0xA7,0x0081);
}

//==================================================================
void LoadRegistersValues_H0(int CHIP, int ENGINE){
	Write_SPI(CHIP,ENGINE,0x80,0x423C);
	Write_SPI(CHIP,ENGINE,0x81,0xA849);
	Write_SPI(CHIP,ENGINE,0x82,0xC5E1);
	Write_SPI(CHIP,ENGINE,0x83,0x7845);
	Write_SPI(CHIP,ENGINE,0x84,0x2DA5);
	Write_SPI(CHIP,ENGINE,0x85,0xC183);
	Write_SPI(CHIP,ENGINE,0x86,0xE501);
	Write_SPI(CHIP,ENGINE,0x87,0x8EC5);
	Write_SPI(CHIP,ENGINE,0x88,0x2FF5);
	Write_SPI(CHIP,ENGINE,0x89,0x0D03);
	Write_SPI(CHIP,ENGINE,0x8A,0x2EEE);
	Write_SPI(CHIP,ENGINE,0x8B,0x299D);
	Write_SPI(CHIP,ENGINE,0x8C,0x94B6);
	Write_SPI(CHIP,ENGINE,0x8D,0xDF9A);
	Write_SPI(CHIP,ENGINE,0x8E,0x95A6);
	Write_SPI(CHIP,ENGINE,0x8F,0xAE97);
}

//==================================================================
void LoadRegistersValues_H1(int CHIP, int ENGINE){
	Write_SPI(CHIP,ENGINE,0x90,0xE667);
	Write_SPI(CHIP,ENGINE,0x91,0x6A09);
	Write_SPI(CHIP,ENGINE,0x92,0xAE85);
	Write_SPI(CHIP,ENGINE,0x93,0xBB67);
	Write_SPI(CHIP,ENGINE,0x94,0xF372);
	Write_SPI(CHIP,ENGINE,0x95,0x3C6E);
	Write_SPI(CHIP,ENGINE,0x96,0xF53A);
	Write_SPI(CHIP,ENGINE,0x97,0xA54F);
	Write_SPI(CHIP,ENGINE,0x98,0x527F);
	Write_SPI(CHIP,ENGINE,0x99,0x510E);
	Write_SPI(CHIP,ENGINE,0x9A,0x688C);
	Write_SPI(CHIP,ENGINE,0x9B,0x9B05);
	Write_SPI(CHIP,ENGINE,0x9C,0xD9AB);
	Write_SPI(CHIP,ENGINE,0x9D,0x1F83);
	Write_SPI(CHIP,ENGINE,0x9E,0xCD19);
	Write_SPI(CHIP,ENGINE,0x9F,0x5BE0);
}

//==================================================================
void LoadRegistersValues_W(int CHIP, int ENGINE){

	Write_SPI(CHIP,ENGINE,0xA0,0x84AC);
	Write_SPI(CHIP,ENGINE,0xA1,0x8BF5);
	Write_SPI(CHIP,ENGINE,0xA2,0x594D);
	Write_SPI(CHIP,ENGINE,0xA3,0x4DB7);
	Write_SPI(CHIP,ENGINE,0xA4,0x021B);
	Write_SPI(CHIP,ENGINE,0xA5,0x5285);
	Write_SPI(CHIP,ENGINE,0xA9,0x8000);

	Write_SPI(CHIP,ENGINE,0xAA,0x0000);
	Write_SPI(CHIP,ENGINE,0xAB,0x0000);
	Write_SPI(CHIP,ENGINE,0xAC,0x0000);
	Write_SPI(CHIP,ENGINE,0xAD,0x0000);
	Write_SPI(CHIP,ENGINE,0xAE,0x0000);
	Write_SPI(CHIP,ENGINE,0xAF,0x0000);

	Write_SPI(CHIP,ENGINE,0xB0,0x0000);
	Write_SPI(CHIP,ENGINE,0xB1,0x0000);
	Write_SPI(CHIP,ENGINE,0xB2,0x0000);
	Write_SPI(CHIP,ENGINE,0xB3,0x0000);
	Write_SPI(CHIP,ENGINE,0xB4,0x0000);
	Write_SPI(CHIP,ENGINE,0xB5,0x0000);
	Write_SPI(CHIP,ENGINE,0xB6,0x0000);
	Write_SPI(CHIP,ENGINE,0xB7,0x0000);
	Write_SPI(CHIP,ENGINE,0xB8,0x0000);
	Write_SPI(CHIP,ENGINE,0xB9,0x0000);
	Write_SPI(CHIP,ENGINE,0xBA,0x0000);
	Write_SPI(CHIP,ENGINE,0xBB,0x0000);
	Write_SPI(CHIP,ENGINE,0xBC,0x0000);
	Write_SPI(CHIP,ENGINE,0xBD,0x0000);

	Write_SPI(CHIP,ENGINE,0xBE,0x0280);
	Write_SPI(CHIP,ENGINE,0xBF,0x0000);

	Write_SPI(CHIP,ENGINE,0xC0,0x0000);
	Write_SPI(CHIP,ENGINE,0xC1,0x8000);
	Write_SPI(CHIP,ENGINE,0xC2,0x0000);
	Write_SPI(CHIP,ENGINE,0xC3,0x0000);
	Write_SPI(CHIP,ENGINE,0xC4,0x0000);
	Write_SPI(CHIP,ENGINE,0xC5,0x0000);
	Write_SPI(CHIP,ENGINE,0xC6,0x0000);
	Write_SPI(CHIP,ENGINE,0xC7,0x0000);
	Write_SPI(CHIP,ENGINE,0xC8,0x0000);
	Write_SPI(CHIP,ENGINE,0xC9,0x0000);
	Write_SPI(CHIP,ENGINE,0xCA,0x0000);
	Write_SPI(CHIP,ENGINE,0xCB,0x0000);
	Write_SPI(CHIP,ENGINE,0xCC,0x0000);
	Write_SPI(CHIP,ENGINE,0xCD,0x0000);
	Write_SPI(CHIP,ENGINE,0xCE,0x0100);
	Write_SPI(CHIP,ENGINE,0xCF,0x0000);
}

// ------------------------------------------------------------------------
void LoadCounterBounds(int CHIP, int ENGINE, int lower, int upper){
	int up_lsb;
	int up_msb;
	int lo_lsb;
	int lo_msb;

	lo_lsb = lower & 0x0000FFFF;
	lo_msb = (lower & 0xFFFF0000) >> 16;
	up_lsb = upper & 0x0000FFFF;
	up_msb = (upper & 0xFFFF0000) >> 16;

	Write_SPI(CHIP,ENGINE,0x40,lo_lsb);
	Write_SPI(CHIP,ENGINE,0x41,lo_msb);
	Write_SPI(CHIP,ENGINE,0x42,up_lsb);
	Write_SPI(CHIP,ENGINE,0x43,up_msb);
}

// ------------------------------------------------------------------------
void Test_Sequence_1(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x8D9CB675, 0x8D9CB680);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_2(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	//LoadCounterBounds	(CHIP, ENGINE, 0x8D9CB670, 0x8D9CB675);
	LoadCounterBounds	(CHIP, ENGINE, 0x0, 0xFFFFFFFF);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 200000);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_3(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x8D9CB670, 0x8D9CB680);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_4(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x8D9CB660, 0x8D9CB670);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_5(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x00000000, 0xFFFFFFFF);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_6(int CHIP, int ENGINE){
	printf(" *** Error: Test 6 uses external manual clocking --- not implemented for this release\n");
	printf("            Choose test 7 (internal clock version) instead.\n");
}

// ------------------------------------------------------------------------
void Test_Sequence_7(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x60000000, 0x8FFFFFFF);

	Set_Regs_Valid	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x10000000, 0x3FFFFFFF);

	Set_Regs_Valid	(CHIP, ENGINE);

	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
	Wait_Busy		(CHIP, ENGINE, 2);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}

// ------------------------------------------------------------------------
void Test_Sequence_8(int CHIP, int ENGINE){

	Enable_HE_Clock	(CHIP, ENGINE);
	Reset_Engine		(CHIP, ENGINE);
	Reset_Err_Flags	(CHIP, ENGINE);

	LoadBarrierAdder	(CHIP, ENGINE);
	LoadLimitRegisters	(CHIP, ENGINE);
	LoadRegistersValues_H0(CHIP, ENGINE);
	LoadRegistersValues_H1(CHIP, ENGINE);
	LoadRegistersValues_W	(CHIP, ENGINE);

	LoadCounterBounds	(CHIP, ENGINE, 0x38E94250, 0x38E94260);

	Set_Regs_Valid	(CHIP, ENGINE);
	Wait_Done		(CHIP, ENGINE, 2);
	Set_Rd_Complete	(CHIP, ENGINE);
}
