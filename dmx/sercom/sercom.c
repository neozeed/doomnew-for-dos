/****************************************************************************
* Serial Communications
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>

#include "types.h"
#include "sercom.h"
#include "8250.h"

LOCAL PORTINFO	gChnl[ MAX_PORTS ] = { { 0 } };
LOCAL UINT		g8259ports[ MAX_PORTS ];

/*
* 8250 Standard Port Addresses
*/
LOCAL UINT gIsaPorts[]	= { 0x3f8, 0x2f8, 0x3e8, 0x2e8 };
LOCAL UINT gMcaPorts[]	= { 0x03f8, 0x02f8, 0x3220, 0x3228,
							0x4220, 0x4228, 0x5220, 0x5228 };
LOCAL UINT g8250Port[ MAX_PORTS ];

/*
* 8250 Standard IRQ Settings
*/
LOCAL BYTE gIsaIRQ[] 	= { 4, 3, 4, 3 };
LOCAL BYTE gMcaIRQ[] 	= { 4, 3, 3, 3, 3, 3, 3, 3 };
LOCAL BYTE g8259Irq[ MAX_PORTS ];

/*
* Interrupt Numbers for all ports
*/
LOCAL BYTE gIsaInt[]	= { 12, 11, 12, 11 };
LOCAL BYTE gMcaInt[]	= { 12, 11, 11, 11, 11, 11, 11, 11 };
LOCAL BYTE gPcInt[ MAX_PORTS ];

LOCAL int  gPorts;
LOCAL int  gSerDataInitted = 0;

/*===========================================================================
* IsMicroChannel
*==========================================================================*/
PRIVATE int		// returns: 1=MCA, 0=ISA
IsMicroChannel(
	void
	)
{
    LOCAL int		MicroChannel = -1;

    BYTE 			*info;
    union REGS		r;

    if ( MicroChannel == -1 )
	{
        MicroChannel = 0;
        r.x.ebx = 0x0000ffffU;
        r.h.ah = 0xc0;
        int386( 0x15, &r, &r );
        if ( r.x.cflag )
            return( 0 );
        info = ( BYTE * )( r.x.ebx );
        info += 5;
        MicroChannel = ( *info & 0x0002 ) ? 1 : 0;
    }
    return( MicroChannel );
}


/*===========================================================================
* serGetPortTable - Converts a port ID into a pointer to a PORT_TABLE
*==========================================================================*/
PRIVATE PORT_TABLE * 
serGetPortTable(
	int		port		// INPUT : Port
	)
{
    if ( port < 0 || port >= MAX_PORTS ) 
		return NULL;

    if ( gChnl[ port ].ptb == NULL ) 
    {
        return NULL;
    }
    return gChnl[ port ].ptb;
}



/*===========================================================================
* serInitData - Initialize port data
*==========================================================================*/
PRIVATE void
serInitData(
	void
	)
{
	UINT	*ports;
	BYTE	*irqs;
	BYTE	*ints;
	int		j;

	memset( gChnl, 0, sizeof( gChnl ) );

	for ( j = COM1; j <= COM8; j++ )
	    gChnl[ j ].handler_type = INT_STANDARD_HANDLER;

	if ( IsMicroChannel() )
	{
		gPorts = 8;
		ports = gMcaPorts;
		irqs  = gMcaIRQ;
		ints  = gMcaInt;

		for ( j = COM2; j <= gPorts; j++ )
		    gChnl[ j ].handler_type = INT_SHARED_IRQ_HANDLER;
	}
	else
	{
		gPorts = 4;
		ports = gIsaPorts;
		irqs  = gIsaIRQ;
		ints  = gIsaInt;
	}
	memcpy( g8250Port, ports, gPorts * sizeof( UINT ) );
	memcpy( g8259Irq, irqs, gPorts * sizeof( BYTE ) );
	memcpy( gPcInt, ints, gPorts * sizeof( BYTE ) );

	for ( j = COM1; j <= gPorts; j++ )
	    g8259ports[ j ] = 0x20;

	gSerDataInitted = 1;
/*
        as_brkdly[ i ] = 3;
        as_wtime[ i ] = 4;
        as_shbmask[ i ] = 0xff00;
        as_out12_mask[ i ] = MCR_OUT2;
        as_ls_ms_ier[ i ] = IER_LINE_STATUS_INTERRUPT + IER_MODEM_STATUS_INTERRUPT;
*/
}

/****************************************************************************
* serInp - Read byte from port
*----------------------------------------------------------------------------
* NOTE: This function exists for the sole purpose of providing enough I/O
*       delay in between port access.
****************************************************************************/
PRIVATE BYTE
serInp(
	int		port		// INPUT : Port Address
	)
{
	return inp( port );
}

/***************************************************************************
* serOutp - Write byte to port
*----------------------------------------------------------------------------
* NOTE: This function exists for the sole purpose of providing enough I/O
*       delay in between port access.
****************************************************************************/
PRIVATE void
serOutp(
	int		port,		// INPUT : Port Address
	BYTE	data		// INPUT : Data to write to port
	)
{
	outp( port, data );
}

int
_serGetc(
	int		addr
	)
{
	BYTE	lsr;

	lsr = serInp( addr + LINE_STATUS_REGISTER );
	if ( lsr & LSR_DATA_READY )
	{
		return serInp( addr );
	}
	return SER_TIMEOUT;
}

int
_serPutc(
	int		addr,
	BYTE	chr
	)
{
	BYTE	lsr;

	lsr = serInp( addr + LINE_STATUS_REGISTER );
	if ( lsr & LSR_THRE )
	{
		outp( addr, chr );
		return SER_OKAY;
	}
	return SER_TIMEOUT;
}


