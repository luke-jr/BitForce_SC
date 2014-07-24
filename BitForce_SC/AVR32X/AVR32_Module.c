/*
 * AVR32_Module.c
 *
 * Created: 21/11/2012 20:58:33
 *  Author: NASSER GHOSEIRI
 * Company: Butterfly Labs
 */ 

#include <avr32/io.h>
#include "intc.h"
#include "../std_defs.h"
#include "AVR32_Module.h"

//////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////

#define XLINK_activate_address_increase     AVR32_GPIO.port[1].ovrs  = __AVR32_CPLD_INCREASE_ADDRESS;
#define XLINK_deactivate_address_increase   AVR32_GPIO.port[1].ovrc  = __AVR32_CPLD_INCREASE_ADDRESS;


// General MCU Functions
void __AVR32_LowLevelInitialize()
{
	// ********* Enable OSC0
	AVR32_MCCTRL  |= 0b100; // OSC0EN = 1
	AVR32_CKSEL   = 0; // All Clocks equal main-clock
	AVR32_OSCCTRL0 = 0b00000000000000000000000100000111;
	
	// Enable watchdog to reset the system if PLL fails
	AVR32_WDT.ctrl = (0x055000000) | (0b0111100000000) | 0b01; // Timeout is set to 142ms
	NOP_OPERATION;
	AVR32_WDT.ctrl = (0x0AA000000) | (0b0111100000000) | 0b01;
	NOP_OPERATION;
	AVR32_WDT.clr = 0x0FFFFFFFF;

	// Wait until it's active (1 << 7 is OSC0 Status bit)
	while ((AVR32_POSCSR & (1 << 7)) == 0);

	#if defined(__OPERATING_FREQUENCY_32MHz__)
		// ********* Enable PLL0
		// Value for PLL0 is 00011111    00000111    00000100   -----------00000101----------
		//                   PLLCNT=31   PLLMUL=7+1  PLLDIV=4   PLLOPT=001, PLLOSC=1, PLLEN=1
		#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) // Remember, here the XTAL is 12MHz
			AVR32_PLL0 = 0b00011111000010100000010000000101; // PLL0 at 32MHz
		#else
			AVR32_PLL0 = 0b00011111000001110000010000000101; // PLL0 at 32MHz
		#endif
	#elif defined(__OPERATING_FREQUENCY_64MHz__)
		// ********* Enable PLL0
		// Value for PLL0 is 00011111    00000111    0000010   -----------00000101----------
		//                   PLLCNT=31   PLLMUL=7+1  PLLDIV=2   PLLOPT=001, PLLOSC=0, PLLEN=1
		#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) // Remember, here the XTAL is 12MHz
			AVR32_PLL0 = 0b00011111000010100000001000000101; // PLL0 at 64MHz
		#else
			AVR32_PLL0 = 0b00011111000001110000001000000101; // PLL0 at 64MHz
		#endif		 	   
		
		// Set wait-state to 1
		AVR32_FLASHC_MAIN = ((1 << 8) | (1 << 6)) | 0x00000000;
		
	#elif defined(__OPERATING_FREQUENCY_48MHz__)
		// ********* Enable PLL0
		// Value for PLL0 is 00011111    00000011    00000000   -----------00000101----------
		//                   PLLCNT=31   PLLMUL=3+1  PLLDIV=0   PLLOPT=000, PLLOSC=1, PLLEN=1
		AVR32_PLL0 = 0b00011111000010000000010000000101; // PLL0 at 48MHz		
	#elif defined(__OPERATING_FREQUENCY_16MHz__)
		// ********* Enable PLL0
		// Value for PLL0 is 00011111    00000011    00000000   -----------00000101----------
		//                   PLLCNT=31   PLLMUL=3+1  PLLDIV=0   PLLOPT=001, PLLOSC=1, PLLEN=1
		AVR32_PLL0 = 0b00011111000001110000010000000101; // PLL0 at 32MHz (same as the first mode here, as the PLL will not be used)
	#endif


	// Wait until PLL0 is stable (1 is PLL0 LOCK status bit)
	while ((AVR32_POSCSR & (1)) == 0);
	
	// Disable watchdog to reset the system if PLL fails
	/*AVR32_WDT.ctrl = (0x055000000) | (0b0101000000000) | 0b00;
	NOP_OPERATION;
	AVR32_WDT.ctrl = (0x0AA000000) | (0b0101000000000) | 0b00;
	NOP_OPERATION;*/

	// ********* Activate Generic clock
	#if defined(__OPERATING_FREQUENCY_32MHz__)
		AVR32_GCCTRL = 0b00000000000000000000000000000110; // This is for 32MHz, with DIVEN=0
		// Second bit in this block is our PLL-SEL=1, OSCSEL=0, DIVEN=0, DIV=0
	#elif defined(__OPERATING_FREQUENCY_64MHz__)
		AVR32_GCCTRL = 0b00000000000000000000000000000110; // This is for 64MHz, with DIVEN=0
		// Second bit in this block is our PLL-SEL=1, OSCSEL=0, DIVEN=0, DIV=0
	#elif defined(__OPERATING_FREQUENCY_48MHz__)
		AVR32_GCCTRL = 0b00000000000000000000000000000110; // This is for 48MHz, with DIVEN=0 (thus 48MHz for the IO)
		// Second bit in this block is our PLL-SEL=1, OSCSEL=0, DIVEN=0, DIV=0		
	#elif defined(__OPERATING_FREQUENCY_16MHz__)
		AVR32_GCCTRL = 0b00000000000000000000000000000100; // This is for 16MHz, with DIVEN=0
		// Second bit in this block is our PLL-SEL=0, OSCSEL=0, DIVEN=0, DIV=0
	#endif

	// ********* Switch system to the new clock
	#if defined(__OPERATING_FREQUENCY_16MHz__)
		AVR32_MCCTRL |= 0b01; 	// MCSEL = 1 -- OSC0 Selected
	#elif defined(__OPERATING_FREQUENCY_32MHz__)
		AVR32_MCCTRL |= 0b010; 	// MCSEL = 1 -- PLL0 Selected
	#elif defined(__OPERATING_FREQUENCY_64MHz__)
		AVR32_MCCTRL |= 0b010; 	// MCSEL = 1 -- PLL0 Selected		
	#elif defined(__OPERATING_FREQUENCY_48MHz__)
		AVR32_MCCTRL |= 0b010; 	// MCSEL = 1 -- PLL0 Selected			
	#endif
			
	// Wait a little, until everything is stable...
	volatile int i_delay = 0;
	while (i_delay < 8000) i_delay++;

	// OK, System is running at oscillator frequency ( i.e. 16MHz)
	i_delay += 1;
}

