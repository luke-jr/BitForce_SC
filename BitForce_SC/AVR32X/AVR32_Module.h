/*
 * AVR32_MODULE.h
 *
 * Created: 11/10/2012 21:59:40
 *  Author: NASSER GHOSEIRI
 */ 
#ifndef AVR32_MODULE_H_
#define AVR32_MODULE_H_

// Register Definitions -- MCU SPECIFIC
#define AVR32_POSCSR								(*((volatile unsigned int*)0xFFFF0C54))
#define AVR32_PLL0									(*((volatile unsigned int*)0xFFFF0C20))
#define AVR32_PLL1									(*((volatile unsigned int*)0xFFFF0C24))
#define AVR32_OSCCTRL0								(*((volatile unsigned int*)0xFFFF0C28))
#define AVR32_MCCTRL								(*((volatile unsigned int*)0xFFFF0C00))
#define AVR32_CKSEL									(*((volatile unsigned int*)0xFFFF0C04))
#define AVR32_GCCTRL								(*((volatile unsigned int*)0xFFFF0C60))
#define AVR32_PBAMASK								(*((volatile unsigned int*)0xFFFF0C10))

#define AVR32_SPI0_CR								(*((volatile unsigned int*)0xFFFF2400))
#define AVR32_SPI0_MR								(*((volatile unsigned int*)0xFFFF2404))
#define AVR32_SPI0_CSR0								(*((volatile unsigned int*)0xFFFF2430))
#define AVR32_SPI0_TDR								(*((volatile unsigned int*)0xFFFF240C))
#define AVR32_SPI0_RDR								(*((volatile unsigned int*)0xFFFF2408))
#define AVR32_SPI0_SR								(*((volatile unsigned int*)0xFFFF2410))

#define AVR32_SPI1_CR								(*((volatile unsigned int*)0xFFFF2800))
#define AVR32_SPI1_MR								(*((volatile unsigned int*)0xFFFF2804))
#define AVR32_SPI1_CSR0								(*((volatile unsigned int*)0xFFFF2830))
#define AVR32_SPI1_TDR								(*((volatile unsigned int*)0xFFFF280C))
#define AVR32_SPI1_RDR								(*((volatile unsigned int*)0xFFFF2808))
#define AVR32_SPI1_SR								(*((volatile unsigned int*)0xFFFF2810))

#define AVR32_FLASHC_MAIN							(*((volatile unsigned int*)0xFFFE1400))
#define AVR32_FLASHC_CONTROL						(*((volatile unsigned int*)0xFFFE1400))
#define AVR32_FLASHC_COMMAND						(*((volatile unsigned int*)0xFFFE1404))
#define AVR32_FLASHC_STATUS							(*((volatile unsigned int*)0xFFFE1408))
#define AVR32_FLASHC_COMMAND_WRITE_PAGE				1
#define AVR32_FLASHC_COMMAND_ERASE_PAGE				2
#define AVR32_FLASHC_COMMAND_READ_PAGE				12
#define AVR32_FLASHC_COMMAND_CLEAR_PAGE_BUFFER		3
#define AVR32_FLASHC_COMMAND_WRITE_USER_PAGE		13
#define AVR32_FLASHC_COMMAND_ERASE_USER_PAGE		14
#define AVR32_FLASHC_COMMAND_READ_USER_PAGE			15

#define	AVR32_SPI_STATUS_RDRF						(0b00000000001)
#define AVR32_SPI_STATUS_TDRE						(0b00000000010)
#define	AVR32_SPI_STATUS_OVRS						(0b00000001000)
#define AVR32_SPI_STATUS_ENDRX						(0b00000010000)
#define AVR32_SPI_STATUS_ENDTX						(0b00000100000)
#define AVR32_SPI_STATUS_RXBUFF						(0b00001000000)
#define AVR32_SPI_STATUS_TXBUFF						(0b00010000000)
#define AVR32_SPI_STATUS_TXEMPTY					(0b01000000000)

