/*
 * std_defs.h
 *
 * Created: 09/10/2012 01:14:49
 *  Author: NASSER GHOSEIRI
 *
 * WARNING: Supported optimization is -O3
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
//#define    __PRODUCT_MODEL_LITTLE_SINGLE__
#define __PRODUCT_MODEL_SINGLE__
//#define __PRODUCT_MODEL_MINIRIG__

/********* TOTAL CHIPS INSTALLED ON BOARD **********/
/////////////////////////////////////////////////////////////////////////

#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
	#define	 TOTAL_CHIPS_INSTALLED	8
#else
	#define	 TOTAL_CHIPS_INSTALLED	16
#endif

/*************** Features modifying the code behaviour *****************/
// Some useful macros

/////////////////////////////////////////////////////////////////////////
// This means DO NOT USE ENGINE 0. It's needed for the actual Version
#define DO_NOT_USE_ENGINE_ZERO				1

/////////////////////////////////////////////////////////////////////////
// -- DO NOT SET THIS MACRO
// -- If set, basically all the chips will go to IDLE state
//#define DISABLE_CLOCK_TO_ALL_ENGINES		1

/********* Pulse Reuqest *************/
/////////////////////////////////////////////////////////////////////////
// Pulse the main LED as long as we're processing jobs...
#define  ENABLE_JOB_PULSING_SYSTEM	1

/***** Chip diagnostics verbose ******/
/////////////////////////////////////////////////////////////////////////
// -- This option will provide detailed information when PROTOCOL_REQ_INFO_REQ
// -- regarding chips behavior
//#define __CHIP_DIAGNOSTICS_VERBOSE		

// -- Also this option enables chip by chip diagnostics
//#define __CHIP_BY_CHIP_DIAGNOSTICS		
//#define __ENGINE_BY_ENGINE_DIAGNOSTICS	
//#define __EXPORT_ENGINE_RANGE_SPREADS	

//-- Reports the busy engines on InfoRequest command
//#define __REPORT_BUSY_ENGINES				
//#define __SHOW_DECOMMISSIONED_ENGINES_LOG	
//#define __SHOW_PIPE_TO_USB_LOG			

#if defined(__EXPORT_ENGINE_RANGE_SPREADS)
	volatile unsigned int __ENGINE_LOWRANGE_SPREADS[TOTAL_CHIPS_INSTALLED][16];
	volatile unsigned int __ENGINE_HIGHRANGE_SPREADS[TOTAL_CHIPS_INSTALLED][16];
#endif

#if defined(__SHOW_DECOMMISSIONED_ENGINES_LOG)
	volatile char szDecommLog[4096];
#endif

/////////////////////////////////////////////////////////////////////////
// Detect frequency of each chip
#define __CHIP_FREQUENCY_DETECT_AND_REPORT 1     // Roundtrip
#define __LIVE_FREQ_DETECTION			   1	 // If set, it'll force the report to be a live detection instead of initial results found
//#define __EXPORT_ENGINE_TIMEOUT_DETAIL	   1	 // ASIC_tune_chip_frequency log will be exported
//#define __PERFORM_CHIP_BY_CHIP_TEST		   1	 // Send job to each chip, get the result and measure speed

/////////////////////////////////////////////////////////////////////////
// This MACRO disables the kernel from trying to increase frequency on ASICS
// on startup if their actual frequency is less than what it should be...
// #define __DO_NOT_TUNE_CHIPS_FREQUENCY 1

/////////////////////////////////////////////////////////////////////////
// ASIC Frequency settings
// ACTUAL VALUES WOULD BE -- const unsigned int __ASIC_FREQUENCY_WORDS [10] = {0x0, 0xFFFF, 0xFFFD, 0xFFF5, 0xFFD5, 0xFF55, 0xFD55, 0xF555, 0xD555, 0x5555};
extern const unsigned int __ASIC_FREQUENCY_WORDS[10];  // Values here are known...
extern const unsigned int __ASIC_FREQUENCY_VALUES[10]; // We have to measure frequency for each word...

#if defined(__PRODUCT_MODEL_JALAPENO)
	#define __ASIC_FREQUENCY_ACTUAL_INDEX   1 // 180MHz for Jalapeno
#else	
	#define __ASIC_FREQUENCY_ACTUAL_INDEX   7 
#endif

#define __MAXIMUM_FREQUENCY_INDEX       9

