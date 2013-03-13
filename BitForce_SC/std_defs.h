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
// #define __OPERATING_FREQUENCY_48MHz__
#define __OPERATING_FREQUENCY_64MHz__

/*************** Product Model *********************/
// #define __PRODUCT_MODEL_JALAPENO__
// #define __PRODUCT_MODEL_LITTLE_SINGLE__
#define __PRODUCT_MODEL_SINGLE__
// #define __PRODUCT_MODEL_MINIRIG__


/*************** XLINK Operations Timeout ***********/
#define __XLINK_WAIT_FOR_DEVICE_RESPONSE__   10000   // 10ms
#define __XLINK_TRANSACTION_TIMEOUT__	     20000   // 20ms. No transaction should take longer. 
#define __XLINK_WAIT_PACKET_TIMEOUT__        440
#define __XLINK_ATTEMPT_RETRY_MAXIMUM__      44

/*************** Firmware Version ******************/
#define __FIRMWARE_VERSION		"1.0.0"

/*************** UNIT ID STRING ********************/
#define  UNIT_ID_STRING			"BitForce SC 1.0"

// Unit Identification String
#define UNIT_FIRMWARE_ID_STRING	">>>>ID: BitFORCE SC SHA256 Version 1.0>>>>\n"
#define UNIT_FIRMWARE_REVISION	">>>>REVISION 1.0>>>>"
#define UNIT_FIRMWARE_TYPE		">>>>JALAPENO>>>>" // OR ">>>>MINIRIG>>>>" OR ">>>>SINGLE>>>>" OR ">>>>LITTLE-SINGLE>>>>"
#define UNIT_FIRMWARE_SPEED		">>>>32>>>>"

// We define our UL32 and Unsigned Long Long
typedef unsigned long long UL64;
typedef unsigned int UL32;


///////////////////////////////////////// typedefs
// Master Tick Counter (Holds clock in 1uS ticks)
volatile UL32 MAST_TICK_COUNTER;

void IncrementTickCounter(void);

// Other definitions
#define TRUE		1
#define FALSE		0

#define MAKE_DWORD(b3,b2,b1,b0) ((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0))
#define GET_BYTE_FROM_DWORD(dword, byte) ((dword >> (byte * 8)) & 0x0FF)

volatile char GLOBAL_InterProcChars[128];

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
	char i_nonce_count;  // Number of nonces in result
	unsigned int  nonce_list[8];
} buf_job_result_packet, *pbuf_job_result_packet;

// String Identification Memory
typedef struct _tag_string_storage
{
	char signature[3];   // 0x0AA, 0x0BB, 0x055
	char string_len;	 // Length of the string (including null termination)
	char string_to_store[256]; // The string to store
} string_storage, *pstring_storage;

// Other variables
int XLINK_ARE_WE_MASTER;
int global_vals[6];

// This should be by default zero
int GLOBAL_BLINK_REQUEST;

// Critical Temperature Warning - Abort Jobs!
volatile static char GLOBAL_CRITICAL_TEMPERATURE;

// This is the main bit mask
volatile unsigned int GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK;

// Used for ASIC handling
#define ASIC_SPI_RW_COMMAND						    0b01000000000000000

// Our Wathdog Reset command
#define WATCHDOG_RESET  AVR32_WDT.clr = 0x0FFFFFFFF

// Basic boolean definition
#define TRUE	1
#define FALSE	0

// Assembly NOP operation
#ifdef __OPERATING_FREQUENCY_64MHz__
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t");
#elif defined(__OPERATING_FREQUENCY_48MHz__)	
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#else
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#endif

#endif /* STD_DEFS_H_ */