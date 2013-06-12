/*
 * AVR32_OptimizedTemplates.h
 *
 * Created: 19/01/2013 19:20:48
 *  Author: NASSER
 */ 


#ifndef AVR32_OPTIMIZEDTEMPLATES_H_
#define AVR32_OPTIMIZEDTEMPLATES_H_


//////////////////////////////////////////////////////////////////
// Assembly optimized functions
//////////////////////////////////////////////////////////////////
void OPTIMIZED__AVR32_CPLD_Write(unsigned char iAdrs, unsigned char iData);
void OPTIMIZED__AVR32_CPLD_Read (unsigned char iAdrs, unsigned char *retVal);
void OPTIMIZED__AVR32_CPLD_BurstTxWrite(char* szData, char iAddress);
void OPTIMIZED__AVR32_CPLD_BurstRxRead(char* szData, char iAddress);

unsigned int OPTO_GetTickCountRet(void);

//////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////
// Auto address increase function
#define CPLD_activate_address_increase     AVR32_GPIO.port[1].ovrs  = __AVR32_CPLD_INCREASE_ADDRESS;
#define CPLD_deactivate_address_increase   AVR32_GPIO.port[1].ovrc  = __AVR32_CPLD_INCREASE_ADDRESS;

#if defined(__OPERATING_FREQUENCY_32MHz__)
	#define MACRO_GetTickCount(x)  (x = (UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv)))
	#define MACRO_GetTickCountRet  ((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv)))
	//#define MACRO_GetTickCount(x)  (x = (UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val * 9)))
	//#define MACRO_GetTickCountRet  ((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val * 9 )))	
#else
	#define MACRO_GetTickCount(x)  (x = (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv))) >> 1) )
	#define MACRO_GetTickCountRet  (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_TC.channel[0].cv))) >> 1)
	//#define MACRO_GetTickCount(x)  (x = (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val))) >> 1) )
	//#define MACRO_GetTickCountRet  (((UL32)((UL32)(MAST_TICK_COUNTER) | (UL32)(AVR32_RTC.val))) >> 1)	
#endif


///////////////////////////////////////////
// INTERLACED MACROS
///////////////////////////////////////////

#define MACRO_INTERLACED_PREPARE_TO_WRITE ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		}) \

#define MACRO_INTERLACED_SET_ADDRESS(address) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		}) \
		
#define MACRO_INTERLACED_WRITE(value) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | value; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		}) 

#define MACRO_INTERLACED_END_WRITE	({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
		}) 
		
		
		


#define MACRO_INTERLACED_PREPARE_TO_READ(x_address)	 ({ \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | x_address; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		 NOP_OPERATION \
		 AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE; \		 
		 }) 
		
#define MACRO_INTERLACED_READ(ret_value) ({ \
		NOP_OPERATION \
		ret_value = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		NOP_OPERATION \
		}) 
		

#define MACRO_INTERLACED_END_READ ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		}) 


//////////////////////////////////////////
// FULL MACROS
/////////////////////////////////////////


#define MACRO__AVR32_CPLD_Read(ret_value, address) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		volatile int iRetVal = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
		ret_value = iRetVal; })


#define MACRO__AVR32_CPLD_Write(address, value) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | value; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION })
		
		
		

#define MACRO__AVR32_CPLD_WriteTxControlAndStart(txControlVal) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00); \		
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | CPLD_ADDRESS_TX_CONTROL; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | txControlVal; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | CPLD_ADDRESS_TX_START; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs  = CPLD_ADDRESS_TX_START_SEND; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \				
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \ 
		NOP_OPERATION })

		
			

