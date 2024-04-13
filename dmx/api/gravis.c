/***************************************************************************
* GRAVIS.C - Gravis Ultrasound Support for F/X and MIDI
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*             Copyright (C) 1993,94 Digital Expressions, Inc. 		     
*                        All Rights Reserved.		   		     
*----------------------------------------------------------------------------
* "This software is furnished under a license and may be used only 	     
* in accordance with the terms of such license and with the inclusion 	     
* of the above copyright notice. This software or any other copies 	     
* thereof may not be provided or otherwise made available to any other 	     
* person. No title to and ownership of the software is hereby transfered."   
*----------------------------------------------------------------------------
*                     Written by Paul J. Radek
*----------------------------------------------------------------------------
* $Log:$
*  
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>
#include <conio.h>
#include <dos.h>
#include <fcntl.h>
#include <io.h>

#include "gf1api.h"
#include "gf1.h"
#include "gf1scale.h"
#include "gf1vol.h"
#include "gf1note.h"

#include "dspapi.h"
#include "dmx.h"
#include "pcdma.h"
#include "mixers.h"

/*-------------------------------------------------------------------------*
*                               D E F I N E S
*--------------------------------------------------------------------------*/
#define MAX_BUFFS			2

#define STAT_UNUSED			0	/* voice unused */
#define STAT_PLAYING		1	/* voice playing sound */
#define STAT_PAUSED			2	/* voice paused or awaiting next dma xfer */
#define STAT_USER_PAUSED	3	/* voice paused by user */
#define STAT_MASK			3
#define DMASTAT_WORKING		4	/* dma in progress */
#define DMASTAT_PENDING 	8	/* application has requested a new dma transfer
								   but dma channel was busy */
#define STAT_SET_ACCUM  	16	/* reset accumulator */
#define STAT_EXTRA_VOICE	32	/* This voice is used to generate interrupts
				   				   for another voice */

#define BUFSTAT_FILLED		1	/* buffer has data */
#define BUFSTAT_DONE		2	/* when done playing send voice_done callback */

#define GF1_FREQ_DIVISOR    22050L
#define MAX_CHANNELS 		32
#define MAX_PATCHES	 		175

#define STAT_VOICE_RUNNING		BIT0
#define STAT_VOICE_SUSTAINING	BIT1
#define STAT_VOICE_RELEASING	BIT2
#define STAT_VOICE_STOPPING		BIT3

#define CHANNEL_SUSTAIN		1
#define NOTE_SUSTAIN 		1
#define NOTE_LEGATO  		2
#define MID_PITCH 			8192

#define CACHE_SIZE			8192
#define DMA_SIZE			8192

#define MASTER_VOLUME 	115 /* range 0 - 127 */

#define NOTINUSE 0
#define INUSE    1

#ifdef PROD
void gf1Enter( void );
int gf1EnterBusy( void );
#else
void _gf1Enter( int );
int _gf1EnterBusy( int );
#define gf1Enter()		_gf1Enter( __LINE__ )
#define gf1EnterBusy()	_gf1EnterBusy( __LINE__ )
#endif

/*-------------------------------------------------------------------------*
*                           S T R U C T U R E S
*--------------------------------------------------------------------------*/
typedef struct
{
	DWORD		base;
	DWORD		memory;
	DWORD		used;
} GF1_BANK;

typedef struct
{
	DWORD		age;
	UINT		priority;
	void		( *steal_notify )( int );
} GF1_VOICE;

typedef struct
{
	BYTE		*ptr;
	DWORD 		size;
	WORD		addr;
	BYTE 		dma_control;
} GF1_DMA;

typedef struct 
{
	GF1_WAVE	*wave;
	long 		percent_complete;
	WORD 		ofc_reg;
	WORD 		fc_register;
	short		vibe_fc_offset;
	BYTE 		trem_start;
	BYTE 		trem_end;
	BYTE 		trem_volinc;
	WORD 		attenuation;	/* 0 - 255: attenuates envelopes */
	BYTE 		status; 		/* status of voice */
	BYTE 		voice_control;
	BYTE 		volume_control;
	BYTE 		extra_envelope_point;
	BYTE 		vibe_ticker;
	BYTE 		vibe_sweep;
	BYTE 		vibe_quarter;
	BYTE 		vibe_count;
	BYTE 		envelope_count;
	BYTE 		velocity;
	BYTE 		channel;
	BYTE 		sust_offset;
	BYTE 		next_offset;
	BYTE 		prev_offset;
} VOC_STATUS;

typedef struct
{
    BYTE 		flags;
    BYTE 		volume;
    WORD 		pitch_bend; /* 1024 means no change.  range = 512 - 2048 */
    GF1_PATCH	*patch;
    BYTE 		balance;
    BYTE 		vib_depth;
    BYTE 		vib_rate;
    BYTE 		vib_sweep;
    BYTE 		trem_depth;
    BYTE 		trem_rate;
    BYTE 		trem_sweep;
    char 		dummy[ 1 ];
} CHAN_STATUS;

typedef struct
{
    char flags;
    char note;
    char channel;
    char dummy;
} NOTE_STATUS;

/*-------------------------------------------------------------------------*
*                               M A C R O S
*--------------------------------------------------------------------------*/
#define	ADDR_HIGH(x) 	((WORD)(((x)>>7)&0x1fffL))
#define	ADDR_LOW(x)  	((WORD)(((x)&0x7fL)<<9))
#define ADDR_16BIT(x)	(((x)&0xfffc0000L)|(((x)&0x0003ffffL)>>1))
#define DMA_ACTIVE		( gf1_dma_active ? TRUE : FALSE )
#define DMA_IDLE		( gf1_dma_active ? FALSE : TRUE )
#define GET_IRQ_STATUS 	(inp(gf1_status_register)&0xE8)

/*-------------------------------------------------------------------------*
*                         L O C A L   V A R I A B L E S
*--------------------------------------------------------------------------*/
LOCAL GF1_BANK		gf1_bank[ 4 ];
LOCAL int			gf1_banks;
LOCAL int			gf1_max_banks;

LOCAL GF1_VOICE		gf1_voice[ MAX_VOICES ];
LOCAL DWORD			voice_mask;
LOCAL DWORD			voice_age;

LOCAL GF1_DMA		gf1_dma_obj;
LOCAL int 		   	gf1_dma_active;					 
LOCAL BYTE			gf1_dma_channel;
LOCAL BYTE			*gf1_dma_handle;
LOCAL BYTE          *gf1_dma_buffer;

LOCAL int 			(FAR *timer2_handler)() = NULL;

LOCAL DWORD			dig_addr;
LOCAL int			dig_left_voice;
LOCAL int			dig_right_voice;
LOCAL BYTE			dig_volume_control;
LOCAL BYTE			dig_voice_control;
LOCAL int			dig_status;
LOCAL WORD 	        dig_dma_control;

LOCAL int 			gf1_os_loaded = 0;
LOCAL int 			gf1_irq_latch[] = { 0, 0, 1, 3, 0, 2, 0, 4, 0, 0, 0, 5, 6, 0, 0, 7 };
LOCAL int			gf1_dma_latch[] = { 0, 1, 0, 2, 0, 3, 4, 5 };

LOCAL volatile int	gf1_voice_semaphore;
LOCAL volatile int	gf1_flags;

LOCAL int 			gf1_adlib_timer_mask;
LOCAL int 			gf1_adlib_control;
LOCAL int			gf1_mixer_mask;
LOCAL BYTE 			*dma_buffer;

LOCAL INT		    Gf1Ready = 0;
LOCAL INT		    PatchesLoaded = 0;
LOCAL BYTE 		    priority[ 16 ] = { 1, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8, 8, 8, 8, 1 };
LOCAL CHAR 		    config_name[] = "DMXGUS.INI";
LOCAL GF1_OS	    os_parms;

LOCAL VOC_STATUS 	voice_status[ MAX_VOICES ];
LOCAL CHAN_STATUS	channel_status[ MAX_CHANNELS ];
LOCAL NOTE_STATUS	note_status[ MAX_VOICES ];

LOCAL short 		other_voices[ 32 ][ 4 ]; /* layered voices group */
LOCAL short 		gf1_m_volume;
LOCAL short 		vibrato_voice_count; /* number of voices modulating */
LOCAL char 			gf1_linear_volumes = 1;
LOCAL GF1_PATCH		*gf1_patch[ MAX_PATCHES ];
LOCAL BYTE			program_num[ MAX_PATCHES ];
LOCAL BYTE			xref[ MAX_PATCHES ];
LOCAL BYTE			*gf1_map = NULL;
LOCAL VOID			(*gf1Stable)(void) = NULL;

LOCAL long			gf1_log_table[] =
{ 1024, 1085, 1149, 1218, 1290, 1367, 1448, 1534, 1625, 1722, 1825, 1933 };

/*
** Pitch bend sensitivity is 2 semitones by default
*/
LOCAL WORD			pbs_msb[ 16 ] = {2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2};
LOCAL WORD			pbs_lsb[ 16 ] = {0};

LOCAL WORD			rpn_msb[ 16 ] = {0};
LOCAL WORD			rpn_lsb[ 16 ] = {0};
/*
** This array keeps track of channel volumes.  Default is 127 for each
** channel.
*/
LOCAL int			channel_volume[ 16 ] = { 100,100,100,100,100,100,100,100,
											 100,100,100,100,100,100,100,100 };

LOCAL int			channel_expression[16] = { 127,127,127,127,127,127,127,127,
											   127,127,127,127,127,127,127,127 };
LOCAL WORD        	blk_smoothing;
LOCAL WORD        	blk_offset;
LOCAL WORD			blk_size;


/*-------------------------------------------------------------------------*
*                     G F 1   R E G I S T E R   I / O     
*--------------------------------------------------------------------------*/
PRIVATE void
gf1OutByte(
	int				index,		// INPUT: Register Select
	BYTE			data		// INPUT: Data to be written
	)
{
	outp( gf1_register_select, index );
	outp( gf1_data_high, data );
}

PRIVATE void
gf1OutWord(
	int				index,		// INPUT: Register Select
	WORD			data		// INPUT: Data to be written
	)
{
	outp( gf1_register_select, index );
	outpw( gf1_data_low, data );
}

PRIVATE BYTE
gf1InByte(
	int				index		// INPUT: Register Select
	)
{
	outp( gf1_register_select, index );
	return inp( gf1_data_high );
}

PRIVATE WORD
gf1InWord(
	int				index		// INPUT: Register Select
	)
{
	outp( gf1_register_select, index );
	return inpw( gf1_data_low );
}

PRIVATE VOID
gf1SelectVoice(
    int             voice       // INPUT: Voice to Select
    )
{
	outp( gf1_page_register, voice );
}

/*-------------------------------------------------------------------------*
*                    G F 1   M E M O R Y   M A N A G E R
*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
* gf1Peek - Peek into DRAM and return byte.
*---------------------------------------------------------------------------*/
PRIVATE BYTE
gf1Peek(
    DWORD       address		// INPUT: GF1 DRAM address to read from
    )
{
	gf1OutWord( SET_DRAM_LOW, LOWORD( address ) );
	gf1OutByte( SET_DRAM_HIGH, LOBYTE( ( address >> 16 ) ) );
	return inp( gf1_dram_io );
}

/*--------------------------------------------------------------------------*
* gf1Poke - Poke a BYTE of data into DRAM.
*---------------------------------------------------------------------------*/
PRIVATE void
gf1Poke(
    DWORD       address,	// INPUT: GF1 DRAM address to write to
    BYTE        data		// INPUT: Byte to write
    )
{
	gf1OutWord( SET_DRAM_LOW, LOWORD( address ) );
	gf1OutByte( SET_DRAM_HIGH, LOBYTE( ( address >> 16 ) ) );
	outp( gf1_dram_io, data );
}

/*--------------------------------------------------------------------------*
* gf1TestDRAM - Test if supplied address on GF1 card is valid
*---------------------------------------------------------------------------*/
PRIVATE BOOL	// Returns: TRUE =GOOD DRAM, FALSE = BAD DRAM
gf1TestDRAM(
    DWORD       address		// INPUT: GF1 DRAM address to test
    )
{
	LOCAL BYTE	pb1[] = { 0x55, 0x00, 0xAA, 0xFF };
	LOCAL BYTE	pb2[] = { 0x00, 0x55, 0x00, 0x00 };
	DWORD       wraparound_loc;
	DWORD       addr20;
	WORD        bank;
	DWORD       tbank;
    BYTE        val;
	BYTE        old_byte;
	BYTE        old_byte1;
	BYTE        old_byte_20;
	int			j;
	
/* writing to two locations will prevent the bus from holding on
	** to the last value written.  addr20 is the second address */
    addr20 = address ^ 0x20;

	old_byte = gf1Peek( address );
	old_byte_20 = gf1Peek( addr20 );

	for ( j = 0; j < ASIZE( pb1 ); j++ )
	{
		gf1Poke( address, pb1[ j ] );
		gf1Poke( addr20, pb2[ j ] );
        val = gf1Peek( address );
		if ( val != pb1[ j ] )
		{
			gf1Poke( address, old_byte );
			gf1Poke( addr20, old_byte_20 );
        	return FALSE;
		}
	}

/* Check for wrap around effect. */
	if ( address >= 0x40000L )
    {
	/* Now write to the corresponding address in the other banks. */
		bank = address >> 18;
		for ( tbank = 0; tbank < 4; tbank++ )
    	{
			if ( tbank == bank )
            	break;

			wraparound_loc = ( address % 0x40000L ) | ( tbank << 18 );
            val = gf1Peek( address );
			old_byte1 = gf1Peek( wraparound_loc );
			gf1Poke( wraparound_loc, 0xaa );
			gf1Poke( addr20, 0x00 );
			if ( gf1Peek( address ) == 0xaa )
        	{
				gf1Poke( address, old_byte );
				gf1Poke( addr20, old_byte_20 );
				return FALSE;
			}
			gf1Poke( wraparound_loc, old_byte1 );
		}
	}
	gf1Poke( address, old_byte );
	gf1Poke( addr20, old_byte_20 );
	return TRUE;
}

/*--------------------------------------------------------------------------*
* gf1Malloc - Allocate bytes on GF1 card.
*---------------------------------------------------------------------------*/
PRIVATE DWORD		// Returns: GF1 DRAM Address, 0 = Failure
gf1Malloc(
	DWORD		size		// INPUT: Amount of audio RAM to allocate
	)
{
	DWORD		addr;
	int			j;
	int			use;
	DWORD		avail;
	DWORD		smallest;
/*
* adjust size so that it is a multiple of 32
*/
	size = ( size + 31 ) & 0x00FFFFE0L;
	use = -1;
	smallest = ~0;
	for ( j = 0; j < gf1_banks; j++ )
	{
		avail = gf1_bank[ j ].memory - gf1_bank[ j ].used;
		if ( avail >= size && avail < smallest )
		{
			smallest = avail;
			use = j;
		}
	}
	if ( use < 0 )
    {
        if ( DmxDebug & DMXBUG_MESSAGES )
        {
            printf( " out of GUS RAM:", size );
            for ( j = 0; j < gf1_banks; j++ )
            {
                printf( "%d%c", gf1_bank[ j ].memory - gf1_bank[ j ].used,
					( j == gf1_banks - 1 ) ? ' ' : ',' );
            }
        }
		return 0L;
    }

	addr = gf1_bank[ use ].base + gf1_bank[ use ].used;
	gf1_bank[ use ].used += size;
	return addr;
}

