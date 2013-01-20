/*
 * AVR32_OptimizedTemplates.c
 *
 * Created: 20/01/2013 01:40:20
 *  Author: NASSER
 */ 

#include "AVR32_OptimizedTemplates.h"
#include <avr32/io.h>
#include "std_defs.h"


void OPTIMIZED__AVR32_CPLD_Write(unsigned char iAdrs, unsigned char iData)
{
	asm volatile ("pushm r0-r12" "\n");
	asm volatile ("popm r0-r12" "\n");
	volatile register unsigned char iAdrs_d asm("r0") = iAdrs;
}