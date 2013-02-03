/*
 * AVR32_OptimizedTemplates.c
 *
 * Created: 20/01/2013 01:40:20
 *  Author: NASSER
 */ 

#include "AVR32_OptimizedTemplates.h"
#include <avr32/io.h>
#include "std_defs.h"


/*
	#define AVR32_GPIO_ADDRESS  0xFFFF1000 <BASE> <PORT 0> = -61440
	#define AVR32_GPIO_ADDRESS  0xFFFF1100 <BASE> <PORT 1> = -61184
	
	PORT0  OVRC    ADDRESS  = 0xFFFF1058  OFFSET = 0x58
	PORT0  OVRS    ADDRESS  = 0xFFFF1054  OFFSET = 0x54
	PORT0  OVR     ADDRESS  = 0xFFFF1050  OFFSET = 0x50
	PORT0  ODERS   ADDRESS  = 0xFFFF1044  OFFSET = 0x44
	PORT0  ODERC   ADDRESS  = 0xFFFF1048  OFFSET = 0x48
	PORT0  ODER    ADDRESS  = 0xFFFF1040  OFFSET = 0x40
	PORT0  PVR	   ADDRESS  = 0xFFFF1060  OFFSET = 0x60
												   
	PORT1  OVRC    ADDRESS  = 0xFFFF1158  OFFSET = 0x58
	PORT1  OVRS    ADDRESS  = 0xFFFF1154  OFFSET = 0x54
	PORT1  OVR     ADDRESS  = 0xFFFF1150  OFFSET = 0x50
	PORT1  ODERS   ADDRESS  = 0xFFFF1144  OFFSET = 0x44
	PORT1  ODERC   ADDRESS  = 0xFFFF1148  OFFSET = 0x48
	PORT0  ODER    ADDRESS  = 0xFFFF1140  OFFSET = 0x40
	PORT1  PVR	   ADDRESS  = 0xFFFF1160  OFFSET = 0x60
	
	<PORT1> AVR32_CPLD_OE		= 0x200	 
	<PORT1> AVR32_CPLD_BUS_ALL	= 0xFF
	<PORT1> AVR32_CPLD_ADRS		= 0x100
	<PORT0> AVR32_CPLD_STROBE	= 0x20000000 = 
	<PORT1> AVR32_CPLD_ADRS_INC	= 0x400 (PORT1 (a.k.a. PORTB)
	<PORT0> AVR32_CPLD_CS		= 0x4000000  
	
*/


inline void OPTIMIZED__AVR32_CPLD_Write(unsigned char iAdrs, unsigned char iData)
{
	// iAdrs is mapped to r12, iData is mapped to r11
	// Declare our result character
	asm volatile ("pushm r0-r3" "\n\t");
	
	// R0 equals PORT0 BASE ADDRESS
	// R1 equals PORT1 BASE ADDRESS

	asm volatile (
		// Save base addresses in r0 and r1 (we'll be using these a lot)
		"mov  r0, -61440"		"\n\t"
		"mov  r1, -61184"		"\n\t"
		
		// PORT1 CPLD_OE Cleared (port1.ovrc = CPLD_OE)
		"mov  r2, 0x200"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
		
		// Port1.ODERS = CPLD_BUS_ALL
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x44], r2"		"\n\t"
		
		// Port1.ovrs = CPLD_ADRS
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"
		
		// Port1.ovr = Address <Note: This is a byte transfer> <Note: r12 is the iAdrs>
		"ld.w r3, r1[0x50]"		"\n\t" // Load initial value
		"andh r3, 0x0FFFF"		"\n\t"
		"andl r3, 0x0FF00"		"\n\t"
		"andh r12, 0x00"		"\n\t"
		"andl r12, 0xFF"		"\n\t"
		"or   r3, r12"			"\n\t" // Set the correct value here
		"st.w r1[0x50], r3"		"\n\t"	
		
		// Perform Strobe
		"mov  r2, 0x0000"		"\n\t"
		"movh r2, 0x2000"		"\n\t"
		"st.w r0[0x54], r2"		"\n\t"
		"st.w r0[0x58], r2"		"\n\t"

		// Port1.ovrc = CPLD_ADRS	
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
		
		// Port1.ovr = Data <Note: This is a byte transfer> <Note: r11 is the iData>
		"ld.w r3, r1[0x50]"		"\n\t" // Load initial value
		"andh r3, 0x0FFFF"		"\n\t"
		"andl r3, 0x0FF00"		"\n\t"
		"or   r3, r11"			"\n\t" // Set the correct value here		
		"st.w r1[0x50], r3"		"\n\t"
		
		// Perform Strobe
		"mov  r2, 0x0000"		"\n\t"
		"movh r2, 0x2000"		"\n\t"
		"st.w r0[0x54], r2"		"\n\t"
		"st.w r0[0x58], r2"		"\n\t"
		
		// Port1.ODERC = CPLD_BUS_ALL
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x48], r2"		"\n\t"	
	);
	
	// Pop all registers
	asm volatile ("popm	r0-r3" "\n\t");
	
}