/*-------------------------------------------------------------------------*
*                    G F 1   V O I C E   M A N A G E R
*--------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------*
* gf1AllocVoice - Allocate voice for usage
*--------------------------------------------------------------------------*/
PRIVATE int		// Returns: NO_MORE_VOICES(-1) or 0..MAX_VOICES-1
gf1AllocVoice(
    int     priority,		// INPUT : 0 = Highest, >0 = Lower
    void    (*steal_notify)(int voice)
    )
{
    int 			v;
    DWORD			minage = ~0; 
    UINT			maxpriority = 1; /* 0 priority won't get stolen */
    int 			steal_voice = NO_MORE_VOICES;

    for ( v = 0; v < gf1_voices; v++ )
    {
		if ( ( voice_mask & ( 1L << v ) ) == 0 )
			break;

		if ( gf1_voice[ v ].priority >= maxpriority )
		{
			if ( gf1_voice[ v ].priority > maxpriority ||
	    	     gf1_voice[ v ].age < minage
			   )
			{
		    	minage = gf1_voice[ v ].age;
		    	steal_voice = v;
			}
			maxpriority = gf1_voice[ v ].priority;
		}
    }
	if ( v == gf1_voices )
	{
    	if ( steal_voice == NO_MORE_VOICES )
			return NO_MORE_VOICES;

		if ( gf1_voice[ steal_voice ].steal_notify )
		{
	    	gf1_voice[ steal_voice ].steal_notify( steal_voice );
		}
		v = steal_voice;
	}
	voice_mask |= ( 1L << v );
	gf1_voice[ v ].priority = priority;
	gf1_voice[ v ].age = voice_age++;
	gf1_voice[ v ].steal_notify = steal_notify;
	return v;
}

/*-------------------------------------------------------------------------*
* gf1FreeVoice - Free Voice from Use
*--------------------------------------------------------------------------*/
PRIVATE void
gf1FreeVoice(
	int			voice_id	// INPUT : Voice ID 0..MAX_VOICES-1
	)
{
    DWORD		bit;

    bit = ( 1L << voice_id );
    voice_mask &= ~bit;
}

/*-------------------------------------------------------------------------*
* gf1AdjustPriority - Adjust voice priority.
*--------------------------------------------------------------------------*/
PRIVATE void
gf1AdjustPriority(
	int			voice_id,	// INPUT : Voice ID 0..MAX_VOICES-1
	int 		priority	// INPUT : New priority 0..MAX_INT
	)
{
	if ( voice_id >= 0 && voice_id < MAX_VOICES )
	{
		gf1_voice[ voice_id ].priority += priority;
	}
}

/*-------------------------------------------------------------------------*
*                    G F 1   D M A   M A N A G E R
*--------------------------------------------------------------------------*/

PRIVATE int		// Returns: TRUE - DMA Continued, FALSE - DMA Inactive
gf1DmaNextBuffer(
	void
	)
{
	WORD		xfer_size;
	UINT		pflags;

	if ( gf1_dma_obj.size == 0 )
	{
		gf1_dma_active = 0;
		return FALSE;
	}
#ifndef PROD
	DmxTrace( "gf1DmaNextBuffer" );
#endif
	xfer_size = (WORD)( DMA_SIZE > gf1_dma_obj.size ? gf1_dma_obj.size : DMA_SIZE );
	memcpy( gf1_dma_buffer, gf1_dma_obj.ptr, xfer_size );

	gf1_dma_obj.ptr 	+= xfer_size;
	gf1_dma_obj.size	-= xfer_size;

	PC_PgmDMA( ( BYTE ) gf1_dma_channel, gf1_dma_handle, xfer_size, FALSE, FALSE );

	/* DMA xfer. begins here. */
	pflags = PUSH_DISABLE();
	gf1OutWord( SET_DMA_ADDRESS, gf1_dma_obj.addr );	
	gf1OutByte( DMA_CONTROL, gf1_dma_obj.dma_control );
	POP_DISABLE( pflags );

	if ( gf1Stable )	// HACK for reliable JOYSTICK readings.
		gf1Stable();

	return TRUE;
}


/*-------------------------------------------------------------------------*
* gf1DramXfer - Write to GF1 DRAM's
*--------------------------------------------------------------------------*/
PRIVATE int		// Returns: DMA_BUSY, or OK
gf1DramXfer(
	BYTE		*ptr,
	DWORD		size,
	DWORD		dram_address,
	BYTE		dma_control
	)
{
	UINT		pflags;
	WORD		extra;
	int			n;

	if ( size == 0 )
		return DMA_NOT_NEEDED;

	pflags = PUSH_DISABLE();

	if ( gf1_dma_active )
	{
		POP_DISABLE( pflags );
	    return DMA_BUSY;
	}
	if ( ( extra = ( dram_address & 0x0000001fL ) ) != 0 )
	{
	    extra = 32 - extra;
	    if ( extra > size )
			extra = size;

		if ( dma_control & DMA_INVERT_MSB )
		{
			for ( n = 0; n < extra; n++ )
			{
				gf1Poke( dram_address++, ( *ptr++ ) ^ 0x80 );
	    	}
		}
		else
		{
			for ( n = 0; n < extra; n++ )
			{
				gf1Poke( dram_address++, *ptr++ );
	    	}
		}
		size -= extra;
	}
	if ( size == 0 )
	{
		POP_DISABLE( pflags );
		return DMA_NOT_NEEDED;
	}
	gf1_dma_active = 1;

	gf1_dma_obj.ptr		= ptr;
	gf1_dma_obj.size	= size;

    if ( gf1_dma_channel >= 4 )
	{ /* 16bits wide. */
		dram_address = ADDR_16BIT( dram_address );
		dma_control |= DMA_WIDTH_16;
    }
	gf1_dma_obj.addr = dram_address >> 4;
	dma_control |= ( DMA_ENABLE | DMA_IRQ_ENABLE );
	gf1_dma_obj.dma_control = dma_control;

	gf1DmaNextBuffer();

	POP_DISABLE( pflags );
	return OK;
}


/****************************************************************************
* gf1WaitDma - Wait for DMA to complete.
*****************************************************************************/
PRIVATE int		// Returns: DMA_HUNG or OK
gf1WaitDma(
	void
	)
{
	DWORD		clk;
/*
* at slow speed of 32K/s  640K will take 20s
* this loop will delay for up to approx 30 seconds
*/
	clk = TSM_Clock() + 30000;

	while ( gf1_dma_active && TSM_Clock() < clk )
	{
		gf1Yield();
	}
	return ( gf1_dma_active ) ? DMA_HUNG : OK;
}

/****************************************************************************
* gf1StopDma - Stop DMA Transfer
*****************************************************************************/
PRIVATE void
gf1StopDma(
	void
	)
{
	gf1Enter();
	if ( gf1_dma_active )
	{
		PC_StopDMA( gf1_dma_channel );

	    gf1_dma_obj.dma_control &= ~(DMA_ENABLE|DMA_IRQ_ENABLE);

		gf1OutByte( DMA_CONTROL, gf1_dma_obj.dma_control );
		gf1OutByte( SAMPLE_CONTROL, gf1_dma_obj.dma_control );

	    gf1_dma_active = 0;
	}
	gf1Leave();
}

/****************************************************************************
* gf1DmaDeInit - Deinit DMA Manager
*****************************************************************************/
PRIVATE void
gf1DmaDeInit(
	void
	)
{
	if ( gf1_dma_handle )
	{
		gf1StopDma();
		PC_FreeDMABuffer( gf1_dma_handle );
		gf1_dma_handle = NULL;
		gf1_dma_active = 1;
	}
}

/*-------------------------------------------------------------------------*
*                    G F 1   P A T C H   L O A D E R
*--------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*
* gf1LoadPatch - loads patch from fully qualified path into GF1 DRAM
*--------------------------------------------------------------------------*/
PRIVATE GF1_PATCH *	// Returns: Ptr to loaded patch, NULL=FAILURE
gf1LoadPatch(
	char			*patch_file,	// INPUT : Name of file to get info on.
	BYTE			*diskbuf,		// INPUT : Pointer to DISK Buffer
	BYTE			*cache,			// INPUT : Pointer to Cache
	int 			flags			// INPUT : Load options
	)
{
	GF1_PATCH_INFO	pi;
	unsigned		bytes_in;
	GF1_WAVE		*wave_info;
	GF1_WAVE		*wave_mem;
	GF1_PATCH		*patch;
	GF1_LAYER		layer_data;
	GF1_PATCH_DATA	patch_data;
	int				layer;
	int				wave;
	int				wavecnt;
	int				db;
	size_t			size;
	WORD			disk_xfersize;
	BYTE			dmac;
	BYTE			*sptr;
	DWORD			len;
	DWORD			mem;
	DWORD			addr;
	void			*fh;

	patch = NULL;

	if ( ( fh = DmxOpenFile( patch_file, diskbuf, CACHE_SIZE ) ) == NULL )
		return NULL;

	bytes_in = DmxReadFile( fh, &pi, sizeof( GF1_PATCH_INFO ) );
	if ( bytes_in != sizeof( GF1_PATCH_INFO ) ||
		 strncmp( pi.header.header, GF1_HEADER_TEXT,
						PATCH_HEADER_SIZE - 4 ) != 0
		)
		goto BAD_LOAD;

	if ( strcmp( ( char FAR * ) pi.header.header + 8, "110" ) < 0 )
		goto BAD_LOAD;
/*
* Allocate Patch Structure
*/
	size = sizeof( GF1_WAVE ) * pi.header.wave_forms;
	size += sizeof( GF1_PATCH );
	if ( ( patch = malloc( size ) ) == NULL )
		goto BAD_LOAD;

	memset( patch, 0, size );
	wave_mem = ( GF1_WAVE * )( patch + 1 );
/*
* init important patch data structures in case of error on load
*/
	patch->nlayers = pi.idata.layers;
/*
* start loading
*/
	wavecnt = 0;
	for ( layer = 0; layer < pi.idata.layers; layer++, wavecnt++ )
	{
	    bytes_in = DmxReadFile( fh, &layer_data, sizeof( GF1_LAYER ) );
	    if ( bytes_in != sizeof( GF1_LAYER ) )
			goto BAD_LOAD;

	    if ( layer_data.layer_duplicate && wavecnt > 0 )
		{
			wave_info = wave_mem + ( wavecnt - 1 );
			patch->layer_waves[ layer ] = wave_info;
			patch->layer_nwaves[ layer ] = patch->layer_nwaves[ layer - 1 ];
			continue;
	    }
		patch->layer_nwaves[ layer ] = layer_data.samples;
	    wave_info = wave_mem;
	    for ( wave = 0; wave < layer_data.samples; wave++, wavecnt++, wave_info++ )
		{
			bytes_in = DmxReadFile( fh, &patch_data, sizeof( GF1_PATCH_DATA ) );
			if ( bytes_in != sizeof( GF1_PATCH_DATA ) )
				goto BAD_LOAD;

			if ( wave == 0 )
				patch->layer_waves[ layer ] = wave_info;

			memcpy( &wave_info->info, &patch_data.info, sizeof( GF1_INFO ) );
			len = wave_info->info.wave_size;

			if ( flags & PATCH_LOAD_8_BIT && ( wave_info->info.modes & PATCH_16 ) )
				size = len >> 1;
			else
				size = len;

        	if ( DmxDebug & DMXBUG_MESSAGES )
        	{
            	printf( ",%d", size );
        	}
			if ( ( mem = gf1Malloc( size ) ) == 0L )
				goto BAD_LOAD;

			wave_info->mem = mem;
		    dmac = 0;
			if ( wave_info->info.modes & PATCH_UNSIGNED )
				dmac |= DMA_INVERT_MSB;

			db = 0;
			while ( len )
			{
				disk_xfersize = ( len < CACHE_SIZE ) ? len : CACHE_SIZE;
				len -= disk_xfersize;
				bytes_in = DmxReadFile( fh, cache + ( db * CACHE_SIZE ), disk_xfersize );
			    if ( bytes_in != disk_xfersize )
					goto BAD_LOAD;

				if ( wave_info->info.modes & PATCH_16 )
				{
					if ( flags & PATCH_LOAD_8_BIT )
					{
						disk_xfersize >>= 1;

						sptr = cache + ( db * CACHE_SIZE );
						dmx16to8bit( sptr, sptr, disk_xfersize );
					/*
						for ( j = 1, i = 0; i < disk_xfersize; i++, j += 2 )
							sptr[ i ] = sptr[ j ];
					*/
					}
					else
					{
						dmac |= DMA_DATA_16;
					}
				}
				gf1WaitDma();

				if ( gf1DramXfer( cache + ( db * CACHE_SIZE ) , disk_xfersize, mem, dmac ) != OK )
					goto BAD_LOAD;

				gf1WaitDma();	// ADD TO POLL DMA
				db ^= 1;
				mem += disk_xfersize;
		    }
			gf1WaitDma();

			if ( ( wave_info->info.modes & PATCH_16 ) &&
				 ( flags & PATCH_LOAD_8_BIT )
				)
			{
		    	wave_info->info.start_loop >>= 1;
		    	wave_info->info.end_loop   >>= 1;
		    	wave_info->info.wave_size  >>= 1;
		    	wave_info->info.modes      &= ~(0x01);
			}
			addr = wave_info->mem;

			if ( wave_info->info.modes & PATCH_16 )
	    		addr = ADDR_16BIT( addr );

			wave_info->start_acc_low = ADDR_LOW( addr );
			wave_info->start_acc_high = ADDR_HIGH( addr );

			addr = wave_info->mem + wave_info->info.start_loop;

			if ( wave_info->info.modes & PATCH_16 )
				addr = ADDR_16BIT( addr );

			wave_info->start_low = ADDR_LOW( addr );
			wave_info->start_high = ADDR_HIGH( addr );

		/* add in fractional addresses. */
			wave_info->start_low |= ((int)(wave_info->info.fractions & 0x0F) << 5);

			addr = wave_info->mem + wave_info->info.end_loop;
			if ( wave_info->info.modes & PATCH_16 )
	    		addr = ADDR_16BIT( addr );

			wave_info->end_low = ADDR_LOW( addr );
			wave_info->end_high = ADDR_HIGH( addr );

		/* add in fractional addresses. */
			wave_info->end_low |= ((int)(wave_info->info.fractions & 0xF0) << 1);

			addr = wave_info->mem + wave_info->info.wave_size - 1;
			if ( wave_info->info.modes & PATCH_16 )
			{
				--addr;
	    		addr = ADDR_16BIT( addr );
			}
			wave_info->end_acc_low = ADDR_LOW( addr );
			wave_info->end_acc_high = ADDR_HIGH( addr );

			wave_info->sample_ratio = ( (DWORD)
                   ( GF1_FREQ_DIVISOR << 9 ) +
					(DWORD)(wave_info->info.sample_rate >> 1 ) ) /
					(DWORD) wave_info->info.sample_rate;
		}
	}
	DmxCloseFile( fh );
	return ( void * ) patch;

BAD_LOAD:
	DmxCloseFile( fh );
	if ( patch )
		free( patch );
	return NULL;
}


/*-------------------------------------------------------------------------*
*                    G F 1   H E L P E R   F U N C T I O N S
*--------------------------------------------------------------------------*/
PRIVATE void
gf1SetVoiceFreq(
    int         voice,      /* INPUT: >= 0 Voice to change for          */
    WORD        frequency   /* INPUT: Frequency to set to               */
    )
{
    if ( voice >= 0 )
        gf1SelectVoice( voice );

	gf1OutWord( SET_FREQUENCY, frequency );
}

/***************************************************************************
*	NAME:  GF1DIG.C
***************************************************************************/

void
gf1ChangeVoice(
	int			ints
	)
{
    gf1SelectVoice( dig_left_voice );
	gf1OutDelay( SET_VOLUME_CONTROL, dig_volume_control );
	gf1OutDelay( SET_CONTROL, ( ints ) ?
				( dig_voice_control | GF1_IRQE ) : dig_voice_control );

    gf1SelectVoice( dig_right_voice );
	gf1OutDelay( SET_VOLUME_CONTROL, dig_volume_control );
	gf1OutDelay( SET_CONTROL, dig_voice_control );
}

PRIVATE int
gf1DigitalVoiceHandler(
	int 		voice
	)
{
    if ( voice != dig_left_voice && voice != dig_right_voice )
        return 0;

	if ( dig_status & ( DMASTAT_PENDING | DMASTAT_WORKING ) )
	{
	    return 1;
	}
	if ( gf1TerminalCount() )
	{
	    return 1;
	}
	return 0;
}

