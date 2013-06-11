/*
 * JohnChengASIC.h
 *
 * Created: 24/03/2013 17:28:32
 */ 


#ifndef JOHNCHENGASIC_H_
#define JOHNCHENGASIC_H_


// ---------------------------------------------------------------------
//   GLOBAL VARIABLES
// ---------------------------------------------------------------------

int DATAREG0;
int DATAIN;
int DATAOUT;

// =================================================================
void SynchronizeSPI_Module(void);

//==================================================================
void Write_SPI(int chip, int engine, int reg, int value);

//==================================================================
int Read_SPI(int chip, int engine, int reg);

//==================================================================
void initEngines(int CHIP);

//==================================================================
void Enable_HE_Clock(int CHIP, int ENGINE);

//==================================================================
void Reset_Engine(int CHIP, int ENGINE);

//=========================================================================
int Wait_Busy(int CHIP, int ENGINE, int max_num_trys);

//=========================================================================
int Wait_Done(int CHIP, int ENGINE, int max_num_trys);

//=========================================================================
void Set_Regs_Valid(int CHIP, int ENGINE);

//=========================================================================
void Set_Rd_Complete(int CHIP, int ENGINE);

//==================================================================
void Reset_Err_Flags(int CHIP, int ENGINE);
//==================================================================
void LoadBarrierAdder(int CHIP, int ENGINE);

//==================================================================
void LoadLimitRegisters(int CHIP, int ENGINE);

//==================================================================
void LoadRegistersValues_H0(int CHIP, int ENGINE);

//==================================================================
void LoadRegistersValues_H1(int CHIP, int ENGINE);

//==================================================================
void LoadRegistersValues_W(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void LoadCounterBounds(int CHIP, int ENGINE, int lower, int upper);

// ------------------------------------------------------------------------
void Test_Sequence_1(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_2(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_3(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_4(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_5(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_6(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_7(int CHIP, int ENGINE);

// ------------------------------------------------------------------------
void Test_Sequence_8(int CHIP, int ENGINE);



#endif /* JOHNCHENGASIC_H_ */