#define DUMMY_REG (*((volatile unsigned int*)0xFFFF1150)) 

inline void OPTIMIZED__AVR32_CPLD_Read (unsigned char iAdrs, unsigned char *retVal)
{
	// iAdrs is mapped to r12, retVal is mapped to r11
	// Declare our result character	
	asm volatile ("pushm r0-r3" "\n\t");
	
	// R0 equals PORT0 BASE ADDRESS
	// R1 equals PORT1 BASE ADDRESS

	asm volatile (
		// Save base addresses in r0 and r1 (we'll be using these a lot)
		"mov   r0, -61440"		"\n\t"
		"mov   r1, -61184"		"\n\t"
	
		// PORT1 CPLD_OE Cleared (port1.ovrc = CPLD_OE)
		"mov   r2, 0x200"		"\n\t"
		"st.w  r1[0x58], r2"	"\n\t"
	
		// Port1.ODERS = CPLD_BUS_ALL
		"mov   r2, 0xFF"		"\n\t"
		"st.w  r1[0x44], r2"	"\n\t"
	
		// Port1.ovrs = CPLD_ADRS
		"mov   r2, 0x100"		"\n\t"
		"st.w  r1[0x54], r2"	"\n\t"
	
		// Port1.ovr = Address <Note: This is a byte transfer> <Note: r12 is the iAdrs>
		"ld.w  r3, r1[0x50]"	"\n\t" // Load initial value
		"andh  r3, 0x0FFFF"		"\n\t"
		"andl  r3, 0x0FF00"		"\n\t"
		"or    r3, r12"			"\n\t" // Set the correct value here		
		"st.w  r1[0x50], r3"	"\n\t"
	
		// Perform Strobe
		"mov   r2, 0x0"			"\n\t"
		"movh  r2, 0x2000"		"\n\t"
		"st.w  r0[0x54], r2"	"\n\t"
		"st.w  r0[0x58], r2"	"\n\t"
	
		// Port1.ovrc = CPLD_ADRS
		"mov   r2, 0x100"		"\n\t"
		"st.w  r1[0x58], r2"	"\n\t"
	
		// Port1.ODERC = CPLD_BUS_ALL < Clear ODER so we can read ports value >
		"mov   r2, 0xFF"		"\n\t"
		"st.w  r1[0x48], r2"	"\n\t"
		
		// Port1.ovrs = CPLD_OE <Enable the OE>
		"mov   r2, 0x200"		"\n\t"
		"st.w  r1[0x54], r2"	"\n\t"		
		
		// Wait a few clock cycles
		"nop"					"\n\t"	
		"nop"					"\n\t"	
		"nop"					"\n\t"	
		"nop"					"\n\t"	
		
		// Read the PVR Byte Value
		"ld.w  r3, r1[0x60]"	"\n\t"	
		"andh  r3, 0x00000"		"\n\t"
		"andl  r3, 0x000FF"		"\n\t"		
		"st.b  r11[0], r3"		"\n\t"	
		
		// PORT1 CPLD_OE Cleared (port1.ovrc = CPLD_OE)
		"mov   r2, 0x200"		"\n\t"
		"st.w  r1[0x58], r2"	"\n\t"		
		
		// Port1.ODERC = CPLD_BUS_ALL
		"mov   r2, 0xFF"		"\n\t"
		"st.w  r1[0x48], r2"	"\n\t"		
	);
	
	// Pop all registers
	asm volatile ("popm	r0-r3" "\n\t");

}