PRIVATE void
gf1SetAddrRegs(
	DWORD		addr,
	int 		portl,
	int 		porth
	)
{
	gf1OutWord( portl, ADDR_LOW( addr ) );
	gf1OutWord( porth, ADDR_HIGH( addr ) );
}

PRIVATE DWORD
gf1GetAddrRegs(
	int 		portl,
	int 		porth
	)
{
	DWORD		addrh;
	DWORD		addrl;

	addrh = (DWORD) gf1InWord( porth ) & 0x1fff;
	addrl = gf1InWord( portl ) >> 9;
	return ( addrh << 7 ) | ( addrl & 0x7f );
}

PRIVATE DWORD
gf1GetVoicePosition(
	int 		voice
	)
{
	int			j;
	DWORD		addr1;
	DWORD		addr2;

    gf1SelectVoice( voice );
	for ( j = 0; j < 4; j++ )
	{
		addr1 = gf1GetAddrRegs( GET_ACC_LOW, GET_ACC_HIGH );
		gf1Delay();
		addr2 = gf1GetAddrRegs( GET_ACC_LOW, GET_ACC_HIGH );

		if ( addr2 - addr1 < 128 )
			break;

		gf1Delay();
	}
	addr1 = gf1GetAddrRegs( GET_START_LOW, GET_START_HIGH );
	return ( DWORD )( addr2 - addr1 );
}

PRIVATE int
gf1TerminalCount(
	void
	)
{
    if ( DMA_IDLE )
	{
		if ( dig_status & DMASTAT_PENDING )
		{
#ifndef PROD
			DmxTrace( "gf1TerminalCount - PENDING!" );
#endif
			gf1XferBlock();
		}
		else if ( dig_status & DMASTAT_WORKING )
        {
#ifndef PROD
            DmxTrace( "gf1TerminalCount - NEXTPAGE!" );
#endif
        	dspNextPage();  // Added
	        dig_status &= ~DMASTAT_WORKING;
		}
#ifndef PROD
		else
        {
            DmxTrace( "gf1TerminalCount - <<?>>" );
        }
#endif
	    return 1;
    }
#ifndef PROD
    else
    {
        DmxTrace( "gf1TerminalCount - BUSY!" );
    }
#endif
    return 0;
}

PRIVATE void
gf1XferBlock(
	void
	)
{
	DWORD		addr;

	gf1_flags &= ~F_BLOCK_READY;
    if ( DMA_IDLE )
	{
		dig_status &= ~DMASTAT_PENDING;
        dig_status |= DMASTAT_WORKING;

		addr = dig_addr + blk_offset;
		gf1WaitDma();
		gf1DramXfer( gf1_dma_buffer, blk_size, addr, dig_dma_control );
    }
	else
	{
		dig_status |= DMASTAT_PENDING;
    }
}

PRIVATE void
gf1SetVol(
	WORD 		ramp_vol
	)
{
	WORD 		old_volume;

	dig_volume_control &= ~( GF1_STOP | GF1_STOPPED );
	gf1OutDelay( SET_VOLUME_CONTROL, dig_volume_control|GF1_STOP|GF1_STOPPED );
    old_volume = gf1InWord( GET_VOLUME ) >> 8;
    gf1OutByte( SET_VOLUME_RATE, 63 ); /* may cause zipper noise */

	if ( ramp_vol < MIN_OFFSET )
		ramp_vol = MIN_OFFSET;
	if ( ramp_vol > MAX_OFFSET )
		ramp_vol = MAX_OFFSET;

	if ( old_volume > ramp_vol )
	{
	    gf1OutByte( SET_VOLUME_START, ramp_vol );
	    gf1OutByte( SET_VOLUME_END, MAX_OFFSET );
	    dig_volume_control |= GF1_DIR_DEC;
	}
	else if ( old_volume < ramp_vol )
	{
        gf1OutByte( SET_VOLUME_START, MIN_OFFSET );
        gf1OutByte( SET_VOLUME_END, ramp_vol );
	    dig_volume_control &= ~GF1_DIR_DEC;
	}
	gf1OutDelay( SET_VOLUME_CONTROL, dig_volume_control );
}

/****************************************************************************
* gf1StopDigital - Stop digital voice from playing
*****************************************************************************/
PRIVATE void
gf1StopDigital(
	)
{
	gf1Enter();
	if ( ( dig_status & STAT_MASK ) != STAT_UNUSED )
	{
	    if ( dig_status & DMASTAT_WORKING )
		{
			gf1StopDma();
	    }
	    dig_status = STAT_UNUSED;
	    dig_volume_control = GF1_STOP|GF1_STOPPED;
	    dig_voice_control = GF1_STOP|GF1_STOPPED;
	    gf1ChangeVoice( 0 );

        gf1SelectVoice( dig_left_voice );
	    gf1SetVol( MIN_OFFSET ); /* ramp volume down */

        gf1SelectVoice( dig_right_voice );
		gf1SetVol( MIN_OFFSET );
	}
	gf1Leave();
}

/***************************************************************************
*	NAME:  GF1INIT.C
***************************************************************************/

LOCAL WORD 	gf1_mix_control;
LOCAL WORD 	gf1_status_register;
LOCAL WORD 	gf1_timer_control;
LOCAL WORD 	gf1_timer_data;
LOCAL WORD 	gf1_irqdma_control;
LOCAL WORD 	gf1_reg_control;
LOCAL WORD 	gf1_midi_control;
LOCAL WORD 	gf1_midi_data;
LOCAL WORD 	gf1_page_register;
LOCAL WORD 	gf1_register_select;
LOCAL WORD 	gf1_data_low;
LOCAL WORD 	gf1_data_high;
LOCAL WORD 	gf1_dram_io;
LOCAL WORD	gf1_channel_out;
LOCAL WORD	gf1_voices;

LOCAL long	gf1_scale_table[ SCALE_TABLE_SIZE ] =
{
    8175L,     8661L,     9177L,     9722L,    10300L,    10913L,    11562L,
   12249L,    12978L,    13750L,    14567L,    15433L,    16351L,    17323L,
   18354L,    19445L,    20601L,    21826L,    23124L,    24499L,    25956L,
   27500L,    29135L,    30867L,    32703L,    34647L,    36708L,    38890L,
   41203L,    43653L,    46249L,    48999L,    51913L,    55000L,    58270L,
   61735L,    65406L,    69295L,    73416L,    77781L,    82406L,    87307L,
   92498L,    97998L,   103826L,   110000L,   116540L,   123470L,   130812L,
  138591L,   146832L,   155563L,   164813L,   174614L,   184997L,   195997L,
  207652L,   220000L,   233081L,   246941L,   261625L,   277182L,   293664L,
  311126L,   329627L,   349228L,   369994L,   391995L,   415304L,   440000L,
  466163L,   493883L,   523251L,   554365L,   587329L,   622253L,   659255L,
  698456L,   739988L,   783990L,   830609L,   880000L,   932327L,   987766L,
 1046502L,  1108730L,  1174659L,  1244507L,  1318510L,  1396912L,  1479977L,
 1567981L,  1661218L,  1760000L,  1864655L,  1975533L,  2093004L,  2217461L,
 2349318L,  2489015L,  2637020L,  2793825L,  2959955L,  3135963L,  3322437L,
 3520000L,  3729310L,  3951066L,  4186009L,  4434922L,  4698636L,  4978031L,
 5274040L,  5587651L,  5919910L,  6271926L,  6644875L,  7040000L,  7458620L,
 7902132L,  8372018L,  8869844L,  9397272L,  9956063L, 10548081L, 11175303L,
 11839821L
};

/*--------------------------------------------------------------------------*
* gf1DispatchInt - Dispatch Voice Interrupt.
*---------------------------------------------------------------------------*/
PRIVATE void
gf1DispatchInt(
	void
	)
{
    DWORD		ignore_voice;
    DWORD		ignore_volume;
    DWORD		ignore_bit;
	UINT		pFlags;
    UINT		irq_status;
	UINT		pval;
    UINT		voice;
    BYTE		irqv_data;
	int			pass = 0;
	int			j;

	pFlags = PUSH_DISABLE();
    ++gf1_voice_semaphore;
	if ( gf1_voice_semaphore > 1 )
	{
	    gf1_flags |= F_REDO_INT;
		--gf1_voice_semaphore;
#ifndef PROD
		DmxTrace( "gf1DispatchInt - REDO!" );
#endif
		POP_DISABLE( pFlags );
		return ;
	}
    /* handle interrupt */

	irq_status = GET_IRQ_STATUS;
	if ( irq_status == 0 )
	{
#ifndef PROD
		DmxTrace( "gf1DispatchInt" );
        DmxTrace( "gf1 irq_status - NONE?" );
#endif
		if ( gf1_flags & F_BLOCK_READY )
		{
			gf1XferBlock();
			gf1_flags &= ~F_REDO_INT;
		}
	}
	while ( ( irq_status != 0 || ( gf1_flags & F_REDO_INT ) != 0 )
		    && ++pass <= 20 )
	{
#ifndef PROD
		char 	*str = "gf1DispatchInt - Pass:a";
		str[ 22 ] = 'A' + pass;
		DmxTrace( str );
#endif
    	ignore_voice = 0L;
    	ignore_volume = 0L;

	    gf1_flags &= ~F_REDO_INT;
		if ( irq_status == 0 )
		{
#ifndef PROD
        	DmxTrace( "gf1 irq_status - NONE?" );
#endif
			if ( gf1_flags & F_BLOCK_READY )
			{
				gf1XferBlock();
				gf1_flags &= ~F_REDO_INT;
			}
			break;
		}
#ifndef PROD
        if ( irq_status & MIDI_IRQ_SEND )
        	DmxTrace( "gf1 irq_status - MIDI_IRQ_SEND" );
        if ( irq_status & MIDI_IRQ_RCV )
        	DmxTrace( "gf1 irq_status - MIDI_IRQ_RCV" );
        if ( irq_status & TIMER_1 )
        	DmxTrace( "gf1 irq_status - TIMER_1" );
        if ( irq_status & ADLIB_DATA )
        	DmxTrace( "gf1 irq_status - ADLIB_DATA" );
#endif
	/*
	* ok, so there is work to do.  Interrupts can be enabled
	*/
		if ( irq_status & DMA_TC_BIT )
		{
#ifndef PROD
        	DmxTrace( "gf1 irq_status - DMA_TC_BIT" );
#endif
	    	/* Clear out interrupt. */
			pval = gf1InByte( DMA_CONTROL );

			if ( pval & DMA_IRQ_PRESENT )
			{
#ifndef PROD
            	DmxTrace( "gf1DmaIRQPresent" );
#endif
				if ( gf1DmaNextBuffer() == FALSE )
				{
					gf1TerminalCount();
				}
	    	}
#ifndef PROD
            else
            {
            	DmxTrace( "gf1DmaIRQ_NOT_Present" );
            }
#endif
		}
		if ( irq_status & TIMER_2 )
		{
#ifndef PROD
        	DmxTrace( "gf1 irq_status - TIMER_2" );
#endif
	    	/* Reset timer interrupt. */
            gf1OutByte( ADLIB_CONTROL, gf1_adlib_control & ~8 );
            gf1OutByte( ADLIB_CONTROL, gf1_adlib_control );
	    	if ( timer2_handler && timer2_handler( voice ) )
			{
				timer2_handler = NULL;
	    	}
		}
		if ( irq_status & ( GF1_WAVETABLE | GF1_ENVELOPE ) )
		{
#ifndef PROD
            if ( irq_status & GF1_WAVETABLE )
        	    DmxTrace( "gf1 irq_status - GF1_WAVETABLE" );
            if ( irq_status & GF1_ENVELOPE )
        	    DmxTrace( "gf1 irq_status - GF1_ENVELOPE" );
#endif
	    	_enable();
	    	irqv_data = gf1InByte( GET_IRQV );
	    	_disable();

			for ( j = 0; j < 250 && ( irqv_data & 0xC0 ) != 0xC0; j++ )
			{
				voice = irqv_data & 0x1F;
				ignore_bit = 1L << voice;
			/*
			* Is the interrupt from a wavetable address.
			*/
				if ( ! ( irqv_data & 0x80 ) )
				{
		    		if ( ignore_voice & ignore_bit )
						ignore_voice &= ~ignore_bit;
					else
					{
						if ( gf1NoteVoiceHandler( voice ) || gf1DigitalVoiceHandler( voice ) )
			    			ignore_voice |= ignore_bit;
		    		}
				}
				/* Is the interrupt from a volume envelope. */
				if ( ! ( irqv_data & 0x40 ) )
				{
		    		if (ignore_volume & ignore_bit)
						ignore_volume &= ~ignore_bit;
					else
					{
						if ( gf1NoteVolumeHandler( voice ) )
			    			ignore_volume |= ignore_bit;
		    		}
				}
                irqv_data = gf1InByte( GET_IRQV );
	    	}
		}
		irq_status = GET_IRQ_STATUS;
	}
    --gf1_voice_semaphore;
	POP_DISABLE( pFlags );
}


/*--------------------------------------------------------------------------*
* gf1ServiceVoice - Voice Service Routine
*---------------------------------------------------------------------------*/
PRIVATE int
gf1ServiceVoice(
	int			irq
    )
{
	BYTE		bPrevColor;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
	DmxTrace( "gf1ServiceVoice" );
#endif
    gf1DispatchInt();
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
	return PCI_OKAY;
}

/*--------------------------------------------------------------------------*
* gf1PollVoice - Voice Service Routine
*---------------------------------------------------------------------------*/
PRIVATE int
gf1PollVoice(
    void
    )
{
	BYTE		bPrevColor;

#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		inp( 0x388 );
		bPrevColor = inp( 0x3c1 );
		outp( 0x3c0, 0x3f );
	}
#endif
    gf1DispatchInt();
#ifndef PROD
	if ( DmxDebug & DMXBUG_CARD )
	{
		inp( 0x3da );
		outp( 0x3c0, 0x31 );
		outp( 0x3c0, bPrevColor );
	}
#endif
	return TS_OKAY;
}



/*--------------------------------------------------------------------------*
* gf1IsrInit - Clear ALL callback functions and clear semaphores
*---------------------------------------------------------------------------*/
PRIVATE void
gf1IsrInit(
    void
    )
{
	timer2_handler = NULL;
	gf1_adlib_control = 0;
	gf1_adlib_timer_mask = 0;
	gf1_voice_semaphore = 0;
}

/****************************************************************************
* gf1EnableTimer2 - Enable Timer 2
*****************************************************************************/
PRIVATE int
gf1EnableTimer2(
	int 		( *callback )( void ),
	int 		resolution
	)
{
	UINT		pflags;

	if ( timer2_handler != NULL )
		return NO_MORE_HANDLERS;

	pflags = PUSH_DISABLE();

	timer2_handler = callback;
	gf1OutByte( ADLIB_TIMER2, resolution );
	gf1_adlib_control |= 0x8;

	gf1OutByte( ADLIB_CONTROL, gf1_adlib_control );

	/* Start timer 2. */
	gf1_adlib_timer_mask |= 0x02;
	outp( gf1_timer_control, 0x04 ); /* adlib register 4 */
	outp( gf1_timer_data, gf1_adlib_timer_mask );

	POP_DISABLE( pflags );
	return OK;
}

/****************************************************************************
* gf1DisableTimer2 - Disable the 2nd timer
*****************************************************************************/
PRIVATE void
gf1DisableTimer2(
	void
	)
{
	gf1Enter();
	timer2_handler = NULL;
	gf1_adlib_control &= ~0x8;
	gf1_adlib_timer_mask &= ~0x2;
	gf1OutByte( ADLIB_CONTROL, gf1_adlib_control );
	gf1Leave();
}

