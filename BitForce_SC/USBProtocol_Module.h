/*
 * USBProtocol_Module.h
 *
 * Created: 13/10/2012 00:29:33
 *  Author: NASSER
 */ 


#ifndef USBPROTOCOL_MODULE_H_
#define USBPROTOCOL_MODULE_H_

void	init_USB(void);

void	USB_wait_packet(char* data,
					    unsigned int *length,    // output
					    unsigned int  req_size,   // input
					    unsigned int  max_len,	  // input
					    unsigned int *time_out);  // time_out is in/out

void	USB_wait_stream(char* data,
						unsigned int *length,    // output
						unsigned int  max_len,   // input
						unsigned int  min_len,   // Minimum length of the stream
						unsigned int *time_out); // Timeout variable

void	USB_send_string(const char* data);
char	USB_write_data (const char* data, unsigned int length);
void	USB_send_immediate(void);
char	USB_inbound_USB_data(void);
void	USB_flush_USB_data(void);
char	USB_outbound_space(void);
char	USB_read_byte(void);
char	USB_read_status(void);
char	USB_write_byte(char data);


#endif /* USBPROTOCOL_MODULE_H_ */