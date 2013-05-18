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
//#define __OPERATING_FREQUENCY_16MHz__
//#define __OPERATING_FREQUENCY_32MHz__ 
//#define __OPERATING_FREQUENCY_48MHz__
#define __OPERATING_FREQUENCY_64MHz__

/*************** Product Model *********************/
// #define __PRODUCT_MODEL_JALAPENO__
#define    __PRODUCT_MODEL_LITTLE_SINGLE__
// #define __PRODUCT_MODEL_SINGLE__
// #define __PRODUCT_MODEL_MINIRIG__

/*************** Features modifying the code behaviour *****************/
// Some useful macros

/////////////////////////////////////////////////////////////////////////
// This means DO NOT USE ENGINE 0. It's needed for the actual Version
#define DO_NOT_USE_ENGINE_ZERO				1

/////////////////////////////////////////////////////////////////////////
// -- DO NOT SET THIS MACRO
// -- If set, basically all the chips will go to IDLE state
//#define DISABLE_CLOCK_TO_ALL_ENGINES		1

/////////////////////////////////////////////////////////////////////////
// -- This macro forces the Queue Operator to send one job to each chip.
// -- Once activated, device will process parallel jobs at nearly the same speed.
// -- This also impacts the ZOX response, where the processor that got the job done will be added to the response list
//#define QUEUE_OPERATE_ONE_JOB_PER_CHIP	1

/////////////////////////////////////////////////////////////////////////
// -- This macro enforces the queue to use one engine per board ( verses one engine per chip )
// -- It will also modify the results queue and no processing chip identifier will exist any longer
#define  QUEUE_OPERATE_ONE_JOB_PER_BOARD		1

/********* TOTAL CHIPS INSTALLED ON BOARD **********/
/////////////////////////////////////////////////////////////////////////
#define	 TOTAL_CHIPS_INSTALLED	8

/********* Pulse Reuqest *************/
/////////////////////////////////////////////////////////////////////////
// Pulse the main LED as long as we're processing jobs...
#define  ENABLE_JOB_PULSING_SYSTEM	1

/***** Chip diagnostics verbose ******/
/////////////////////////////////////////////////////////////////////////
// -- This option will provide detailed information when PROTOCOL_REQ_INFO_REQ
// -- regarding chips behavior
// #define	__CHIP_DIAGNOSTICS_VERBOSE		1
//
// -- Also this option enables chip by chip diagnostics
// #define __CHIP_BY_CHIP_DIAGNOSTICS		1
// #define __ENGINE_BY_ENGINE_DIAGNOSTICS	1
// #define __EXPORT_ENGINE_RANGE_SPREADS	1

#if defined(__EXPORT_ENGINE_RANGE_SPREADS)
	volatile unsigned int __ENGINE_LOWRANGE_SPREADS[TOTAL_CHIPS_INSTALLED][16];
	volatile unsigned int __ENGINE_HIGHRANGE_SPREADS[TOTAL_CHIPS_INSTALLED][16];
#endif

/////////////////////////////////////////////////////////////////////////
// Detect frequency of each chip
#define __CHIP_FREQUENCY_DETECT_AND_REPORT 1 
#define __LIVE_FREQ_DETECTION 1				 // If set, it'll force the report to be a live detection instead of initial results found

/////////////////////////////////////////////////////////////////////////
// This MACRO disables the kernel from trying to increase frequency on ASICS
// on startup if their actual frequency is less than what it should be...
#define __DO_NOT_TUNE_CHIPS_FREQUENCY 1

/////////////////////////////////////////////////////////////////////////
// Interleaved job loading enabled?
// #define __INTERLEAVED_JOB_LOADING

/////////////////////////////////////////////////////////////////////////
// Enabling this macro will force the results buffer to be cleared when 
// QUEUE_FLUSH Command is issued
// #define FLUSH_CLEAR_RESULTS_BUFFER			1

/////////////////////////////////////////////////////////////////////////
// FAN SUBSYSTEM: FAN REMAIN AT FULL SPEED
#define FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED		1

