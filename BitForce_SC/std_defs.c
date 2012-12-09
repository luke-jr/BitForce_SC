/*
 * std_defs.c
 *
 * Created: 21/11/2012 20:56:46
 *  Author: NASSER
 */ 

#include "std_defs.h"

unsigned long	GetTickCount(void)
{
	return MAST_TICK_COUNTER;
}

void	   IncrementTickCounter(void)
{
	MAST_TICK_COUNTER++;
}