/***************************************************************************
* serInitPort - Init serial I/O port
***************************************************************************/
serInitPort(
	int		port,		// INPUT : Port
	long	baud,		// INPUT : 50-9600,19200,38400,57600,115200
	int		databits,	// INPUT : 5,6,7, or 8
	int		stopbits,	// INPUT : 1 or 2
	int		parity		// INPUT : P_NONE, P_ODD, P_EVEN, P_S_STICK, P_M_STICK
	)
{
	static char	uart_parity[ 5 ] =
	{
		LCR_PARITY_N, LCR_PARITY_O, LCR_PARITY_E, LCR_PARITY_S, LCR_PARITY_M
	};
    int 		divisor;
    int 		ier;
    int 		lsr;
	int			lcr;

	if ( ! gSerDataInitted ) 
		serInitData();

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	port = g8250Port[ port ];

    if ( baud < 1 || baud > 115200L )
        return( SER_ILLEGALBAUDRATE );
    divisor = ( int ) ( 115200L / baud ) ;

	if ( databits < 5 || databits > 8 )
		return SER_ILLEGALWORDLENGTH;

	lcr = databits - 5;
    switch ( stopbits )
	{
        case 1: 	break;
        case 2: 	lcr |= LCR_NUMBER_OF_STOP_BITS; break;
        default:	return( SER_ILLEGALSTOPBITS );
    }
	if ( parity < 0 || parity >= sizeof( uart_parity ) )
		return SER_ILLEGALPARITY;

	lcr |= uart_parity[ parity ];

    ier = serInp( port + INTERRUPT_ENABLE_REGISTER );
	if ( ier == 0xFF )
		return SER_ILLEGALPORT;

	serOutp( port + INTERRUPT_ENABLE_REGISTER, 0 );
/*
* Wait for the current character to be shifted out before messing
* with the UART.
*/
	serOutp( port + LINE_CONTROL_REGISTER, LCR_DIVISOR_LATCH_ACCESS );
	if ( serInp( port + DIVISOR_LATCH_LSB ) ||
		 serInp( port + DIVISOR_LATCH_MSB ) )
	{
		do
		{
			lsr = serInp( port + LINE_STATUS_REGISTER );
		}
		while ( ! ( lsr & LSR_THRE ) );
	}
/*
* Set Baud Rate
*/
	serOutp( port + DIVISOR_LATCH_LSB, divisor & 0xFF );
	serOutp( port + DIVISOR_LATCH_MSB, divisor >> 8 );
	serOutp( port + LINE_CONTROL_REGISTER, lcr & ~LCR_DIVISOR_LATCH_ACCESS );
	_disable();
/*
* Write out the IER value twice, based on National Information.
*/
	serOutp( port + INTERRUPT_ENABLE_REGISTER, ier );
	serOutp( port + INTERRUPT_ENABLE_REGISTER, ier );
	_enable();

	return SER_OKAY;
}


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
	)
{
	if ( ! gSerDataInitted ) 
		serInitData();

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

    if ( addr != NULL )
        *addr = g8250Port[ port ];

    if ( irq != NULL ) 
    {
        if ( gPcInt[ port ] >= 0x70 )
            *irq = gPcInt[ port ] - 0x70 + IRQ8;
        else
            *irq = gPcInt[ port ] - 8;
    }
    return SER_OKAY;
}

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
	)
{
	if ( ! gSerDataInitted ) 
		serInitData();

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	if ( serGetPortTable( port ) != NULL )
		return SER_INUSE;

	if ( irq < IRQ1 || irq > IRQ15 ) 
		return SER_ILLEGALIRQ;

    if ( addr != -1 )
	{
	    if ( ( serInp( addr + INTERRUPT_ID_REGISTER ) & 0x30 ) != 0 )
    	    return SER_NOUART;
        g8250Port[ port ] = addr;
	}

    if ( irq != -1 ) 
    {
        if ( irq > IRQ7 ) 
        {
            gPcInt[ port ] = ( BYTE ) ( irq - IRQ8 + 0x70 );
            g8259ports[ port ] = 0xa0;
            g8259Irq[ port ] = ( BYTE ) ( irq % 8 );
        } 
        else 
        {
            gPcInt[ port ] = ( BYTE ) ( irq + 8 );
            g8259ports[ port ] = 0x20;
            g8259Irq[ port ] = ( BYTE ) irq;
        }
    }
    return SER_OKAY;
}



PUBLIC int 
serStart( 
	int 		port, 
	UINT		option 
	)
{
	PORT_TABLE 	*p;
	int			base;
	int			lcr;
	int			ier;
	int			mcr;

	if ( ! gSerDataInitted ) 
		serInitData();

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    switch ( option ) 
    {
        case ASINOUT   :
        case ASIN      :
            p->chmode_bits.is_rxint = 1;
            if ( option == ASIN )
                break;

            case ASOUT :
                p->chmode_bits.is_txint = 1;
                break;

            default:
                return SER_INVPARM;
    }

    base = p->base_8250;
    lcr	= serInp( base + LINE_CONTROL_REGISTER );
    lcr &= ~LCR_DIVISOR_LATCH_ACCESS;
    serOutp( base + LINE_CONTROL_REGISTER, lcr );
    serInp( base );
/*
* The resulting port structure will have a few limitations,
* such as an inability to perform hardware handshaking, but there may
* be reasons when you want to do this.  To modify these settings, change
* as_ls_ms_ier[ port ] to a different value than it is normally initialized
* to.
*/
    ier = p->ls_ms_ier;
    if ( p->chmode_bits.is_rxint )
        ier |= IER_RECEIVE_DATA_INTERRUPT;
    if ( p->chmode_bits.is_txint )
        ier |= IER_TRANSMIT_DATA_INTERRUPT;
    serOutp( base + INTERRUPT_ENABLE_REGISTER, ier );
/*
* Most 8250 based I/O cards require OUT2 to be asserted in order for 
* interrupts to run. 
*/
    mcr = serInp( base + MODEM_CONTROL_REGISTER );
    mcr &= ~( MCR_OUT1 | MCR_OUT2 );
    mcr |= p->out12_mask;
    serOutp( base + MODEM_CONTROL_REGISTER, mcr );
    return SER_OKAY;
}


PUBLIC void 
serSetTxLimit( 
	int 	port, 
	int 	tlimit
	)
{
    PORT_TABLE *p;

	if ( port < 0 || port >= gPorts )
		return;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return;

    switch ( tlimit )
    {
		default:
        case TRIGGER_01 : p->tx_limit = 1; break;
        case TRIGGER_04 : p->tx_limit = 4; break;
        case TRIGGER_08 : p->tx_limit = 8; break;
        case TRIGGER_14 : p->tx_limit = 14; break;
    }
}


PUBLIC void 
serSet16550Fifo( 
	int port, 
	int tlevel 
	)
{
    PORT_TABLE *p;

    p = gChnl[ port ].ptb;
	p->rx_16550_level = tlevel;
}



PRIVATE int 
serFreeAll(
	int		port,		// INPUT : Port Address
	int		reason		// INPUT : Error Condition
	)
{
    PORT_TABLE	*p;

    p = gChnl[ port ].ptb;
    if ( p ) 
    {
        if ( p->tx.buffer )
			free( p->tx.buffer );
        if ( p->rx.buffer )
			free( p->rx.buffer );
		free( p );
        gChnl[ port ].ptb = NULL;
    }
    return reason;
}

PUBLIC int 
serSetDtr( 
	int		port,
	int 	control 
	)
{
    int			old_value;
	int			addr;
    PORT_TABLE	*p;

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    addr = p->base_8250 + MODEM_CONTROL_REGISTER;
    if ( control ) 
    {
        _disable();
        old_value = serInp( addr );
        serOutp( addr, old_value | MCR_DATA_TERMINAL_READY );
        _enable();
    } 
    else 
    {
        _disable();
        old_value = serInp( addr );
        serOutp( addr, old_value & ~MCR_DATA_TERMINAL_READY );
        _enable();
    }
    return( old_value & MCR_DATA_TERMINAL_READY );
}

PUBLIC int 
serSetRts( 
	int		port,
	int 	control
    )
{
    int 		old_value;
	int			addr;
    PORT_TABLE	*p;

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    addr = p->base_8250 + MODEM_CONTROL_REGISTER;

    if ( control ) 
    {
        _disable();
        old_value = serInp( addr );
        serOutp( addr, old_value | MCR_REQUEST_TO_SEND );
        p->chst_bits.rtsactive = 1;
        _enable();
    } 
    else 
    {
        _disable();
        old_value = serInp( addr );
        serOutp( addr, old_value & ~MCR_REQUEST_TO_SEND );
        p->chst_bits.rtsactive = 0;
        _enable();
    }
    return( ( old_value & MCR_REQUEST_TO_SEND ) ? 1 : 0  );
}

/*===========================================================================
* _serPeekChr - Peek into RX buffer without the overhead of handshaking.
*----------------------------------------------------------------------------
* RETURNS
*
* The next character in the buffer.  I wide track is on, the wide
* track status will be in the high byte returned.
*
*==========================================================================*/
PRIVATE int
_serPeekChr(
    PORT_TABLE	*p
	)
{
	VOID		*ptr;

	ptr = ( VOID * )( &p->rx.buffer[ p->rx.head ] );
	if ( p->rx.cell == 1 )
		return (int)( *( ( BYTE * ) ptr ) );
	else
		return (int)( *( ( WORD * ) ptr ) );
}


