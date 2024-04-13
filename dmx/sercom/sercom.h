/****************************************************************************
* Serial Communications - HEADER FILE
*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif


#define	SER_OKAY				0
#define	SER_INUSE				-1
#define	SER_NOMEMORY			-2
#define	SER_NOUART				-3
#define	SER_INVPARM				-4
#define	SER_INVBUFSIZE			-5
#define	SER_ILLEGALPORT			-6
#define	SER_ILLEGALWORDLENGTH	-7
#define	SER_ILLEGALPARITY		-8
#define	SER_ILLEGALSTOPBITS		-9
#define	SER_ILLEGALBAUDRATE		-10
#define	SER_ILLEGALIRQ			-11
#define	SER_TIMEOUT				-12
#define SER_BUFFERFULL			-13

/*
* Port IRQ's
*/
#define IRQ1   1
#define IRQ2   2
#define IRQ3   3
#define IRQ4   4
#define IRQ5   5
#define IRQ6   6
#define IRQ7   7
#define IRQ8   8
#define IRQ9   9
#define IRQ10 10
#define IRQ11 11
#define IRQ12 12
#define IRQ13 13
#define IRQ14 14
#define IRQ15 15

/*
* Parity types
*/
#define P_NONE			0
#define P_ODD			1
#define P_EVEN			2
#define P_S_STICK		3
#define P_M_STICK		4

/*
* Port ID's
*/
#define COM1			0
#define COM2			1
#define COM3			2
#define COM4			3
#define COM5			4
#define COM6			5
#define COM7			6
#define COM8			7
#define MAX_PORTS		8

/*
* Port Modes
*/
#define ASIN            1
#define ASOUT           2
#define ASINOUT         3
#define BINARY          0
#define ASCII           4
#define NORMALRX        0
#define WIDETRACKRX     128
#define USE_16550		64

#define isalert( p )                        _iswhat( ( p ), 1 )
#define isrxempty( p )                      _iswhat( ( p ), 2 )
#define isrxfull( p )                       _iswhat( ( p ), 3 )
#define isrxovflow( p )                     _iswhat( ( p ), 4 )
#define istxempty( p )                      _iswhat( ( p ), 5 )
#define istxfull( p )                       _iswhat( ( p ), 6 )
#define islinerr( p )                       _iswhat( ( p ), 7 )
#define ismodemerr( p )                     _iswhat( ( p ), 8 )
#define istxintrunning( p )                 _iswhat( ( p ), 9 )
#define isrxintrunning( p )                 _iswhat( ( p ), 10 )
#define AlertFlagStopsRXAndTX( p )          _iswhat( ( p ), 11 )
#define CTSLowHoldsTXInterrupts( p )        _iswhat( ( p ), 12 )
#define DSRLowDiscardsRXData( p )           _iswhat( ( p ), 13 )
#define CDLowDiscardsRXData( p )            _iswhat( ( p ), 14 )
#define ModemStatusChangesSetAlert( p )     _iswhat( ( p ), 15 )
#define LineStatusErrorsSetAlert( p )       _iswhat( ( p ), 16 )
#define isncts( p )                 _iswhat( ( p ), 17 )
#define isndsr( p )                 _iswhat( ( p ), 18 )
#define isncd( p )                  _iswhat( ( p ), 19 )
#define isring( p )                 _iswhat( ( p ), 20 )
#define isxmrxing( p )              _iswhat( ( p ), 21 )
#define isxoffblocked( p )          _iswhat( ( p ), 22 )
#define isctsblocked( p )           _iswhat( ( p ), 23 )
#define is16550( p )                _iswhat( ( p ), 24 )

#define isoverrun( p, o )          _isstat( ( p ), o, 1 )
#define isparityerr( p, o )        _isstat( ( p ), o, 2 )
#define isframerr( p, o )          _isstat( ( p ), o, 3 )
#define iscts( p, o )              _isstat( ( p ), o, 5 )
#define isdsr( p, o )              _isstat( ( p ), o, 6 )
#define iscd( p, o )               _isstat( ( p ), o, 7 )
#define isri( p, o )               _isstat( ( p ), o, 8 )
#define ischgcts( p, o )           _isstat( ( p ), o, 9 )
#define ischgdsr( p, o )           _isstat( ( p ), o, 10 )
#define ischgcd( p, o )            _isstat( ( p ), o, 11 )
#define ischgri( p, o )            _isstat( ( p ), o, 12 )

/*
 *Define status modes for all those is...() functions
 */
#define IMMEDIATE       0
#define CUMULATIVE      1

/*
* Buffer size Ranges
*/
#define MAX_BUF_SIZE    65535U
#define MIN_BUF_SIZE    2


typedef enum 
{
    TRIGGER_DISABLE    = 0x00,
    TRIGGER_01         = 0x01,
    TRIGGER_04         = 0x41,
    TRIGGER_08         = 0x81,
    TRIGGER_14         = 0xc1
} TRIGGER_LEVEL;


/***************************************************************************
* serGetPortHardware - Get I/O Port Hardware Parameters.
*---------------------------------------------------------------------------
* Returns:
*	SER_ILLEGAPORT
*	SER_OKAY
***************************************************************************/
PUBLIC int 
serGetPortHardware(
	int		port,		// INPUT : COM1..COM8
	int		*addr,		// OUTPUT: Physical Port Address
	int		*irq 		// OUTPUT: Port IRQ
	);

/***************************************************************************
* serSetPortHardware - Set I/O Port Hardware Parameters.
*---------------------------------------------------------------------------
* Returns:
*	SER_ILLEGAPORT
*	SER_ILLEGAIRQ
*	SER_NOUART
*	SER_OKAY
***************************************************************************/
PUBLIC int 
serSetPortHardware( 
	int		port,		// INPUT : COM1..COM8
	int		addr,		// INPUT : Physical Port Address
	int		irq			// INPUT : Port IRQ
	);

/***************************************************************************
* serOpenPort - Init serial I/O port
*---------------------------------------------------------------------------
*  "mode" specifies five different kinds of operational parameters.
*  Choose one from each of the following five categories.  See documentation
*  for further details:
*       1.  Direction (ASIN, ASOUT, ASINOUT)
*       2.  Data Type (BINARY, ASCII)
*       3.  Receive Buffer Width (NORMALRX, WIDETRACKRX)
***************************************************************************/
PUBLIC int
serOpenPort(
	int		port,		// INPUT : Port COM1..COM8 (0..MAX_PORT-1)
	UINT 	mode,		// INPUT : Merged options
	UINT 	size_rx_buf,// INPUT : Length of Rx Buffer in bytes
	UINT 	size_tx_buf,// INPUT : Length of Tx Buffer in bytes
	long 	baud,		// INPUT : Baud Rate (50L-9600L,19000L,38400L,57600L,115200L
	int 	wordlength,
	int 	stopbits,
	int 	parity,
	UINT	tx_limit,	// INPUT : Max Tx Chrs per Interrupt
	UINT	rx_trigger 	// INPUT : 16550 RX FIFO Setting
	);

/***************************************************************************
* serClosePort - Close an opened port.
***************************************************************************/
PUBLIC int 
serClosePort( 
	int		port		// INPUT : Port
	);

#ifdef __cplusplus
}
#endif	/* __cplusplus */