/////////////////////////////////////////////////////////////////////////
// Engine activity supervision
// Enabling this feature will force the MCU to decommission the malfunctioning
// engines detected on runtime
#define __ENGINE_ENABLE_TIMESTAMPING				  // Enable timestamping, meaning we mark the job initiation and termination
#define __ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION	  // For one job per chip, you need this...
//#define __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION   1 // The same as Activity-Supervision, except that it's used for progressive engine job loading system
//#define TOTAL_FAILURES_BEFORE_DECOMMISSIONING       300 // Meaning after 300 consequtive failures, decommission the engine

// Used with __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION
// #define ENABLED_SINGLE_JOB_ISSUE_MONITORING			// ASIC_job_issue sets time of its execution. If this time has taken more than a second, and it's still processing, then we restart the chips

// #define DECOMMISSION_ENGINES_IF_LATE	// If set, engines will be decommissioned if they are too late, otherwise they'll be simply reset...

////////////////////////////////////////////
// Activate job-load balancing
#define __ACTIVATE_JOB_LOAD_BALANCING	1

////////////////////////////////////////////
// Report the mining speed by testing it
//#define __REPORT_TEST_MINING_SPEED		1


/////////////////////////////////////////////////////////////////////////
// Pipe Operation Testing...
// If set, the PROTOCOL_GET_INTO_REQUEST will submit 3 jobs in the pipe and
// measures how long it takes for the unit to finish it...
// #define __TEST_PIPE_PERFORMANCE 

/*********************************** DIAGNOSTICS *********************************/
// Full Nonce Range diagnostics ( Runs each engine with 8 nonce job, 
// and verifies the results

//// BEGIN
#define __RUN_HEAVY_DIAGNOSTICS_ON_EACH_ENGINE	1
//#define __HEAVY_DIAGNOSTICS_STRICT_8_NONCES	1
//#define __HEAVY_DIAGNOSTICS_MODERATE_7_NONCES	1
#define __HEAVY_DIAGNOSTICS_MODERATE_3_NONCES	1
//// END

//// BEGIN
//#define __RUN_SCATTERED_DIAGNOSTICS 		// Sends 45 different jobs and checks if all nonces were detected.
#if defined(__PRODUCT_MODEL_MINIRIG__) || defined(__PRODUCT_MODEL_SINGLE__)
	#define __TOTAL_SCATTERED_JOBS_TO_TEST 45
#elif defined(__PRODUCT_MODEL_LITTLE_SINGLE__)
	#define __TOTAL_SCATTERED_JOBS_TO_TEST 13
#else // Jalapeno model
	#define __TOTAL_SCATTERED_JOBS_TO_TEST 5
#endif
//// END

/////////////////////////////////////////////////////////////////////////
// Total of runs we perform in diagnostics (for Engine busy-fault error)
#define ASIC_DIAGNOSE_ENGINE_REMAINING_BUSY 1
#define __TOTAL_DIAGNOSTICS_RUN  10

/////////////////////////////////////////////////////////////////////////
// Report balancing 
// #define __REPORT_BALANCING_SPREADS	1

/******************************** BUFFER MANAGEMENT **************************/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// -- This macro forces the Queue Operator to send one job to each chip.
// -- Once activated, device will process parallel jobs at nearly the same speed.
// -- This also impacts the ZOX response, where the processor that got the job done will be added to the response list
#define QUEUE_OPERATE_ONE_JOB_PER_CHIP	 

  
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// -- This macro enforces the queue to use one engine per board ( verses one engine per chip )
// -- It will also modify the results queue and no processing chip identifier will exist any longer
// #define  QUEUE_OPERATE_ONE_JOB_PER_BOARD		1
	
	////////////////////////////////////////////////////////////////////////
	// Progressive per-engine job submission
	//#define __PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION	1

	// OR Interleaved job loading enabled? (With the option of interleaved job loading (i.e. Pipelined))
	//#define __IMMEDIATE_ENGINE_JOB_SUBMISSION
	//#define __INTERLEAVED_JOB_LOADING

/***************************************************************************/

/////////////////////////////////////////////////////////////////////////
// General Engine Reseting - if too many engines have failed...
// We'll reset the ASICs. It must have been a while since we last produced a result...
#define GENERAL_ASIC_RESET_ON_LOW_ENGINE_COUNT	1

/////////////////////////////////////////////////////////////////////////
// Enabling this macro will force the results buffer to be cleared when 
// QUEUE_FLUSH Command is issued
// #define FLUSH_CLEAR_RESULTS_BUFFER			1