/*===========================================================================
* _serInsertChr - Insert character into specified queue.
*----------------------------------------------------------------------------
* RETURNS
*
* 0 = queue is not full
* 1 = queue is NOW full
*
*==========================================================================*/
PRIVATE int	
_serInsertChr( 
	QUEUE		*q,
	int			chr
	)
{
	VOID		*ptr;

	q->count++;
	
	ptr = ( VOID * )( &q->buffer[ q->tail ] );
	q->tail += q->cell;
	if ( q->tail >= ( q->size * q->cell ) )
		q->tail = 0;

	if ( q->cell == 1 )
		*( BYTE * ) ptr = ( BYTE ) chr;
	else
		*( WORD * ) ptr = ( WORD ) chr;
	return ( q->head == q->tail );
}

/*===========================================================================
* _serRemoveChr - Remove character from specified queue.
*----------------------------------------------------------------------------
* RETURNS
*
* The next character in the buffer.  I wide track is on, the wide
* track status will be in the high byte returned.
*
*==========================================================================*/
PRIVATE int	
_serRemoveChr( 
	QUEUE		*q
	)
{
	VOID		*ptr;
	int			chr;

	ptr = ( VOID * )( &q->buffer[ q->head ] );
	q->head += q->cell;
	if ( q->head >= ( q->size * q->cell ) )
		q->head = 0;


	if ( q->cell == 1 )
		chr = ( int ) *( ( BYTE * ) ptr );
	else
		chr = ( int ) *( ( WORD * ) ptr );

	q->count--;

	return chr;
}


PRIVATE void
_serRestartTx(
    PORT_TABLE	*p
	)
{
	int			chr;

	if ( p->chst_bits.txempty == 0 )
	{
		_disable();
		p->chst_bits.txfull = 0;
		chr = _serRemoveChr( &p->tx );
		if ( p->tx.count == 0 )
			p->chst_bits.txempty = 1;
		else
		{
			p->chst_bits.xchrun = 1;
			p->chst_bits.txiflag = 1;
		}
		_enable();
		serOutp( p->base_8250, chr );
	}
}


/*===========================================================================
* _serGetChr - Get character from RX buffer.
* 
* This is the low level routine that is used to pull a character out
* of the RX buffer for the port.  It has to take care of a bunch of
* stuff like handshaking and blocking, which makes it a lot more
* complicated than you might think it ought to be.
*
*----------------------------------------------------------------------------
* RETURNS
*
* The next character in the buffer.  I wide track is on, the wide
* track status will be in the high byte returned.
*
*==========================================================================*/
PRIVATE int
_serGetChr( 
    PORT_TABLE	*p
	)
{
	int			chr;
	int			addr;

	_disable();
	p->chst_bits.rxfull = 0;
	chr = _serRemoveChr( &p->rx );
	if ( p->rx.count == 0 )
		p->chst_bits.rxempty = 1;
	_enable();

	if ( p->chmode_bits.is_ascii )
		chr &= ~0x80;

	if ( ! p->chmode_bits.is_blocking )
	{
		if ( p->chst_bits.xoffsent && p->rx.count < p->rx_lowater )
		{
/*
; At this point, we have to send an XON.  Since interrupts may
; be running, I go through this ugly scene where I wait for the
; THRE to be empty, then stuff the character.  I have to keep interrupts
; disabled while this is happening.  Hope it works!
*/				
			addr = p->base_8250 + LINE_STATUS_REGISTER;
			_disable();
			while ( ( serInp( addr ) & LSR_THRE ) == 0 )
			{
				_enable();
				_disable();
			}
			serOutp( p->base_8250, p->start_rem_xmt );
			_enable();

			p->chst_bits.xoffsent = 0;
			if ( p->chmode_bits.is_txint )
				p->chst_bits.xchrun = 1;
		}

		if ( p->chmode_bits.is_rtscontrol && p->chst_bits.rtsactive == 0 &&
			 p->rts_lowater >= p->rx.count )
		{
			_disable();
			p->chst_bits.rtsactive = 1;
			addr = p->base_8250 + MODEM_CONTROL_REGISTER;
			serOutp( addr, serInp( addr ) | MCR_REQUEST_TO_SEND );
			_enable();
		}
	}
	return chr;
}



/*===========================================================================
* _serPutChr - Put character into TX buffer.
* 
* This is the low level routine that is used to put a character into
* the TX buffer for the port.  It has to take care of a bunch of
* stuff like handshaking and blocking, which makes it a lot more
* complicated than you might think it ought to be.  Note that the
* high level routine always checks to see if the buffer is full
* *before* this routine is called.
*
*==========================================================================*/
PRIVATE void
_serPutChr(
    PORT_TABLE	*p,
	int			chr
	)
{
	if ( p->chmode_bits.is_ascii )
		chr &= ~0x80;

	_disable();
	if ( _serInsertChr( &p->tx, chr ) )
		p->chst_bits.txfull = 1;
	_enable();

	if ( p->chst_bits.txempty )
	{
		p->chst_bits.txempty = 0;

		if ( p->chmode_bits.is_txint 
			&& ! p->chst_bits.txwxon
			&& ! p->chst_bits.txwcts
			&& ! p->chst_bits.txwalert 
			&& ! p->chst_bits.xchrun )
		{
			int		addr;

			addr = p->base_8250 + LINE_STATUS_REGISTER;

			while ( ( serInp( addr ) & LSR_THRE ) == 0 )
				;
			_serRestartTx( p );
		}
	}
}

/****************************************************************************
* serPutChr - Put character into TX buffer for specified port
*****************************************************************************/
PUBLIC int
serPutChr(
	int		port,	// INPUT : Port to write character to
	int		chr		// INPUT : Character to write
	)
{
	PORT_TABLE 	*p;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

	if ( p->chst_bits.txfull )
		return SER_BUFFERFULL;

	_serPutChr( p, chr );
	return SER_OKAY;
}


/****************************************************************************
* serGetChr - Get character from RX buffer for specified port
*****************************************************************************/
PUBLIC int
serGetChr(
	int		port	// INPUT : Port to write character to
	)
{
	PORT_TABLE 	*p;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

	if ( p->chst_bits.rxempty )
		return SER_BUFFERFULL;

	return _serGetChr( p );
}


/****************************************************************************
* _isstat - returns status of given option
*****************************************************************************/
PUBLIC int 
_isstat( 
	int		port,	// INPUT : Port 
	int 	option, 
	int 	cmd 
	)
{
    PORT_TABLE	*p;
    unsigned 	temp;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    if ( ( option != CUMULATIVE ) && ( option != IMMEDIATE ) )
        return ( SER_INVPARM );

    switch ( cmd ) 
    {

        case 1: /* Has a receiver overrun error occurred */
            if ( option == CUMULATIVE )
                temp = p->line_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + LINE_STATUS_REGISTER );
            return( ( temp & 2 ) ? TRUE : FALSE );

        case 2: /* Has a parity error occurred */
            if ( option == CUMULATIVE )
                temp = p->line_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + LINE_STATUS_REGISTER );
            return( ( temp & 0x0004 ) ? TRUE : FALSE );

        case 3: /* Has a framing error occurred */
            if ( option == CUMULATIVE )
                temp = p->line_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + LINE_STATUS_REGISTER );
            return( ( temp & 0x0008 ) ? TRUE : FALSE );

        case 4: /* Has a break signal been received */
            if ( option == CUMULATIVE )
                temp = p->line_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + LINE_STATUS_REGISTER );
            return( ( temp & 0x0010 ) ? TRUE : FALSE );

        case 5: /* Return the state of CTS */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) p->wide_stat;
            return( ( temp & 0x0010 ) ? TRUE : FALSE );

        case 6: /* Return the state of DSR */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) p->wide_stat;
            return( ( temp & 0x0020 ) ? TRUE : FALSE );

        case 7: /* Return the state of Carrier Detect */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) p->wide_stat;
            return( ( temp & 0x0080 ) ? TRUE : FALSE );

        case 8: /* Return the state of Ring Indicator */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + MODEM_STATUS_REGISTER );
            return( ( temp & 0x0040 ) ? TRUE : FALSE );

        case 9: /* Has there been a change in CTS */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + MODEM_STATUS_REGISTER );
            return( ( temp & 0x0001 ) ? TRUE : FALSE );

        case 10: /* Has there been a change in DSR */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + MODEM_STATUS_REGISTER );
            return( ( temp & 0x0002 ) ? TRUE : FALSE );

        case 11: /* Has there been a change in CD */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + MODEM_STATUS_REGISTER );
            return( ( temp & 0x0008 ) ? TRUE : FALSE );

        case 12: /* Has there been a change in RI */
            if ( option == CUMULATIVE )
                temp = p->modem_stat;
            else
                temp = (unsigned) serInp( p->base_8250 + MODEM_STATUS_REGISTER );
            return( ( temp & 0x0004 ) ? TRUE : FALSE );

        default:
            return( SER_INVPARM );
    }
}