inline void OPTIMIZED__AVR32_CPLD_BurstTxWrite(char* szData, char iAddress)
{
	// In this case, szData is r12, iAddress is r11

	// Declare our result character
	asm volatile ("pushm r0-r3, r4-r7" "\n\t");

	// R0 equals PORT0 BASE ADDRESS
	// R1 equals PORT1 BASE ADDRESS
	asm volatile (

		// Save base addresses in r0 and r1 (we'll be using these a lot)
		"mov  r0, -61440"		"\n\t"
		"mov  r1, -61184"		"\n\t"

		// PORT1 CPLD_OE Cleared (port1.ovrc = CPLD_OE)
		"mov  r2, 0x200"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"

		// Port1.ODERS = CPLD_BUS_ALL
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x44], r2"		"\n\t"

		// Port1.ovrs = CPLD_ADRS
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"

		// Port1.ovr = Address <Note: This is a byte transfer> <Note: r12 is the iAdrs>
		"ld.w  r3, r1[0x50]"	"\n\t" // Load initial value		
		"andh  r3, 0x0FFFF"		"\n\t"
		"andl  r3, 0x0FF00"		"\n\t"
		"or    r3, r11"			"\n\t" // Set the correct value here		
		"st.w r1[0x50], r3"		"\n\t"

		// Perform Strobe
		"mov  r2, 0x0000"		"\n\t"
		"movh r2, 0x2000"		"\n\t"
		"st.w r0[0x54], r2"		"\n\t"
		"st.w r0[0x58], r2"		"\n\t"

		// Port1.ovrc = CPLD_ADRS
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
		
		// Port1.ovrs = CPLD_ADDRESS_INCREMENT
		"mov  r2, 0x400"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"
				
		// Port1.ovr = Data <Note: Read 4 times with address increment>
		"mov   r2, 0x0000"		"\n\t"
		"movh  r2, 0x2000"		"\n\t"
		
			// We store the original value of the register
			"ld.w  r4, r1[0x50]"	"\n\t" // Load initial value
			"andh  r4, 0x0FFFF"		"\n\t"
			"andl  r4, 0x0FF00"		"\n\t"
			"mov   r5, r4"			"\n\t" // Keep a copy in r5
						
			// ------------ Load data from r12 and increment it
			"ld.ub r3, r12++"		"\n\t"
			"or    r4, r3"			"\n\t" // Set the correct value here			
			"st.w  r1[0x50], r4"	"\n\t"
			// Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"st.w r0[0x58], r2"		"\n\t"
						
			// ------------ Load data from r12 and increment it
			"mov   r4, r5"			"\n\t"
			"ld.ub r3, r12++"		"\n\t"
			"or    r4, r3"			"\n\t" // Set the correct value here			
			"st.w  r1[0x50], r4"	"\n\t"
			// Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"st.w r0[0x58], r2"		"\n\t"			
				
			// ------------ Load data from r12 and increment it
			"mov   r4, r5"			"\n\t"			
			"ld.ub r3, r12++"		"\n\t"
			"or    r4, r3"			"\n\t"
			"st.w  r1[0x50], r4"	"\n\t"
			// Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"st.w r0[0x58], r2"		"\n\t"
						
			// ------------ Load data from r12 and increment it
			"mov   r4, r5"			"\n\t"			
			"ld.ub r3, r12"			"\n\t"
			"or    r4, r3"			"\n\t"			
			"st.w  r1[0x50], r4"	"\n\t"
			// Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"st.w r0[0x58], r2"		"\n\t"		
		
		// Port1.ovrc = CPLD_ADDRESS_DECREMENT
		"mov  r2, 0x400"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"		

		// Port1.ODERC = CPLD_BUS_ALL
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x48], r2"		"\n\t"
	);

	// Pop all registers
	asm volatile ("popm	r0-r3, r4-r7" "\n\t");	
}

