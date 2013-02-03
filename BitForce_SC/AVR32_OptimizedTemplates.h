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


//////////////////////////////////////////////////////////////////
// MACROS
/////////////////////////////////////////////////////////////////
// Auto address increase function
#define CPLD_activate_address_increase     AVR32_GPIO.port[1].ovrs  = __AVR32_CPLD_INCREASE_ADDRESS;
#define CPLD_deactivate_address_increase   AVR32_GPIO.port[1].ovrc  = __AVR32_CPLD_INCREASE_ADDRESS;

#define MACRO_GetTickCount(x)  (x = (MAST_TICK_COUNTER << 16) + (AVR32_TC.channel[0].cv))
#define MACRO_GetTickCountRet  ((MAST_TICK_COUNTER << 16) + (AVR32_TC.channel[0].cv))

#define MACRO__AVR32_CPLD_Read(ret_value, address) ({ \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE; \
		volatile int iRetVal = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
		ret_value = iRetVal; })

#define MACRO__AVR32_CPLD_Write(address, value) ({ \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | value; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; })
		
#define MACRO__AVR32_CPLD_WriteTxControlAndStart(txControlVal) ({ \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00); \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | CPLD_ADDRESS_TX_CONTROL; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		CPLD_activate_address_increase; \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | txControlVal; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | CPLD_ADDRESS_TX_START_SEND; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \	
		CPLD_deactivate_address_increase; \				
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; })		

#define MACRO__AVR32_CPLD_BurstTxWrite(szdata, address_begin) ({ \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | address_begin; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		CPLD_activate_address_increase; \
		volatile unsigned int iInitialOvrValue = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00); \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[0]; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[1]; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[2]; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovr  = (iInitialOvrValue) | szdata[3]; \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		CPLD_deactivate_address_increase; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; }) 
		
#define MACRO__AVR32_CPLD_BurstRxRead(iData,iAddress) ({ \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		AVR32_GPIO.port[1].oders = __AVR32_CPLD_BUS_ALL; \
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].ovr  = (AVR32_GPIO.port[1].ovr & 0x0FFFFFF00) | iAddress; \	
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_ADRS; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \	
		AVR32_GPIO.port[1].ovrs = __AVR32_CPLD_OE; \
		CPLD_activate_address_increase; \
		iData[0] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		iData[1] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		iData[2] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[0].ovrs = __AVR32_CPLD_STROBE; \
		AVR32_GPIO.port[0].ovrc = __AVR32_CPLD_STROBE; \
		iData[3] = (AVR32_GPIO.port[1].pvr & 0x000000FF); \
		AVR32_GPIO.port[1].ovrc = __AVR32_CPLD_OE; \
		CPLD_deactivate_address_increase; \
		AVR32_GPIO.port[1].oderc = __AVR32_CPLD_BUS_ALL; \
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

#define MACRO_XLINK_send_packet(iadrs, szdata, ilen, lp, bc) ({ \
		char read_tx_status = 0x0FF; \
		while ((read_tx_status & CPLD_TX_STATUS_TxInProg) != 0) { MACRO_XLINK_get_TX_status(read_tx_status);} \
		MACRO_XLINK_set_target_address(iadrs); \
		unsigned char iTotalToSend = (ilen << 1); \
		char szMMR[4]; \
		MACRO__AVR32_CPLD_BurstTxWrite(szdata, CPLD_ADDRESS_TX_BUF_BEGIN); \
		char iTxControlVal = 0b00000000; \
		iTxControlVal |= iTotalToSend;	\
		iTxControlVal |= (lp != 0) ? CPLD_TX_CONTROL_LP : 0; \
		iTxControlVal |= (bc != 0) ? CPLD_TX_CONTROL_BC : 0; \
		MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_CONTROL, iTxControlVal); \
		MACRO__AVR32_CPLD_Write(CPLD_ADDRESS_TX_START, CPLD_ADDRESS_TX_START_SEND); \
		})	

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
			UL64 iTimeoutHolder; \
			MACRO_GetTickCount(iTimeoutHolder); \
			UL64 iTickHolder; \
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
	UL64 iTimeoutHolder; \
	MACRO_GetTickCount(iTimeoutHolder); \
	UL64 iTickHolder; \
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
			UL64 iTimeoutHolder; \
			MACRO_GetTickCount(iTimeoutHolder); \
			UL64 iTickHolder; \
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