PUBLIC int 
_iswhat( 
	int		port,	// INPUT : Port 
	int 	option
	)
{
    PORT_TABLE	*p;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    switch( option ) 
    {
        case 1: /* Check Alert flag isalert() */
            return( ( p->chst_bits.alert ) ? TRUE : FALSE );

        case 2: /* Is RX-Buffer empty */
            return( ( p->chst_bits.rxempty ) ? TRUE : FALSE );

        case 3: /* Is RX-Buffer full? */
            return( ( p->chst_bits.rxfull ) ? TRUE : FALSE );

        case 4: /* Is RX-Buffer overflow? */
            return( ( p->chst_bits.rxovflow ) ? TRUE : FALSE );

        case 5: /* Is TX-Buffer empty? */
            return( ( p->chst_bits.txempty ) ? TRUE : FALSE );

        case 6: /* Is TX-Buffer full? */
            return( ( p->chst_bits.txfull ) ? TRUE : FALSE );

        case 7: /* Is there a line error? */
            return( ( p->chst_bits.linerr ) ? TRUE : FALSE );

        case 8: /* Is there a modem error */
            return( ( p->chst_bits.modchg ) ? TRUE : FALSE );

        case 9: /* Are TX-Interrupts running */
            if( !p->chmode_bits.is_txint || p->chst_bits.txwxon ||
                p->chst_bits.txwcts || p->chst_bits.txwalert )
                return( FALSE );
            else
                return( TRUE );

        case 10: /* Are RX-Interrupts running */
            return( ( p->chmode_bits.is_rxint ) ? TRUE : FALSE );

        case 11: /* Is alert being ignored AlertFlagStopsRXAndTX() */
            if ( p->chmode_bits.alert_flag_stops_rx_and_tx )
                return( TRUE );
            else
                return( FALSE );

        case 12: /* Is CTS being ignored CTSLowHoldsTXInterrupts() */
            if ( p->chmode_bits.cts_low_holds_tx_interrupts )
                return( TRUE );
            else
                return( FALSE );

        case 13: /* Is DSR being ignored DSRLowDiscardsRXData() */
            if ( p->chmode_bits.dsr_low_discards_rx_data )
                return( TRUE );
            else
                return( FALSE );

        case 14: /* Is CD being ignored CDLowDiscardsRXData() */
            if ( p->chmode_bits.cd_low_discards_rx_data )
                return( TRUE );
            else
                return( FALSE );

        case 15: /* Are modem status errors being
                    ignored ModemStatusChangesSetAlert() */
            if ( p->chmode_bits.modem_status_changes_set_alert )
                return( TRUE );
            else
                return( FALSE );

        case 16: /* Are receiver errors being ignored? */
            if ( p->chmode_bits.line_errors_set_alert )
                return( TRUE );
            else
                return( FALSE );

        case 17:  /* Has CTS gone low this block ? */
            return( ( p->modem_stat & 0x1000 ) ? TRUE : FALSE );

        case 18:  /* Has DSR gone low this block */
            return( ( p->modem_stat & 0x2000 ) ? TRUE : FALSE );

        case 19:  /* Has CD gone low this block */
            return( ( p->modem_stat & 0x8000 ) ? TRUE : FALSE );

        case 20:  /* Has Ring indicator been asserted */
            return( ( p->modem_stat & 0x0004 ) ? TRUE : FALSE );

        case 21:  /* is receiver count > 0 */
            return( ( p->rx_accum ) ? TRUE : FALSE );

        case 22:  /* isxoffblocked(p) */
            return( ( p->chmode_bits.is_transmitter_xoffmode &&
                    p->chst_bits.txwxon ) ? TRUE : FALSE );

        case 23:  /* isctsblocked(p) */
            return( ( p->chmode_bits.cts_low_holds_tx_interrupts &&
                      p->chst_bits.txwcts ) ? TRUE : FALSE );

        case 24: /* is16550(p) */
            return( p->chmode_bits.is_16550 ? TRUE : FALSE );

        default:
            return( SER_INVPARM );
    }
}

/*##########################################################################*
*                   I N T E R R U P T   H A N D L E R S 
**##########################################################################*/
PRIVATE void
_serIntModemStatus(
    PORT_TABLE	*p
	)
{
	int			addr;
	int			status;
		
	addr = p->base_8250;
	status = serInp( addr + MODEM_STATUS_REGISTER );
	if ( status & MSR_TRAILING_EDGE_RI )
		status |= MSR_RI;

	p->modem_stat = status | ( ( ~status << 8 ) & 0xFF00 ); 
	
	status = ( ( p->wide_stat & 0x0F ) | ( status & 0xF0 ) ) << 8;
	p->wide_stat &= ~0xF0;
	p->chst_bits.modchg = 1;
		
	if (  p->chmode_bits.modem_status_changes_set_alert 
	   || p->chmode_bits.alert_flag_stops_rx_and_tx
	   || p->chmode_bits.cts_low_holds_tx_interrupts )
	{
		if ( p->chmode_bits.modem_status_changes_set_alert )
			p->chst_bits.alert = 1;

		if ( p->chmode_bits.alert_flag_stops_rx_and_tx )
		{
			p->chst_bits.txiflag = 0;
			serOutp( addr, serInp( addr + INTERRUPT_ENABLE_REGISTER ) 
					 & ~IER_RECEIVE_DATA_INTERRUPT );
			p->chst_bits.xchrun = 0;
			p->chst_bits.rchrun = 0;
			p->chst_bits.txwalert = 1;
			return;
		}

		if ( p->chmode_bits.cts_low_holds_tx_interrupts 
		     && p->chmode_bits.is_txint && ( status & MSR_DELTA_CTS ) )
		{
			if ( status & MSR_CTS )
			{
				addr = p->base_8250 + LINE_STATUS_REGISTER;

				while ( ( serInp( addr ) & LSR_THRE ) == 0 )
					;
				_serRestartTx( p );
				p->chst_bits.txwcts = 0;
			}
			else
			{
			   serInp( addr + INTERRUPT_ENABLE_REGISTER );
			   p->chst_bits.txiflag = 0;
			   p->chst_bits.xchrun = 0;
			   p->chst_bits.txwcts = 1;
			}
		}
	}
}


PRIVATE void
_serIntTx(
    PORT_TABLE	*p
	)
{
	int			addr;
	int			limit;
	int			chr;
	
	limit = p->tx_limit;
	if ( p->chst_bits.txiflag )
	{
		addr = p->base_8250;

		if ( ( serInp( addr + LINE_STATUS_REGISTER ) & LSR_THRE ) == 0 )
			return;

		for ( limit = p->tx_limit; limit >= 0 ; limit-- )
		{
			if ( p->chst_bits.txempty )
				break;

			if ( ! p->chst_bits.txwxon )
			{
				_disable();
				chr = _serRemoveChr( &p->tx );
				if ( p->tx.count == 0 )
					p->chst_bits.txempty = 1;
				p->chst_bits.txfull = 0;
				_enable();
				serOutp( addr, chr );
			}
			if ( p->chmode_bits.is_16550 )
			{
				if ( ! p->chmode_bits.is_receiver_xoffmode )
					continue;
			}
			break;
		}
	}
	p->chst_bits.xchrun = 0;
	p->chst_bits.txiflag = 0;
}