/////////////////////////////////////////////////////////////////////////
// ASIC Frequency settings
// ACTUAL VALUES WOULD BE :: const unsigned int __ASIC_FREQUENCY_WORDS [10] = {0x0, 0xFFFF, 0xFFFD, 0xFFF5, 0xFFD5, 0xFF55, 0xFD55, 0xF555, 0xD555, 0x5555};
extern const unsigned int __ASIC_FREQUENCY_WORDS[10];  // Values here are known...
extern const unsigned int __ASIC_FREQUENCY_VALUES[10]; // We have to measure frequency for each word...
#define __ASIC_FREQUENCY_ACTUAL_INDEX   0 // 
#define __MAXIMUM_FREQUENCY_INDEX       9


/////////////////////////////////////////////////////////////////////////
// If this is defined, the chip will no longer be used
// #define DECOMISSION_CHIP_15				1
// #define DECOMISSION_CHIP_14				1
// #define DECOMISSION_CHIP_13				1
// #define DECOMISSION_CHIP_12				1
// #define DECOMISSION_CHIP_11				1
// #define DECOMISSION_CHIP_10				1
// #define DECOMISSION_CHIP_9				1
// #define DECOMISSION_CHIP_8				1
// #define DECOMISSION_CHIP_7				1
// #define DECOMISSION_CHIP_6				1
// #define DECOMISSION_CHIP_5				1
// #define DECOMISSION_CHIP_4				1
// #define DECOMISSION_CHIP_3				1
// #define DECOMISSION_CHIP_2				1
// #define DECOMISSION_CHIP_1				1
// #define DECOMISSION_CHIP_0				1


/*************** Used for debugging ****************/
#define CHIP_TO_TEST 0

/*************** XLINK Operations Timeout ***********/
#define __XLINK_WAIT_FOR_DEVICE_RESPONSE__   10000   // 10ms
#define __XLINK_TRANSACTION_TIMEOUT__	     20000   // 20ms. No transaction should take longer. 
#define __XLINK_WAIT_PACKET_TIMEOUT__        440
#define __XLINK_ATTEMPT_RETRY_MAXIMUM__      44

/*************** Firmware Version ******************/
#define __FIRMWARE_VERSION		"1.0.0"

/*************** UNIT ID STRING ********************/
#define  UNIT_ID_STRING			"BitForce SHA256 SC 1.0\n"

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

volatile unsigned short GLOBAL_ChipActivityLEDCounter[TOTAL_CHIPS_INSTALLED];

// Some global definitions
typedef struct _tag_job_packet
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	char signature;	// 1 byte, it's 0xAA
} job_packet, *pjob_packet;

typedef struct _tag_job_packet_p2p
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	unsigned int  nonce_begin; // 4 Bytes of Nonce-Begin (Little-Endian)
	unsigned int  nonce_end; // 4 Bytes of Nonce-End (Little-Endian)
	char signature; // 1 byte, it's 0xAA
} job_packet_p2p, *pjob_packet_p2p;

typedef struct _tag_buf_job_result_packet
{
	#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
		char iProcessingChip; // The chip that processed the job
	#endif
	
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

// Contains information about frequency of all the chips installed
extern unsigned int GLOBAL_CHIP_FREQUENCY_INFO[TOTAL_CHIPS_INSTALLED];

// This should be by default zero
int GLOBAL_BLINK_REQUEST;

// Global Pulse-Request section
int  GLOBAL_PULSE_BLINK_REQUEST;
void System_Request_Pulse_Blink(void);

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
#define CHIP_EXISTS(x)					 ((__chip_existence_map[x] != 0))
#define IS_PROCESSOR_OK(xchip, yengine)  ((__chip_existence_map[xchip] & (1 << yengine)) != 0)

unsigned int __chip_existence_map[TOTAL_CHIPS_INSTALLED]; // Bit 0 to Bit 16 in each word says if the engine is OK or not...
	
// Our sleep function
volatile void Sleep(unsigned int iSleepPeriod);

// Assembly NOP operation
#ifdef __OPERATING_FREQUENCY_64MHz__
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t");
#elif defined(__OPERATING_FREQUENCY_48MHz__)	
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#else
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#endif

#endif /* STD_DEFS_H_ */