#define MACRO__AVR32_CPLD_BurstTxWrite(szdata, address_begin) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc =  __AVR32_CPLD_OE; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address_begin; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \		
		CPLD_activate_address_increase; \
		NOP_OPERATION \
		volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00); \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[0]; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[1]; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[2]; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[3]; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		CPLD_deactivate_address_increase; \
		NOP_OPERATION \		
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION }) 
		
		
#define MACRO__AVR32_CPLD_BurstRxRead(iData,iAddress) ({ \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAddress; \	
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \	
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		CPLD_activate_address_increase; \
		NOP_OPERATION \
		iData[0] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		iData[1] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		iData[2] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		NOP_OPERATION \
		iData[3] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		NOP_OPERATION \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		NOP_OPERATION \
		CPLD_deactivate_address_increase; \
		NOP_OPERATION \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		NOP_OPERATION \
	 }) 




/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define __NO_ASSEMBLY_OPTIMIZATION__	1

#ifdef	__NO_ASSEMBLY_OPTIMIZATION__

#define MACRO_XLINK_clear_RX				   MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);
#define MACRO_XLINK_get_TX_status(ret_value)  (MACRO__AVR32_CPLD_Read(ret_value, CPLD_ADDRESS_TX_STATUS))
#define MACRO_XLINK_get_RX_status(ret_value)  (MACRO__AVR32_CPLD_Read(ret_value, CPLD_ADDRESS_RX_STATUS))
#define MACRO_XLINK_set_target_address(x)	  (MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_TARGET_ADRS, x & 0b011111))


// WORKING SUPER OK

#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
		volatile char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { MACRO_XLINK_get_TX_status(read_tx_status);} \
		MACRO_XLINK_set_target_address(iadrs); \
		volatile unsigned char iTotalToSend = (ilen << 1); \
		volatile char szMMR[4]; \
		MACRO__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		volatile char iTxControlVal = 0b00000000; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal); \
		MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_START, CPLD_ADDRESS_TX_START_SEND); \
		})	


/*
// In test
#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
		volatile char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { MACRO_XLINK_get_TX_status(read_tx_status);} \
		MACRO_XLINK_set_target_address(iadrs); \
		volatile unsigned char iTotalToSend = (ilen << 1); \
		volatile char szMMR[4]; \
		MACRO__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		volatile char iTxControlVal = 0b00000000; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		MACRO__AVR32_CPLD_WriteTxControlAndStart(iTxControlVal); \
		})	
*/

/*
// IN TEST
#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
		MACRO_INTERLACED_PREPARE_TO_READ(CPLD_ADDRESS_TX_STATUS); \
		volatile char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) \
		{ \
			MACRO_INTERLACED_READ(read_tx_status); \
		} \
		MACRO_INTERLACED_END_READ; \			
		MACRO_XLINK_set_target_address(iadrs); \
		volatile unsigned char iTotalToSend = (ilen << 1); \
		volatile char szMMR[4]; \
		MACRO__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		volatile char iTxControlVal = 0b00000000; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		MACRO_INTERLACED_PREPARE_TO_WRITE; \
		MACRO_INTERLACED_SET_ADDRESS(CPLD_ADDRESS_TX_CONTROL); \
		MACRO_INTERLACED_WRITE(iTxControlVal); \
		MACRO_INTERLACED_SET_ADDRESS(CPLD_ADDRESS_TX_START); \
		MACRO_INTERLACED_WRITE(CPLD_ADDRESS_TX_START_SEND); \
		MACRO_INTERLACED_END_WRITE; \
		})	
*/

/*
//ORIGINAL WORKING
#define MACRO_XLINK_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
		while(TRUE) \
		{ \
			BC = 0; \
			LP = 0; \
			timeout_detected = FALSE; \
			length = 0; \
			senders_address = 0; \
			volatile char iActualRXStatus = 0; \
			volatile unsigned char us1 = 0; \
			volatile unsigned char us2 = 0; \
			MACRO_XLINK_get_RX_status(iActualRXStatus); \
			volatile UL32 iTimeoutHolder; \
			MACRO_GetTickCount(iTimeoutHolder); \
			volatile UL32 iTickHolder; \
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0) \
			{ \
				while (TRUE) \ 
				{ \
					MACRO_XLINK_get_RX_status(iActualRXStatus); \
					if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break; \
					MACRO_GetTickCount(iTickHolder); \
					if ((UL32)(iTickHolder - iTimeoutHolder) > (UL32)time_out) \
					{ \
						timeout_detected = TRUE; \
						length = 0; \
						senders_address = 0; \
						break; \
					}  \
				} \				
				if (timeout_detected == TRUE) break; \
			} \
			volatile char imrLen = 0; \
			imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
			length = imrLen; \
			LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
			BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
			MACRO__AVR32_CPLD_Read(senders_address, CPLD_ADDRESS_SENDERS_ADRS); \
			MACRO__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
			MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
			break; \
		} \
	}) 
*/