PRIVATE void
_serIntRx(
    PORT_TABLE	*p
	)
{
	int			linestat;
	int			chr;


	linestat = p->base_8250 + LINE_STATUS_REGISTER;
	do 
	{
		chr = serInp( p->base_8250 );
		p->rx_accum++;

		if ( p->chmode_bits.cd_low_discards_rx_data )
		{
			if ( p->wide_stat & MSR_CD )
				continue;
		}
		if ( p->chmode_bits.dsr_low_discards_rx_data )
		{
			if ( ! ( p->wide_stat & MSR_DSR ) )
				continue;
		}
		if ( p->chmode_bits.alert_flag_stops_rx_and_tx )
		{
			if ( p->chst_bits.alert )
				continue;
		}
		if ( p->chmode_bits.is_transmitter_xoffmode )
		{
			if ( chr == p->stop_xmt )
			{
				p->chst_bits.txiflag = 0;
				p->chst_bits.txwxon = 1;
				p->chst_bits.xchrun = 0;	
				continue;
			}
			else if ( chr == p->start_xmt )
			{
				if ( p->chmode_bits.is_txint )
				{
					linestat = p->base_8250 + LINE_STATUS_REGISTER;

					while ( ( serInp( linestat ) & LSR_THRE ) == 0 )
						;
					_serRestartTx( p );
					p->chst_bits.txwxon = 0;
				}
				continue;
			}
		}
		if ( p->chst_bits.rxfull )
		{
			p->chst_bits.rxovflow = 1;
		}
		else
		{
		/*
		* Put Character into RX Queue
		*/
			_disable();
			p->chst_bits.rxempty = 0;
			chr |= ( p->wide_stat << 8 );
			p->wide_stat &= 0xF0;
			if ( _serInsertChr( &p->rx, chr ) )
			{
				p->chst_bits.rxfull = 1;
			}
			_enable();
		}
		if ( p->chmode_bits.is_receiver_xoffmode )
		{
			if ( ! p->chst_bits.xoffsent )
			{
				if ( p->rx.count < p->rx_hiwater )
					continue;

				p->chst_bits.xoffsent = 1;

				while ( ( serInp( linestat ) & LSR_THRE ) == 0 )
					;
				serOutp( p->base_8250, p->stop_rem_xmt );
			}
		}
		if ( p->chmode_bits.is_rtscontrol )
		{
			if ( p->chst_bits.rtsactive )
			{
				int		addr;

				if ( p->rx.count < p->rts_hiwater )
					continue;
				p->chst_bits.rtsactive = 0;
				addr = p->base_8250 + MODEM_CONTROL_REGISTER;
				serOutp( addr, serInp( addr ) & ~MCR_REQUEST_TO_SEND );
			}
		}
	} while ( p->chmode_bits.is_16550 && 
			 ( serInp( linestat ) & LSR_DATA_READY ) != 0 );
}


PRIVATE void
_serIntLineStatus(
    PORT_TABLE	*p
	)
{
	int			addr;
	int			status;
	int			ier;
	int			hb;

	status = serInp( p->base_8250 + LINE_STATUS_REGISTER );
	hb = status << 8;
	p->line_stat = status | (~hb);
	p->wide_stat &= (~0x0f);
	p->wide_stat |= ( status & 0x0e );

	p->chst_bits.linerr = 1;
	if ( p->chmode_bits.line_errors_set_alert )
	{
		p->chst_bits.alert = 1;
		if ( p->chmode_bits.alert_flag_stops_rx_and_tx )
		{
			addr = p->base_8250 + INTERRUPT_ENABLE_REGISTER;
			ier = serInp( addr ) & ~IER_RECEIVE_DATA_INTERRUPT;
			serOutp( addr, ier );
			p->chst_bits.txiflag = 0;
			serOutp( addr, ier );
			p->chst_bits.xchrun = 0;
			p->chst_bits.rchrun = 0;
			p->chst_bits.txwalert = 1;
		}
	}
}

LOCAL void (*intjmp[] )(
    PORT_TABLE	*p
	) =
{
	_serIntModemStatus,
	_serIntTx,
	_serIntRx,
	_serIntLineStatus
};

PRIVATE void
serMcaDispatcher(
    PORTINFO	*pi
	)
{
	PORT_TABLE	*p;
	int			iid;
	int			port;
/*
* Do this for COM2..COM8
*/
	_enable();
	for ( port = COM2; port <= COM8 ; port++ )
	{
		if ( ( p = pi->ptb ) != NULL )
		{
			iid = serInp( p->base_8250 + INTERRUPT_ID_REGISTER ) & 0x07;
			while ( ( iid & 1 ) == 0 )
			{
				intjmp[ iid >> 1 ]( p );
				iid = serInp( p->base_8250 + INTERRUPT_ID_REGISTER ) & 0x07;
			}
		}
	}
}


PRIVATE void
serIsaDispatcher(
	PORT_TABLE	*p
	)
{
	int			iid;

	_enable();
	iid = serInp( p->base_8250 + INTERRUPT_ID_REGISTER ) & 0x07;
	while ( ( iid & 1 ) == 0 )
	{
		intjmp[ iid >> 1 ]( p );
		iid = serInp( p->base_8250 + INTERRUPT_ID_REGISTER ) & 0x07;
	}
	intjmp[ 1 ]( p );
}