#ifdef PROD
/****************************************************************************
* gf1EnterBusy - Entry into critical section of code, only if not busy.
*****************************************************************************/
PRIVATE int	// returns: 0 = OK, 1 = Busy
gf1EnterBusy(
	void
	)
{
	++gf1_voice_semaphore;
	if ( gf1_voice_semaphore > 1 )
	{
		--gf1_voice_semaphore;
		return 1;
	}
#ifndef PROD
	DmxTrace( "gf1EnterBusy" );
#endif
	return 0;
}


/****************************************************************************
* gf1Enter - Entry into critical section of code.
*****************************************************************************/
PRIVATE void
gf1Enter(
	void
	)
{
	++gf1_voice_semaphore;
#ifndef PROD
	DmxTrace( "gf1Enter" );
#endif
}
#else
/****************************************************************************
* gf1EnterBusy - Entry into critical section of code, only if not busy.
*****************************************************************************/
PRIVATE int	// returns: 0 = OK, 1 = Busy
_gf1EnterBusy(
	int		line
	)
{
#ifndef PROD
	char	*str = "gf1EnterBusy @ 000000";
#endif
	++gf1_voice_semaphore;
	if ( gf1_voice_semaphore > 1 )
	{
		--gf1_voice_semaphore;
		return 1;
	}
#ifndef PROD
	itoa( line, str + 15, 10 );
	DmxTrace( str );
#endif
	return 0;
}


/****************************************************************************
* gf1Enter - Entry into critical section of code.
*****************************************************************************/
PRIVATE void
_gf1Enter(
	int		line
	)
{
#ifndef PROD
	char	*str = "gf1Enter @ 000000";
	itoa( line, str + 11, 10 );
	DmxTrace( str );
#endif
	++gf1_voice_semaphore;
}
#endif
/****************************************************************************
* gf1Leave - Leave critical section of code.
*****************************************************************************/
PRIVATE void
gf1Leave(
	void
	)
{
#ifndef PROD
	DmxTrace( "gf1Leave" );
#endif
	if ( gf1_voice_semaphore == 1 )
	{
		if ( gf1_flags & F_BLOCK_READY )
		{
			gf1XferBlock();
		}
		if ( gf1_flags & F_REDO_INT )
		{
			--gf1_voice_semaphore;
			while ( gf1_flags & F_REDO_INT )
			{
				gf1DispatchInt();
			}
		}
	}
	if ( gf1_voice_semaphore )
		--gf1_voice_semaphore;
}

/****************************************************************************
* gf1Delay - Force delay by reading I/O port.
*****************************************************************************/
PRIVATE void
gf1Delay(
	void
	)
{
	int				j;

	for ( j = 40; j > 0; j-- )
	{
		inp( gf1_mix_control );
	}
}

/****************************************************************************
* gf1OutDelay - Select index and out byte to port twice.
*****************************************************************************/
PRIVATE void
gf1OutDelay(
	BYTE		select,		// INPUT : Register Select
	BYTE		data		// INPUT : Data to out to card
	)
{
	gf1OutByte( select, data );
	gf1Delay();
	outp( gf1_data_high, data );
}

/*--------------------------------------------------------------------------*
* gf1InitPorts - Initialize port addresses relative to base I/O port.
*---------------------------------------------------------------------------*/
PRIVATE BOOL		// Returns: TRUE - base OK, FALSE - Bad Port #
gf1InitPorts(
	int		base_port
	)
{
	if ( base_port < 0x210 || base_port > 0x260 )
		return FALSE;

	base_port 		   -= 0x220;
	gf1_mix_control 	= 0x220 + base_port;
	gf1_status_register = 0x226 + base_port;
	gf1_timer_control	= 0x228 + base_port;
	gf1_timer_data		= 0x229 + base_port;
	gf1_irqdma_control	= 0x22b + base_port;
	gf1_reg_control		= 0x22f + base_port;
	gf1_midi_control	= 0x320 + base_port;
	gf1_midi_data		= 0x321 + base_port;
	gf1_page_register	= 0x322 + base_port;
	gf1_register_select = 0x323 + base_port;
	gf1_data_low		= 0x324 + base_port;
	gf1_data_high		= 0x325 + base_port;
	gf1_dram_io			= 0x327 + base_port;

	return TRUE;
}
	
/*--------------------------------------------------------------------------*
* gf1ClearInts - Clear GF1 interrupts.
*---------------------------------------------------------------------------*/
PRIVATE void
gf1ClearInts(
	void
	)
{
	int		i;

	/* Clear all interrupts. */
	gf1OutByte( DMA_CONTROL, 0x00 );
	gf1OutByte( ADLIB_CONTROL, 0x00 );
	gf1OutByte( SAMPLE_CONTROL, 0x00 );
		
	inp( gf1_status_register );

	gf1InByte( DMA_CONTROL );
	gf1InByte( SAMPLE_CONTROL );

	for ( i = 0; i < 32; i++ )
    {
    	gf1InByte( GET_IRQV );
	}
}

/*--------------------------------------------------------------------------*
* gf1RampDown - Ramp down volume level of active voices.
*---------------------------------------------------------------------------*/
PRIVATE void
gf1RampDown(
	void
	)
{
	UINT 	i;
	UINT	volume;
	UINT	vc;

	gf1Enter();
	for ( i = 0; i < gf1_voices; i++ )
    {
        gf1SelectVoice( i );

        gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED | GF1_STOP );

		gf1Delay();
		volume = gf1InWord( GET_VOLUME ) >> 8;

		if ( volume > MIN_OFFSET )
		{
			gf1OutByte( SET_VOLUME_START, MIN_OFFSET );
			gf1OutByte( SET_VOLUME_RATE, 1 );
			gf1OutByte( SET_VOLUME_CONTROL, GF1_DIR_DEC );
		}
	}
	for ( i = 0; i < gf1_voices; i++ )
	{
        gf1SelectVoice( i );
		do
		{
			vc = gf1InByte( GET_VOLUME_CONTROL );
		} while ( ( vc & ( GF1_STOPPED | GF1_STOP ) ) == 0 );
		gf1OutDelay( SET_CONTROL, GF1_STOPPED | GF1_STOP );
	}
	gf1Leave();
}

/*--------------------------------------------------------------------------*
* gf1SetInterface - Inform GF1 how PC environment is setup.
*---------------------------------------------------------------------------*/
PRIVATE void
gf1SetInterface(
	BYTE		irq_control,
	BYTE		dma_control
	)
{
	_disable();
/*
* do setup for digital asic
*/
	outp( gf1_reg_control, 0x05 );			/* clear old IRQs */
	outp( gf1_mix_control, gf1_mixer_mask );/* enable IRQ & DMA */
	outp( gf1_irqdma_control, 0 );			/* clears power reset latch */
	outp( gf1_reg_control, 0 );				/* set irq and dma control
						   					   registers to default location */


/* DMA CONTROL REG - clear power up reset latch */
	outp( gf1_mix_control, gf1_mixer_mask );             
	outp( gf1_irqdma_control, dma_control | 0x80 );

/* IRQ CONTROL REG */
	outp( gf1_mix_control, gf1_mixer_mask | GF1_CONTROL_REGISTER_SELECT );
	outp( gf1_irqdma_control, irq_control);

/* DMA CONTROL REG - select new dma channels */
	outp( gf1_mix_control, gf1_mixer_mask );
	outp( gf1_irqdma_control, dma_control );

/* IRQ CONTROL - select new IRQ's */
	outp( gf1_mix_control, gf1_mixer_mask | GF1_CONTROL_REGISTER_SELECT );
	outp( gf1_irqdma_control, irq_control );

/* DISABLE IOW TO 2XB */
	outp( gf1_reg_control, 0 );

/* IRQ CONTROL, ENABLE IRQ */
	gf1_mixer_mask |= GF1_ENABLE_IRQ_DMA;
 	outp( gf1_mix_control, gf1_mixer_mask );

/* DISABLE IOW TO 2XB */
	outp( gf1_reg_control, 0 );
	
	_enable();
}

/*--------------------------------------------------------------------------*
* gf1ResetUltra - Reset Card
*---------------------------------------------------------------------------*/
PRIVATE void
gf1ResetUltra(
	int		voices
	)
{
	UINT 	i;

	if ( voices < MIN_VOICES )
		voices = MIN_VOICES;

	if ( voices > MAX_VOICES )
		voices = MAX_VOICES;
	
	_disable();

	/* Set up GF1 Chip for no interrupts. */
	gf1_voices = MAX_VOICES;

	gf1OutByte( MASTER_RESET, 0x00 );
	gf1Delay();
	gf1Delay();
	gf1Delay();
	gf1Delay();

	gf1OutByte( MASTER_RESET, 0x01 );
	gf1Delay();
	gf1Delay();
	gf1Delay();
	gf1Delay();

	gf1OutByte( SET_VOICES, (BYTE)( ( MAX_VOICES - 1 ) | 0xC0 ) );
	gf1ClearInts();
	
	for( i = 0; i < MAX_VOICES; i++ )
	{
        gf1SelectVoice( i );
		gf1OutDelay( SET_CONTROL, GF1_STOPPED | GF1_STOP );
		gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED | GF1_STOP );

		gf1OutWord( SET_START_HIGH, 0 );
		gf1OutWord( SET_START_LOW, 0 );
		gf1OutWord( SET_END_HIGH, 0 );
		gf1OutWord( SET_END_LOW, 0 );

		gf1OutByte( SET_VOLUME_RATE, 63 );
		gf1OutByte( SET_VOLUME_START, MIN_OFFSET );
		gf1OutByte( SET_VOLUME_END, MAX_OFFSET);

		gf1OutWord( SET_VOLUME, MIN_OFFSET << 8 );
		gf1OutWord( SET_ACC_HIGH, 0 );
		gf1OutWord( SET_ACC_LOW, 0x30 << 9 );
	}
	gf1ClearInts();

	gf1_voices = voices;
	gf1OutByte( SET_VOICES, (BYTE)( ( gf1_voices - 1) | 0xC0 ) );
	gf1ClearInts();

	gf1OutByte( MASTER_RESET, 0x07 ); /* enable dac, etc */
	gf1Delay();
	gf1Delay();
	_enable();
}

/*--------------------------------------------------------------------------*
* gf1Initialize - Initialize card based upon specified port information.
*---------------------------------------------------------------------------*/
PUBLIC int
gf1Initialize(
    int     forced_base_port,
    int     forced_gf1_irq,
    int     forced_midi_irq,
    int     forced_channel_out,
    int     voices
    )
{
	BOOL	shared_irq;
	BYTE	irq_control;
	BYTE	dma_control;
/*
* Determine the base_port setup on the GF1.
*/
	if ( gf1InitPorts( forced_base_port ) == FALSE )
		return BASE_NOT_FOUND;

	if ( ( irq_control = gf1_irq_latch[ forced_gf1_irq ] ) == 0 )
		return BAD_IRQ;

	if ( gf1_irq_latch[ forced_midi_irq ] == 0 )
		return BAD_IRQ;

/* initialize interrupt handlers */
	gf1IsrInit();

/* Set the interrupt configuration. */	
	pcInstallISR( forced_gf1_irq, gf1ServiceVoice, PCI_POST_EOI );

//	if ( TSM_NewService( gf1PollVoice, 140 /*hz*/, 255, 0 ) < 0 )
//		return BAD_IRQ;

	if ( forced_midi_irq == forced_gf1_irq )
		shared_irq = TRUE;
	else
		shared_irq = FALSE;

	gf1_channel_out = forced_channel_out;

/* Put The GF1 into a stable state. */
	gf1ResetUltra( voices );
	
	if ( shared_irq )
		irq_control |= GF1_COMBINE_IRQ;
	else
		irq_control |= ( gf1_irq_latch[ forced_midi_irq ] << 3 );

	dma_control = gf1_dma_latch[ gf1_channel_out ] | GF1_COMBINE_DMA;

	gf1SetInterface( irq_control, dma_control );

	return OK;
}

/****************************************************************************
* gf1Yield - Yield control to GF1
*****************************************************************************/
PRIVATE void
gf1Yield(
	void
	)
{
	BYTE 	irq_status;

	irq_status = GET_IRQ_STATUS;
	if ( irq_status & DMA_TC_BIT )
	{
		gf1DispatchInt();
	}
	while ( gf1_flags & F_REDO_INT )
	{
		gf1_flags &= ~F_REDO_INT;
		gf1DispatchInt();
	}
}

/****************************************************************************
* gf1DetectCard - Detects card at specified port address
*****************************************************************************/
PRIVATE int		// returns: 0 = Not found, 1 = Found
gf1DetectCard(
	WORD		port	// INPUT: Card address to verify
    )
{
	WORD		source = port + 8;
	WORD		dest = port + 10;

	outp( source, 0xaa );
	if ( inp( dest ) != 0xaa )
    {
	    return 0;
	}
	outp( source, 0x55 );
	if ( inp( dest ) != 0x55 )
    {
	    return 0;
	}
	return 1;
}


/****************************************************************************
* gf1LoadOS - Loads GF1 OS
*****************************************************************************/
PRIVATE int
gf1LoadOS(
	GF1_OS		*os			// INPUT : Ptr to GF1_OS block
    )
{
	int			i;
    int 		status;
	CHAN_STATUS	*cs;
	VOC_STATUS	*vs;
	int 		voice;
	int			channel;

	if ( gf1_os_loaded )
    {
	    ++gf1_os_loaded;
	    return OK;
	}
	gf1_flags = 0;
	dma_buffer = NULL;
	
	if ( ! gf1DetectCard( os->forced_base_port ) )
    {
		return CARD_NOT_FOUND;
	}
	gf1_mixer_mask = GF1_DISABLE_OUT | GF1_DISABLE_LINE;
	outp( os->forced_base_port, gf1_mixer_mask );
	
	status = gf1Initialize(
            os->forced_base_port,
            os->forced_gf1_irq,
			os->forced_midi_irq,
			os->forced_channel_out,
			os->voices
            );
	if ( status )
    {
		return status;
	}
    if ( DmxDebug & DMXBUG_MESSAGES )
    {
        printf( "Ultrasound @ Port:%03Xh, IRQ:%d,  DMA:%d\n",
            os->forced_base_port, os->forced_gf1_irq, os->forced_midi_irq );
    }
	gf1_flags |= F_OS_LOADED;
/*
** Initialize GF1 Memory Manager
*/
	memset( gf1_bank, 0, sizeof( gf1_bank ) );
	gf1_bank[ 0 ].used = 32;
    gf1_banks = 0;
    if ( DmxDebug & DMXBUG_MESSAGES )
    {
        printf( "Sizing GUS RAM:" );
    }

	for ( i = 0; i < gf1_max_banks; i++ )
    {
		if ( gf1TestDRAM( i << 18 ) )
        {
            if ( DmxDebug & DMXBUG_MESSAGES )
                printf( "BANK%d:OK ", i + 1 );
			gf1_bank[ gf1_banks ].base = i << 18;
			gf1_bank[ gf1_banks ].memory = ( 256L*1024L );
			gf1_banks++;
		}
		else if ( DmxDebug & DMXBUG_MESSAGES )
		{
            printf( "BANK%d:N/A ", i + 1 );
		}
	}
	if ( DmxDebug & DMXBUG_MESSAGES )
	{
        printf( "\n" );
	}
	if ( gf1_banks )
	{
		/* kludge -- all of the spare voices point here */
		gf1Poke( 30, 0x00 );
		gf1Poke( 31, 0x00 );
	}
	if ( gf1_banks < 1 )
		return UPGRADE_CARD_MEMORY;
/*
** Initialize Voice GF1 Voice Manager
*/
	voice_mask = 0L;
	voice_age = 1L;
	memset( gf1_voice, 0, sizeof( gf1_voice ) );
/*
** Initialize DMA
*/
	gf1_dma_channel = gf1_channel_out;
	gf1_dma_active = 0;

	gf1_dma_handle = PC_AllocDMABuffer( DMA_SIZE );
	if ( gf1_dma_handle == NULL )
		return NO_DMA_MEMORY;

#ifdef __386__
    gf1_dma_buffer = gf1_dma_handle;
#else
    gf1_dma_buffer =Addr32Bit( gf1_dma_handle );
#endif

/*
** Initialize MIDI
*/
	gf1_linear_volumes = 1;

	memset( voice_status, 0, sizeof( voice_status ) );
	for ( vs = voice_status, voice = 0; voice < MAX_VOICES; voice++, vs++ )
	{
	    vs->volume_control = GF1_DIR_DEC;
	    vs->velocity       = MAX_VOLUME;
	    vs->attenuation    = 255;
	    vs->vibe_count     = 1;
	}
	memset( channel_status, 0, sizeof( channel_status ) );
    cs = channel_status;
	for ( channel = 0; channel < MAX_CHANNELS; channel++, cs++ )
	{
	    cs->volume 		= 100;
	    cs->pitch_bend	= 1024;
	    cs->balance 	= 16;	/* 16 is a special case - use wave balance */
	}
	vibrato_voice_count = 0; /* number of voices currently modulating */
	gf1_m_volume = MAX_VOLUME;
/*
** Initialize SFX
*/
    dig_status = STAT_UNUSED;
	dig_voice_control  = GF1_STOPPED|GF1_STOP;
    dig_volume_control = GF1_STOPPED|GF1_STOP;

	gf1_mixer_mask &= ~(GF1_DISABLE_OUT|GF1_DISABLE_LINE);
	outp( os->forced_base_port, gf1_mixer_mask );
    ++gf1_os_loaded;

	return OK;
}