/////////////////////////////////////////////////////////////////////////
// FAN SUBSYSTEM: FAN REMAIN AT FULL SPEED
// #define FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED	1

/*
#if defined(__PRODUCT_MODEL_LITTLE_SINGLE__) || defined(__PRODUCT_MODEL_JALAPENO__)
	#if !defined(FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED)
		#error For this model of product, FAN subsystem is best remained at high-speed
	#endif
#endif
*/

///////////////////////////////////////////////////////////////////////// 
// Reinitialize the ASICs after high-temp recovery
#define __ASICS_RESTART_AFTER_HIGH_TEMP_RECOVERY 1

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
// #define ENFORCE_USAGE_CHIP_15			1
// #define ENFORCE_USAGE_CHIP_14			1
// #define ENFORCE_USAGE_CHIP_13			1
// #define ENFORCE_USAGE_CHIP_12			1
// #define ENFORCE_USAGE_CHIP_11			1
// #define ENFORCE_USAGE_CHIP_10			1
// #define ENFORCE_USAGE_CHIP_9				1
// #define ENFORCE_USAGE_CHIP_8				1
// #define ENFORCE_USAGE_CHIP_7				1
// #define ENFORCE_USAGE_CHIP_6				1
// #define ENFORCE_USAGE_CHIP_5				1
// #define ENFORCE_USAGE_CHIP_4				1
// #define ENFORCE_USAGE_CHIP_3				1
// #define ENFORCE_USAGE_CHIP_2				1
// #define ENFORCE_USAGE_CHIP_1				1
// #define ENFORCE_USAGE_CHIP_0				1


/////////////////////////////////////////////////////////////////////////
// Error detection
#if defined(ENABLED_SINGLE_JOB_ISSUE_MONITORING) && !defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
	#error The feature ENABLED_SINGLE_JOB_ISSUE_MONITORING can only be used with __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION
#endif

#if (defined(__PRODUCT_MODEL_SINGLE__) || defined(__PRODUCT_MODEL_MINIRIG__)) && defined(FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED)
	#pragma  GCC warning "Using FAN_SUBSYSTEM_REMAIN_AT_FULL_SPEED with PRODUCT_MODEL_SINGLE or PRODUCT_MODEL_MINIRIG is not recommended"
#endif

#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
	#if defined(__TEST_PIPE_PERFORMANCE)
		#error Unable to use __TEST_PIPE_PERFORMANCE on board operating in QUEUE_OPERATE_ONE_JOB_PER_CHIP mode
	#endif 
#endif

#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP) && defined(QUEUE_OPERATE_ONE_JOB_PER_BOARD)
	#error Unable to use both QUEUE_OPERATE_ONE_JOB_PER_CHIP and QUEUE_OPERATE_ONE_JOB_PER_BOARD at the same time
#endif

#if !defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP) && !defined(QUEUE_OPERATE_ONE_JOB_PER_BOARD)
	#error Either QUEUE_OPERATE_ONE_JOB_PER_CHIP or QUEUE_OPERATE_ONE_JOB_PER_BOARD must be selected
#endif

#if defined(__PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION) && defined(__IMMEDIATE_ENGINE_JOB_SUBMISSION)
	#error Unable to use __PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION and IMMEDIATE-ENGINE-JOB-SUBMISSION at the same time
#endif

#if defined(__PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION) && defined(__INTERLEAVED_JOB_LOADING)
	#error Unable to use __PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION and __INTERLEAVED_JOB_LOADING at the same time
#endif

#if !defined(__PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION) && defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
	#error __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION can only work when __PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION is enabled
#endif

#if defined(__ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION) && defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
	#error  Cannot have both __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION and __ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION
	#error  ONLY one is allowed at a time
#endif

#if defined (__ENGINE_ENABLE_TIMESTAMPING) && (!defined(__ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION) && !defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION))
	#error  Timestamping (__ENGINE_ENABLE_TIMESTAMPING) enabled but no monitoring authority defined
#endif

#if (defined(__ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION) || defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)) && !defined(__ENGINE_ENABLE_TIMESTAMPING)
	#error  __ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION but __ENGINE_ENABLE_TIMESTAMPING not defined
#endif