// A2D Functions
void  __AVR32_A2D_Initialize()
{
	// Initialize A2D unit
	AVR32_A2D_MODE_REGISTER = MAKE_DWORD(0b00001000, // Setup/Hold Time = 1000 (binary)
										0b00001000, // Statup Time = 1000 (binar)
										0b00001001, // PRESCALE set to 9 (effect is (9+1) * 2 = 20)
										0b00000000); // No Trigger, resultion = 10 bits, no RESET or SLEEP
	
	// Enable A2D Channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
												   0b00000000,
												   0b00000000,
												   (1<<AVR32_A2D_TEMP1_CHANNEL) |
												   (1<<AVR32_A2D_TEMP2_CHANNEL) |
												   (1<<AVR32_A2D_VCHANNEL_3P3V) |
												   (1<<AVR32_A2D_VCHANNEL_1V)   |
												   (1<<AVR32_A2D_VCHANNEL_PWR_MAIN)); // Enables channel 1 by default (CORRECT THIS!)
}

void  __AVR32_A2D_SetAccess()
{
	// Nothing special for this function
	return;
}

volatile int __AVR32_A2D_GetTemp1 ()
{
	// Activate the channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
												   0b00000000,
												   0b00000000,
												   (1<<AVR32_A2D_TEMP1_CHANNEL)); // Enables channel 1 by default (CORRECT THIS!)
	// Initiate Conversion
	AVR32_A2D_CONTROL_REGISTER |= (0b010); // Second-Bit activates the START command... thus conversion will start

	// Keep reading until the channel is ready
	volatile unsigned int a2d_val = 0;

	// Wait until conversion finishes
	while ((AVR32_A2D_CHANNEL_STATUS_REGISTER & MAKE_DWORD(0,0,0,(1<<AVR32_A2D_TEMP1_CHANNEL))) == 0x0); // Wait until the conversion finishes

	// Read the value
	a2d_val = AVR32_A2D_TEMP1_CHANNEL_CDR;

	// Make correct AND operation
	a2d_val &= 0b01111111111;

	// Now, the A2D converter will give between 0b0000000000 and 0b1111111111.
	// Each LSB in the A2D range means 3.2 mV. To get the real voltage
	volatile float voltage_value = 3.2f * a2d_val;

	// Now we have the voltage.
	// Convert the returned value into temperature
	// Note that 500mV means 0 degrees (that's our offset there...), and each 10mV equals one degree
	volatile float res = ((voltage_value - 500) / 10);

	// We're done
	return (int)res;
}

volatile int __AVR32_A2D_GetTemp2 ()
{
	// Activate the channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
													0b00000000,
													0b00000000,
													(1<<AVR32_A2D_TEMP2_CHANNEL)); // Enables channel 1 by default (CORRECT THIS!)

	// Initiate Conversion
	AVR32_A2D_CONTROL_REGISTER |= (0b010); // Second-Bit activates the START command... thus conversion will start
	
	// Keep reading until the channel is ready
	volatile unsigned int a2d_val = 0;
	
	// Wait until conversion finishes
	while ((AVR32_A2D_CHANNEL_STATUS_REGISTER & MAKE_DWORD(0,0,0,(1<<AVR32_A2D_TEMP2_CHANNEL))) == 0x0); // Wait until the conversion finishes
	
	// Read the value
	a2d_val = AVR32_A2D_TEMP2_CHANNEL_CDR;
	
	// Make correct AND operation
	a2d_val &= 0b01111111111;
	
	// Now, the A2D converter will give between 0b0000000000 and 0b1111111111.
	// Each LSB in the A2D range means 3.2 mV. To get the real voltage
	volatile float voltage_value = 3.2f * a2d_val;
	
	// Now we have the voltage.	
	// Convert the returned value into temperature
	// Note that 500mV means 0 degrees (that's our offset there...), and each 10mV equals one degree
	volatile float res = ((voltage_value - 500) / 10);
	
	// We're done
	return (int)res;
}

volatile int __AVR32_A2D_Get3P3V  ()
{
	// Activate the channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
		0b00000000,
		0b00000000,
		(1<<AVR32_A2D_VCHANNEL_3P3V)); // Enables channel 1 by default (CORRECT THIS!)
		
	// Initiate Conversion
	AVR32_A2D_CONTROL_REGISTER |= (0b010); // Second-Bit activates the START command... thus conversion will start
	
	// Keep reading until the channel is ready
	volatile unsigned int a2d_val = 0;
	
	// Wait until conversion finishes
	while ((AVR32_A2D_CHANNEL_STATUS_REGISTER & MAKE_DWORD(0,0,0,(1<<AVR32_A2D_VCHANNEL_3P3V))) == 0x0); // Wait until the conversion finishes
	
	// Read the value
	a2d_val = AVR32_A2D_VCHANNEL_3P3V_CDR;
	
	// Make correct AND operation
	a2d_val &= 0b01111111111;
	
	// Now, the A2D converter will give between 0b0000000000 and 0b1111111111.
	// Each LSB in the A2D range means 3.2 mV. To get the real voltage
	volatile float voltage_value = 3.2f * a2d_val;
	
	// Now we have the voltage.
	return (int)voltage_value;
}

volatile int __AVR32_A2D_Get1V()
{
	// Activate the channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
		0b00000000,
		0b00000000,
		(1<<AVR32_A2D_VCHANNEL_1V)); // Enables channel 1 by default (CORRECT THIS!)
		
	// Initiate Conversion
	AVR32_A2D_CONTROL_REGISTER |= (0b010); // Second-Bit activates the START command... thus conversion will start
	
	// Keep reading until the channel is ready
	volatile unsigned int a2d_val = 0;
	
	// Wait until conversion finishes
	while ((AVR32_A2D_CHANNEL_STATUS_REGISTER & MAKE_DWORD(0,0,0,(1<<AVR32_A2D_VCHANNEL_1V))) == 0x0); // Wait until the conversion finishes
	
	// Read the value
	a2d_val = AVR32_A2D_VCHANNEL_1V_CDR;
	
	// Make correct AND operation
	a2d_val &= 0b01111111111;
	
	// Now, the A2D converter will give between 0b0000000000 and 0b1111111111.
	// Each LSB in the A2D range means 3.2 mV. To get the real voltage
	volatile float voltage_value = 3.2f * a2d_val;

	// We're done
	return (int)voltage_value;
}