/****************************************************************************
* gf1UnloadOS - Unloads GF1 OS
*****************************************************************************/
PRIVATE int
gf1UnloadOS(
	void
	)
{
	if ( ! ( gf1_flags & F_OS_LOADED ) )
		return NOT_LOADED;

	if ( --gf1_os_loaded )
		return OK;

	_disable();
	gf1OutByte( MASTER_RESET, 0x00 );
	gf1Delay();
	gf1Delay();

	gf1OutByte( MASTER_RESET, 0x01 );
	gf1Delay();
	gf1Delay();

	gf1ClearInts();
	pcRemoveISR( os_parms.forced_gf1_irq );

	gf1_flags &= ~F_OS_LOADED;
	_enable();
	return OK;
}

/***************************************************************************
*	NAME:  GF1MIDI.C
***************************************************************************/
PRIVATE int
gf1DoNoteVibrato(
	void
	)
{
    VOC_STATUS	*vs;
    UINT 		voice;
    int 		vibe_increment;

    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( vs->status & STAT_VOICE_RUNNING )
		{
	    	if ( vs->wave->info.vibrato_depth || channel_status[ vs->channel ].vib_depth )
			{
				vibe_increment = ( DWORD ) vs->vibe_fc_offset *
									    vs->vibe_ticker / vs->vibe_count;
				switch ( vs->vibe_quarter )
				{
		    		case 0: break;
		    		case 1:
						vibe_increment = vs->vibe_fc_offset - vibe_increment;
						break;

		    		case 2:
						vibe_increment = -vibe_increment;
						break;

		    		case 3:
						vibe_increment = ( vs->vibe_fc_offset
						    			- vibe_increment ) * -1;
						break;
				}
				if ( vs->vibe_sweep && vs->wave->info.vibrato_sweep )
				{
		    		vibe_increment = ( long ) vibe_increment *
						( long )( vs->wave->info.vibrato_sweep - vs->vibe_sweep--) /
						( long ) vs->wave->info.vibrato_sweep;
				}
                gf1SetVoiceFreq( voice, ( ( vs->fc_register+vibe_increment ) << 1 ) );
				if ( ++vs->vibe_ticker >= vs->vibe_count )
				{
		    		vs->vibe_quarter = ( vs->vibe_quarter + 1 ) % 4;
		    		vs->vibe_ticker = 0;
				}
	    	}
		}
    }
    return 0;
}

PRIVATE void
gf1CalcVibrato(
	int 		voice,
	int 		restart,
	int 		channel
	)
{
	VOC_STATUS 	*vs;
	CHAN_STATUS	*cs;
	GF1_WAVE	*wave;
	GF1_INFO	*inf;
	DWORD		vib_hertz;
	DWORD		f1_index, f2_index, f1, f2;
	UINT 		f1_power, f2_power;
	UINT 		depth, rate, sweep;

	vs = &voice_status[ voice ];
	cs = &channel_status[ channel ];
	wave = vs->wave;
	inf = &wave->info;

	depth = cs->vib_depth;
	if ( depth )
	{
	    rate	= cs->vib_rate;
	    sweep	= cs->vib_sweep;
	}
	else
	{
	    depth	= inf->vibrato_depth;
	    rate 	= inf->vibrato_rate;
	    sweep	= inf->vibrato_sweep;
	}
	vib_hertz = (2334L * rate + 5000L ) / 100L;

	vs->vibe_count = 1000000000L /
		( (256-VIBRATO_RESOLUTION ) * 320L ) /
		( vib_hertz * 4L );

	vs->vibe_count +=
		( ( 1000000000L / ( (256-VIBRATO_RESOLUTION ) * 320L )
		% ( vib_hertz * 4L ) ) > ( vib_hertz * 2L ) ) ? 1 : 0;

	if ( vs->vibe_count == 0 )
		vs->vibe_count = 1;

	f1_index = (depth / 21 ) % LOG_TAB_SIZE;
	f2_index = (depth / 21 + 1 ) % LOG_TAB_SIZE;
	f1_power = (depth / 21 ) / LOG_TAB_SIZE;
	f2_power = (depth / 21 + 1 ) / LOG_TAB_SIZE;
	f1 = gf1_log_table[f1_index] << f1_power;
	f2 = gf1_log_table[f2_index] << f2_power;
	vs->vibe_fc_offset = ( ( (f2 - f1 ) * ( long )(depth % 21L ) / 21L + f1 ) * ( long ) vs->fc_register / 1024 - vs->fc_register ) / 2L;

	if ( restart )
	{
		vs->vibe_sweep = sweep;
		vs->vibe_ticker = 0;
		vs->vibe_quarter = 0;
	}
}


PRIVATE void
gf1CalcTremolo(
	int  		voice,
	int  		channel
	)
{
	VOC_STATUS 	*vs;
	CHAN_STATUS	*cs;
	GF1_INFO	*inf;
	DWORD		volume;
	DWORD		rr_period10M;
	DWORD		period10M;
	UINT 		d;
	UINT 		d1;
	UINT 		rr;
	UINT 		depth;
	UINT 		rate;
		
	vs = &voice_status[ voice ];
	inf = &vs->wave->info;

	cs = &channel_status[ channel ];

	depth = cs->trem_depth;
	if ( depth )
	    rate 	= cs->trem_rate;
	else
	{
	    depth	= inf->tremolo_depth;
	    rate	= inf->tremolo_rate;
	}
	volume = ( (
	    ( DWORD )( inf->envelope_offset[ SUSTAIN_POINT ] ) *
			    vs->attenuation ) / RANGE_MASTER );
	d = ( depth + 1 ) / 8;
	d1 = d >> 1;
	if ( ( volume - d1 ) < MIN_OFFSET )
		volume = MIN_OFFSET + d1;

	if ( ( volume + d1 + ( d & 1 ) ) > MAX_OFFSET )
		volume = MAX_OFFSET - d1 - (d&1 );

	vs->trem_start = volume - d1;
	vs->trem_end = volume + d1 + ( d & 1 );

	/* calculate rate for volinc */
	rr_period10M = 16L * gf1_voices * ( vs->trem_end - vs->trem_start << 4 );
	period10M = 10000000L / 2L / (2334L * rate + 5000L ) * 100000L;

	/* find range */
	for ( rr = 0; rr <= 3; rr++ )
	{
	    if ( rr_period10M > period10M )
			break;
	    rr_period10M *= 8L;
	}
	if ( rr > 3 )
		rr = 3;
	vs->trem_volinc = ( rr_period10M + ( period10M >> 1 ) ) / period10M;
	if ( vs->trem_volinc > 63 )
		vs->trem_volinc = 63;
	vs->trem_volinc |= rr << 6;
}

PRIVATE void
gf1EnableVibrato(
    void
    )
{
	gf1EnableTimer2( gf1DoNoteVibrato, VIBRATO_RESOLUTION );
}

PRIVATE void
gf1DisableVibrato(
    void
    )
{
	gf1DisableTimer2();
}


PRIVATE void
gf1ChangeVoiceVolume(
	int 		voice,
	UINT		volume
	)
{
    VOC_STATUS	*vs;
	GF1_INFO	*inf;
    UINT 		old_volume;
    UINT 		new_volume;
	WORD		vol;
    int 		prev_env;
    int 		next_env;
    int 		index;

    gf1Enter();
    vs = &voice_status[ voice ];
	inf = &vs->wave->info;

    if ( gf1_linear_volumes )
		vol = vol_table[ vs->velocity ] * vol_table[ volume ];
	else if ( vs->velocity && volume )
		vol = ( vs->velocity + 128 ) * ( volume + 128 );
	else
		vol = 0;

	vs->attenuation = ( DWORD )(
						( ( DWORD ) gf1_m_volume + 64L ) * vol ) / 48705U;

    if ( inf->tremolo_depth ||
		 channel_status[ vs->channel ].trem_depth )
	{
		gf1CalcTremolo( voice, vs->channel );
    }
    gf1SelectVoice( voice );

    /* stop the envelope hardware */
    vs->volume_control &= ~( GF1_LPE|GF1_BLE|GF1_IRQE|GF1_IRQP);
	gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED | GF1_STOP );

	old_volume = gf1InWord( GET_VOLUME ) >> 4;

    index = vs->envelope_count;
    if ( ! vs->extra_envelope_point )
	{
		if ( vs->next_offset == vs->prev_offset ||
	  		( ( index == ( SUSTAIN_POINT + 1 ) )&& 
		      ( inf->modes & PATCH_SUSTAIN ) 	&&
		      ( vs->status & STAT_VOICE_SUSTAINING )
			) )
		{
	    	vs->percent_complete = 1024;
		}
		else
		{
	    	vs->percent_complete =
				( ( long )( old_volume ) - ( ( long ) vs->prev_offset<<4 ) ) *
				1024L / ( ( ( long ) vs->next_offset - ( long ) vs->prev_offset ) << 4 );

		    if ( vs->percent_complete < 0 )
				vs->percent_complete *= -1;
			else if ( vs->percent_complete == 0 )
				vs->percent_complete = 1024;
		}
	/* tremolo can make the volume be greater than the sustain point */
		if ( vs->percent_complete > 1024 )
			vs->percent_complete = 1024;
	}
    if ( index == 0 )
	{
		prev_env = 0;
		next_env = ( ( long ) inf->envelope_offset[ 0 ] *
					( long ) vs->attenuation ) / RANGE_MASTER;
    }
	else if ( index >= ENVELOPES )
	{
		next_env = ( ( long ) inf->envelope_offset[ ENVELOPES - 1 ] *
					( long ) vs->attenuation ) / RANGE_MASTER;
		prev_env = next_env;
    }
	else if ( ( index == ( SUSTAIN_POINT + 1 ) ) && 
	  		  ( inf->modes & PATCH_SUSTAIN ) )
	{
	  	if ( vs->status & STAT_VOICE_SUSTAINING )
		{
			next_env = prev_env = ( ( long ) vs->sust_offset *
						( long ) vs->attenuation ) / RANGE_MASTER;
		}
		else
		{
			next_env = ( ( long ) inf->envelope_offset[ index ] *
				( long ) vs->attenuation ) / RANGE_MASTER;
			prev_env = ( ( long ) vs->sust_offset *
				( long ) vs->attenuation ) / RANGE_MASTER;
	  	}
    }
	else
	{
		next_env = ( ( long ) inf->envelope_offset[ index ] *
			( long ) vs->attenuation ) / RANGE_MASTER;
		prev_env = ( ( long ) inf->envelope_offset[ index-1 ] *
			( long ) vs->attenuation ) / RANGE_MASTER;
    }
    if ( next_env < MIN_OFFSET )
		next_env = MIN_OFFSET;

    if ( prev_env < MIN_OFFSET )
		prev_env = MIN_OFFSET;

    new_volume = ( ( ( long )next_env - ( long )prev_env ) *
				   ( long ) vs->percent_complete + 512L ) / 1024L + prev_env;
    if ( new_volume > MAX_OFFSET )
		new_volume = MAX_OFFSET;
    else if ( new_volume < MIN_OFFSET )
		new_volume = MIN_OFFSET;

    old_volume >>= 4;
    if ( new_volume < old_volume )
	{
		vs->volume_control |= GF1_DIR_DEC;
		gf1OutByte( SET_VOLUME_START, ( BYTE ) ( new_volume ) );
		gf1OutByte( SET_VOLUME_END, ( BYTE ) ( old_volume ) );
    }
	else if ( new_volume > old_volume )
	{
		if ( index > SUSTAIN_POINT+1 )
		{
	    /* after decay part of release */
	    	new_volume = old_volume;
		}
		else
		{
	    	vs->volume_control &= ~GF1_DIR_DEC;
			gf1OutByte( SET_VOLUME_START, ( BYTE ) ( old_volume ) );
			gf1OutByte( SET_VOLUME_END, ( BYTE ) ( new_volume ) );
		}
    }
    if ( new_volume != old_volume )
	{
		vs->next_offset = next_env;
		vs->prev_offset = prev_env;
		gf1OutByte( SET_VOLUME_RATE, 66 );
    }
    vs->extra_envelope_point = vs->envelope_count;
    vs->volume_control |= GF1_IRQE; /* re-enable ints */
	gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );

    gf1Leave();
}

PRIVATE void
gf1MidiChangeVolume(
	int 		channel,
	UINT		volume
	)
{
    VOC_STATUS	*vs;
    UINT        voice;

    if ( volume < MIN_VOLUME )
		volume = MIN_VOLUME;
    else if ( volume > MAX_VOLUME )
		volume = MAX_VOLUME;

    gf1Enter();
    channel_status[ channel ].volume = volume;
    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( vs->status & STAT_VOICE_RUNNING && vs->channel == channel )
	    	gf1ChangeVoiceVolume( voice, volume );
    }
    gf1Leave();
}

PRIVATE void
gf1ChannelPitchBend(
	int 		channel,
	UINT 		bend
	)
{
    VOC_STATUS	*vs;
    UINT		voice;
    BYTE 		vib_depth;

    gf1Enter();
	vib_depth = channel_status[ channel ].vib_depth;
    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( vs->status & STAT_VOICE_RUNNING && vs->channel == channel )
		{
		    vs->fc_register = ( ( long ) vs->ofc_reg * ( long )bend) >> 10;

		    if ( vs->wave->info.vibrato_depth || vib_depth )
				gf1CalcVibrato( voice, 1, channel );

            gf1SetVoiceFreq( voice, vs->fc_register << 1 );
		}
    }
    channel_status[ channel ].pitch_bend = bend;
    gf1Leave();
}

/****************************************************************************
* gf1MidiMasterVolume - Set master MIDI volume.
*****************************************************************************/
PRIVATE void
gf1MidiMasterVolume(
	int 		master_volume
	)
{
	VOC_STATUS	*vs;
	UINT 		voice;

	if ( master_volume < MIN_VOLUME )
		master_volume = MIN_VOLUME;
	else if ( master_volume > MAX_VOLUME )
		master_volume = MAX_VOLUME;

	gf1_m_volume = master_volume;
	vs = voice_status;
	gf1Enter();
	for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
	    if ( vs->status & STAT_VOICE_RUNNING )
			gf1ChangeVoiceVolume( voice, channel_status[ vs->channel ].volume );
	}
	gf1Leave();
}