PUBLIC int 
serFirst( 
	int		port,		// INPUT : Port
	UINT	mode,		// INPUT : 
    UINT	size_rxbuf, // INPUT : Sizeof RX Buffer
    UINT	size_txbuf,	// INPUT : Sizeof TX Buffer
	UINT	tx_limit,	// INPUT : Max Tx Chrs per Interrupt
	UINT	rx_trigger 	// INPUT : 16550 RX FIFO Setting
    )
{
    int 		base;
    int 		lcr;
    int 		mcr;
    int 		lsr;
    int 		msr;
    int 		temp_iir;
    PORT_TABLE	*p;
    unsigned 	rx_alloc;

	if ( ! gSerDataInitted ) 
		serInitData();

	if ( port < COM1 || port >= gPorts )
		return SER_ILLEGALPORT;

    if ( gChnl[ port ].ptb != NULL )
        return SER_INUSE;
/*
*  Determine sixe of Tx and Rx buffers
*/
    if ( mode & WIDETRACKRX ) 
    {
        if ( size_rxbuf > MAX_BUF_SIZE / 2 )
            return SER_INVBUFSIZE;
        rx_alloc = size_rxbuf * 2;
    } 
    else
        rx_alloc = size_rxbuf;


    if ( ( rx_alloc < MIN_BUF_SIZE )    ||
         ( size_txbuf < MIN_BUF_SIZE ) )
        return SER_INVBUFSIZE;

/*
*  Now allocate memory for the port's main parameter structure.
*/
    if ( ( p = (PORT_TABLE*) calloc( 1, sizeof( PORT_TABLE ) ) ) == NULL )
        return SER_NOMEMORY;

    gChnl[ port ].ptb = p;

/*
* Allocate memory for Tx and Rx buffers for the port.  *
*/
    if ( ( p->tx.buffer = (char*)calloc( 1, size_txbuf ) ) == NULL )
        return serFreeAll( port, SER_NOMEMORY );

    if ( ( p->rx.buffer = (char*)calloc( 1, rx_alloc ) ) == NULL )
        return serFreeAll( port, SER_NOMEMORY );

    p->tx.size = size_txbuf;
    p->rx.size = size_rxbuf;
    p->tx_limit = 14;
/* 
* Initialize all port status bits.
*/
    p->chst_bits.rxempty = p->chst_bits.txempty = 1;
    p->intrpt_num = gPcInt[ port ];
    p->base_8250  = g8250Port[ port ];
    p->tx.cell = 1;
    if ( mode & WIDETRACKRX ) 
    {
        p->rx.cell = 2;
        p->chmode_bits.is_rxerror = 1;
    } 
    else
        p->rx.cell = 1;
    if ( mode & ASIN )
        p->chmode_bits.is_rxint = 1;
    if ( mode & ASOUT )
        p->chmode_bits.is_txint = 1;
    if ( mode & ASCII )
        p->chmode_bits.is_ascii = 1;

    p->irq_8259  = g8259Irq[ port ];
    p->port_8259 = g8259ports[ port ];
    p->out12_mask = MCR_OUT2;
    p->ls_ms_ier = IER_LINE_STATUS_INTERRUPT + IER_MODEM_STATUS_INTERRUPT;

    base = p->base_8250;
/*
* The serInp reads in a character if there was one pending.
*/
    serInp( base + RECEIVE_BUFFER_REGISTER );
/*
* A not-sophisticated check to see if a part is really there.
*/
    if ( ( serInp( base + INTERRUPT_ID_REGISTER ) & 0x30 ) != 0 )
        return SER_NOUART;
/*
* The way we check to see if it is a 16550 is to try to enable the
* FIFOs, then see if they really got turned on.  If they did,
* the 16550 bit is set.  Note that if TRIGGER_DISABLE is the default
* setting, the is_16550 element will not get turned on.
*/
    p->p_8250[ 1 ] = 0;
	p->tx_limit = tx_limit;
	serSet16550Fifo( port, rx_trigger );
	if ( ( mode & USE_16550 ) && p->rx_16550_level != TRIGGER_DISABLE )
	{
        temp_iir = serInp( base + INTERRUPT_ID_REGISTER );
        serOutp( base + FIFO_CONTROL_REGISTER, p->rx_16550_level );

        if ( ( serInp( base + INTERRUPT_ID_REGISTER ) &
               IID_FIFO_ENABLED_MASK ) == IID_FIFO_ENABLED_MASK ) 
        {
            p->chmode_bits.is_16550 = 1;
            if ( ( temp_iir & IID_FIFO_ENABLED_MASK ) == IID_FIFO_ENABLED_MASK )
                p->p_8250[ 1 ] = ( p->rx_16550_level & 0xc0 ) + 0x20;
        } 
        else
		{
            serOutp( base + FIFO_CONTROL_REGISTER, 0 );
		}
	}
/*
* First I save off the LCR.  Then, I get the MCR and save it off as
* well.  I set the rtsactive bit if RTS is active.
*/
    lcr = serInp( base + LINE_CONTROL_REGISTER );
    p->p_8250[ 0 ] = lcr;
    serOutp( base + LINE_CONTROL_REGISTER, lcr & ~LCR_DIVISOR_LATCH_ACCESS );
    mcr = serInp( base + MODEM_CONTROL_REGISTER );
    p->p_8250[ 1 ] |= mcr & 0x1f;
    if ( mcr & MCR_REQUEST_TO_SEND )
        p->chst_bits.rtsactive = 1;
/*
* Next I read in and save off the IER, and the two bytes of the Divisor
* Latch.
*/
    p->p_8250[ 2 ] = serInp( base + INTERRUPT_ENABLE_REGISTER );
    serOutp( base + INTERRUPT_ENABLE_REGISTER, 0 );
    serOutp( base + LINE_CONTROL_REGISTER, lcr | LCR_DIVISOR_LATCH_ACCESS );
    p->p_8250[ 3 ] = serInp( base + DIVISOR_LATCH_LSB );
    p->p_8250[ 4 ] = serInp( base + DIVISOR_LATCH_MSB );
    serOutp( base + LINE_CONTROL_REGISTER, lcr & ~LCR_DIVISOR_LATCH_ACCESS );
/*
* Now I save my first copies of the line status and the modem status.
*/
    lsr = serInp( base + LINE_STATUS_REGISTER );
    p->line_stat = lsr;
    msr = serInp( base + MODEM_STATUS_REGISTER );
    p->wide_stat = ( msr & 0xf0 ) | ( lsr & 0x0e );
/*
* Next, I take the 8250 interrupt line out of tri-state mode with
* interrupts disabled.  This should guarantee that no false interrupts
* occur.
*/
    serOutp( base + INTERRUPT_ENABLE_REGISTER, 0 );
    serOutp( base + INTERRUPT_ENABLE_REGISTER, 0 );
    mcr &= ~( MCR_OUT1 + MCR_OUT2 + MCR_LOOPBACK );
    mcr |= p->out12_mask;
    serOutp( base + MODEM_CONTROL_REGISTER, mcr );

    if ( gChnl[ port ].handler_type == INT_SHARED_IRQ_HANDLER ) 
    {
/*
* Right now the shared handler is only being used for MicroChannel COM2-COM8
* I don't even check to see if the Hook fails, because I don't care, and it
* saves a few lines of code.  Note that I don't have a tear down function
* associated with the handler.  There is no way the user can uninstall
* the handler, so I rely on the port closing hook to shut things down.
*/
        HookInterrupt(
                       p->intrpt_num,
                       serMcaDispatcher,
//                       0,
                       (void *) &gChnl[ 1 ],
                       0,
                       0x20,
                       ( p->port_8259 == 0x20 ) ? 0 : p->port_8259,
                       1 << p->irq_8259 );
    } 
    else if ( gChnl[ port ].handler_type == INT_STANDARD_HANDLER ) 
    {
        HookInterrupt( 
                       p->intrpt_num,
                       serIsaDispatcher,
//                       0,
                       (void *) p,
                       0,
                       0x20,
                       ( p->port_8259 == 0x20 ) ? 0 : p->port_8259,
                       1 << p->irq_8259 );
    }
    serInp( base + RECEIVE_BUFFER_REGISTER ); /* Just in case there is a char pending */
/*
* Finally, I take the steps needed to turn on interrupts.
*/
    serInp( base + INTERRUPT_ID_REGISTER );
    _disable();
    serOutp( base + INTERRUPT_ENABLE_REGISTER, p->ls_ms_ier );
    serOutp( base + INTERRUPT_ENABLE_REGISTER, p->ls_ms_ier );
    _enable();
    return SER_OKAY;
}


#define MAXIMUM_HANDLERS 8
/*
*  Note that real far pointers are saved as unsigned longs.  This
*  prevents protected mode code from accidentally choking if and
*  when it treats a real mode address as a pointer
*/

typedef struct 
{
    void 		( __interrupt __far *old_handler )( void );
    DWORD		old_real_handler;
    void 		( *handler )( void *data );
    DWORD		real_handler;
    void 		*data;
    DWORD		real_data;
    void 		( *tear_down_function )( void *data );
    int 		interrupt_number;
    int 		saved_8259_bit;
    int 		primary_8259;
    int 		secondary_8259;
    int 		mask_8259;
    int 		int_count;
    int 		real_int_count;
} IRQDATA;


LOCAL IRQDATA _irqtable[ MAXIMUM_HANDLERS ];