#if !defined(__ENGINE_ENABLE_TIMESTAMPING)
	#if defined(__ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION)
		#error  __ENGINE_PROGRESSIVE_ACTIVITY_SUPERVISION was requested by Timestamping has not been enabled
	#endif
	#if defined(__ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION)
		#error  __ENGINE_AUTHORITIVE_ACTIVITY_SUPERVISION was requested by Timestamping has not been enabled
	#endif
#endif

#if defined(__INTERLEAVED_JOB_LOADING) && !defined(__ACTIVATE_JOB_LOAD_BALANCING)
	#error With __INTERLEAVED_JOB_LOADING submission, __ACTIVATE_JOB_LOAD_BALANCING must be defined as well
#endif

#if defined(__PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION) && !defined(__ACTIVATE_JOB_LOAD_BALANCING)
	#error With __PROGRESSIVE_PER_ENGINE_JOB_SUBMISSION, __ACTIVATE_JOB_LOAD_BALANCING must be defined as well
#endif




/*************** Used for debugging ****************/
#define CHIP_TO_TEST 0

/*************** XLINK Operations Timeout ***********/
#define __XLINK_WAIT_FOR_DEVICE_RESPONSE__   10000   // 10ms
#define __XLINK_TRANSACTION_TIMEOUT__	     20000   // 20ms. No transaction should take longer. 
#define __XLINK_WAIT_PACKET_TIMEOUT__        440
#define __XLINK_ATTEMPT_RETRY_MAXIMUM__      44

/*************** Firmware Version ******************/
#define __FIRMWARE_VERSION		"1.2.2"	// This is firmware 1.2.0 [ CHIP PARALLELIZATION supported on this version and after ]

// **** Change log Vs 1.2.1
// 1) Chip frequency auto-tune enabled in this version

// **** Change log Vs 1.2.0
// 1) Less restrictive engine validation (checking 3 nonces instead of 7)


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

// Total engines detected first time on startup
unsigned int GLOBAL_TotalEnginesDetectedOnStartup;
unsigned int GLOBAL_LastJobResultProduced;

///////////////////////////////////////// typedefs
// Master Tick Counter (Holds clock in 1uS ticks)
volatile UL32 MAST_TICK_COUNTER;
void IncrementTickCounter(void);

// Was internal reset activated?
unsigned char GLOBAL_INTERNAL_ASIC_RESET_EXECUTED;

// Other definitions
#define TRUE		1
#define FALSE		0

#define MAKE_DWORD(b3,b2,b1,b0) ((b3 << 24) | (b2 << 16) | (b1 << 8) | (b0))
#define GET_BYTE_FROM_DWORD(dword, byte) ((dword >> (byte * 8)) & 0x0FF)

volatile char GLOBAL_InterProcChars[128];

volatile unsigned int GLOBAL_SCATTERED_DIAG_TIME;

volatile unsigned short GLOBAL_ChipActivityLEDCounter[TOTAL_CHIPS_INSTALLED];

// Some global definitions
typedef struct _tag_job_packet
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	char signature;	// 1 byte, it's 0xAA
} job_packet, *pjob_packet;