PRIVATE void
gf1MidiSetVibrato(
	int 		channel,
	int 		value
	)
{
    VOC_STATUS 	*vs;
	CHAN_STATUS	*cs;
    UINT 		depth, old_depth, old_chan_depth, new_depth;
    UINT 		rate, voice;

	if ( value == 0 )
		rate = depth = 0;
	else
	{
		rate = 200;
		depth = 2 + value * 30 / 128;
	}

    cs = &channel_status[ channel ];
    old_chan_depth	= cs->vib_depth;
    cs->vib_depth	= depth;
    cs->vib_rate	= rate;
    cs->vib_sweep	= 0;

    gf1Enter();
    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( vs->status & STAT_VOICE_RUNNING && vs->channel == channel )
		{
	    	old_depth = ( old_chan_depth || vs->wave->info.vibrato_depth );
	    	new_depth = ( depth || vs->wave->info.vibrato_depth );
	    	if ( ( old_depth != 0 ) ^ ( new_depth != 0 ) )
			{
				if ( new_depth && ( vibrato_voice_count++ == 0 ) )
					gf1EnableVibrato();
				else if ( ! new_depth && ( --vibrato_voice_count == 0 ) )
					gf1DisableVibrato();
		    }
	    	if ( vs->wave->info.vibrato_depth || cs->vib_depth )
			{
				gf1CalcVibrato( voice, !depth, channel );
	    	}
		}
    }
    gf1Leave();
}


USHORT
gf1CalcFC(
	UINT		sample_ratio,
	long		root,
	long		frequency
	)
{
	ULONG		freq_register;

	freq_register  = ( ( ( DWORD ) frequency << 9 ) +
			( DWORD )( root >> 1 ) ) /
			( DWORD ) root;

	freq_register  = ( ( ( DWORD ) freq_register << 9 ) +
			( DWORD )( sample_ratio >> 1 ) ) / 
			sample_ratio ;

	return( ( USHORT ) freq_register );
}

PRIVATE long
gf1CalcScaledFreq(
	GF1_WAVE	*wave,
	int 		note
	)
{
	long lnote = note;
	long interp1, f1;
	long frequency;

	lnote -= wave->info.scale_frequency;
	lnote *= wave->info.scale_factor;
	interp1 = lnote & ( ( 1L << 10 )-1L );
	lnote = ( lnote >> 10 ) + wave->info.scale_frequency;

	if ( lnote < 0 )
        lnote = 0;
	if ( lnote >= SCALE_TABLE_SIZE )
	    frequency = gf1_scale_table[ SCALE_TABLE_SIZE-1 ];
	else
    {
	    frequency = gf1_scale_table[ lnote ];
	    f1 = gf1_scale_table[ lnote+1 ];
        if ( lnote < 95 )
		    frequency = ( ( (f1-frequency ) * interp1 )>>10 ) + frequency;
        else
        {
		    frequency >>= 5;
		    frequency = ( ( (f1-frequency ) * interp1 )>>10 ) + frequency;
		    frequency <<= 5;
	    }
	}
	return(frequency );
}

PRIVATE void
gf1MidiStopVoice(
    int     voice
    )
{
	VOC_STATUS  *vs;
	UINT        volume;

	gf1Enter();
	vs = &voice_status[ voice ];
	if ( vs->status & STAT_VOICE_RUNNING )
    {
	    vs->status &= ~STAT_VOICE_SUSTAINING;
	    vs->status |= ( STAT_VOICE_RELEASING | STAT_VOICE_STOPPING );

        gf1SelectVoice( voice );

	    /* stop volume and then restart */
        gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED | GF1_STOP );

		volume = gf1InWord( GET_VOLUME ) >> 8;
	    if ( volume > MIN_OFFSET )
        {
		    vs->volume_control = GF1_DIR_DEC;
			gf1OutByte( SET_VOLUME_START, MIN_OFFSET );
			gf1OutByte( SET_VOLUME_RATE, 63 );

            gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );
	    }
	}
	gf1Leave();
}

void
gf1MidiWaitVoice(
	int     voice
	)
{
	VOC_STATUS  *vs;
	UINT        vc;
/*
* make sure volume has stopped and is inaudible, 
* then free voice
*/
	gf1Enter();
	vs = &voice_status[ voice ];
	if ( vs->status & STAT_VOICE_RUNNING )
	{
        gf1SelectVoice( voice );
	    do
		{
			vc = gf1InByte( GET_VOLUME_CONTROL );
	    } while ( ( vc & ( GF1_STOP | GF1_STOPPED ) ) == 0 );

        gf1OutDelay( SET_CONTROL, GF1_STOPPED | GF1_STOP );

	    vs->status = 0;
	    vs->extra_envelope_point = 0;

	    gf1FreeVoice( voice );

	    note_status[ voice ].flags = 0;
	    note_status[ voice ].note = 0;
	    if ( vs->wave->info.vibrato_depth || channel_status[ vs->channel ].vib_depth )
        {
    		if ( --vibrato_voice_count == 0 )
                gf1DisableVibrato();
	    }
	}
	gf1Leave();
}

PRIVATE void
gf1MidiStopNote(
    int     note_voice
    )
{
	int i, voice;

	gf1Enter();

	for ( i = 0; i < 4; i++ )
    {
	    voice = other_voices[ note_voice ][ i ];
	    if ( voice == -1 )
            continue;
	    gf1MidiStopVoice( voice );
	}
	/* make sure all volumes have stopped and are inaudible */
	for ( i = 0; i < 4; i++ )
    {
	    voice = other_voices[ note_voice ][ i ];
	    if ( voice == -1 )
            continue;
	    if ( voice_status[ voice ].status & STAT_VOICE_RUNNING )
        {
		    gf1MidiWaitVoice( voice );
	    }
	}
	gf1Leave();
}

/****************************************************************************
* gf1MidiNoteOff - Turn note OFF for specified MIDI channel
*****************************************************************************/
PRIVATE void
gf1MidiNoteOff(
    int         channel,
    int         note
    )
{
	VOC_STATUS  *vs;
	int         offset;
	int         note_voice;
	int         voice;
	int         i;
	WORD        current_v;
	
	gf1Enter();
	if ( channel_status[ channel ].flags & CHANNEL_SUSTAIN )
    {
	    for ( note_voice = 0; note_voice < MAX_VOICES; note_voice++ )
        {
		    if ( note_status[ note_voice ].note == note &&
		        note_status[ note_voice ].channel == channel )
            {
		        note_status[ note_voice ].flags |= NOTE_SUSTAIN;
		    }
	    }
	    gf1Leave();
	    return;
	}
	for ( note_voice = 0; note_voice < MAX_VOICES; note_voice++ )
    {
	    if ( note_status[ note_voice ].note == note &&
		     note_status[ note_voice ].channel == channel
           )
           break;
	}
	if ( note_voice != MAX_VOICES )
    {
	    for ( i = 0; i < 4; i++ )
        {
		    voice = other_voices[ note_voice ][ i ];
		    if ( voice == -1 )
                continue;

		    vs = &voice_status[ voice ];
		    gf1AdjustPriority( voice, 32 );
		    if ( ( vs->status & STAT_VOICE_RUNNING ) &&
		        !( vs->status & STAT_VOICE_RELEASING )
                )
            {
		        note_status[ voice ].flags &= ~NOTE_SUSTAIN;
		        note_status[ voice ].note = 0;
		        vs->status &= ~STAT_VOICE_SUSTAINING;
		        vs->status |= STAT_VOICE_RELEASING;

		        if ( vs->wave->info.modes & PATCH_FAST_REL )
                {
                    gf1SelectVoice( voice );

    			    /* stop voice and than restart */
                    gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED|GF1_STOP );

			        vs->volume_control |= GF1_DIR_DEC|GF1_IRQE;
			        vs->volume_control &= ~( GF1_BLE|GF1_LPE );
			        vs->envelope_count = ENVELOPES - 1;

					current_v = gf1InWord( GET_VOLUME ) >> 8;
    			    if ( vs->sust_offset == 0 )
                    {
			            vs->sust_offset = current_v * RANGE_MASTER /
									        vs->attenuation;
    			    }
			        vs->prev_offset = current_v;
			        vs->next_offset = MIN_OFFSET;
					gf1OutByte( SET_VOLUME_RATE, vs->wave->info.envelope_rate[ 5 ] );
					gf1OutByte( SET_VOLUME_START, MIN_OFFSET );
                    gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );
		        }
                else if ( vs->wave->info.modes & PATCH_SUSTAIN )
                {
                    gf1SelectVoice( voice );
    			    /* stop voice and than restart */
                    gf1OutDelay( SET_VOLUME_CONTROL, GF1_STOPPED|GF1_STOP );

			        if ( vs->envelope_count < 3 )
                        vs->envelope_count = 3;

			        vs->volume_control |= GF1_IRQE;
			        vs->volume_control &= ~( GF1_BLE|GF1_LPE );
					gf1OutByte( SET_VOLUME_RATE, vs->wave->info.envelope_rate[ 3 ] );

					current_v = gf1InWord( GET_VOLUME ) >> 8;
			        if ( vs->sust_offset == 0 )
                    {
			            vs->sust_offset = current_v * RANGE_MASTER /
                    				        vs->attenuation;
			        }
    			    vs->prev_offset = current_v;
			        if ( vs->wave->info.envelope_offset[ 3 ] > current_v )
                    {
			            vs->volume_control &= ~GF1_DIR_DEC;
						gf1OutByte( SET_VOLUME_START, current_v );

			            offset = ( ( UINT ) ( vs->wave->info.envelope_offset[ 3 ] ) * 
                    				        vs->attenuation ) / RANGE_MASTER;
			            if ( offset < MIN_OFFSET )
                            offset = MIN_OFFSET;
			            vs->next_offset = offset;
						gf1OutByte( SET_VOLUME_END, ( BYTE ) vs->next_offset );
                    }
                    else
                    {
			            vs->volume_control |= GF1_DIR_DEC; 
			            offset = ( ( UINT )( vs->wave->info.envelope_offset[ 3 ] ) * 
				                            vs->attenuation ) / RANGE_MASTER;
			            if ( offset < MIN_OFFSET )
                            offset = MIN_OFFSET;
			            vs->next_offset = offset;
						gf1OutByte( SET_VOLUME_START, ( BYTE ) offset );
						gf1OutByte( SET_VOLUME_END, current_v );
	    		    }
                    gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );
		        }
		    }
	    }
	}
	gf1Leave();
}


/****************************************************************************
* gf1MidiNoteOn - Turn note ON for specified channel
*****************************************************************************/
PRIVATE void
gf1MidiNoteOn(
	int 		channel,
	int 		note,
	int 		velocity,
	int 		priority
	)
{
	int         layer;
	int         i;
	int         nwaves;
	int         offset;
	int         nvoices;
    int         voice;
    GF1_PATCH   *patch;
	GF1_WAVE    *wave;
	GF1_WAVE    *nwave;
	VOC_STATUS  *vs;
	CHAN_STATUS *cs;
	long        frequency; 
	int         voices[ 4 ];

	if ( velocity == 0 )
    {
		gf1MidiNoteOff( channel, note );
		return;
	}
	cs = &channel_status[ channel ];
	if ( channel == 15 )
	{
		if ( note < 35 || note > 81 )
			return;

		if ( program_num[ note + 93 ] == 255 )
			return;

		patch = gf1_patch[ program_num[ note + 93 ] ];
	}
	else
	{
	    patch = cs->patch;
	}
	if ( patch == NULL )
    {
        return;
    }
	gf1Enter();
	nvoices = patch->nlayers;
	voices[ 0 ] = -1;

	for ( layer = 0; layer < nvoices; layer++ )
    {
	    voices[ layer ] = NO_MORE_VOICES;
	    wave = patch->layer_waves[ layer ];

	    if ( wave->info.scale_factor != 1024 )
		    frequency = gf1CalcScaledFreq( wave, note );
	    else
       		frequency = gf1_scale_table[ note ];

	    /* search for correct wave in wave list */
	    nwaves = patch->layer_nwaves[ layer ];
	    for ( i = 0; i < nwaves; i++, wave = nwave )
        {
		    nwave = wave+1;
		    if ( wave->info.high_frequency >= frequency &&
		         wave->info.low_frequency <= frequency )
            {
		        if ( i < ( nwaves - 1 ) )
                {
			        if ( nwave->info.high_frequency >= frequency &&
      			         nwave->info.low_frequency <= frequency )
                    {
    			        continue;
	    		    }
		        }
		        break;
		    }
	    }
	    if ( i == nwaves )
            continue; /* ignore this layer */

	    voice = gf1AllocVoice( priority, gf1MidiStopNote );

	    if ( voice == NO_MORE_VOICES )
            continue;

	    voices[ layer ] = voice;
	    note_status[ voice ].note = note;
	    note_status[ voice ].channel = channel;
	    note_status[ voice ].flags = 0;
	    vs = &voice_status[ voice ];
	    vs->channel = channel;

	    if ( velocity < MIN_VOLUME )
            velocity = MIN_VOLUME;
	    else if ( velocity > MAX_VOLUME )
            velocity = MAX_VOLUME;

	    vs->velocity = velocity;
	    if ( gf1_linear_volumes )
        {
		    vs->attenuation = ( ( ( long ) gf1_m_volume + 64L ) *
		        ( ( long ) vol_table[ velocity] ) *
		        ( ( long ) vol_table[ cs->volume ] )
		        / 48705L );
        }
	    else
        {
		    vs->attenuation = ( ( ( long ) gf1_m_volume + 64L ) * 
			    ( ( long ) velocity + 128L ) *
			    ( ( long )cs->volume + 128L )
			    / 48705L );
	    }
	    vs->wave = wave;
	    vs->ofc_reg = gf1CalcFC( wave->sample_ratio, wave->info.root_frequency, frequency );

	    if ( cs->pitch_bend == 1024 )
		    vs->fc_register = vs->ofc_reg;
        else
		    vs->fc_register = ( ( long ) vs->ofc_reg * ( long ) cs->pitch_bend ) >> 10;

	    /* Set page register for all I-O's. */
        gf1SetVoiceFreq( voice, vs->fc_register << 1 );

		gf1OutByte( SET_BALANCE, ( cs->balance != 16 ) ? cs->balance : wave->info.balance );

	    vs->voice_control = 0;
	    vs->volume_control = 0;

	    if ( wave->info.modes & PATCH_LOOPEN )
		    vs->voice_control |= GF1_LPE;

	    if ( wave->info.modes & PATCH_16 )
    		vs->voice_control |= GF1_WT16;

	    if ( wave->info.modes & PATCH_BIDIR )
    		vs->voice_control |= GF1_BLE;

	    if ( wave->info.modes & PATCH_BACKWARD )
        {
		    vs->voice_control |= GF1_DIR_DEC;
			gf1OutWord( SET_ACC_LOW, wave->end_acc_low );
		    gf1OutWord( SET_ACC_HIGH, wave->end_acc_high );
	    }
        else
        {
		    gf1OutWord( SET_ACC_LOW, wave->start_acc_low );
		    gf1OutWord( SET_ACC_HIGH, wave->start_acc_high );
	    }
	    gf1OutWord( SET_START_LOW, wave->start_low );
	    gf1OutWord( SET_START_HIGH, wave->start_high );
	    gf1OutWord( SET_END_LOW, wave->end_low );
	    gf1OutWord( SET_END_HIGH, wave->end_high );

	    vs->envelope_count = 0;

		gf1OutByte( SET_VOLUME_RATE, wave->info.envelope_rate[ 0 ] );
		gf1OutByte( SET_VOLUME_START, MIN_OFFSET );

	    offset = ( ( UINT )( wave->info.envelope_offset[ 0 ] ) * 
			        	       vs->attenuation ) / RANGE_MASTER;
	    if ( offset < MIN_OFFSET )
            offset = MIN_OFFSET;

	    vs->sust_offset = 0;
	    vs->prev_offset = 0;
	    vs->next_offset = offset;

		gf1OutByte( SET_VOLUME_END, offset );
	   
	    vs->volume_control |= GF1_IRQE;

	    if ( wave->info.vibrato_depth || cs->vib_depth )
        {
		    gf1CalcVibrato( voice, 1, channel );
		    if ( vibrato_voice_count++ == 0 )
                gf1EnableVibrato();
	    }
	    if ( wave->info.tremolo_depth || cs->trem_depth )
    		gf1CalcTremolo( voice, channel );

	    vs->status = STAT_VOICE_RUNNING;
	    vs->extra_envelope_point = 0;

        gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );
        gf1OutDelay( SET_CONTROL, vs->voice_control );
	}
	/* N^2 -- but 4 makes 16 */
	for ( layer = 0; layer < nvoices; layer++ )
    {
	    if ( voices[ layer ] != NO_MORE_VOICES )
        {
		    for ( i = 0; i < nvoices; i++ )
            {
		        other_voices[ voices[ layer ]][ i ] = voices[ i ];
		    }
		    for ( ; i < 4; i++ )
            {
                other_voices[ voices[ layer ]][ i ] = -1;
            }
	    }
	}
	gf1Leave();
}


