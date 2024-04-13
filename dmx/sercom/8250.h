#ifndef _8250_H
#define _8250_H

#define TRANSMIT_HOLDING_REGISTER          0
#define RECEIVE_BUFFER_REGISTER            0
#define DIVISOR_LATCH_LSB                  0
#define DIVISOR_LATCH_MSB                  1
#define INTERRUPT_ENABLE_REGISTER          1
#define   IER_RECEIVE_DATA_INTERRUPT       0x01
#define   IER_TRANSMIT_DATA_INTERRUPT      0x02
#define   IER_LINE_STATUS_INTERRUPT        0x04
#define   IER_MODEM_STATUS_INTERRUPT       0x08
#define INTERRUPT_ID_REGISTER              2
#define   IID_FIFO_ENABLED_MASK            0xc0
#define FIFO_CONTROL_REGISTER              2
#define   FCR_FIFO_ENABLE                  0x01
#define   FCR_RCVR_FIFO_RESET              0x02
#define   FCR_XMIT_FIFO_RESET              0x04
#define   FCR_DMA_MODE_SELECT              0x08
#define   FCR_RCVR_TRIGGER_LSB             0x40
#define   FCR_RCVR_TRIGGER_MSB             0x80
#define LINE_CONTROL_REGISTER              3
#define   LCR_WORD_LENGTH_SELECT_BITS      0x03
#define     LCR_WORD_LENGTH_SELECT_BIT_0   0x01
#define     LCR_WORD_LENGTH_SELECT_BIT_1   0x02
#define       LCR_WORD_LENGTH_5            0x00
#define       LCR_WORD_LENGTH_6            0x01
#define       LCR_WORD_LENGTH_7            0x02
#define       LCR_WORD_LENGTH_8            0x03
#define   LCR_NUMBER_OF_STOP_BITS          0x04
#define   LCR_PARITY_BITS                  0x38
#define     LCR_PARITY_ENABLE              0x08
#define     LCR_EVEN_PARITY_SELECT         0x10
#define     LCR_STICK_PARITY               0x20
#define       LCR_PARITY_N                 0x00
#define       LCR_PARITY_O                 0x08
#define       LCR_PARITY_E                 0x18
#define       LCR_PARITY_S                 0x38
#define       LCR_PARITY_M                 0x28
#define   LCR_SET_BREAK                    0x40
#define   LCR_DIVISOR_LATCH_ACCESS         0x80
#define MODEM_CONTROL_REGISTER             4
#define   MCR_DATA_TERMINAL_READY          0x01
#define   MCR_REQUEST_TO_SEND              0x02
#define   MCR_OUT1                         0x04
#define   MCR_OUT2                         0x08
#define   MCR_LOOPBACK                     0x10
#define LINE_STATUS_REGISTER               5
#define   LSR_DATA_READY                   0x01
#define   LSR_OVERRUN_ERROR                0x02
#define   LSR_PARITY_ERROR                 0x04
#define   LSR_FRAMING_ERROR                0x08
#define   LSR_BREAK_INTERRUPT              0x10
#define   LSR_THRE                         0x20
#define   LSR_TEMT                         0x40
#define   LSR_FIFO_ERROR                   0x80
#define MODEM_STATUS_REGISTER              6
#define   MSR_DELTA_CTS                    0x01
#define   MSR_DELTA_DSR                    0x02
#define   MSR_TRAILING_EDGE_RI             0x04
#define   MSR_DELTA_CD                     0x08
#define   MSR_CTS                          0x10
#define   MSR_DSR                          0x20
#define   MSR_RI                           0x40
#define   MSR_CD                           0x80
#define SCRATCH_REGISTER                   7


typedef enum 
{ 
	INT_STANDARD_HANDLER = 0,
	INT_SHARED_IRQ_HANDLER = 1
} INT_TYPE;

/*
* Port Status Bits (from interrupt process)
*/
typedef struct 
{
    unsigned alert     : 1;
    unsigned rxempty   : 1;
    unsigned rxfull    : 1;
    unsigned rxovflow  : 1;
    unsigned txempty   : 1;
    unsigned txfull    : 1;
    unsigned linerr    : 1;
    unsigned modchg    : 1;
    unsigned xchrun    : 1;
    unsigned rchrun    : 1;
    unsigned txwxon    : 1;
    unsigned txwcts    : 1;
    unsigned txwalert  : 1;
    unsigned xoffsent  : 1;
    unsigned rtsactive : 1;
    unsigned txiflag   : 1;
} STBITS;

