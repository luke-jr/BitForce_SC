/*
 * std_defs.c
 *
 * Created: 21/11/2012 20:56:46
 *  Author: NASSER
 */ 

#include "std_defs.h"
#include "Generic_Module.h"


long	GetTickCount(void)
{
	#if defined(__OPERATING_FREQUENCY_32MHz__)
		return ((MAST_TICK_COUNTER << 16) | (MCU_Timer_GetValue() & 0x0FFFF));
	#elif defined(__OPERATING_FREQUENCY_48MHz__)
		return ((((MAST_TICK_COUNTER << 16) | (MCU_Timer_GetValue() & 0x0FFFF)) * 6) / 10);
	#elif defined(__OPERATING_FREQUENCY_64MHz__)
		return ((MAST_TICK_COUNTER << 16) | (MCU_Timer_GetValue() & 0x0FFFF)) >> 1; // Divide by 2 for 64MHz
	#elif defined(__OPERATING_FREQUENCY_16MHz__)
		MAST_TICK_COUNTER += 40;
	#else
		return 0;
	#endif

}

void	IncrementTickCounter(void)
{
	MAST_TICK_COUNTER++;
}