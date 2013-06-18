/*
 * STM32_Module.h
 *
 * Created: 11/10/2012 23:20:21
 *  Author: NASSER GHOSEIRI
 * Company: Butterfly Labs
 */ 


#ifndef STM32_MODULE_H_
#define STM32_MODULE_H_

//////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////

// General MCU Functions
 void  __STM32_LowLevelInitialize(); // Initialize the CPU, PLL, SPI, IOs... basically all...

// A2D Functions
 void  __STM32_A2D_Initialize();
 void  __STM32_A2D_SetAccess(); // This function prepares access to A2D module
 float __STM32_A2D_GetTemp1 ();
 float __STM32_A2D_GetTemp2 ();
 float __STM32_A2D_Get3P3V  ();
 float __STM32_A2D_Get1V    ();
 float __STM32_A2D_GetPWR_MAIN();

// USB Chip Functions
 void			 __STM32_USB_Initialize();
 void			 __STM32_USB_SetAccess(); // This function prepares access to USB device (may contain nothing)
 char		 __STM32_USB_WriteData(char* iData, unsigned int iCount);
 char		 __STM32_USB_GetData  (char* iData, unsigned int iMaxCount);
 char		 __STM32_USB_GetInformation();
 void			 __STM32_USB_FlushInputData();
 void			 __STM32_USB_FlushOutputData();

// XLINK Functions
 void		__STM32_CPLD_Initialize();
 void		__STM32_CPLD_SetAccess();
 void		__STM32_CPLD_Write    (char iAdrs, char iData);
 char	__STM32_CPLD_Read     (char iAdrs);

// SC Chips
 void		__STM32_SC_Initialize();
 void		__STM32_SC_SetAccess();
 unsigned int	__STM32_SC_GetDone  (char iChip);
 unsigned int	__STM32_SC_ReadData (char iChip, char iEngine, unsigned char iAdrs);
 unsigned int	__STM32_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData);

// Main LED
 void		__STM32_MainLED_Initialize();
 void		__STM32_MainLED_Set();
 void		__STM32_MainLEd_Reset();

// LEDs
 void		__STM32_LED_Initialize();
 void		__STM32_LED_SetAccess();
 void		__STM32_LED_Set  (char iLed);
 void		__STM32_LED_Reset(char iLed);

// Timer
 void	    __STM32_Timer_Initialize();
 void	    __STM32_Timer_SetInterval(unsigned int iPeriod);
 void		__STM32_Timer_Start();
 void		__STM32_Timer_Stop();
 int		__STM32_Timer_GetValue();


#endif /* STM32_MODULE_H_ */