/*
*Interrupt Options (written from application side, read by interrupts)
*/
typedef struct 
{
    unsigned is_txint                       : 1;
    unsigned is_rxint                       : 1;
    unsigned is_ascii                       : 1;
    unsigned is_rxerror                     : 1;
    unsigned is_receiver_xoffmode           : 1;
    unsigned cts_low_holds_tx_interrupts    : 1;
    unsigned alert_flag_stops_rx_and_tx     : 1;
    unsigned dsr_low_discards_rx_data       : 1;
    unsigned cd_low_discards_rx_data        : 1;
    unsigned modem_status_changes_set_alert : 1;
    unsigned line_errors_set_alert          : 1;
    unsigned is_16550                       : 1;
    unsigned is_blocking                    : 1;
    unsigned is_rchking                     : 1;
    unsigned is_rtscontrol                  : 1;
    unsigned is_transmitter_xoffmode        : 1;
} MODEBITS;

typedef struct
{
    UINT 		cell;   			// size of cell ( for wide-track )
    UINT 		size;   			// size of buffer            
    UINT 		count;  			// number of bytes in buffer 
    UINT 		head;   			// offset of buffer head     
    UINT 		tail;   			// offset of buffer tail     
    char 		*buffer;			// always points to base of buffer
} QUEUE;

typedef struct 
{
    int			intrpt_num; 		// 8250 interrupt no. 0C...
    UINT		base_8250;  		// base i/o address of 8250
    int 		p_8250[ 5 ];		// previous values for 8250 registers:
                            		//	0 = line control register,
                            		//	1 = modem control register, bits 5-7 are FCR
                            		//	2 = interrupt enable register,
                            		//	3 = divisor latch LSB,
                            		//	4 = divisor latch MSB
                            		
    void ( far * p_vector )( void );// previous value for interrupt vector
    UINT 		line_stat;          // Cumulative line status
    UINT 		modem_stat;         // Cumulative modem status
    UINT 		wide_stat;          // Current wide-track status

    int 		irq_8259;       	// Interrupt # in 8259 (com0 = 4)
    int 		saved_8259_bits;	// Previous value of irq_8259 bit
    UINT		port_8259;          // I/O Address of 8259           

	QUEUE		rx;					// receive queue
	QUEUE		tx;					// transmit queue
    STBITS 		chst_bits;      	// port status bits                  
    MODEBITS	chmode_bits;  		// port mode bits IN,OUT,XON,etc     

    UINT		rts_lowater;   		// when to assert RTS                
    UINT		rts_hiwater;   		// when to de-assert RTS             
    UINT		rx_accum;      		// counter for received characters   
    UINT		rx_lowater;    		// When to send an XON               
    UINT		rx_hiwater;    		// When to send an XOFF              
    int			stop_xmt;      		// The incoming XOFF character       
    int			start_xmt;     		// The incoming XON character        
    int			stop_rem_xmt;  		// The outbound XOFF character       
    int			start_rem_xmt; 		// The outbound XON character        

    int			break_delay;   		// The number of ticks in a break    

    int			aswmodem;      		// Polled mode timeout values        
    int			aswtime;
    int			asrtime;

    UINT		chkchr[ 3 ];   		// Each character occupies the low
        		               		//	8 bits, the high order bit (bit 15)
        		               		//	is set to 1 if rx-interrupts are to
        		               		//	look at this character, bit 14 is set
        		               		//	by interrupt routines and is to be
        		               		//	checked by asirchk().  Bit 8 & 9
        		               		//	determine the mode or option of what
        		               		//	to do                              
    UINT		mscount;       		// Modem status interrupt counter
    UINT		lscount;       		// Line status interrupt counter 
    UINT		txcount;       		// Transmit interrupt counter    
    UINT		rxcount;       		// Receive interrupt counter     
    int 		out12_mask;    		// The OUT1/OUT2 mask            
    int 		ls_ms_ier;     		// The interrupt enable initial mask
    UINT 		tx_limit;			// FIFO buffer limit
	UINT		rx_16550_level;		// 16650 FIFO Interrupt Rate
} PORT_TABLE;

typedef struct
{
	PORT_TABLE	*ptb;
    unsigned 	as_shport;
    unsigned 	as_shbits;
    unsigned 	as_mask;
    unsigned 	as_xorv;
    INT_TYPE 	handler_type;
} PORTINFO;

#endif	/*ifndef _8250_H */