#define AVR32_A2D_BASE_ADDRESS						0xFFFF3C00
#define AVR32_A2D_CONTROL_REGISTER_ADRS				(AVR32_A2D_BASE_ADDRESS + 0x000)
#define AVR32_A2D_MODE_REGISTER_ADRS 				(AVR32_A2D_BASE_ADDRESS + 0x004)
#define AVR32_A2D_CHANNEL_ENABLE_REGISTER_ADRS		(AVR32_A2D_BASE_ADDRESS + 0x010)
#define AVR32_A2D_CHANNEL_STATUS_REGISTER_ADRS		(AVR32_A2D_BASE_ADDRESS + 0x018)
#define AVR32_A2D_STATUS_REGISTER_ADRS				(AVR32_A2D_BASE_ADDRESS + 0x01C)
#define AVR32_A2D_CDR0_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x030)
#define AVR32_A2D_CDR1_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x034)
#define AVR32_A2D_CDR2_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x038)
#define AVR32_A2D_CDR3_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x03C)
#define AVR32_A2D_CDR4_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x040)
#define AVR32_A2D_CDR5_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x044)
#define AVR32_A2D_CDR6_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x048)
#define AVR32_A2D_CDR7_ADRS							(AVR32_A2D_BASE_ADDRESS + 0x04C)

#define AVR32_A2D_CONTROL_REGISTER    				(*((volatile unsigned int*)AVR32_A2D_CONTROL_REGISTER_ADRS))
#define AVR32_A2D_MODE_REGISTER      				(*((volatile unsigned int*)AVR32_A2D_MODE_REGISTER_ADRS))
#define AVR32_A2D_CHANNEL_ENABLE_REGISTER			(*((volatile unsigned int*)AVR32_A2D_CHANNEL_ENABLE_REGISTER_ADRS))
#define AVR32_A2D_CHANNEL_STATUS_REGISTER			(*((volatile unsigned int*)AVR32_A2D_CHANNEL_STATUS_REGISTER_ADRS))
#define AVR32_A2D_STATUS_REGISTER					(*((volatile unsigned int*)AVR32_A2D_STATUS_REGISTER_ADRS))

#define AVR32_A2D_CDR0								(*((volatile unsigned int*)AVR32_A2D_CDR0_ADRS))
#define AVR32_A2D_CDR1								(*((volatile unsigned int*)AVR32_A2D_CDR1_ADRS))
#define AVR32_A2D_CDR2								(*((volatile unsigned int*)AVR32_A2D_CDR2_ADRS))
#define AVR32_A2D_CDR3								(*((volatile unsigned int*)AVR32_A2D_CDR3_ADRS))
#define AVR32_A2D_CDR4								(*((volatile unsigned int*)AVR32_A2D_CDR4_ADRS))
#define AVR32_A2D_CDR5								(*((volatile unsigned int*)AVR32_A2D_CDR5_ADRS))
#define AVR32_A2D_CDR6								(*((volatile unsigned int*)AVR32_A2D_CDR6_ADRS))
#define AVR32_A2D_CDR7								(*((volatile unsigned int*)AVR32_A2D_CDR7_ADRS))

#define AVR32_A2D_TEMP1_CHANNEL						(0) // This is the MCU channel we use for the first  temperature sensor
#define AVR32_A2D_TEMP2_CHANNEL						(1) // This is the MCU channel we use for the second temperature sensor

#define AVR32_A2D_VCHANNEL_3P3V						(3)
#define AVR32_A2D_VCHANNEL_1V						(2)
#define AVR32_A2D_VCHANNEL_PWR_MAIN					(4)

#define AVR32_A2D_TEMP1_CHANNEL_CDR					AVR32_A2D_CDR0 // This is the MCU channel we use for the first  temperature sensor
#define AVR32_A2D_TEMP2_CHANNEL_CDR					AVR32_A2D_CDR1 // This is the MCU channel we use for the second temperature sensor

#define AVR32_A2D_VCHANNEL_3P3V_CDR					AVR32_A2D_CDR3
#define AVR32_A2D_VCHANNEL_1V_CDR					AVR32_A2D_CDR2
#define AVR32_A2D_VCHANNEL_PWR_MAIN_CDR				AVR32_A2D_CDR4

#define __AVR32_FAN_CTRL0	 (1<<20)
#define __AVR32_FAN_CTRL1	 (1<<21)
#define __AVR32_FAN_CTRL2	 (1<<22)
#define __AVR32_FAN_CTRL3	 (1<<23)

#define __AVR32_ADX_MUX0	 (1<<24)
#define __AVR32_ADX_MUX1	 (1<<25)
#define __AVR32_ADX_MUX2	 (1<<26)
#define __AVR32_ADX_MUX3	 (1<<27)