PRIVATE void __interrupt _irq_prot_stub_0( void ) { dispatcher( 0 ); }
PRIVATE void __interrupt _irq_prot_stub_1( void ) { dispatcher( 1 ); }
PRIVATE void __interrupt _irq_prot_stub_2( void ) { dispatcher( 2 ); }
PRIVATE void __interrupt _irq_prot_stub_3( void ) { dispatcher( 3 ); }
PRIVATE void __interrupt _irq_prot_stub_4( void ) { dispatcher( 4 ); }
PRIVATE void __interrupt _irq_prot_stub_5( void ) { dispatcher( 5 ); }
PRIVATE void __interrupt _irq_prot_stub_6( void ) { dispatcher( 6 ); }
PRIVATE void __interrupt _irq_prot_stub_7( void ) { dispatcher( 7 ); }

LOCAL void ( __interrupt *_dispatcher_stubs[ MAXIMUM_HANDLERS ] )( void ) =
{
	_irq_prot_stub_0,	
	_irq_prot_stub_1,	
	_irq_prot_stub_2,	
	_irq_prot_stub_3,	
	_irq_prot_stub_4,	
	_irq_prot_stub_5,	
	_irq_prot_stub_6,
	_irq_prot_stub_7	
};


PRIVATE void 
dispatcher( 
	int 	i 
	)
{
    _irqtable[ i ].handler( _irqtable[ i ].data );
    _disable();
    if ( _irqtable[ i ].secondary_8259 != 0 )
        serOutp( _irqtable[ i ].secondary_8259, 0x20 );
    if ( _irqtable[ i ].primary_8259 != 0 )
        serOutp( _irqtable[ i ].primary_8259, 0x20 );
}


/*
 *  int UnHookInterrupt( int interrupt_number )
 *
 *  ARGUMENTS
 *
 *   interrupt_number : The number of the interrupt that needs to be
 *                      unhooked from the system.
 *
 *  DESCRIPTION
 *
 *   When a routine is done with a particular interrupt, it calls this
 *   routine to unhook things.  This routine first calls the teardown
 *   function for the particular interrupt, then takes care of the
 *   unhooking.  It also takes care of the 8259 enable/disable bits
 *   if it is a hardware interrupt.
 *
 *  SIDE EFFECTS
 *
 *   None.
 *
 *  RETURNS
 *
 *   A status code, hopefully SER_OKAY if things go well.
 */

PRIVATE int 
UnHookInterrupt( 
	int 	interrupt_number 
	)
{
    int i;
    int temp;

    for ( i = 0 ; i < MAXIMUM_HANDLERS ; i++ ) 
    {
        if ( _irqtable[ i ].interrupt_number == interrupt_number ) 
        {
            if ( _irqtable[ i ].handler == NULL )
				break;
				
            if ( _irqtable[ i ].tear_down_function )
                _irqtable[ i ].tear_down_function( _irqtable[ i ].data );

            _irqtable[ i ].interrupt_number = 0;

			_dos_setvect( interrupt_number, _irqtable[ i ].old_handler );

            if ( _irqtable[ i ].secondary_8259 != 0 ) 
            {
                temp = serInp( _irqtable[ i ].secondary_8259 + 1 );
                temp &= ~_irqtable[ i ].mask_8259;
                temp |= _irqtable[ i ].saved_8259_bit;
                serOutp( _irqtable[ i ].secondary_8259 + 1, temp );
            } 
            else if ( _irqtable[ i ].primary_8259 != 0 ) 
            {
                temp = serInp( _irqtable[ i ].primary_8259 + 1 );
                temp &= ~_irqtable[ i ].mask_8259;
                temp |= _irqtable[ i ].saved_8259_bit;
                serOutp( _irqtable[ i ].primary_8259 + 1, temp );
            }
            return SER_OKAY;
        }
    }
    return SER_ILLEGALIRQ;
}

/*
 * void  _unhook_everything( void )
 *
 *  ARGUMENTS
 *
 *   None.
 *
 *  DESCRIPTION
 *
 *  This is a function that gets hooked into the chain of routines
 *  called at exit time by atexit().  It goes through and unhooks every
 *  interrupt currently taken over by the system.
 *
 *  This is nice, because every part of CommLib that uses interrupts
 *  can now forget about having to clean up after themselves.  This guy
 *  takes care of it, as well as calling the cleanup routine specified
 *  when the interrupt was hooked.
 *
 *  Before unhooking all the interrupts, the routine to close all the
 *  open ports is called.  This saves some trouble.
 *
 *  SIDE EFFECTS
 *
 *   An enormous side effect.
 *
 *  RETURNS
 *
 *   Nothing.
 */

PRIVATE void  
_unhook_everything( 
	void 
	)
{
    int		i;
/*
 * By virtue of being here, I know that we are going to shut down, so
 * I call the routine that closes all the ports.
 */
	serClosePort( -1 );
    for ( i = MAXIMUM_HANDLERS - 1 ; i >= 0 ; i-- )
    {
        if ( _irqtable[ i ].interrupt_number != 0 )
            UnHookInterrupt( _irqtable[ i ].interrupt_number );
    }
}

/*
 *  int HookInterrupt( int interrupt_number,
 *                     void ( GF_CDECL GF_FAR *handler )( void GF_FAR *data ),
 *                     void ( GF_CDECL GF_FAR *real_handler )( void GF_FAR *data ),
 *                     void GF_FAR *data,
 *                     void ( GF_CONV *tear_down_function )( void GF_FAR *real_data ),
 *                     int primary_8259,
 *                     int secondary_8259,
 *                     int mask_8259 )
 *
 *
 *  ARGUMENTS
 *
 *   interrupt_number       : The interrupt number that the calling function
 *                            wants to take over.  This can either be
 *                            a hardware or software interrupt, it makes no
 *                            difference.
 *
 *   handler                : A pointer to the interrupt handler.  The handler
 *                            has to be a far cdecl, but it doesn't have to
 *                            be an _interrupt routine, in fact it shouldn't
 *                            be one.  The IRQ manager dispatcher takes care
 *                            of all the interrupt type stuff, like saving
 *                            registers and so on.  The only thing this routine
 *                            might need to do is load DS with DGROUP, and
 *                            then only if global data is being accessed.
 *
 *  real_handler            : A pointer to the real mode handler.  You only
 *                            need to supply a real pointer here if you are
 *                            running under a DOS Extender.  Otherwise a
 *                            parameter of 0 will do just fine.
 *
 *  data                    : A pointer to a block of data used by the ISR.
 *                            The IRQ manager dispatcher will pass this as
 *                            a parameter to your interrupt handler when it
 *                            is called.  This basically means the interrupt
 *                            handler gets things on a silver platter.
 *
 * tear_down_function       : When the interrupt is unhooked, either at exit()
 *                            time or when the UnHook..() routine is called,
 *                            it will always call your teardown function
 *                            first.  This can let you clean things up
 *                            that need to be cleaned up before the interrupt
 *                            is unhooked.  This will frequently consist
 *                            of closing all the ports on a multiport board,
 *                            or something like that.
 *
 * primary_8259             : The I/O address of the primary 8259 for this
 *                            interrupt.  For PC hardware interrupts, this
 *                            will always be 0x20.  For softare interrupts,
 *                            just put a 0 here.  This data is needed so
 *                            the IRQ dispatcher can send an EOI to the PIC
 *                            once your handler has finished.
 *
 * secondary_8259           : If you are using one of the high IRQ lines on
 *                            the PC, you will need to specify a secondary
 *                            IRQ as well.  This will always be 0x70 for PCs.
 *
 * mask_8259                : The bit in the 8259 that corresponds to the
 *                            hardware interrupt you are using.
 *
 *  DESCRIPTION
 *
 *   This function takes care of hooking the interrupt you specify to your
 *   handler.  Every time time interrupt fires off, your handler will be
 *   called with a pointer to the data block you specify.  The details of
 *   this all occur in this routine and in the dispatcher routine, you just
 *   get to sit back and relax.
 *
 *  SIDE EFFECTS
 *
 *   The unhook_everything() routine gets added to the atexit() stack.
 *
 *  RETURNS
 *
 *   Either ASSUCCESS, or one of zillions of possible error codes.
 *
 *  AUTHOR
 *
 *   SM          December 12, 1992
 *
 *  MODIFICATIONS
 *
 *   December 12, 1992     4.00A : Initial release
 *
 */