void
gf1MidiChannelSustain(
    int         channel,
    int         sustain
    )
{
	UINT        i;

	gf1Enter();
	if ( sustain )
	{
	    channel_status[ channel ].flags |= CHANNEL_SUSTAIN;
	}
	else
    {
	    channel_status[ channel ].flags &= ~CHANNEL_SUSTAIN;
	    for ( i = 0; i < gf1_voices; i++ )
        {
		    if ( note_status[ i ].channel == channel
				 && ( note_status[ i ].flags & NOTE_SUSTAIN )
				 && note_status[ i ].note
			   )
            {
		        gf1MidiNoteOff( note_status[ i ].channel, note_status[ i ].note );
		    }
	    }
	}
	gf1Leave();
}

PRIVATE int
gf1NoteVoiceHandler(
    int         voice
    )
{
	VOC_STATUS  *vs;

	vs = &voice_status[ voice ];
	if ( vs->status & STAT_VOICE_RUNNING )
    {
	    vs->voice_control &= ~( GF1_IRQE|GF1_IRQP );
        gf1SelectVoice( voice );
        gf1OutDelay( SET_CONTROL, vs->voice_control );

	    if ( vs->status & STAT_VOICE_STOPPING )
        {
		    if ( vs->envelope_count != 0 )
            {
		        gf1MidiStopVoice( voice );
		        gf1MidiWaitVoice( voice );
    		}
	    }
	    return 1;
	}
	return 0;
}

PRIVATE int
gf1NoteVolumeHandler(
    int         voice
    )
{
	VOC_STATUS  *vs;
	int         index;
	char        extra;
	USHORT      temp_vol;
	USHORT      offset;

	vs = &voice_status[ voice ];
	if ( vs->status & STAT_VOICE_RUNNING )
    {
	    vs->volume_control &= ~( GF1_IRQP | GF1_IRQE );
        gf1SelectVoice( voice );
        gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );

	    if ( vs->extra_envelope_point )
        {
		    vs->envelope_count = vs->extra_envelope_point;
		    vs->extra_envelope_point = 0;
		    extra = 1;
	    }
        else
        {
		    vs->envelope_count++;
		    extra = 0;
	    }
	    index = vs->envelope_count;			
	    if ( index >= ENVELOPES )
        {
		    if ( ! extra )
                vs->prev_offset = vs->next_offset;

		    vs->status &= ~STAT_VOICE_SUSTAINING;
		    vs->status |= ( STAT_VOICE_RELEASING | STAT_VOICE_STOPPING );

		    /* play sampled release */
		    /* Stop voice on boundary with interrupt. */
		    vs->voice_control &= ~GF1_LPE; /* sampled release */
		    vs->voice_control |= GF1_IRQE;

		    /* Set end or start address depending on direction.*/
		    if ( vs->wave->info.modes & PATCH_BACKWARD )
            {
		        gf1OutWord( SET_START_LOW, vs->wave->start_acc_low );
		        gf1OutWord( SET_START_HIGH, vs->wave->start_acc_high );
		    }
            else
            {
		        gf1OutWord( SET_END_LOW, vs->wave->end_acc_low );
		        gf1OutWord( SET_END_HIGH, vs->wave->end_acc_high );
		    }
            gf1OutDelay( SET_CONTROL, vs->voice_control );
	    }
        else if ( ( index == (SUSTAIN_POINT+1 ) ) && 
		          ( vs->wave->info.modes & PATCH_SUSTAIN ) &&
		         !( vs->status & STAT_VOICE_RELEASING )
                )
        {
		    vs->status |= STAT_VOICE_SUSTAINING;
		    if ( ! extra )
                vs->prev_offset = vs->next_offset;

		    vs->sust_offset = vs->wave->info.envelope_offset[ SUSTAIN_POINT ];
		    /* Check here for tremolo on/off. */
		    if ( vs->wave->info.tremolo_depth || channel_status[ vs->channel ].trem_depth )
            {
				temp_vol = gf1InWord( GET_VOLUME ) >> 4;
		        if ( temp_vol > vs->trem_start )
			        vs->volume_control |= GF1_DIR_DEC;
		        else
			        vs->volume_control &= ~GF1_DIR_DEC;

				gf1OutByte( SET_VOLUME_RATE, vs->trem_volinc );
				gf1OutByte( SET_VOLUME_START, vs->trem_start );
				gf1OutByte( SET_VOLUME_END, vs->trem_end );

		        vs->volume_control |= ( GF1_BLE | GF1_LPE );
		    }
	    }
        else
        {
		    vs->volume_control |= GF1_IRQE;
			gf1OutByte( SET_VOLUME_RATE, vs->wave->info.envelope_rate[ index ] );

		    if ( extra )
		        offset = vs->next_offset;
            else
            {
		        vs->prev_offset = vs->next_offset;
		        offset = ( ( UINT )( vs->wave->info.envelope_offset[ index ] )* 
			                        vs->attenuation ) / RANGE_MASTER;
		        if ( offset < MIN_OFFSET )
                    offset = MIN_OFFSET;
		        vs->next_offset = offset;
		    }

		    /* Set direction bit for decreasing volumes. */

		    if ( vs->wave->info.envelope_offset[ ( index-1 ) ] >
		         vs->wave->info.envelope_offset[ index ] )
            { /* Decrease volume. */
		        vs->volume_control |= GF1_DIR_DEC;
				gf1OutByte( SET_VOLUME_START, ( BYTE ) offset );
		    }
            else
            {
		        vs->volume_control &= ~GF1_DIR_DEC;
				gf1OutByte( SET_VOLUME_END, ( BYTE ) offset );
		    }
	    }
        gf1OutDelay( SET_VOLUME_CONTROL, vs->volume_control );
	    return 1;
	}
	return 0;
}

/****************************************************************************
* gf1MidiChangeProgram - Change program for specified MIDI channel.
*****************************************************************************/
PRIVATE void
gf1MidiChangeProgram(
	int 		channel,
	int			program
	)
{
	int			pgm;

    pgm = program_num[ program ];

	if ( pgm == 255 )
		channel_status[ channel ].patch = NULL;
	else
		channel_status[ channel ].patch = gf1_patch[ pgm ];
}

/****************************************************************************
* gf1MidiSetBalance - Set channel balance (panpot)
*****************************************************************************/
PRIVATE void
gf1MidiSetBalance(
	int 		channel,
	int 		balance
	)
{
    VOC_STATUS	*vs;
    UINT 		voice;
    
    if ( balance == -1 )
	{
		channel_status[ channel ].balance = 16;
		return;
    }
    balance >>= 3;
    channel_status[ channel ].balance = balance;
    vs = voice_status;
    gf1Enter();
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( ( vs->status & STAT_VOICE_RUNNING ) &&
			   vs->channel == channel )
		{
            gf1SelectVoice( voice );
        	gf1OutByte( SET_BALANCE, balance );
		}
    }
    gf1Leave();
}

/****************************************************************************
* gf1MidiAllNotesOff - Turn ALL notes of for the specified MIDI channel
*****************************************************************************/
PRIVATE void
gf1MidiAllNotesOff(
	int 	channel
	)
{
    VOC_STATUS	*vs;
    UINT		voice;
    
    gf1Enter();
    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( ( vs->status & STAT_VOICE_RUNNING ) &&
			   vs->channel == channel )
		{
			gf1MidiStopVoice( voice );
		}
    }
    vs = voice_status;
    for ( voice = 0; voice < gf1_voices; voice++, vs++ )
	{
		if ( ( vs->status & STAT_VOICE_RUNNING ) &&
			   vs->channel == channel )
		{
	    	gf1MidiWaitVoice( voice );
		}
    }
    gf1Leave();
}

/****************************************************************************
* gf1MidiPitchBend - apply pitch bend to MIDI channel
*****************************************************************************/
PRIVATE void
gf1MidiPitchBend(
	int 	channel,
	int 	lsb,
	int 	msb
	)
{
	UINT	bend;
	DWORD	wheel;
	long 	sensitivity;
	UINT	semitones;
	DWORD	mantissa;
	DWORD	f1, f2;
	UINT	f1_index, f2_index, f1_power, f2_power;
	char 	bend_down = 0; 

/*
** this equation will turn the 14 bit number into
** a frequency multiplier ranging from 512 to 2048 (.5 to 2 ).
** This should give a possible 1 octave swing in either
** direction.
*/
	if ( pbs_msb[ channel ] )
	{
		wheel = ( ( long )msb << 7) + ( long )lsb - MID_PITCH;
		sensitivity = pbs_msb[ channel ] * wheel;
		if ( sensitivity < 0 )
		{
			bend_down = 1;
			sensitivity = -sensitivity;
		}
		semitones = sensitivity >> 13;
		mantissa = sensitivity % MID_PITCH;
		f1_index = semitones % LOG_TAB_SIZE;
		f2_index = (semitones + 1 ) % LOG_TAB_SIZE;
		f1_power = semitones / LOG_TAB_SIZE;
		f2_power = (semitones + 1 ) / LOG_TAB_SIZE;
		f1 = gf1_log_table[f1_index] << f1_power;
		f2 = gf1_log_table[f2_index] << f2_power;
		bend = ( ( (f2 - f1 ) * mantissa) >> 13 ) + f1;
		if ( bend_down )
			bend = 1048576L / ( long )bend;
	}
	else
	{
		bend = 1024;
	}
	gf1ChannelPitchBend( channel, bend );
}

/****************************************************************************
* gf1MidiParameter - Set MIDI RPN 
*****************************************************************************/
PRIVATE void
gf1MidiParameter(
	int 	channel,
	int 	control,
	int 	value
	)
{
	int		attenuation;

	switch ( control )
	{
	    case 1: /* modulation wheel */
			gf1MidiSetVibrato( channel, value );
			break;

	    case 6: /* data entry msb */
			if (rpn_msb[ channel ] == 0 && rpn_lsb[ channel ] == 0 )
				pbs_msb[ channel ] = value;
			break;

	    case 38: /* data entry lsb */
			if (rpn_msb[ channel ] == 0 && rpn_lsb[ channel ] == 0 )
			    pbs_lsb[ channel ] = value;
			break;

	    case 96: /* data increment */
			if (rpn_msb[ channel ] == 0 && rpn_lsb[ channel ] == 0 )
		    	pbs_lsb[ channel ] += value;
			break;

	    case 97: /* data decrement */
			if (rpn_msb[ channel ] == 0 && rpn_lsb[ channel ] == 0 )
		    	pbs_lsb[ channel ] -= value;
			break;

	    case 7: /* channel volume */
			channel_volume[ channel ] = value;
			attenuation = ( long ) value * channel_expression[ channel ] / 127L;
			gf1MidiChangeVolume( channel, attenuation );
			break;

	    case 39:
			/* volume lsb ignored */
			break;

	    case 10: /* pan (balance ) */			
			gf1MidiSetBalance( channel, value );
			break;

	    case 11: /* channel exporession ( volume ) */
			channel_expression[ channel ] = value;
			attenuation = ( long ) value * channel_volume[ channel ] / 127L;
			gf1MidiChangeVolume( channel, attenuation );
			break;

	    case 43: /* expression lsb ignored */
			break;

	    case 64: /* sustain (damper pedal ) */
			gf1MidiChannelSustain( channel, value >= 24 );
			break;

	    case 100: /* set registered parameter number */
			rpn_lsb[ channel ]=value;
			break;

		case 101: /* set registered parameter number */
			rpn_msb[ channel ]=value;
			break;

	    case 121: /* reset all controllers */
			gf1MidiChannelSustain( channel, 0 );
			gf1MidiSetVibrato( channel, 0 );
			gf1ChannelPitchBend( channel, 1024 );
			channel_volume[ channel ] = 100;
			channel_expression[ channel ] = 127;
			gf1MidiChangeVolume( channel, 100 );
			gf1MidiSetBalance( channel, -1 ); /* use patch builtin pan */
			break;

	    case 120: /* all sounds off */
	    case 123: /* all notes off */
	    case 124: /* omni off */
	    case 125: /* omni on */
	    case 126: /* mono */
	    case 127: /* mono */
			gf1MidiAllNotesOff( channel );
			break;
	}
}

/****************************************************************************
* gf1LoadPatches - Loads patches into GF1 DRAM
*****************************************************************************/
PRIVATE int
gf1LoadPatches(
	char 		*name		// INPUT : Name of configuration file
	)
{
	char 		*text;
	char		fname[ 255 + 1 ];
	char 		*cp;
	WORD 		xref_num;
	char 		*ud;
	BYTE		*diskbuf;
	BYTE		*cache;
	int			patches;
	int			map_size;
	int  		i;
	int			n;
	int			fh;
	int			load_flags;
	BYTE 		index;

	memset( gf1_patch, 0, sizeof( gf1_patch ) );
	memset( program_num, 255, sizeof( program_num ) );
	memset( xref, 255, sizeof( xref ) );

	load_flags = PATCH_LOAD_8_BIT;

	if ( ( ud = getenv( "ULTRADIR" ) ) == NULL )
		return FILE_NOT_FOUND;

	strcpy( fname, ud );
	strcat( fname, "\\midi\\" );

	if ( ( diskbuf = malloc( CACHE_SIZE ) ) == NULL )
		return NO_MEMORY;

	if ( ( cache = malloc( CACHE_SIZE * 2 ) ) == NULL )
	{
		free( diskbuf );
		return NO_MEMORY;
	}
	if ( gf1_map == NULL )
	{
		if ( ( fh = open( name, O_RDONLY | O_BINARY ) ) == -1 )
		{
			strcat( fname, name );
			if ( ( fh = open( name, O_RDONLY | O_BINARY ) ) == - 1 )
				return FILE_NOT_FOUND;
		}
		map_size = (int) lseek( fh, 0L, SEEK_END );
		if ( ( gf1_map = malloc( map_size + 1 ) ) == NULL )
		{
			close( fh );
			return NO_MEMORY;
		}
		lseek( fh, 0L, SEEK_SET );
		read( fh, gf1_map, map_size );
		close( fh );
		gf1_map[ map_size ] = '\0';
	}
	patches = 0;
	text = strtok( gf1_map, "\r\n" );
    if ( DmxDebug & DMXBUG_MESSAGES )
    {
        printf( "Loading GUS Patches\n" );
    }
	for ( ; text != NULL; text = strtok( NULL, "\r\n" ) )
	{
		if ( *text == '\0' || *text == '#' )
			continue;

		cp = text;

		index = gf1Atoi( &cp, 10 );	// Get Patch #
        if ( ( index > 127 && index < 163 ) || index > 209 )
            continue;

        if ( index >= 163 )
            index -= 35;

		for ( i = 0; i < 4; i++ )
		{
			n = gf1Atoi( &cp, 10 );
		    if ( i == ( gf1_banks - 1 ) )
				xref_num = n;
		}
		for ( i = 0; i < patches; i++ )
		{
			if ( xref[ i ] == xref_num )
				break;
		}
		if ( i < patches )
		{
			program_num[ index ] = i;
			continue;
		}
		strcpy( fname, ud );
		strcat( fname, "\\midi\\" );
		strcat( fname, cp );
		strcat( fname, ".pat" );
        if ( DmxDebug & DMXBUG_MESSAGES )
        {
            printf( "%s", fname );
        }
		gf1_patch[ patches ] = gf1LoadPatch( fname, diskbuf, cache, load_flags );
        if ( DmxDebug & DMXBUG_MESSAGES )
        {
            if ( gf1_patch[ patches ] == NULL )
                printf( " - FAILED\n" );
            else
                printf( " - OK\n" );
        }
		xref[ patches ] = xref_num;
		program_num[ index ] = patches;
		patches++;
	}
	free( gf1_map );
	gf1_map = NULL;

	free( diskbuf );
	free( cache );

	if ( patches == 0 )
		return NO_PATCHES;

	return 0;
}

