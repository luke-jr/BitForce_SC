/*
 * std_defs.h
 *
 * Created: 09/10/2012 01:14:49
 *  Author: NASSER
 */ 

#ifndef STD_DEFS_H_
#define STD_DEFS_H_

/*************** MCU Platform Selection ******************/
#define __COMPILING_FOR_AVR32__
// #define __COMPILING_FOR_STM32__
// #define __COMPILING_FOR_PIC32__

/*************** Operating Frequency ******************/
// #define __OPERATING_FREQUENCY_16MHz__
// #define __OPERATING_FREQUENCY_32MHz__ 
#define __OPERATING_FREQUENCY_48MHz__
//#define __OPERATING_FREQUENCY_64MHz__

/*************** Product Model *********************/
// #define __PRODUCT_MODEL_JALAPENO__
// #define __PRODUCT_MODEL_LITTLE_SINGLE__
#define __PRODUCT_MODEL_SINGLE__
// #define __PRODUCT_MODEL_MINIRIG__


/*************** Firmware Version ******************/
#define __FIRMWARE_VERSION	"1.0.0"

/*************** UNIT ID STRING ********************/
#define  UNIT_ID_STRING "BitForce SC 1.0"

///////////////////////////////////////// typedefs
// Master Tick Counter (Holds clock in 1uS ticks)
unsigned long	MAST_TICK_COUNTER;

unsigned long	GetTickCount(void);
void IncrementTickCounter(void);

// Other definitions
#define TRUE		1
#define FALSE		0

#define MAKE_DWORD(u,v,x,y) ((u << 24) + (v << 16) + (x << 8) + (y))
#define GET_MSB_BYTE(u, v)  ((((unsigned int)(u)) >> ((3 - v) * 8)) & 0x0FF)


// Some global definitions
typedef struct _tag_job_packet
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
} job_packet, *pjob_packet;

typedef struct _tag_job_packet_p2p
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	unsigned int  nonce_begin; // 4 Bytes of Nonce-Begin (Little-Endian)
	unsigned int  nonce_end; // 4 Bytes of Nonce-End (Little-Endian)
} job_packet_p2p, *pjob_packet_p2p;

typedef struct _tag_buf_job_result_packet
{
	char midstate[32];   // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	char nonce_begin[4]; // 4 Bytes of NoncDynaclock_GetFrequencyFactore-Begin (Little-Endian)
	char nonce_end[4];   // 4 Bytes of Nonce-End (Little-Endian)
	char i_nonce_count;  // Number of nonces in result
	unsigned int  nonce_list[16];
} buf_job_result_packet, *pbuf_job_result_packet;

int XLINK_ARE_WE_MASTER;

// Used for ASIC handling
#define ASIC_SPI_RW_COMMAND						    0b01000000000000000

// Basic boolean definition
#define TRUE	1
#define FALSE	0

#endif /* STD_DEFS_H_ */