PRIVATE int
HookInterrupt( 
	int		interrupt_number,
    void 	( *handler )( void *data ),
//    void 	( *real_handler )( void *data ),
    void 	*data,
    void 	( *tear_down_function )( void *real_data ),
    int 	primary_8259,
    int 	secondary_8259,
    int 	mask_8259 
    )
{
    LOCAL int first_time = 1;

    int 	i;
    int 	temp;
/*
 * The dispatcher_stubs[] and real_dispatcher_stubs[] are arrays of
 * pointers to the tiny stub routines that actually get hooked to
 * interrupt vector.  The stub routines just load a parameter and then
 * call the main routine.  The DOS Extenders need to make sure that these
 * little stubs are locked in place in memory.  They also need to be
 * sure that your protected mode handler gets locked in memory as well.
 */
    if ( first_time ) 
    {
        first_time = 0;
        atexit( _unhook_everything );
    }
/*
 * I need to check to see if somebody else has already hooked the interrupt.
 * If so, I return an error.  Also, I can only handle so many interrupts.
 * If you have already used them all up, I return a different error.
 */
    for ( i = 0 ; i < MAXIMUM_HANDLERS ; i++ )
    {
        if ( _irqtable[ i ].interrupt_number == interrupt_number )
            return SER_INUSE;
    }
/*
 * This section of code contains the meat of the routine.  When an interrupt
 * is hooked, all of the important data about the interrupt goes into the
 * _irqtable[] structure.  When the interrupt actually occurs, the
 * dispatcher looks in this structure to figure out just what to do with
 * the interrupt.  Everything it needs to know is stored there.  The
 * slot in the _irqtable[] are assigned as they are needed.  When an
 * interrupt is assigned to slot "i", it's vector is directed to the
 * stub routine "i".  Stub routine "i" is what actually gets jumped to
 * when the interrupt occurs.  All it does is load up the value "i", then
 * jump to the common dispatcher.  The dispatcher then looks in _irqtable[i]
 * to figure out what to do.  Very simple, right?
 */
    for ( i = 0 ; i < MAXIMUM_HANDLERS ; i++ )
	{
        if ( _irqtable[ i ].interrupt_number == 0 )
		{
            _irqtable[ i ].handler = handler;
            _irqtable[ i ].data = data;
            _irqtable[ i ].handler = handler;

            _irqtable[ i ].real_data = 0;
            _irqtable[ i ].tear_down_function = tear_down_function;
            _irqtable[ i ].primary_8259 = primary_8259;
            _irqtable[ i ].secondary_8259 = secondary_8259;
            _irqtable[ i ].mask_8259 = mask_8259;
            _irqtable[ i ].interrupt_number = interrupt_number;
            _irqtable[ i ].int_count = 0;
            _irqtable[ i ].real_int_count = 0;

			_irqtable[ i ].old_handler = (void *) _dos_getvect( interrupt_number );
		    _dos_setvect( interrupt_number, _dispatcher_stubs[ i ] );
/*
 * Finally (!) if this is a hardware interrupt, we enable it at the
 * 8259 PIC, so it can really start firing.
 */
            if ( secondary_8259 != 0 ) 
            {
                temp = serInp( secondary_8259 + 1 );
                _irqtable[ i ].saved_8259_bit = temp & mask_8259;
                serOutp( secondary_8259 + 1, temp & ~mask_8259 );
            } 
            else if ( primary_8259 != 0 ) 
            {
                temp = serInp( primary_8259 + 1 );
                _irqtable[ i ].saved_8259_bit = temp & mask_8259;
                serOutp( primary_8259 + 1, temp & ~mask_8259 );
            }
            return SER_OKAY;
        }
    }
    return SER_ILLEGALIRQ;
}

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
	)			
{
    int 	rc;

    if ( ( rc = serFirst( port, mode, size_rx_buf, size_tx_buf,
    					tx_limit, rx_trigger ) ) != SER_OKAY )
        return rc;

	rc = serInitPort( port, baud, wordlength, stopbits, parity );
	rc = serStart( port, mode & ( ASIN | ASOUT | ASINOUT ) );
	rc = serSetDtr( port, 1 );
	rc = serSetRts( port, 1 );

	if ( ( rc = serInitPort( port, baud, wordlength, stopbits, parity ) ) == SER_OKAY 
	   &&( rc = serStart( port, mode & ( ASIN | ASOUT | ASINOUT ) ) ) == SER_OKAY
	   &&( rc = serSetDtr( port, 1 ) ) >= SER_OKAY 
	   &&( rc = serSetRts( port, 1 ) ) >= SER_OKAY )
		return SER_OKAY;

    serClosePort( port );
    return rc;
}


PRIVATE int 
_serDown( 
	int		port		// INPUT : Port
	)
{
    PORT_TABLE	*p;
    int 		temp;
    int 		i;

	if ( port < 0 || port >= gPorts )
		return SER_ILLEGALPORT;

	if ( ( p = serGetPortTable( port ) ) == NULL )
		return SER_ILLEGALPORT;

    serOutp( p->base_8250 + INTERRUPT_ENABLE_REGISTER, 0 );
    serOutp( p->base_8250 + LINE_CONTROL_REGISTER, LCR_DIVISOR_LATCH_ACCESS );
    serOutp( p->base_8250 + DIVISOR_LATCH_LSB, p->p_8250[ 3 ] );
    serOutp( p->base_8250 + DIVISOR_LATCH_MSB, p->p_8250[ 4 ] );
    serOutp( p->base_8250 + LINE_CONTROL_REGISTER, p->p_8250[ 0 ] );
    serOutp( p->base_8250 + MODEM_CONTROL_REGISTER, p->p_8250[ 1 ] & 0x1f );
/*
 * The three important FCR bits are packed into the upper three unused bits
 * of the MCR.  I unpack them here and stuff them into the FCR.
 */
    if ( p->chmode_bits.is_16550 ) 
   	{
        temp = ( p->p_8250[ 1 ] & 0x20 ) >> 5;
        temp |= p->p_8250[ 1 ] & 0xc0;
        serOutp( p->base_8250 + FIFO_CONTROL_REGISTER, temp );
    }        
		
    if ( gChnl[ port ].handler_type == INT_STANDARD_HANDLER )
        UnHookInterrupt( p->intrpt_num );
    else if ( gChnl[ port ].handler_type == INT_SHARED_IRQ_HANDLER ) 
    {
        temp = 0;
        for ( i = COM2 ; i <= COM8 ; i++ )
        {
            if ( gChnl[ i ].ptb )
                temp++;
        }
        if ( temp == 1 )
        {
			UnHookInterrupt( p->intrpt_num );
        }
    }
/*
 * Now that we have restored the old ISR (if there was one), we just
 * set the IER back to its old value.
 */
    serOutp( p->base_8250 + INTERRUPT_ENABLE_REGISTER, p->p_8250[ 2 ] );

	return serFreeAll( port, SER_OKAY );
}


/***************************************************************************
* serClosePort - Close an opened port.
***************************************************************************/
PUBLIC int 
serClosePort( 
	int		port		// INPUT : Port
	)
{
    if ( port == -1 )
    {
        for ( port = MAX_PORTS -1 ; port >= 0 ; port-- )
            _serDown( port );
	    return SER_OKAY;
    }
	return _serDown( port );
}


