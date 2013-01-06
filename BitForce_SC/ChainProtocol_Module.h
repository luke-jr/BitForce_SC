/*
 * ChainProtocol_Module.h
 *
 * Created: 08/10/2012 21:39:56
 *  Author: NASSER
 */ 
#ifndef CHAINPROTOCOL_MODULE_H_
#define CHAINPROTOCOL_MODULE_H_

#define CPLD_TX_STATUS_TxDone   0b010
#define CPLD_TX_STATUS_TxInProg	0b0100

#define CPLD_RX_STATUS_DATA		0b01
#define CPLD_RX_STATUS_ERR		0b010
#define CPLD_RX_STATUS_OVERFLOW	0b0100
#define CPLD_RX_STATUS_LENGTH	0b01000
#define CPLD_RX_STATUS_LP		0b01000000
#define CPLD_RX_STATUS_BC		0b010000000

#define CPLD_RX_CONTROL_CLEAR	0b01

#define CPLD_TX_CONTROL_SEND	0b01
#define CPLD_TX_CONTROL_LENGTH	0b010
#define CPLD_TX_CONTROL_LP		0b010000
#define CPLD_TX_CONTROL_BC		0b010000000 // BitCorrect

#define CPLD_MASTER_CONTROL_MSTR			0b01
#define CPLD_MASTER_CONTROL_CPLD_ADDRESS	0b010
#define CPLD_MASTER_CONTROL_PASSTHROUGH		0b010000000

#define CPLD_ADDRESS_MASTER_CONTROL	 23
#define CPLD_ADDRESS_CHAIN_STATUS 	 24
#define CPLD_ADDRESS_TX_CONTROL 	 19
#define CPLD_ADDRESS_TX_TARGET_ADRS	 33
#define CPLD_ADDRESS_RX_CONTROL		 20
#define CPLD_ADDRESS_TX_STATUS		 21
#define CPLD_ADDRESS_RX_STATUS		 22
#define CPLD_ADDRESS_TX_BUF_BEGIN 	 1
#define CPLD_ADDRESS_TX_BUF_END   	 8
#define CPLD_ADDRESS_RX_BUF_BEGIN 	 10
#define CPLD_ADDRESS_RX_BUF_END		 17
#define CPLD_ADDRESS_IDENTIFICATION	 25
#define CPLD_ADDRESS_SENDERS_ADRS	 26

#define CPLD_ADDRESS_TX_START		27
#define CPLD_ADDRESS_TX_START_SEND	0b01

#define XLINK_DEVICE_STATUS_PROCESSING	1
#define XLINK_DEVICE_STATUS_FINISHED	2
#define XLINK_DEVICE_STATUS_NO_TRANS	3

// Addressing is as following:
// 'XLNK' returns the total bytes in XLink General Buffer
// 'XK<x><y>' Returns the 4 bytes starting at address x * 0x0100 + y

// *** IREG - RX Status (8 Bit)
//
// MSB                                              LSB
// +-------------------------+----------+-----+------+
// | BC | LP | Length (3Bit) | OVERFLOW | ERR | DATA |
// +----+----+---------------+----------+-----+------+
//   7    6       5..3            2        1      0


// *** IREG - TX Status (8 Bit)
//
// MSB                                             LSB
// +-----------+----------+--------+-----------------+
// | N/C (5Bit)| TxInProg | TxDone |       N/C       |
// +-----------+----------+--------+-----------------+
//    7 .. 3        2          1            0


// *** IREG - RX Control (8 Bit)
//
// MSB                                             LSB
// +-------------------------------+-----------------+
// |  N/C (7 Bits)                 | Clear Registers |
// +-------------------------------+-----------------+
//              7 .. 1                      0


// *** IREG - TX Control (8 Bit)
//
// MSB                                             LSB
// +-----------+--------+----+----------------+------+
// |BitCorrect | XXXXXX | LP | Length (3Bits) | SEND |
// +-----------+--------+----+----------------+------+
//      7         6..5     4        3 .. 1        0


// *** IREG - TX TARGET ADRS (8 Bit)
//
// MSB                                             LSB
// +------------------------------+------------------+
// |  N/C (3 Bits)                | 5Bit Target ADRS |
// +------------------------------+------------------+
//              7 .. 5     	          4 .. 0


// *** IREG - Chain Status (8 Bit)
//
// MSB                                             LSB
// +--------------------------+----------+-----------+
// |  N/C (6 Bits)            | Chain IN | Chain OUT |
// +--------------------------+----------+-----------+
//              7 .. 2     	     1           0


// *** IREG - Master Control Register (8 Bit)
//
// MSB                                             LSB
// +-------------+------+---------------------+------+
// | Passthrough | NC   | CPLD ADDRESS (5Bit) | MSTR |
// +-------------+------+---------------------+------+
//      7           6           5..1             0

///////////////////////////////////////////////////////////////////
// Now comes to our functions
///////////////////////////////////////////////////////////////////

// Variables
unsigned short XLINK_chain_device_count;

// Our general dispatch address
#define XLINK_GENERAL_DISPATCH_ADDRESS 0x01F

// Procedures
void init_XLINK();

void XLINK_send_packet( char iAdrs, 
						char* szData, 
						unsigned short iLen, 
						char LP, 
						char BC);
					   
// This function sends a string to our HOST					   
void XLINK_SLAVE_respond_string(char* szStringToSend);


// This is used by master
void XLINK_MASTER_transact (char   iAdrs,
							char*  szData,
							unsigned short  iLen,
							char*  szResp,
							unsigned short* response_length,
							unsigned short  iMaxRespLen,
							long    transaction_timeout, // Master timeout
							char   *bDeviceNotResponded, // Device did not respond, even to the first packet
							char   *bTimeoutDetected, // Was a timeout detected?
							char   bWeAreMaster); // The address we expect from the device to respond to
							
				
// Called by the master, used to determine the chain length
int XLINK_MASTER_chainDevicesExists(void);

// How many devices exist in chain?
int XLINK_MASTER_getChainLength(void);							
							
// This is used by slave...
void XLINK_SLAVE_wait_transact (char  *data,
								unsigned int *length,
								unsigned int  max_len,
								long transaction_timeout,
								char  *bTimeoutDetected,
								char  bWeAreMaster);
								
// This is used by slave as well								
void XLINK_SLAVE_respond_transact  (char  *data,
									unsigned int length,
									long transaction_timeout,
									char  *bTimeoutDetected,
									char  bWeAreMaster); // If zero, we'll use CPLDs address

// This function receives data
void XLINK_wait_packet (char  *data,
						unsigned int   *length,
						long  time_out,
						char  *timeout_detected,
						char  *senders_address,
						char  *LP,
						char  *BC);
									  
char	 XLINK_data_inbound				(void);
void	 XLINK_set_cpld_id				(char iID);
char	 XLINK_get_cpld_id				(void);
void	 XLINK_set_cpld_master			(char bMaster);
void	 XLINK_set_cpld_passthrough		(char bPassthrough);
char	 XLINK_get_chain_status			(void);
char	 XLINK_get_TX_status			(void);
char	 XLINK_get_RX_status			(void);
void	 XLINK_set_target_address		(char uAdrs);
void	 XLINK_clear_RX					(void);
int		 XLINK_detect_if_we_are_master  (void);
int		 XLINK_is_cpld_present			(void);

char	 XLINK_get_device_status		(void);
void	 XLINK_set_device_status		(char  iDevState);
void	 XLINK_set_outbox				(char* szData, short iLen);


///////////////////////////////////////////////////////////////////


#endif /* CHAINPROTOCOL_MODULE_H_ */