// In test
#define MACRO_XLINK_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
		while(TRUE) \
		{ \
			BC = 0; \
			LP = 0; \
			timeout_detected = FALSE; \
			length = 0; \
			senders_address = 0; \
			volatile char iActualRXStatus = 0; \
			volatile unsigned char us1 = 0; \
			volatile unsigned char us2 = 0; \
			MACRO_INTERLACED_PREPARE_TO_READ(CPLD_ADDRESS_RX_STATUS); \
			MACRO_INTERLACED_READ(iActualRXStatus); \
			volatile UL32 iTimeoutHolder; \
			MACRO_GetTickCount(iTimeoutHolder); \
			volatile UL32 iTickHolder; \
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0) \
			{ \
				while (TRUE) \
				{ \
					MACRO_INTERLACED_READ(iActualRXStatus); \
					if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break; \
					MACRO_GetTickCount(iTickHolder); \
					if ((UL32)(iTickHolder - iTimeoutHolder) > (UL32)time_out) \
					{ \
						timeout_detected = TRUE; \
						length = 0; \
						senders_address = 0; \
						MACRO_INTERLACED_END_READ; \
						break; \
					}  \
				} \
				if (timeout_detected == TRUE) break; \
			} \
			MACRO_INTERLACED_END_READ; \			
			volatile char imrLen = 0; \
			imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
			length = imrLen; \
			LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
			BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
			MACRO__AVR32_CPLD_Read(senders_address, CPLD_ADDRESS_SENDERS_ADRS); \
			MACRO__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
			MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
			break; \
		} \
	}) 

	
#define MACRO_XLINK_simulate_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
while(TRUE) \
{ \
	BC = 0; \
	LP = 0; \
	timeout_detected = FALSE; \
	length = 0; \
	senders_address = 0; \
	volatile char iActualRXStatus = 0; \
	volatile unsigned char us1 = 0; \
	volatile unsigned char us2 = 0; \
	MACRO_XLINK_get_RX_status(iActualRXStatus); \
	UL32 iTimeoutHolder; \
	MACRO_GetTickCount(iTimeoutHolder); \
	UL32 iTickHolder; \
	volatile char imrLen = 0; \
	imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
	length = imrLen; \
	LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
	BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
	MACRO__AVR32_CPLD_Read(senders_address, CPLD_ADDRESS_SENDERS_ADRS); \
	MACRO__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
	MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
	break; \
} \
}) 	


#else

// Nothing really...
#define MACRO_XLINK_clear_RX				   OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR);
#define MACRO_XLINK_get_TX_status(ret_value)  (OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_TX_STATUS, &ret_value))
#define MACRO_XLINK_get_RX_status(ret_value)  (OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_RX_STATUS, &ret_value))
#define MACRO_XLINK_set_target_address(x)	  (OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_TARGET_ADRS, x & 0b011111))
	

#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
							char read_tx_status = 0x0FF; \
							while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { MACRO_XLINK_get_TX_status(read_tx_status);} \
							MACRO_XLINK_set_target_address(iadrs); \
							unsigned char iTotalToSend = (ilen << 1); \
							char szMMR[4]; \
							OPTIMIZED__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
							char iTxControlVal = 0b00000000; \
							iTxControlVal |= iTotalToSend;	\
							iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
							iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
							OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal); \
							OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_TX_START, CPLD_ADDRESS_TX_START_SEND); \
						})