/****************************************************************************
* GF1_SetMap - Sets MAP for gf1 to read from.
*****************************************************************************/
PUBLIC void
GF1_SetMap(
	char		*gf1ini_map,
	int			map_size
	)
{
	gf1_map = malloc( map_size + 1 );
	if ( gf1_map != NULL )
	{
		memcpy( gf1_map, gf1ini_map, map_size );
	}
	gf1_map[ map_size ] = '\0';
}

/***************************************************************************
*	NAME:  GF1UTILS.C
***************************************************************************/
/****************************************************************************
* gf1Atoi - GF1 Special ASCII to Integer conversion.
*****************************************************************************/
int		// returns: result of conversion
gf1Atoi(
	char		**cp,	// INPUT : String to convert to integer
	int 		base	// INPUT : Base (radix) of number
	)
{
	int 		c;
	int 		result = 0;

	while ( **cp != '\0' )
	{
	    c = **cp;
		if ( c >= '0' && c <= '9' )
		{
		    result = result * base + c - '0';
		    (*cp)++;
		    continue;
		}
	    if ( base == 16 )
		{
			c |= 0x20;
			if ( c >= 'a' && c <= 'f' )
			{
		    	result = result * base + c - 'a' + 10;
		    	(*cp)++;
		    	continue;
			}
		}
		break;
	}
	if ( **cp == ',' )
		(*cp)++;

	while ( **cp == ' ' || **cp == '\t' )
		(*cp)++;

	return result;
}

/****************************************************************************
* gf1GetUltraCfg - Get config. parameters from ULTRASND environment var.
*****************************************************************************/
int		// returns 1=Good Env, 0=Bad Env.
gf1GetUltraCfg(
	GF1_OS		*config		// OUTPUT: configuration parms.
	)
{
	char		*ustr;
	int			banks;

	if ( ( ustr = getenv( "ULTRASND" ) ) == 0L )
		 return 0;

	gf1_max_banks = 4;
	config->forced_base_port	= gf1Atoi( &ustr, 16 );
	config->forced_channel_out	= gf1Atoi( &ustr, 10 );
	config->forced_channel_in	= gf1Atoi( &ustr, 10 );
	config->forced_gf1_irq		= gf1Atoi( &ustr, 10 );
	config->forced_midi_irq		= gf1Atoi( &ustr, 10 );
	banks = gf1Atoi( &ustr, 10 );
	if ( banks >= 1 && banks <= 4 )
		gf1_max_banks = banks;

	return (
		config->forced_base_port &&
		config->forced_channel_in &&
		config->forced_channel_out &&
		config->forced_gf1_irq &&
		config->forced_midi_irq
		);
}

/*===========================================================================
* GF1.C - Gravis Forte 1 (UltraSound) driver
*---------------------------------------------------------------------------*/


PRIVATE int
Gf1InitHardware(
    void
    )
{
	int			rc;

	if ( Gf1Ready )
		return 0;
/*
* os_parms should have been initialized when checking to see if
* board was installed
*/
	os_parms.voices = 28;
/*
* Attempt to load the GF1 operating system with appropriate parameters.
*/
	if ( ( rc = gf1LoadOS( &os_parms ) ) != 0 )
		return rc;

	if ( ( dig_addr = gf1Malloc( DMA_SIZE ) ) == 0L )
	{
		gf1UnloadOS();
		return RS_NOT_ENOUGH_MEMORY;
	}
	dig_left_voice  = gf1AllocVoice( 0, NULL );
	dig_right_voice = gf1AllocVoice( 0, NULL );

	Gf1Ready = 1;
	return 0;
}

PRIVATE VOID
Gf1StopPCM(
    VOID
    )
{
    gf1StopDigital();
}


PRIVATE void
Gf1DeInitHardware(
    void
    )
{
	if ( Gf1Ready )
    {
		gf1RampDown(); /* ramp voices down to zero */
		Gf1StopPCM();
		gf1DmaDeInit();
        gf1ResetUltra( MAX_VOICES );
		gf1UnloadOS();
		Gf1Ready = 0;
		PatchesLoaded = 0;
	}
}

/*--------------------------------------------------------------------------*
* Gf1NoteOff - Turn Note OFF
*---------------------------------------------------------------------------*/
PRIVATE VOID 
Gf1NoteOff(
	WORD		Channel,	// INPUT : Channel 					0..15
	WORD		Note		// INPUT : Note To Kill     		0..107
	)
{
	if ( Gf1Ready )
	{
		gf1MidiNoteOff( Channel, Note );
	}
}


/*--------------------------------------------------------------------------*
* Gf1NoteOn - Turn Note ON
*---------------------------------------------------------------------------*/
PRIVATE VOID 
Gf1NoteOn(
 	WORD		Channel,	// INPUT : Channel to Note On		0..15
	WORD		Note,		// INPUT : Note To Play      		0..107
	WORD		Velocity	// INPUT : Velocity of Note		1..127
	)
{
	if ( Gf1Ready )
	{
		gf1MidiNoteOn( Channel, Note, Velocity, priority[ Channel ] );	
	}
}

/*--------------------------------------------------------------------------*
* Gf1PitchBend - Perform 
*---------------------------------------------------------------------------*/
PRIVATE VOID 
Gf1PitchBend(
	WORD		Channel,	// INPUT : Channel to Note On	  0..14
	WORD		Bend		// INPUT : Pitch Bend             0..255 127=Normal
	)
{
	if ( Gf1Ready )
	{
		Bend <<= 6;
		gf1MidiPitchBend( Channel, ( Bend & 0x007F ), ( Bend >> 7 ) );
	}
}

/*--------------------------------------------------------------------------*
* Gf1Command - Perform Command Logic
*---------------------------------------------------------------------------*/
PRIVATE VOID 
Gf1Command(
	WORD		Channel,// INPUT : Channel           	  0..15
	WORD		CmdCode,// INPUT : Command Code
	WORD		CmdData	// INPUT : Command Related Data
	)
{
	if ( ! Gf1Ready )
		return;

	switch ( CmdCode )
	{
		case CMD_CHANGE_PROGRAM:
			if ( Channel != 15 )
			{
				gf1MidiChangeProgram( Channel, CmdData );
			}
			break;

		case CMD_BANK_SELECT:
			break;

		case CMD_MODULATION:
			gf1MidiParameter( Channel, 1, CmdData );
			break;

		case CMD_VOLUME:
			gf1MidiParameter( Channel, 7, CmdData );
			break;

		case CMD_PANPOT:
			gf1MidiParameter( Channel, 10, CmdData );
			break;

		case CMD_EXPRESSION:
			gf1MidiParameter( Channel, 11, CmdData );
			break;

		case CMD_REVERB_DEPTH:
			break;

		case CMD_CHORUS_DEPTH:
			break;

		case CMD_HOLD1:
			gf1MidiParameter( Channel, 64, CmdData );
			break;

		case CMD_SOFT:
			break;

		case CMD_ALL_SOUNDS_OFF:
			gf1MidiAllNotesOff( Channel );
			break;

		case CMD_RESET_CHANNEL:
			gf1MidiParameter( Channel, 0x79, CmdData );

		case CMD_ALL_NOTES_OFF:
			gf1MidiAllNotesOff( Channel );
			break;

		case CMD_MONO:
		case CMD_POLY:
		default:
			break;
	}
}

/*--------------------------------------------------------------------------*
* Gf1Reset
*---------------------------------------------------------------------------*/
PRIVATE VOID
Gf1Reset(
	void
	)
{
	int 	i;

	if ( Gf1Ready )
	{
	    for ( i = 0; i < 16; i++ )
		{
			Gf1Command( i, CMD_ALL_SOUNDS_OFF, 0 );
			Gf1Command( i, CMD_RESET_CHANNEL, 0 );
	    }
	}
}

/*--------------------------------------------------------------------------*
* Init - Initialize Driver
*---------------------------------------------------------------------------*/
PRIVATE DMX_STATUS
Gf1Init(
	void
	)
{
	if ( Gf1Ready )
		return RS_OKAY;

	if ( ! gf1GetUltraCfg( &os_parms ) )
		return RS_BOARD_NOT_FOUND;

	if ( ! gf1DetectCard( os_parms.forced_base_port ) )
		return RS_BOARD_NOT_FOUND;

	if ( Gf1InitHardware() )
		return RS_FAIL;

	Gf1Reset();
	return RS_OKAY;
}

PRIVATE DMX_STATUS
Gf1MidiInit(
	VOID
	)
{
	DMX_STATUS	rc;
	
	if ( ( rc = Gf1Init() ) != RS_OKAY )
		return rc;

/*
* get_configuration reads a list of patches out of the and the
* associated program numbers out of a configuration file.
*/
	if ( ( rc = gf1LoadPatches( config_name ) ) )
		return rc;

	gf1MidiMasterVolume( MASTER_VOLUME );
	PatchesLoaded = 1;
	return RS_OKAY;
}

/*--------------------------------------------------------------------------*
* DeInit - Reset GUS
*---------------------------------------------------------------------------*/
PRIVATE VOID 
Gf1DeInit(
	void
	)
{
//	Gf1Reset();
	Gf1DeInitHardware();
}

GLOBAL DMX_MUS Gf1Mus =
{
	Gf1MidiInit,
	Gf1DeInit,
	Gf1NoteOff,
	Gf1NoteOn,
	Gf1PitchBend,
	Gf1Command,
#ifdef PROD
	gf1EnterBusy,
	gf1Leave
#endif
};


/****************************************************************************
* GF1_Detect - Detects Gravis Ultrasound
*
* NOTE:
* 	Call this BEFORE DMX_Init
*	0 = Card Found
*	1 = Could not find card address (port)
*****************************************************************************/
PUBLIC int		// Returns: 0=Found, >0 = Error Condition
GF1_Detect(
	VOID
	)
{
	if ( ! gf1GetUltraCfg( &os_parms ) )
		return 1;

	if ( gf1DetectCard( os_parms.forced_base_port ) )
		return 0;

	return 1;
}

/****************************************************************************/
/* DIGITAL AUDIO ************************************************************/
/****************************************************************************/

PRIVATE WORD
gf1GetPlayCounter(
    WORD    wBufferSize     // INPUT : Size of DMA Ring
    )
{
	LOCAL WORD	waddr = 0L;
	DWORD		addr;
    DWORD       pflags;

    pflags = PUSH_DISABLE();
	if ( ! gf1EnterBusy() )
	{
		addr = gf1GetVoicePosition( dig_left_voice ) + 544;
		if ( blk_smoothing )	// 11025 hz
		{
			addr >>= 1;
			addr = ( addr & 0x00000F00L );
		}
		else // 22050 hz
		{
			addr = ( addr & 0x00000E00L );
		}
		gf1Leave();
		waddr = (WORD)addr;
	}
    POP_DISABLE( pflags );
	return waddr;
}

void
gf1BlockReady(
    BYTE        *dmabuf,        // INPUT : Mixed DMA data
    WORD        size,           // INPUT : Size of mixed data
    WORD        offset          // INPUT : Offset into buffer
	)
{
	DWORD		pflags;

	if ( blk_smoothing )	// 11025 hz
	{
    	WORD	*t,*s;
        int     j;

		blk_size = size * 2;
		blk_offset = offset * 2;

        t = ( WORD * ) gf1_dma_buffer;
        s = ( WORD * ) dmabuf;
	    for ( j = 0; j < size; j++ )
	    {
            *t++ = *s;
            *t++ = *s++;
        }
	}
	else
	{
		blk_size = size;
		blk_offset = offset;
		memcpy( gf1_dma_buffer, dmabuf, size );
	}

	pflags = PUSH_DISABLE();
	gf1_flags |= F_BLOCK_READY;
	if ( gf1EnterBusy() == 0 )
		gf1Leave();
	POP_DISABLE( pflags );
}


VOID
gf1StartPCM(
	int		iChannels,		// INPUT : # of Channels 1..8
	WORD	wSampleRate
	)
{
	WORD 	fc;
    UINT    pflags;
	DWORD	m;
	int		dmasize;

    pflags = PUSH_DISABLE();

	if ( wSampleRate < 22050 )
	{
		blk_smoothing = 1;
		wSampleRate = 11025;
		dmasize = DMA_SIZE;
	}
	else
	{
		wSampleRate = 22050;
		blk_smoothing = 0;
		dmasize = DMA_SIZE / 2;
	}
    if ( dspInit( DSP_STEREO_LR8, wSampleRate, iChannels, gf1GetPlayCounter,
                 gf1BlockReady, FALSE ) )
//    if ( dspInit( DSP_STEREO_LR8, wSampleRate, iChannels, gf1GetPlayCounter,
//                 gf1BlockReady, TRUE ) )
    {
        POP_DISABLE( pflags );
        return ;
    }

	dig_dma_control = ( DMA_RATE_DIV_1 | DMA_INVERT_MSB );

	wSampleRate = 22050;
	/* calc fc register */
	fc = ( ( ( wSampleRate << 10L ) + ( GF1_FREQ_DIVISOR >> 1 ) ) /
			GF1_FREQ_DIVISOR ) << 1;

    dig_status |= STAT_PLAYING;

	dig_voice_control = GF1_LPE;
	gf1SetVoiceFreq( dig_left_voice, fc );
	gf1OutByte( SET_BALANCE, 0 );
    gf1SetAddrRegs( dig_addr, SET_ACC_LOW, SET_ACC_HIGH );
    gf1SetAddrRegs( dig_addr, SET_START_LOW, SET_START_HIGH );
	gf1SetAddrRegs( dig_addr + dmasize - 2, SET_END_LOW, SET_END_HIGH );
	gf1OutByte( SET_VOLUME_RATE, 63 ); /* may cause zipper noise */
    gf1SetVol( MAX_OFFSET - 10 );

	gf1SetVoiceFreq( dig_right_voice, fc );
	gf1OutByte( SET_BALANCE, 15 );
    gf1SetAddrRegs( dig_addr + 1, SET_ACC_LOW, SET_ACC_HIGH );
    gf1SetAddrRegs( dig_addr + 1, SET_START_LOW, SET_START_HIGH );
	gf1SetAddrRegs( dig_addr + dmasize - 1, SET_END_LOW, SET_END_HIGH );

	gf1OutByte( SET_VOLUME_RATE, 63 ); /* may cause zipper noise */
    gf1SetVol( MAX_OFFSET - 10 );

	for ( m = 0; m < dmasize; m++ )
	{
		gf1Poke( dig_addr + m, 0 );
	}
	dig_voice_control |= GF1_LPE;
	dig_voice_control &= ~( GF1_STOP | GF1_STOPPED );
	gf1ChangeVoice( 1 );

    POP_DISABLE( pflags );
}


PRIVATE int
Gf1SetCallback(
	VOID	(* func )( void )
	)
{
	gf1Stable = func;
	return 1;
}

GLOBAL DMX_WAV	Gf1Wav =
{
	Gf1Init,        /* OK */
	Gf1DeInit,      /* OK */
	gf1StartPCM,
	Gf1StopPCM,
	Gf1SetCallback
};