// DEPRECATED: Very Low Latency issues 
typedef struct _tag_job_packet_p2p
{
	char midstate[32]; // 256Bits of midstate
	char block_data[12]; // 12 Bytes of data
	unsigned int  nonce_begin; // 4 Bytes of Nonce-Begin 
	unsigned int  nonce_end;   // 4 Bytes of Nonce-End
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
extern unsigned int  GLOBAL_CHIP_FREQUENCY_INFO[TOTAL_CHIPS_INSTALLED];
extern unsigned char GLOBAL_CHIP_PROCESSOR_COUNT[TOTAL_CHIPS_INSTALLED];
unsigned int GLOBAL_CHIP_PROCESSOR_ENGINE_LOWBOUND[TOTAL_CHIPS_INSTALLED][16]; // Calculated on initialization
unsigned int GLOBAL_CHIP_PROCESSOR_ENGINE_HIGHBOUND[TOTAL_CHIPS_INSTALLED][16]; // Calculated on initialization

// Monitoring Authority arrays
unsigned char GLOBAL_ENGINE_PROCESSING_STATUS[TOTAL_CHIPS_INSTALLED][16]; // If 1, the engine is processing
unsigned char GLOBAL_ENGINE_PROCESSING_FAILURE_SCORES[TOTAL_CHIPS_INSTALLED][16]; // If this number reaches 5 failures, we'll decommission the engine
unsigned int  GLOBAL_ENGINE_PROCESSING_START_TIMESTAMP[TOTAL_CHIPS_INSTALLED][16]; // When did this engine start processing?

// Maximum operating time for engines
unsigned int  GLOBAL_ENGINE_MAXIMUM_OPERATING_TIME[TOTAL_CHIPS_INSTALLED][16];
#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
	#define __ENGINE_OPERATING_TIME_OVERHEAD 5000 // Maximum 5ms error is allowed!
#else // QUEUE_OPERATE_ONE_JOB_PER_BOARD
	#define __ENGINE_OPERATING_TIME_OVERHEAD 50000 // Maximum 50ms error is allowed!
#endif

// This should be by default zero
int GLOBAL_BLINK_REQUEST;

// Global Pulse-Request section
int  GLOBAL_PULSE_BLINK_REQUEST;
void System_Request_Pulse_Blink(void);

// Critical Temperature Warning - Abort Jobs!
volatile char GLOBAL_CRITICAL_TEMPERATURE;

// This is the main bit mask
volatile unsigned int GLOBAL_XLINK_DEVICE_AVAILABILITY_BITMASK;

// Used for ASIC handling
#define ASIC_SPI_RW_COMMAND						    0b01000000000000000

// Our Wathdog Reset command
#define WATCHDOG_RESET  AVR32_WDT.clr = 0x0FFFFFFFF

// Have we already detected this? If so, just return the previously known number
unsigned int __internal_global_iChipCount;

// For Test only
volatile unsigned int iMark1;
volatile unsigned int iMark2;
volatile unsigned int GLOBAL_LAST_ASIC_IS_PROCESSING_LATENCY;

// Basic boolean definition
#define TRUE	1
#define FALSE	0
#define CHIP_EXISTS(x)						   (((__chip_existence_map[x] & 0xFF) != 0))
#define IS_PROCESSOR_OK(xchip, yengine)		   ((__chip_existence_map[xchip] & (1 << yengine)) != 0)
#define DECOMMISSION_PROCESSOR(xchip, yengine) (__chip_existence_map[xchip] &= ~(1<<yengine))


unsigned int __chip_existence_map[TOTAL_CHIPS_INSTALLED]; // Bit 0 to Bit 16 in each word says if the engine is OK or not...
	
// Our sleep function
volatile void Sleep(unsigned int iSleepPeriod);

// How long did it take to sent the results data to USB?
volatile unsigned int GLOBAL_BufResultToUSBLatency;
volatile unsigned int GLOBAL_USBToHostLatency;
volatile unsigned int GLOBAL_ResBufferCompilationLatency;
volatile unsigned int GLOBAL_LastJobIssueToAllEngines;

#if defined(QUEUE_OPERATE_ONE_JOB_PER_CHIP)
	volatile unsigned int GLOBAL_LastJobIssuedToChip[TOTAL_CHIPS_INSTALLED];
#endif

// FAN Controls
#define FAN_CONTROL_BYTE_VERY_SLOW			(0b0010)
#define FAN_CONTROL_BYTE_SLOW				(0b0100)
#define FAN_CONTROL_BYTE_MEDIUM				(0b1000)
#define FAN_CONTROL_BYTE_FAST				(0b0001)
#define FAN_CONTROL_BYTE_VERY_FAST			(0b1111)
#define FAN_CONTROL_BYTE_REMAIN_FULL_SPEED	(FAN_CTRL0 | FAN_CTRL1 | FAN_CTRL2 | FAN_CTRL3)	// Turn all mosfets off...

#define FAN_CTRL0	 0b00001
#define FAN_CTRL1	 0b00010
#define FAN_CTRL2	 0b00100
#define FAN_CTRL3	 0b01000

// For Debug
volatile unsigned int DEBUG_TraceTimer0;
volatile unsigned int DEBUG_TraceTimer1;
volatile unsigned int DEBUG_TraceTimer2;
volatile unsigned int DEBUG_TraceTimers[8];
volatile unsigned int DEBUG_TraceTimersIndex;

// Assembly NOP operation
#ifdef __OPERATING_FREQUENCY_64MHz__
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t nop \n\t");
#elif defined(__OPERATING_FREQUENCY_48MHz__)	
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#else
	#define NOP_OPERATION asm volatile ("nop \n\t nop \n\t nop \n\t nop \n\t");
#endif

#endif /* STD_DEFS_H_ */