volatile int __AVR32_A2D_GetPWR_MAIN()
{
	// Activate the channel
	AVR32_A2D_CHANNEL_ENABLE_REGISTER = MAKE_DWORD(0b00000000,
													0b00000000,
													0b00000000,
													(1<<AVR32_A2D_VCHANNEL_PWR_MAIN)); // Enables channel 1 by default (CORRECT THIS!)
				
	// Initiate Conversion
	AVR32_A2D_CONTROL_REGISTER |= (0b010); // Second-Bit activates the START command... thus conversion will start
	
	// Keep reading until the channel is ready
	volatile unsigned int a2d_val = 0;
	
	// Wait until conversion finishes
	while ((AVR32_A2D_CHANNEL_STATUS_REGISTER & MAKE_DWORD(0,0,0,(1<<AVR32_A2D_VCHANNEL_PWR_MAIN))) == 0x0); // Wait until the conversion finishes
	
	// Read the value
	a2d_val = AVR32_A2D_VCHANNEL_PWR_MAIN_CDR;
	
	// Make correct AND operation
	a2d_val &= 0b01111111111;
	
	// Now, the A2D converter will give between 0b0000000000 and 0b1111111111.
	// Each LSB in the A2D range means 3.2 mV. To get the real voltage
	volatile float voltage_value = 3.2f * a2d_val;
	
	// We're done
	return (int)voltage_value;
}

/////////////////////////////////////////////
// USB Chip Functions
/////////////////////////////////////////////
volatile void __AVR32_USB_Initialize()
{
	// Enable GPIOs on Port A
	AVR32_GPIO.port[0].gpers =  __AVR32_USB_AD0 | __AVR32_USB_AD1 | __AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 | __AVR32_USB_AD6 | __AVR32_USB_AD7 |
								__AVR32_USB_WR  | __AVR32_USB_RD;
	
	AVR32_GPIO.port[0].oders = 	__AVR32_USB_WR	| __AVR32_USB_RD;
	AVR32_GPIO.port[0].ovrs =   __AVR32_USB_WR	| __AVR32_USB_RD;
	
	// Enable GPIOs on Port C
	AVR32_GPIO.port[2].gpers =  __AVR32_USB_SIWUA |	__AVR32_USB_A0 | __AVR32_USB_CS;
	AVR32_GPIO.port[2].oders =  __AVR32_USB_SIWUA |	__AVR32_USB_A0 | __AVR32_USB_CS;
	
	// Permanently select this chip
	AVR32_GPIO.port[0].ovrc  =  __AVR32_USB_A0;
}

volatile void __AVR32_USB_SetAccess()
{
	// No conditional access here, we simply return...
	return;
}

volatile char __AVR32_USB_WriteData(char* iData, unsigned int iCount)
{
	// Return if we're zero
	if (iCount == 0) return 0;
	
	// Write data...
	AVR32_GPIO.port[0].oders =  __AVR32_USB_AD0 | __AVR32_USB_AD1 | __AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 | __AVR32_USB_AD6 | __AVR32_USB_AD7;
	NOP_OPERATION;
	
	for (unsigned int vx = 0; vx < iCount; vx++)
	{
		// Is there space available?
		if ((__AVR32_USB_GetInformation() & 0b010) == FALSE) 
		{
			// Activate Send Immediate and wait until data is sent
			__AVR32_USB_FlushOutputData();
			
			
			// Wait until there is space available (With a countdown)			
			volatile unsigned int iRevCounter = 0x0010F0000;
			while ((iRevCounter-- > 0) && ((__AVR32_USB_GetInformation() & 0b010) == FALSE))
			{
				// Do nothing
			}
		
			// Did we timeout? If so, break
			if (iRevCounter == 0) break;
		}			
		
		// Write data
		AVR32_GPIO.port[0].ovr  = (AVR32_GPIO.port[0].ovr & 0x0FFFFFF00) | iData[vx];
		NOP_OPERATION;
		AVR32_GPIO.port[0].ovrc = __AVR32_USB_WR;
		NOP_OPERATION;
		AVR32_GPIO.port[0].ovrs = __AVR32_USB_WR;
		NOP_OPERATION;
		
	}
	
	// Disable output drivers on bus lines
	AVR32_GPIO.port[0].oderc =  __AVR32_USB_AD0 | __AVR32_USB_AD1 |	__AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 |	__AVR32_USB_AD6 | __AVR32_USB_AD7;
	NOP_OPERATION;
	// Return
	return iCount;
}

volatile int  __AVR32_USB_GetInformation()
{
	// Save actual port state
	volatile int iActualPortState = (AVR32_GPIO.port[0].oder & 0x0FF);
	volatile int iRetVal = 0;
	
	// Write data...
	AVR32_GPIO.port[0].oderc =  __AVR32_USB_AD0 | __AVR32_USB_AD1 |	__AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 |	__AVR32_USB_AD6 | __AVR32_USB_AD7;
	NOP_OPERATION;
									
	// Set A0
	AVR32_GPIO.port[2].ovrs =  __AVR32_USB_A0;
	NOP_OPERATION;
	
	// Get the value
	AVR32_GPIO.port[0].ovrc = __AVR32_USB_RD;
	NOP_OPERATION;
	iRetVal  = ((AVR32_GPIO.port[0].pvr) & 0x0FF);
	NOP_OPERATION;
	AVR32_GPIO.port[0].ovrs = __AVR32_USB_RD;
	NOP_OPERATION;
	
	// Clear A0
	AVR32_GPIO.port[2].ovrc =  __AVR32_USB_A0;
	NOP_OPERATION;
	
	// Restore state
	AVR32_GPIO.port[0].oder = (AVR32_GPIO.port[0].oder & 0x0FFFFFF00) | iActualPortState;
	NOP_OPERATION;
	
	// Return the value
	return iRetVal;
}

volatile char __AVR32_USB_GetData(char* iData, char iMaxCount)
{
	// Return if we're zero
	if (iMaxCount == 0) return 0;
	
	// Write data...
	AVR32_GPIO.port[0].oderc =  __AVR32_USB_AD0 | __AVR32_USB_AD1 |	__AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 |	__AVR32_USB_AD6 | __AVR32_USB_AD7 ;
	NOP_OPERATION;
	// Clear A0
	AVR32_GPIO.port[2].ovrc =  __AVR32_USB_A0;
	NOP_OPERATION;
	
	// Write as many bytes as needed
	volatile unsigned int i_total_read = 0;
	for (unsigned int vx = 0; vx < iMaxCount; vx++)
	{
		// Is there data?
		if (USB_inbound_USB_data() == FALSE) break;
		
		// Get the value
		NOP_OPERATION;
		AVR32_GPIO.port[0].ovrc = __AVR32_USB_RD;
		NOP_OPERATION;
		iData[vx]  = ((AVR32_GPIO.port[0].pvr) & 0x0FF);
		AVR32_GPIO.port[0].ovrs = __AVR32_USB_RD;
		NOP_OPERATION;
		
		// Increase our total read count
		i_total_read++;
	}
	
	return i_total_read;
}