// Definitions
#define __AVR32_USB_AD0		 (1<<0)	// PORT A
#define __AVR32_USB_AD1		 (1<<1)	// PORT A
#define __AVR32_USB_AD2		 (1<<2)	// PORT A
#define __AVR32_USB_AD3		 (1<<3)	// PORT A
#define __AVR32_USB_AD4		 (1<<4)	// PORT A
#define __AVR32_USB_AD5		 (1<<5)	// PORT A
#define __AVR32_USB_AD6		 (1<<6)	// PORT A
#define __AVR32_USB_AD7		 (1<<7)	// PORT A

#define __AVR32_USB_WR		 (1<<8)	// PORT A
#define __AVR32_USB_RD		 (1<<9)	// PORT A

#define __AVR32_USB_SIWUA	 (1<<1)  // PORT C
#define __AVR32_USB_A0		 (1<<4)  // PORT C
#define __AVR32_USB_CS		 (1<<5)  // PORT C

// Chain function definitions
#define __AVR32_CPLD_BUS0	 (1<<0) // PORT B
#define __AVR32_CPLD_BUS1	 (1<<1) // PORT B
#define __AVR32_CPLD_BUS2	 (1<<2) // PORT B
#define __AVR32_CPLD_BUS3	 (1<<3) // PORT B
#define __AVR32_CPLD_BUS4	 (1<<4) // PORT B
#define __AVR32_CPLD_BUS5	 (1<<5) // PORT B
#define __AVR32_CPLD_BUS6	 (1<<6) // PORT B
#define __AVR32_CPLD_BUS7	 (1<<7) // PORT B
#define __AVR32_CPLD_BUS_ALL (__AVR32_CPLD_BUS0 | __AVR32_CPLD_BUS1 | __AVR32_CPLD_BUS2 | __AVR32_CPLD_BUS3 | __AVR32_CPLD_BUS4 | __AVR32_CPLD_BUS5 | __AVR32_CPLD_BUS6 | __AVR32_CPLD_BUS7)

#define __AVR32_CPLD_INCREASE_ADDRESS (1<<10) // PORT B
							 
#define __AVR32_CPLD_ADRS	 (1<<8) // PORT B
#define __AVR32_CPLD_OE		 (1<<9) // PORT B
							 
#define __AVR32_CPLD_RES0	 (1<<10) // PORT B
#define __AVR32_CPLD_RES1	 (1<<11) // PORT B
							 
#define __AVR32_CPLD_CS		 (1<<26) // PORT A
#define __AVR32_CPLD_STROBE	 (1<<29) // PORT A
							 
// SC Chip interface		 
#define AVR32_SPI0_PIN1		 (1 << 13) // PORT A
#define AVR32_SPI0_PIN2		 (1 << 12) // PORT A
#define AVR32_SPI0_PIN3		 (1 << 11) // PORT A
#define AVR32_SPI0_PIN_NPCS	 (1 << 10) // PORT A

// SC Chip interface for 16 chip version
#define AVR32_SPI1_PIN1		 (1 << 17) // PORT A
#define AVR32_SPI1_PIN2		 (1 << 16) // PORT A
#define AVR32_SPI1_PIN3		 (1 << 15) // PORT A
#define AVR32_SPI1_PIN_NPCS	 (1 << 14) // PORT A
							 
#define AVR32_SC_CHIP_DONE0	 (1 << 12) // PORT B
#define AVR32_SC_CHIP_DONE1	 (1 << 13) // PORT B
#define AVR32_SC_CHIP_DONE2	 (1 << 14) // PORT B
#define AVR32_SC_CHIP_DONE3	 (1 << 15) // PORT B
#define AVR32_SC_CHIP_DONE4	 (1 << 16) // PORT B
#define AVR32_SC_CHIP_DONE5	 (1 << 17) // PORT B
#define AVR32_SC_CHIP_DONE6	 (1 << 18) // PORT B
#define AVR32_SC_CHIP_DONE7	 (1 << 19) // PORT B

// MAIN LED					 
#define __AVR32_MAIN_LED_PIN (1<<18)    // Port A

// Side LEDs
#define __AVR32_ENGINE_LED1  (1<<24)   // PORT B
#define __AVR32_ENGINE_LED2  (1<<25)   // PORT B
#define __AVR32_ENGINE_LED3  (1<<26)   // PORT B
#define __AVR32_ENGINE_LED4  (1<<27)   // PORT B
#define __AVR32_ENGINE_LED5  (1<<28)   // PORT B
#define __AVR32_ENGINE_LED6  (1<<29)   // PORT B
#define __AVR32_ENGINE_LED7  (1<<30)   // PORT B
#define __AVR32_ENGINE_LED8  (1<<31)   // PORT B

