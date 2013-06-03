/*
 * Generic_Module.h
 *
 * Created: 12/10/2012 00:10:54
 *  Author: NASSER GHOSEIRI
 */ 
#ifndef GENERIC_MODULE_H_
#define GENERIC_MODULE_H_

// Now it depends which MCU we have chosen
// General MCU Functions
void MCU_LowLevelInitialize();

// A2D Functions
void  MCU_A2D_Initialize();
void  MCU_A2D_SetAccess();
volatile int MCU_A2D_GetTemp1 ();
volatile int MCU_A2D_GetTemp2 ();
volatile int MCU_A2D_Get3P3V  ();
volatile int MCU_A2D_Get1V();
volatile int MCU_A2D_GetPWR_MAIN();

// USB Chip Functions
void	MCU_USB_Initialize();
void	MCU_USB_SetAccess();

char	MCU_USB_WriteData(char* iData, unsigned int iCount);
char	MCU_USB_GetData(char* iData, unsigned int iMaxCount);
char	MCU_USB_GetInformation();

void	MCU_USB_FlushInputData();
void	MCU_USB_FlushOutputData();
 
// XLINK Functions
void	MCU_CPLD_Initialize();
void	MCU_CPLD_SetAccess();
void	MCU_CPLD_Write(char iAdrs, char iData);
char    MCU_CPLD_Read(char iAdrs);

// SC Chips
void	MCU_SC_Initialize();
void	MCU_SC_SetAccess();
unsigned int MCU_SC_GetDone  (char iChip);
unsigned int MCU_SC_ReadData (char iChip, char iEngine, unsigned char iAdrs);
unsigned int MCU_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData);

// Main LED
void	MCU_MainLED_Initialize();
void	MCU_MainLED_Set();
void	MCU_MainLED_Reset();

// LEDs
void	MCU_LED_Initialize();
void	MCU_LED_SetAccess();
void	MCU_LED_Set(char iLed);
void	MCU_LED_Reset(char iLed);

// Timer
void    MCU_Timer_Initialize();
void    MCU_Timer_SetInterval(unsigned int iPeriod);
void	MCU_Timer_Start();
void	MCU_Timer_Stop();
int		MCU_Timer_GetValue();

// FAN unit
void	MCU_FAN_Initialize();
void	MCU_FAN_SetAccess();
void	MCU_FAN_SetSpeed(char iSpeed);

// Some ASIC definitions
void    __MCU_ASIC_Activate_CS(char iBank);
void    __MCU_ASIC_Deactivate_CS(char iBank);


#endif /* GENERIC_MODULE_H_ */