volatile void __AVR32_USB_FlushInputData()
{
	// Write data...
	AVR32_GPIO.port[0].oderc =  __AVR32_USB_AD0 | __AVR32_USB_AD1 |	__AVR32_USB_AD2 | __AVR32_USB_AD3 |
								__AVR32_USB_AD4 | __AVR32_USB_AD5 |	__AVR32_USB_AD6 | __AVR32_USB_AD7;

	// Write as many bytes as needed
	while ((__AVR32_USB_GetInformation() & 0b01) == 0b01)
	{
		// Get the value
		AVR32_GPIO.port[0].ovrc = __AVR32_USB_RD;
		NOP_OPERATION;
		AVR32_GPIO.port[0].ovrs = __AVR32_USB_RD;
		NOP_OPERATION;
	}
}

volatile void __AVR32_USB_FlushOutputData()
{
	// Set A0
	AVR32_GPIO.port[2].ovrs =  __AVR32_USB_SIWUA;
	NOP_OPERATION;
	AVR32_GPIO.port[2].ovrc =  __AVR32_USB_SIWUA;
	NOP_OPERATION;
}

////////////////////////////////////////////
// CHAIN Functions
////////////////////////////////////////////
void __AVR32_CPLD_Initialize()
{
	// Enable GPIOs on Port A
	AVR32_GPIO.port[1].oderc =  __AVR32_CPLD_BUS_ALL | __AVR32_CPLD_RES0 |  __AVR32_CPLD_RES1; // Disable bus output for this time...
	NOP_OPERATION;
	AVR32_GPIO.port[1].gpers =  __AVR32_CPLD_BUS_ALL | __AVR32_CPLD_ADRS | 	__AVR32_CPLD_OE | __AVR32_CPLD_RES0 | __AVR32_CPLD_RES1;
	NOP_OPERATION;
	AVR32_GPIO.port[1].oders =  __AVR32_CPLD_ADRS    | __AVR32_CPLD_OE;
	NOP_OPERATION;
	AVR32_GPIO.port[0].gpers =  __AVR32_CPLD_CS		 | __AVR32_CPLD_STROBE;
	NOP_OPERATION;
	AVR32_GPIO.port[0].oders =  __AVR32_CPLD_CS		 | __AVR32_CPLD_STROBE;
	NOP_OPERATION;
	
	// Deselect CPLD and deactivate OE
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	NOP_OPERATION;
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_CS;
	NOP_OPERATION;
	
	// We activate address-increase pin and de-assert it
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_INCREASE_ADDRESS;
	NOP_OPERATION;
	AVR32_GPIO.port[1].gpers = __AVR32_CPLD_INCREASE_ADDRESS;	
	NOP_OPERATION;
	AVR32_GPIO.port[1].ovrc  = __AVR32_CPLD_INCREASE_ADDRESS;
	NOP_OPERATION;
}

inline void __AVR32_CPLD_SetAccess()
{
	// Nothing needed, the bus is not multiplexed here...
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_CS; // Activate CS and leave it active...
}

inline void __AVR32_CPLD_Write (char iAdrs, char iData)
{
	// Disable CPLDs OE and Select the chip
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Activate bus output first AND set the ADRS pin
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL;
	
	// Set address
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAdrs;
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Set Data and disable address
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iData;
	
	// Write data
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Ok now disable output bus
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
}

inline void __AVR32_CPLD_StartTX(char iTxControlValue)
{
	// Disable CPLDs OE and Select the chip
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;

	// Activate bus output first AND set the ADRS pin
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL;

	// Set address
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | 34; // 34 is CPLD_TX_ADDRESS_COTROL

	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;

	// Set Data and disable address
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS;

	// Activate Address-increase
	XLINK_activate_address_increase;

	// Write data
	volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00);
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | iTxControlValue;
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | 0b01;
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;

	// Disable Address-Increase
	XLINK_deactivate_address_increase;

	// Ok now disable output bus
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
}


inline void __AVR32_CPLD_BurstTxWrite(char* iData, char iAddress)
{
	// Disable CPLDs OE and Select the chip
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Activate bus output first AND set the ADRS pin
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL;
	
	// Set address
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAddress;
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Set Data and disable address
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS;
	
	// Activate Address-increase
	XLINK_activate_address_increase;

	// Write data
	volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00);
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | iData[0];
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | iData[1];
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | iData[2];
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | iData[3];
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;	
	
	// Disable Address-Increase
	XLINK_deactivate_address_increase;		
	
	// Ok now disable output bus
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
}

inline void __AVR32_CPLD_BurstRxRead(char* iData, char iAddress)
{
	// Disable CPLD's OE and Select the chip
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Activate bus output first AND set the ADRS pin
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL;
	
	// Set address
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAddress;
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Disable address
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS;
	
	// Ok now set bus to input mode
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
	
	// Activate OE
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE;
	
	// Activate Address-increase
	XLINK_activate_address_increase;
	
	// Get the result
	iData[0] = (AVR32_GPIO.port[1].pvr & 0x000000FF);
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Get the result
	iData[1] = (AVR32_GPIO.port[1].pvr & 0x000000FF);
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;	
	
	// Get the result
	iData[2] = (AVR32_GPIO.port[1].pvr & 0x000000FF);
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;	
	
	// Get the result
	iData[3] = (AVR32_GPIO.port[1].pvr & 0x000000FF);
	
	// Disable OE
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Disable Address-Increase
	XLINK_deactivate_address_increase;	
	
	// Ok now disable output bus
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
}

inline unsigned int	__AVR32_CPLD_Read (char iAdrs)
{
	// Disable CPLD's OE and Select the chip
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Activate bus output first AND set the ADRS pin
	AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL;
	
	// Set address
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS;
	AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAdrs;
	
	// Strobe
	AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE;
	AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE;
	
	// Disable address
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS;
	
	// Ok now set bus to input mode
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
	
	// Activate OE
	AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE;
	
	// Get the result
	volatile int iRetVal = (AVR32_GPIO.port[1].pvr & 0x000000FF);
	
	// Disable OE
	AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE;
	
	// Ok now disable output bus
	AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL;
	
	// return data
	return iRetVal;
}