//////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////

// General MCU Functions
volatile void  __AVR32_LowLevelInitialize(void);

// A2D Functions
volatile void  __AVR32_A2D_Initialize(void);
volatile void  __AVR32_A2D_SetAccess(void);
volatile int   __AVR32_A2D_GetTemp1(void);
volatile int   __AVR32_A2D_GetTemp2(void);
volatile int   __AVR32_A2D_Get3P3V(void);
volatile int   __AVR32_A2D_Get1V(void);
volatile int   __AVR32_A2D_GetPWR_MAIN(void);

/////////////////////////////////////////////
// USB Chip Functions
/////////////////////////////////////////////

void	__AVR32_USB_Initialize(void);
void	__AVR32_USB_SetAccess(void);
char	__AVR32_USB_WriteData(char* iData, unsigned int iCount);
int		__AVR32_USB_GetInformation(void);
char	__AVR32_USB_GetData(char* iData, char iMaxCount);
void	__AVR32_USB_FlushInputData(void);
void	__AVR32_USB_FlushOutputData(void);

////////////////////////////////////////////
// CHAIN Functions
////////////////////////////////////////////
void		 __AVR32_CPLD_Initialize(void);
void		 __AVR32_CPLD_SetAccess(void);
void		 __AVR32_CPLD_Write (char iAdrs, char iData);
unsigned int __AVR32_CPLD_Read (char iAdrs);
void		 __AVR32_CPLD_BurstTxWrite(char* iData, char iAddress);
void		 __AVR32_CPLD_BurstRxRead(char* iData, char iAddress);
void		 __AVR32_CPLD_StartTX(char iTxControlValue);

//////////////////////////////////////////////
// SC Chips
//////////////////////////////////////////////
void __AVR32_SC_Initialize(void);
void __AVR32_SC_SetAccess(void);
unsigned int __AVR32_SC_GetDone  (char iChip);
unsigned int __AVR32_SC_ReadData (char iChip, char iEngine, unsigned char iAdrs);
void __AVR32_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData);
void __AVR32_SC_WriteData_Express(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData);
void __AVR32_ASIC_Activate_CS(char iBank); 
void __AVR32_ASIC_Deactivate_CS(char iBank);
void __AVR32_SPI0_SendWord(unsigned short data);
void __AVR32_SPI1_SendWord(unsigned short data);
unsigned short __AVR32_SPI0_ReadWord(void);
unsigned short __AVR32_SPI1_ReadWord(void);

////////////////////////////////////////////////////
// Main LED
////////////////////////////////////////////////////
void	__AVR32_MainLED_Initialize(void);
void	__AVR32_MainLED_Set(void);
void	__AVR32_MainLED_Reset(void);

//////////////////////////////////////////////////
// LEDs
// (Works only with JALAPENO and Little Singles)
//////////////////////////////////////////////////
void	__AVR32_LED_Initialize(void);
void	__AVR32_LED_SetAccess(void);
void	__AVR32_LED_Set  (char iLed);
void	__AVR32_LED_Reset(char iLed);

/////////////////////////////////////////////
// Timer
/////////////////////////////////////////////
void	__AVR32_Timer_Initialize(void);
void	__AVR32_Timer_SetInterval(unsigned int iPeriod);
void	__AVR32_Timer_Start(void);
void	__AVR32_Timer_Stop(void);
int		__AVR32_Timer_GetValue();

/////////////////////////////////////////////////
// FAN Controller
/////////////////////////////////////////////////
void	__AVR32_FAN_Initialize(void);
void	__AVR32_FAN_SetAccess(void);
void	__AVR32_FAN_SetSpeed(char iSpeed);

/////////////////////////////////////////////////
// A2D MUX Controller 
// (Works only with MiniRig and Single models)
/////////////////////////////////////////////////
void	__AVR32_A2D_MUX_Initialize(void);
void	__AVR32_A2D_MUX_SetAccess(void);
void	__AVR32_A2D_MUX_SelectChannel(int iChannel);
int		__AVR32_A2D_MUX_Convert(void);

/////////////////////////////////////////////////
// Flash programming
/////////////////////////////////////////////////
void	__AVR32_Flash_Initialize(void);
void	__AVR32_Flash_WriteUserPage(char* szData);
void	__AVR32_Flash_ReadUserPage (char* szData);

#endif /* AVR32_MODULE_H_ */