/*
#define MACRO_XLINK_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
while(TRUE) \
{ \
	BC = 0; \
	LP = 0; \
	timeout_detected = FALSE; \
	length = 0; \
	senders_address = 0; \
	volatile char iActualRXStatus = 0; \
	volatile unsigned char us1 = 0; \
	volatile unsigned char us2 = 0; \
	MACRO_XLINK_get_RX_status(iActualRXStatus); \
	UL32 iTimeoutHolder; \
	MACRO_GetTickCount(iTimeoutHolder); \
	UL32 iTickHolder; \
	if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0) \
	{ \
		while (TRUE) \
		{ \
			MACRO_XLINK_get_RX_status(iActualRXStatus); \
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break; \
			MACRO_GetTickCount(iTickHolder); \
			if ((iTickHolder - iTimeoutHolder) > time_out) \
			{ \
				timeout_detected = TRUE; \
				length = 0; \
				senders_address = 0; \
				break; \
			}  \
		} \
		if (timeout_detected == TRUE) break; \
	} \
	volatile char imrLen = 0; \
	imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
	length = imrLen; \
	LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
	BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
	OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_SENDERS_ADRS, &senders_address); \
	OPTIMIZED__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
	OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
	break; \
	} \
}) 

*/



#define MACRO_XLINK_wait_packet(data, length, time_out, timeout_detected, senders_address, LP, BC) ({ \
		while(TRUE) \
		{ \
			BC = 0; \
			LP = 0; \
			timeout_detected = FALSE; \
			length = 0; \
			senders_address = 0; \
			volatile char iActualRXStatus = 0; \
			volatile unsigned char us1 = 0; \
			volatile unsigned char us2 = 0; \
			MACRO_XLINK_get_RX_status(iActualRXStatus); \
			UL32 iTimeoutHolder; \
			MACRO_GetTickCount(iTimeoutHolder); \
			UL32 iTickHolder; \
			if ((iActualRXStatus & CPLD_RX_STATUS_DATA) == 0) \
			{ \
				while (TRUE) \ 
				{ \
					MACRO_XLINK_get_RX_status(iActualRXStatus); \
					if ((iActualRXStatus & CPLD_RX_STATUS_DATA) != 0) break; \
					MACRO_GetTickCount(iTickHolder); \
					if ((iTickHolder - iTimeoutHolder) > time_out) \
					{ \
						timeout_detected = TRUE; \
						length = 0; \
						senders_address = 0; \
						break; \
					}  \
				} \				
				if (timeout_detected == TRUE) break; \
			} \
			volatile char imrLen = 0; \
			imrLen = ((iActualRXStatus & 0b0111000) >> 3); \
			length = imrLen; \
			LP = ((iActualRXStatus & CPLD_RX_STATUS_LP) != 0) ? 1 : 0; \
			BC = ((iActualRXStatus & CPLD_RX_STATUS_BC) != 0) ? 1 : 0; \
			OPTIMIZED__AVR32_CPLD_Read(CPLD_ADDRESS_SENDERS_ADRS, &senders_address); \
			OPTIMIZED__AVR32_CPLD_BurstRxRead(data, CPLD_ADDRESS_RX_BUF_BEGIN); \
			OPTIMIZED__AVR32_CPLD_Write(CPLD_ADDRESS_RX_CONTROL, CPLD_RX_CONTROL_CLEAR); \
			break; \
		} \
	}) 



#endif



/////////////////////////////////////////////////////////////////
/*
#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
		char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { MACRO_XLINK_get_TX_status(read_tx_status);} \
		MACRO_XLINK_set_target_address(iadrs); \
		unsigned char iTotalToSend = (ilen << 1); \
		char szMMR[4]; \
		MACRO__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		unsigned int iTxControlVal = 0; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		MACRO__AVR32_CPLD_WriteTxControlAndStart(iTxControlVal); \
		})		
*/

#endif /* AVR32_OPTIMIZEDTEMPLATES_H_ */