//////////////////////////////////////////////
// SC Chips
//////////////////////////////////////////////
volatile void __AVR32_SC_Initialize()
{
	////////////////// Set SPI1 GPIO settings (Function-A)
	AVR32_GPIO.port[0].gperc = (AVR32_SPI0_PIN1)  |  (AVR32_SPI0_PIN2)  |  (AVR32_SPI0_PIN3);// SPI0_PIN_MISO;
	AVR32_GPIO.port[0].pmr1c = (AVR32_SPI0_PIN1)  |  (AVR32_SPI0_PIN2)  |  (AVR32_SPI0_PIN3);// SPI0_PIN_MOSI;
	AVR32_GPIO.port[0].pmr0c = (AVR32_SPI0_PIN1)  |  (AVR32_SPI0_PIN2)  |  (AVR32_SPI0_PIN3);// SPI0_PIN_SCK;

	AVR32_GPIO.port[0].gpers = AVR32_SPI0_PIN_NPCS;
	AVR32_GPIO.port[0].oders = AVR32_SPI0_PIN_NPCS;
	AVR32_GPIO.port[0].ovrs  = AVR32_SPI0_PIN_NPCS;

	// Activate SPI1
	AVR32_SPI0_CR 	= (1 << 0); // SPIEN = 1
	AVR32_SPI0_MR   = (0b01 | (1<<4)); // MSTR = 1, The rest have their default value... 
	// NOTE: SPI FREQ REDUCED, PLEASE CORRECT FOR PRODUCTION
	AVR32_SPI0_CSR0   = (0b010) | (0b0100 << 8) | (0b01000 << 4); // NPCHA = 1, CPOL = 0 (SPI MODE 0), SCBR = 100 (Clk/4=16MHz), BITS = 1000 (16 Bit mode)	
	
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Now, if we are a full-single or a mini-rig, we need to activate second SPI as well
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		// Set SPI1 GPIO settings (Function-A)
		AVR32_GPIO.port[0].gperc = (AVR32_SPI1_PIN1)  |  (AVR32_SPI1_PIN2)  |  (AVR32_SPI1_PIN3);// SPI1_PIN_MISO;
		AVR32_GPIO.port[0].pmr1c = (AVR32_SPI1_PIN1)  |  (AVR32_SPI1_PIN2)  |  (AVR32_SPI1_PIN3);// SPI1_PIN_MOSI;
		AVR32_GPIO.port[0].pmr0s = (AVR32_SPI1_PIN1)  |  (AVR32_SPI1_PIN2)  |  (AVR32_SPI1_PIN3);// SPI1_PIN_SCK;
		AVR32_GPIO.port[0].gpers = AVR32_SPI1_PIN_NPCS;
		AVR32_GPIO.port[0].oders = AVR32_SPI1_PIN_NPCS;
		AVR32_GPIO.port[0].ovrs  = AVR32_SPI1_PIN_NPCS;
		// Activate SPI1
		AVR32_SPI1_CR 	= (1 << 0); // SPIEN = 1
		AVR32_SPI1_MR   = (0b01 | (1<<4)); // MSTR = 1, The rest have their default value...
		// NOTE: SPI FREQ REDUCED, PLEASE CORRECT FOR PRODUCTION
		AVR32_SPI1_CSR0   = (0b010) | (0b0100 << 8) | (0b01000 << 4); // NPCHA = 1, CPOL = 0 (SPI MODE 0), SCBR = 100 (Clk/4=16MHz), BITS = 1000 (16 Bit mode)			
	#endif	
	
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%		
	// Only Jalapeno and Little-Single support DONE pins
	//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Activate SC_CHIP_DONE
		AVR32_GPIO.port[1].oderc = (AVR32_SC_CHIP_DONE0)  |  (AVR32_SC_CHIP_DONE1)  |  (AVR32_SC_CHIP_DONE2) | (AVR32_SC_CHIP_DONE3) |
								   (AVR32_SC_CHIP_DONE4)  |  (AVR32_SC_CHIP_DONE5)  |  (AVR32_SC_CHIP_DONE6) | (AVR32_SC_CHIP_DONE7) ;
		NOP_OPERATION;
		NOP_OPERATION;
		AVR32_GPIO.port[1].gpers = (AVR32_SC_CHIP_DONE0)  |  (AVR32_SC_CHIP_DONE1)  |  (AVR32_SC_CHIP_DONE2) | (AVR32_SC_CHIP_DONE3) |
								   (AVR32_SC_CHIP_DONE4)  |  (AVR32_SC_CHIP_DONE5)  |  (AVR32_SC_CHIP_DONE6) | (AVR32_SC_CHIP_DONE7) ;
		NOP_OPERATION;
		NOP_OPERATION;
	#endif
}

volatile inline void __AVR32_ASIC_Activate_CS(char iBank)
{
	if (iBank == 1)
	{
		AVR32_GPIO.port[0].ovrc   = AVR32_SPI0_PIN_NPCS;	
	}	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		if (iBank == 2)
		{
			AVR32_GPIO.port[0].ovrc   = AVR32_SPI1_PIN_NPCS;	
		}		
	#endif
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
}

volatile inline void __AVR32_ASIC_Deactivate_CS(char iBank)
{
	if (iBank == 1)
	{
		AVR32_GPIO.port[0].ovrs   = AVR32_SPI0_PIN_NPCS;	
	}
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		if (iBank == 2) 
		{
			AVR32_GPIO.port[0].ovrs   = AVR32_SPI1_PIN_NPCS;
		}			
	#endif
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
}

volatile inline void __AVR32_SPI1_SendWord(unsigned short data)
{
	// Put data in register and wait until its sent
	AVR32_SPI1_TDR = (data & 0x0FFFF);
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	while ((AVR32_SPI1_SR & (1 << 9)) == 0);
}

volatile unsigned short __AVR32_SPI1_ReadWord()
{
	__AVR32_SPI1_SendWord(0x0FFFF); // FF stands for no-care
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	return (AVR32_SPI1_RDR & 0x0FFFF);
}

volatile inline void __AVR32_SPI0_SendWord(unsigned short data)
{
	// Put data in register and wait until its sent
	AVR32_SPI0_TDR = (data & 0x0FFFF);
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	while ((AVR32_SPI0_SR & (1 << 9)) == 0);
}

volatile unsigned short __AVR32_SPI0_ReadWord()
{
	__AVR32_SPI0_SendWord(0x0FFFF); // FF stands for no-care
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	NOP_OPERATION;
	return (AVR32_SPI0_RDR & 0x0FFFF);
}

volatile void	__AVR32_SC_SetAccess()
{
	// Nothing, this is not multiplexed
}

volatile inline unsigned int __AVR32_SC_GetDone  (char iChip)
{
	if (iChip == 0) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE0) != 0);
	if (iChip == 1) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE1) != 0);
	if (iChip == 2) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE2) != 0);
	if (iChip == 3) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE3) != 0);
	if (iChip == 4) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE4) != 0);
	if (iChip == 5) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE5) != 0);
	if (iChip == 6) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE6) != 0);
	if (iChip == 7) return ((AVR32_GPIO.port[1].pvr & AVR32_SC_CHIP_DONE7) != 0);
	return FALSE; // SHOULDNT REACH HERE
}

#define MACRO__AVR32_SPI0_SendWord(x) \
		{ \
			AVR32_SPI0_TDR = (x & 0x0FFFF); \
			while ((AVR32_SPI0_SR & (1 << 9)) == 0); \
		}
		
