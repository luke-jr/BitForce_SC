/*
 * Generic_Module.c
 *
 * Created: 21/11/2012 00:54:04
 *  Author: NASSER
 */ 


#include "Generic_Module.h"
#include "std_defs.h"

// Include all headers
#include "AVR32_Module.h"
#include "PIC32_Module.h"
#include "STM32_Module.h"


// Now it depends which MCU we have chosen
// General MCU Functions
void MCU_LowLevelInitialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_LowLevelInitialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_LowLevelInitialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_LowLevelInitialize();
	#endif
}

// A2D Functions
void  MCU_A2D_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_A2D_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_A2D_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_A2D_Initialize();
	#endif
}

void  MCU_A2D_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_A2D_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_A2D_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_A2D_SetAccess();
	#endif
}

volatile int MCU_A2D_GetTemp1 ()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_A2D_GetTemp1();
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_A2D_GetTemp1();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_A2D_GetTemp1();
	#endif
}

volatile int MCU_A2D_GetTemp2 ()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	volatile int iRetVal = __AVR32_A2D_GetTemp2();
	return iRetVal;
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_A2D_GetTemp2();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_A2D_GetTemp2();
	#endif
}

volatile int MCU_A2D_Get3P3V  ()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_A2D_Get3P3V();
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_A2D_Get3P3V();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_A2D_Get3P3V();
	#endif
}

volatile int MCU_A2D_Get1V    ()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_A2D_Get1V();
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_A2D_Get1V();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_A2D_Get1V();
	#endif
}

volatile int MCU_A2D_GetPWR_MAIN()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_A2D_GetPWR_MAIN();
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_A2D_GetPWR_MAIN();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_A2D_GetPWR_MAIN();
	#endif
}

// USB Chip Functions
void	MCU_USB_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_USB_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_USB_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_USB_Initialize();
	#endif
}

void	MCU_USB_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_USB_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_USB_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_USB_SetAccess();
	#endif
}

char MCU_USB_WriteData(char* iData, unsigned int iCount)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_USB_WriteData(iData, iCount);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_USB_WriteData(iData, iCount);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_USB_WriteData(iData, iCount);
	#endif
}

char MCU_USB_GetData(char* iData, unsigned int iMaxCount)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_USB_GetData(iData, iMaxCount);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_USB_GetData(iData, iMaxCount);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_USB_GetData(iData, iMaxCount);
	#endif
}

char MCU_USB_GetInformation()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_USB_GetInformation();
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_USB_GetInformation();
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_USB_GetInformation();
	#endif
}

void	MCU_USB_FlushInputData()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_USB_FlushInputData();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_USB_FlushInputData();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_USB_FlushInputData();
	#endif
}

void	MCU_USB_FlushOutputData()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_USB_FlushOutputData();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_USB_FlushOutputtData();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_USB_FlushOutputData();
	#endif
}

// XLINK Functions
void	MCU_CPLD_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_CPLD_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_CPLD_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_CPLD_Initialize();
	#endif
}

void	MCU_CPLD_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_CPLD_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_CPLD_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_CPLD_SetAccess();
	#endif
}

void	MCU_CPLD_Write(char iAdrs, char iData)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_CPLD_Write(iAdrs, iData);
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_CPLD_Write(iAdrs, iData);
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_CPLD_Write(iAdrs, iData);
	#endif
}

char MCU_CPLD_Read(char iAdrs)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_CPLD_Read(iAdrs);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_CPLD_Read(iAdrs);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_CPLD_Read(iAdrs);
	#endif
}

// SC Chips
void MCU_SC_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_SC_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_SC_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_SC_Initialize();
	#endif
}

void MCU_SC_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_SC_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_SC_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_SC_SetAccess();
	#endif
}

unsigned int MCU_SC_GetDone(char iChip)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_SC_GetDone(iChip);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_SC_GetDone(iChip);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_SC_GetDone(iChip);
	#endif
}

void __MCU_ASIC_Activate_CS(void)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	 __AVR32_ASIC_Activate_CS();
	#elif defined( __COMPILING_FOR_STM32__)
	 __STM32_ASIC_Activate_CS();
	#elif defined( __COMPILING_FOR_PIC32__)
	 __PIC32_ASIC_Activate_CS();
	#endif
}
 
void __MCU_ASIC_Deactivate_CS(void)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	 __AVR32_ASIC_Deactivate_CS();
	#elif defined( __COMPILING_FOR_STM32__)
	 __STM32_ASIC_Deactivate_CS();
	#elif defined( __COMPILING_FOR_PIC32__)
	 __PIC32_ASIC_Deactivate_CS();
	#endif
}

unsigned int MCU_SC_ReadData(char iChip, unsigned short iAdrs)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_SC_ReadData(iChip, iAdrs);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_SC_ReadData(iChip, iAdrs);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_SC_ReadData(iChip, iAdrs);
	#endif
}

unsigned int MCU_SC_WriteData(char iChip, unsigned short iAdrs, unsigned int iData)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	return __AVR32_SC_WriteData(iChip, iAdrs, iData);
	#elif defined( __COMPILING_FOR_STM32__)
	return __STM32_SC_WriteData(iChip, iAdrs, iData);
	#elif defined( __COMPILING_FOR_PIC32__)
	return __PIC32_SC_WriteData(iChip, iAdrs, iData);
	#endif
}

// Main LED
void	MCU_MainLED_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_MainLED_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_MainLED_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_MainLED_Initialize();
	#endif
}

void	MCU_MainLED_Set()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_MainLED_Set();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_MainLED_Set();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_MainLED_Set();
	#endif
}

void	MCU_MainLED_Reset()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_MainLED_Reset();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_MainLEd_Reset();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_MainLEd_Reset();
	#endif
}

// LEDs
void	MCU_LED_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_LED_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_LED_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_LED_Initialize();
	#endif
}

void	MCU_LED_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_LED_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_LED_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_LED_SetAccess();
	#endif
}

void	MCU_LED_Set(char iLed)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_LED_Set(iLed);
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_LED_Set(iLed);
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_LED_Set(iLed);
	#endif
}

void	MCU_LED_Reset(char iLed)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_LED_Reset(iLed);
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_LED_Reset(iLed);
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_LED_Reset(iLed);
	#endif
}

// Timer
void MCU_Timer_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_Timer_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_Timer_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_Timer_Initialize();
	#endif
}

void	MCU_Timer_SetInterval(unsigned int iPeriod)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_Timer_SetInterval(iPeriod);
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_Timer_SetInterval(iPeriod);
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_Timer_SetInterval(iPeriod);
	#endif
}

void	MCU_Timer_Start()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_Timer_Start();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_Timer_Start();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_Timer_Start();
	#endif
}

void	MCU_Timer_Stop()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_Timer_Stop();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_Timer_Stop();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_Timer_Stop();
	#endif
}

// FAN unit
void	MCU_FAN_Initialize()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_FAN_Initialize();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_FAN_Initialize();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_FAN_Initialize();
	#endif
}

void	MCU_FAN_SetAccess()
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_FAN_SetAccess();
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_FAN_SetAccess();
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_FAN_SetAccess();
	#endif
}

void	MCU_FAN_SetSpeed(char iSpeed)
{
	#if   defined( __COMPILING_FOR_AVR32__)
	__AVR32_FAN_SetSpeed(iSpeed);
	#elif defined( __COMPILING_FOR_STM32__)
	__STM32_FAN_SetSpeed(iSpeed);
	#elif defined( __COMPILING_FOR_PIC32__)
	__PIC32_FAN_SetSpeed(iSpeed);
	#endif
}