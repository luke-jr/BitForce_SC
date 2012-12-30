/*
 * PIC32_Module.h
 *
 * Created: 11/10/2012 23:20:37
 *  Author: NASSER
 */ 
#ifndef PIC32_MODULE_H_
#define PIC32_MODULE_H_

//////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////

// General MCU Functions
 void __PIC32_LowLevelInitialize(); // Initialize the CPU, PLL, SPI, IOs... basically all...

// A2D Functions
 void  __PIC32_A2D_Initialize();
 void  __PIC32_A2D_SetAccess(); // This function prepares access to A2D module
 float __PIC32_A2D_GetTemp1 ();
 float __PIC32_A2D_GetTemp2 ();
 float __PIC32_A2D_Get3P3V  ();
 float __PIC32_A2D_Get1V    ();
 float __PIC32_A2D_GetPWR_MAIN();

// USB Chip Functions
 void		__PIC32_USB_Initialize();
 void		__PIC32_USB_SetAccess(); // This function prepares access to USB device (may contain nothing)
 char	__PIC32_USB_WriteData(char* iData, unsigned int iCount);
 char	__PIC32_USB_GetData  (char* iData, unsigned int iMaxCount);
 char	__PIC32_USB_GetInformation();
 void		__PIC32_USB_FlushInputData();
 void		__PIC32_USB_FlushOutputData();

// XLINK Functions
 void		__PIC32_CPLD_Initialize();
 void		__PIC32_CPLD_SetAccess();
 void		__PIC32_CPLD_Write    (char iAdrs, char iData);
 char	__PIC32_CPLD_Read     (char iAdrs);

// SC Chips
 void			__PIC32_SC_Initialize();
 void			__PIC32_SC_SetAccess();
 unsigned int	__PIC32_SC_GetDone  (char iChip);
 unsigned int	__PIC32_SC_ReadData (char iChip, char iEngine, unsigned char iAdrs);
 unsigned int	__PIC32_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData);

// Main LED
 void		__PIC32_MainLED_Initialize();
 void		__PIC32_MainLED_Set();
 void		__PIC32_MainLEd_Reset();

// LEDs
 void		__PIC32_LED_Initialize();
 void		__PIC32_LED_SetAccess();
 void		__PIC32_LED_Set  (char iLed);
 void		__PIC32_LED_Reset(char iLed);

// Timer
 void	    __STM32_Timer_Initialize();
 void	    __PIC32_Timer_SetInterval(unsigned int iPeriod);
 void		__PIC32_Timer_Start();
 void		__PIC32_Timer_Stop();

// FAN unit
 void	MCU_FAN_Initialize();
 void	MCU_FAN_SetAccess ();
 void	MCU_FAN_SetSpeed  (char iSpeed);

#endif /* PIC32_MODULE_H_ */