#define MACRO__AVR32_SPI0_SendWord_Express(x) \
		{ \
			AVR32_SPI0_TDR = (x & 0x0FFFF); \
			while ((AVR32_SPI0_SR & (1 << 1)) == 0); \
		}
		
		
#define MACRO__AVR32_SPI0_ReadWord(x) \
		{ \
			MACRO__AVR32_SPI0_SendWord(0x0FFFF); \ 
			x = (AVR32_SPI0_RDR & 0x0FFFF); \
		}		
		
		
// For 16 chip model, using SPI1
#define MACRO__AVR32_SPI1_SendWord(x) \
		{ \
			AVR32_SPI1_TDR = (x & 0x0FFFF); \
			while ((AVR32_SPI1_SR & (1 << 9)) == 0); \
		}

#define MACRO__AVR32_SPI1_SendWord_Express(x) \
		{ \
			AVR32_SPI1_TDR = (x & 0x0FFFF); \
			while ((AVR32_SPI1_SR & (1 << 1)) == 0); \
		}

#define MACRO__AVR32_SPI1_ReadWord(x) \
		{ \
			MACRO__AVR32_SPI1_SendWord(0x0FFFF); \
			x = (AVR32_SPI1_RDR & 0x0FFFF); \
		}		
		

inline unsigned int __AVR32_SC_ReadData (char iChip, char iEngine, unsigned char iAdrs)
{
	unsigned int iCommand = 0;
			
	// Send it via SPI
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__) 
	
		if (iChip < 8)	
		{
			// Generate the command
			iCommand = (ASIC_SPI_RW_COMMAND) |  // RW COMMAND (1 = Read)
						(((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
						(((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
						(((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
						
			__AVR32_SPI0_SendWord(iCommand);
			return (__AVR32_SPI0_ReadWord() & 0x0FFFF);						
		}
		else
		{
			// Generate the command
			iCommand = (ASIC_SPI_RW_COMMAND) |  // RW COMMAND (1 = Read)
						(((unsigned int)((iChip-8)   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
						(((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
						(((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
						
			// We're on an 16 chip model
			__AVR32_SPI1_SendWord(iCommand);
			return (__AVR32_SPI1_ReadWord() & 0x0FFFF);						
		}
	
		
	#else 
		// Generate the command
		iCommand = (ASIC_SPI_RW_COMMAND) |  // RW COMMAND (1 = Read)
					(((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
					(((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
					(((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
		
		// We're on an 8 chip model
		__AVR32_SPI0_SendWord(iCommand);
		return (__AVR32_SPI0_ReadWord() & 0x0FFFF);		
	#endif
}

inline void __AVR32_SC_WriteData_Express(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData)
{
	unsigned int iCommand = 0;
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		// 16 Chip model
		
		// Generate the command
		if (iChip < 8)
		{
			// RW COMMAND (0 = WRITE)
			iCommand = (((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
					   (((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
					   (((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
		
			// Send it via SPI
			MACRO__AVR32_SPI0_SendWord_Express(iCommand);
			MACRO__AVR32_SPI0_SendWord_Express(iData & 0x0FFFF);			
		}
		else
		{
			// RW COMMAND (0 = WRITE)
			iCommand = (((unsigned int)((iChip - 8)   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
					   (((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
					   (((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
			
			// Send it via SPI
			MACRO__AVR32_SPI1_SendWord_Express(iCommand);
			MACRO__AVR32_SPI1_SendWord_Express(iData & 0x0FFFF);			
		}
	
	#else
		// 8 Chip model
		
		// Generate the command
		// RW COMMAND (0 = WRITE)
		iCommand = (((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
		(((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
		(((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
	
		// Send it via SPI
		MACRO__AVR32_SPI0_SendWord_Express(iCommand);
		MACRO__AVR32_SPI0_SendWord_Express(iData & 0x0FFFF);	
	#endif
	

}

inline void __AVR32_SC_WriteData(char iChip, char iEngine, unsigned char iAdrs, unsigned int iData)
{
	unsigned int iCommand = 0;
	
	#if defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)
		// 16 Chip model
		// Generate the command
		if (iChip < 8)
		{
			// RW COMMAND (0 = WRITE)
			iCommand = (((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
					   (((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
					   (((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
		
			// Send it via SPI
			__AVR32_SPI0_SendWord(iCommand);
			__AVR32_SPI0_SendWord(iData & 0xFFFF);			
		}
		else
		{
			// RW COMMAND (0 = WRITE)
			iCommand = (((unsigned int)((iChip - 8)   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
					   (((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
					   (((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
			
			// Send it via SPI
			__AVR32_SPI1_SendWord(iCommand);
			__AVR32_SPI1_SendWord(iData & 0xFFFF);			
		}	
	#else
		// Generate the command
		// RW COMMAND (0 = WRITE)
		iCommand = (((unsigned int)(iChip   & 0x0FF) << 12 ) &  0b0111000000000000) | // Chip address
				   (((unsigned int)(iEngine & 0x0FF) << 8  ) &  0b0000111100000000) |
				   (((unsigned int)(iAdrs   & 0x0FF)       ) &  0b0000000011111111); // Memory Address
							  							  
		// Send it via SPI
		__AVR32_SPI0_SendWord(iCommand);
		__AVR32_SPI0_SendWord(iData & 0xFFFF);		
	#endif	
	

}

////////////////////////////////////////////////////
// Main LED
////////////////////////////////////////////////////
#define __AVR32_MAIN_LED_PIN	(1<<18)    // Port A

void	__AVR32_MainLED_Initialize()
{
	// Enable GPIOs on Port A
	AVR32_GPIO.port[0].gpers =  __AVR32_MAIN_LED_PIN;
	AVR32_GPIO.port[0].oders =  __AVR32_MAIN_LED_PIN;
	AVR32_GPIO.port[0].ovrc  =  __AVR32_MAIN_LED_PIN;
}

void	__AVR32_MainLED_Set()
{
	AVR32_GPIO.port[0].ovrs  =  __AVR32_MAIN_LED_PIN;
}

void	__AVR32_MainLED_Reset()
{
	AVR32_GPIO.port[0].ovrc  =  __AVR32_MAIN_LED_PIN;
}

///////////////////////////////////////////////////////
// LEDs
// (Works only with JALAPENO and LittleSingle models)
///////////////////////////////////////////////////////
void	__AVR32_LED_Initialize()
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Enable GPIOs on Port A
		NOP_OPERATION;
		AVR32_GPIO.port[1].gpers =  __AVR32_ENGINE_LED1 | __AVR32_ENGINE_LED2 |	__AVR32_ENGINE_LED3 | __AVR32_ENGINE_LED4 |
									__AVR32_ENGINE_LED5 | __AVR32_ENGINE_LED6 |	__AVR32_ENGINE_LED7 | __AVR32_ENGINE_LED8 ;
		NOP_OPERATION;
		AVR32_GPIO.port[1].oders =  __AVR32_ENGINE_LED1 | __AVR32_ENGINE_LED2 | __AVR32_ENGINE_LED3 | __AVR32_ENGINE_LED4 |
									__AVR32_ENGINE_LED5 | __AVR32_ENGINE_LED6 |	__AVR32_ENGINE_LED7 | __AVR32_ENGINE_LED8 ;
		NOP_OPERATION;			
	#endif					
}

void	__AVR32_LED_SetAccess()
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// return
	#endif
}

void	__AVR32_LED_Set  (char iLed)
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		if (iLed == 1)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED1;
		else if (iLed == 2)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED2;
		else if (iLed == 3)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED3;
		else if (iLed == 4)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED4;
		else if (iLed == 5)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED5;
		else if (iLed == 6)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED6;
		else if (iLed == 7)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED7;
		else if (iLed == 8)
			AVR32_GPIO.port[1].ovrs = __AVR32_ENGINE_LED8;
		
		// IO Sync Delay
		NOP_OPERATION;		
	#endif
}

void	__AVR32_LED_Reset(char iLed)
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)	
		if (iLed == 1)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED1;
		else if (iLed == 2)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED2;
		else if (iLed == 3)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED3;
		else if (iLed == 4)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED4;
		else if (iLed == 5)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED5;
		else if (iLed == 6)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED6;
		else if (iLed == 7)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED7;
		else if (iLed == 8)
			AVR32_GPIO.port[1].ovrc = __AVR32_ENGINE_LED8;
		
		// IO Sync Delay
		NOP_OPERATION;
	#endif
}

/////////////////////////////////////////////
// Timer
/////////////////////////////////////////////

// This function will receiver our Interrupt
ISR(__avr32_tmr0_interrupt, 0, 3)
{
	// Just increase our counter
	MAST_TICK_COUNTER += 0x10000;
	
	// Clear the interrupt.
	volatile unsigned int umx = AVR32_TC.channel[0].sr; // Read the SR flag to clear the interrupt
		
	//AVR32_RTC.val = 0;
	//volatile unsigned long uTop = AVR32_RTC.top;
	/*AVR32_RTC.top = -1;
	while (AVR32_RTC.CTRL.busy); // Wait...	*/
	//AVR32_RTC.ICR.topi = 1 ; // Read the ICR to clear the interrupt
}

void __AVR32_Timer_Initialize()
{
	// Reset master tick counter
	MAST_TICK_COUNTER = 0;

	// Timer0 System
	AVR32_TC.BCR.sync = 0;
	
	AVR32_TC.BMR.tc0xc0s = 0b01;
	AVR32_TC.BMR.tc1xc1s = 0b01;
	AVR32_TC.BMR.tc2xc2s = 0b01;
	
	AVR32_TC.channel[0].CMR.waveform.wave = 1; // Activate Wave-Form mode
	AVR32_TC.channel[0].CMR.waveform.tcclks = 0b011; // TIMER_CLOCK4 , equals PBA Clock / 32 (equals 1us on 32MHz clock)
	AVR32_TC.channel[0].CMR.waveform.wavsel = 0b010; // Reset counter on RC Compare match
	
	AVR32_TC.channel[0].RC.rc = 65535; // Value set to maximum
	
	// Enable interrupt
	AVR32_TC.channel[0].IER.cpcs = 1;
	
	// Activate it
	AVR32_TC.channel[0].CCR.clken = 1;
	AVR32_TC.channel[0].CCR.swtrg = 1;	
	
	// Wait for BUSY to clear
	// Now proceed
	/*
	AVR32_RTC.IER.topi = 1;	
	while (AVR32_RTC.CTRL.busy); // Wait...	
	AVR32_RTC.top = 0x0FFFFFFFF;
	while (AVR32_RTC.CTRL.busy); // Wait...	
	AVR32_RTC.val = 0x0;
	while (AVR32_RTC.CTRL.busy); // Wait...	
	AVR32_RTC.ctrl = (1);
	while (AVR32_RTC.CTRL.busy); // Wait...
	
	// Get the value
	volatile unsigned long umx = 0;
	umx = AVR32_RTC.top;
	*/
		
	// Disable all interrupts first
	Disable_global_interrupt();
	
	// Initialize the intc sw module.
	irq_initialize_vectors();
	
	// Register the TC interrupt handler
	// irq_register_handler(__avr32_tmr0_interrupt, AVR32_RTC_IRQ, 0);
	irq_register_handler(__avr32_tmr0_interrupt, AVR32_TC_IRQ0, 0);
			
    // Enable all interrupts now...
	Enable_global_interrupt();
}

void __AVR32_Timer_SetInterval(unsigned int iPeriod)
{
	// Set the RC compare value
	AVR32_RTC.top = iPeriod; 
}

void __AVR32_Timer_Start()
{
	AVR32_RTC.CTRL.en = 1;
}

void __AVR32_Timer_Stop()
{
	// Enable the Timer
	AVR32_RTC.CTRL.en = 0;
}

int __AVR32_Timer_GetValue()
{
	// Return the RC Value of the timer
	return 	AVR32_TC.channel[0].cv;
}

/////////////////////////////////////////////////
// FAN Controller
/////////////////////////////////////////////////
 void __AVR32_FAN_Initialize()
{
	// Enable GPIOs on Port B
	AVR32_GPIO.port[1].gpers =  __AVR32_FAN_CTRL0 |	__AVR32_FAN_CTRL1 |	__AVR32_FAN_CTRL2 |	__AVR32_FAN_CTRL3;
	AVR32_GPIO.port[1].oders =  __AVR32_FAN_CTRL0 |	__AVR32_FAN_CTRL1 |	__AVR32_FAN_CTRL2 |	__AVR32_FAN_CTRL3;
	AVR32_GPIO.port[1].ovrc  =  __AVR32_FAN_CTRL0 |	__AVR32_FAN_CTRL1 |	__AVR32_FAN_CTRL2 |	__AVR32_FAN_CTRL3;
}

 void __AVR32_FAN_SetAccess()
{
	// nothing special to do
}

 void __AVR32_FAN_SetSpeed(char iSpeed)
{
	volatile unsigned int ovr_value;
	ovr_value = ~( __AVR32_FAN_CTRL3 | __AVR32_FAN_CTRL2 | __AVR32_FAN_CTRL1 | __AVR32_FAN_CTRL0);
	
	volatile unsigned int iFinalVal = AVR32_GPIO.port[1].ovr & ovr_value;
	volatile unsigned int iMXVal = 0;
	
	iMXVal |= (iSpeed & 0b00001) ? __AVR32_FAN_CTRL0 : 0;
	iMXVal |= (iSpeed & 0b00010) ? __AVR32_FAN_CTRL1 : 0;
	iMXVal |= (iSpeed & 0b00100) ? __AVR32_FAN_CTRL2 : 0;
	iMXVal |= (iSpeed & 0b01000) ? __AVR32_FAN_CTRL3 : 0;
	iFinalVal |= iMXVal;
	
	AVR32_GPIO.port[1].ovr = iFinalVal;
}


/////////////////////////////////////////////////
// A2D MUX Controller
// (Works only with MiniRig and Single models)
/////////////////////////////////////////////////
void __AVR32_A2D_MUX_Initialize()
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Not valid
	#else
		// Enable GPIOs on Port B
		AVR32_GPIO.port[1].gpers =  __AVR32_ADX_MUX0 |	__AVR32_ADX_MUX1 |	__AVR32_ADX_MUX2 |	__AVR32_ADX_MUX3;
		AVR32_GPIO.port[1].oders =  __AVR32_ADX_MUX0 |	__AVR32_ADX_MUX1 |	__AVR32_ADX_MUX2 |	__AVR32_ADX_MUX3;
		AVR32_GPIO.port[1].ovrc  =  __AVR32_ADX_MUX0 |	__AVR32_ADX_MUX1 |	__AVR32_ADX_MUX2 |	__AVR32_ADX_MUX3;	
	#endif
}

void __AVR32_A2D_MUX_SetAccess()
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Not valid
	#else
		// Nothing to do
	#endif	
}

void __AVR32_A2D_MUX_SelectChannel(int iChannel)
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Not valid
	#else
		// Extract first 4 bits
		iChannel = iChannel & 0xF;
		
		// Right shift it to reach __AVR32_ADX_MUX0
		iChannel <<= __AVR32_ADX_MUX0;
		
		// Set correct output
		AVR32_GPIO.port[1].ovr = (AVR32_GPIO.port[1].ovr & ~(__AVR32_ADX_MUX0 |	__AVR32_ADX_MUX1 |	__AVR32_ADX_MUX2 |	__AVR32_ADX_MUX3)) | iChannel;
	#endif	
}

int	__AVR32_A2D_MUX_Convert()
{
	#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
		// Not valid	
	#else
		__AVR32_A2D_GetPWR_MAIN(); // A2D MUX OUTPUT is connected to PWR_MAIN for the LITTLE-SINGLE and JALAPENO 
	#endif	
}


/////////////////////////////////////////////////
// Flash programming
/////////////////////////////////////////////////
void		__avr32_flash_writeRegister(uint16_t offset, int32_t value);
int32_t		__avr32_flash_readRegister(uint16_t offset);
void		__avr32_flash_writeUserData(char* szStream);
void		__avr32_flash_readUserData(char* szStream);
void		__avr32_flash_eraseUserData(void);
void		__avr32_flash_waitForFlashReady(void);

//
// Write to Flash Controller register with an offset and value provided
//
volatile void __avr32_flash_writeRegister(uint16_t offset, int32_t value) {
	volatile int32_t *p = (int32_t *)(AVR32_FLASHC_ADDRESS + offset);
	*p = value;
}

//
// Read from Flash Controller register with an offset
//
volatile int32_t __avr32_flash_readRegister(uint16_t offset) {
	volatile int32_t *p = (int32_t *)(AVR32_FLASHC_ADDRESS + offset);
	return *p;
}

//
// Write words into Flash user page
//
volatile void __avr32_flash_writeUserData(char* szStream) {
	// Send Clear Buffer Page command to the Flash controller
	__avr32_flash_writeRegister(AVR32_FLASHC_FCMD, (AVR32_FLASHC_FCMD_KEY_KEY << AVR32_FLASHC_FCMD_KEY_OFFSET) | AVR32_FLASHC_FCMD_CMD_CPB);
	__avr32_flash_waitForFlashReady();
	// Copy data to buffer page
	volatile uint32_t *dst = (uint32_t *)AVR32_FLASHC_USER_PAGE_ADDRESS;
	volatile uint32_t bytes = 512;
	volatile uint32_t word_to_write = 0;
	
	// Now write page-buffer
	for (int x = 0; x < 512; x += 4)
	{
		word_to_write = MAKE_DWORD(szStream[x], szStream[x+1], szStream[x+2],szStream[x+3]);
		*dst++ = word_to_write;
	}
	// Send Write User Page command to the Flash controller
	__avr32_flash_writeRegister(AVR32_FLASHC_FCMD, (AVR32_FLASHC_FCMD_KEY_KEY << AVR32_FLASHC_FCMD_KEY_OFFSET) | AVR32_FLASHC_FCMD_CMD_WUP);
	__avr32_flash_waitForFlashReady();
}

//
// Read words from Flash user page
//
volatile void __avr32_flash_readUserData(char* szStream) {
	// Copy data from buffer page
	volatile uint32_t *src = (uint32_t *)AVR32_FLASHC_USER_PAGE_ADDRESS;
	volatile uint32_t read_word = 0;
	// Now read page-buffer
	for (int x = 0; x < 512; x += 4)
	{
		read_word = *src++;
		szStream[x]   = GET_BYTE_FROM_DWORD(read_word, 3) ;
		szStream[x+1] = GET_BYTE_FROM_DWORD(read_word, 2) ;
		szStream[x+2] = GET_BYTE_FROM_DWORD(read_word, 1) ;
		szStream[x+3] = 
		GET_BYTE_FROM_DWORD(read_word, 0) ;
	}
}

//
// We erase the user-page here
//
volatile void __avr32_flash_eraseUserData(void)
{
	// Send erase-user-page command to the Flash controller
	__avr32_flash_writeRegister(AVR32_FLASHC_FCMD, (AVR32_FLASHC_FCMD_KEY_KEY << AVR32_FLASHC_FCMD_KEY_OFFSET) | AVR32_FLASHC_CMD_EUP);
	__avr32_flash_waitForFlashReady();	
}

//
// Wait for Flash Controller Ready status
//
volatile void __avr32_flash_waitForFlashReady() {
	while (!__avr32_flash_readRegister(AVR32_FLASHC_FSR) & AVR32_FLASHC_FSR_FRDY_MASK);
}


// Main functions
void	__AVR32_Flash_Initialize(void)
{
	// Nothing special here
}

void	__AVR32_Flash_WriteUserPage(char* szData)
{
	// Erase the flash first...
	__avr32_flash_eraseUserData();
	
	// Write...
	__avr32_flash_writeUserData(szData);
}

void	__AVR32_Flash_ReadUserPage (char* szData)
{
	// Write...
	__avr32_flash_readUserData(szData);	
}