inline void OPTIMIZED__AVR32_CPLD_BurstRxRead(char* szData, char iAddress)
{
	
	/*OPTIMIZED__AVR32_CPLD_Read(iAddress, &szData[0]);
	OPTIMIZED__AVR32_CPLD_Read(iAddress+1, &szData[1]);
	OPTIMIZED__AVR32_CPLD_Read(iAddress+2, &szData[2]);
	OPTIMIZED__AVR32_CPLD_Read(iAddress+3, &szData[3]);*/
	
	// In this case, szData is r12, iAddress is r11 
	asm volatile ("pushm r0-r3, r4-r7" "\n\t");
	
	// R0 equals PORT0 BASE ADDRESS
	// R1 equals PORT1 BASE ADDRESS

	asm volatile (		
		// Save base addresses in r0 and r1 (we'll be using these a lot)
		"mov  r0, -61440"		"\n\t"
		"mov  r1, -61184"		"\n\t"
		
		// PORT1 CPLD_OE Cleared (port1.ovrc = CPLD_OE)
		"mov  r2, 0x200"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
		
		// Port1.oders = CPLD_BUS_ALL
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x44], r2"		"\n\t"
		
		// Port1.ovrs = CPLD_ADRS
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"
		
		// Port1.ovr = Address <Note: This is a byte transfer> <Note: r12 is the iAdrs>
		"ld.w  r3, r1[0x50]"	"\n\t" // Load initial value
		"andh  r3, 0x0FFFF"		"\n\t"
		"andl  r3, 0x0FF00"		"\n\t"
		"or    r3, r11"			"\n\t" // Set the correct value here
		"st.w r1[0x50], r3"		"\n\t"
		
		// Perform Strobe
		"mov  r2, 0x00000000"	"\n\t"
		"movh r2, 0x2000"		"\n\t"
		"st.w r0[0x54], r2"		"\n\t"
		"nop"					"\n\t"
		"nop"					"\n\t"			
		"st.w r0[0x58], r2"		"\n\t"
		
		// Port1.ovrc = CPLD_ADRS
		"mov  r2, 0x100"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
		
		// Port1.ODERC = CPLD_BUS_ALL < Clear ODER so we can read ports value >
		"mov  r2, 0xFF"			"\n\t"
		"st.w r1[0x48], r2"		"\n\t"
		
		// Port1.ovrs = CPLD_OE <Enable the OE>
		"mov  r2, 0x200"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"
		
		// Enable the CPLD-Address-Increment
		// Port1.ovrs = CPLD_ADDRESS_INCREMENT
		"mov  r2, 0x400"		"\n\t"
		"st.w r1[0x54], r2"		"\n\t"		
		
		// Perform Quad Reads
		"mov  r2, 0x0000"		"\n\t"
		"movh r2, 0x2000"		"\n\t"
		"nop"					"\n\t"
		"nop"					"\n\t"
				
			// ------------- Read the PVR Byte Value
			"ld.w  r3, r1[0x60]"	"\n\t"
			"st.b  r12[0], r3"		"\n\t"		
			// >> Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"			
			"st.w r0[0x58], r2"		"\n\t"
			
			// ------------- Read the PVR Byte Value
			"ld.w  r3, r1[0x60]"	"\n\t"
			"st.b  r12[1], r3"		"\n\t"		
			// >> Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"			
			"st.w r0[0x58], r2"		"\n\t"					


			// ------------- Read the PVR Byte Value
			"ld.w  r3, r1[0x60]"	"\n\t"
			"st.b  r12[2], r3"		"\n\t"		
			// >> Perform Strobe
			"st.w r0[0x54], r2"		"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"
			"nop"					"\n\t"			
			"st.w r0[0x58], r2"		"\n\t"	
	
			
			// ------------- Read the PVR Byte Value
			"ld.w  r3, r1[0x60]"	"\n\t"
			"st.b  r12[3], r3"		"\n\t"
			
		"nop"					"\n\t"
		"nop"					"\n\t"			
				
		// Port1.ovrc = CPLD_OE (OE Cleared)
		"mov  r2, 0x200"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"
				
		// Disable the CPLD-Address-Increment		
		// Port1.ovrc = CPLD_ADDRESS_DECREMENT
		"mov  r2, 0x400"		"\n\t"
		"st.w r1[0x58], r2"		"\n\t"		
		
		// Port1.ODERC = CPLD_BUS_ALL < NOT NEEDED, It's already in this state >
		// "mov  r2, 0xFF"			"\n\t"
		// "st.w r1[0x48], r2"		"\n\t"
		
	
	);
		
	// Pop all registers
	asm volatile ("popm	r0-r3, r4-r7" "\n\t");
}

