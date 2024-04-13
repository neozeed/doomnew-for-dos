#ifndef GF1_API_H
#define GF1_API_H

#ifndef _TYPES_H
#include "types.h"
#endif

typedef struct
{
	BYTE	patch_name[ 8 + 1 ];
	BYTE	program;
	BYTE	pgm_idx[ 4 ];
} GF1_PATCH_MAP;

/* error codes */
#define NO_MORE_VOICES		-1
typedef enum
{
	OK,
	BASE_NOT_FOUND,
	BAD_IRQ,
	BAD_DMA,
	OS_LOADED,
	NOT_LOADED,
	NO_MEMORY,
	DMA_BUSY,
	DMA_NOT_NEEDED,
	NO_MORE_HANDLERS,
	DMA_HUNG,
	CARD_NOT_FOUND,
	CARD_BEING_USED,
	NO_MORE_INTERRUPT,
	BAD_TIMER,
	BAD_PATCH,
	OLD_PATCH,
	DOS_ERROR,
	FILE_NOT_FOUND,
	UPGRADE_CARD_MEMORY,
	NO_DMA_MEMORY,
	NO_PATCHES,
} GF1_ERROR;


/* bits */
#define	BIT0	0x01
#define	BIT1	0x02
#define	BIT2	0x04
#define	BIT3	0x08
#define	BIT4	0x10
#define	BIT5	0x20
#define	BIT6	0x40
#define	BIT7	0x80

/* bounds for volume enveloping functions */
#define MIN_OFFSET      5U
#define MAX_OFFSET      251U

/* bounds for voice allocation */
#define MIN_VOICES	14
#define MAX_VOICES	32

/* DMA control bits */
#define DMA_ENABLE		BIT0
#define DMA_READ		BIT1
#define DMA_WIDTH_16	BIT2 /* width of DMA channel */
#define DMA_RATE_DIV_1	BIT3
#define DMA_RATE_DIV_2	BIT4
#define DMA_IRQ_ENABLE	BIT5
#define DMA_IRQ_PRESENT	BIT6
#define DMA_DATA_16		BIT6 /* width of data */
#define DMA_INVERT_MSB	BIT7

/* SAMPLE control bits */
#define DMA_STEREO		2

/* digital playback flags */
#define TYPE_8BIT		BIT0	/* 1 use 8 bit data */
								/* 0 use 16 bit data */
#define TYPE_PRELOAD	BIT1	/* preload data */
#define TYPE_INVERT_MSB BIT2	/* invert most significant bit during dma */
#define TYPE_STEREO		BIT3	/* 1 for stereo data */

/* 
* patch macros 
*/
#define MAX_LAYERS		   			4
#define ENVELOPES		   			6
#define PATCH_DATA_RESERVED_SIZE	36
#define INST_NAME_SIZE		   		16
#define INST_RESERVED_SIZE	        40
#define LAYER_RESERVED_SIZE	   		40
#define PATCH_HEADER_SIZE	        12
#define PATCH_HDR_RESERVED_SIZE		36
#define PATCH_DESC_SIZE 	        60
#define PATCH_ID_SIZE		        10

#define GF1_HEADER_TEXT            "GF1PATCH110"

/* patch modes */
#define PATCH_16			BIT0
#define PATCH_UNSIGNED		BIT1
#define PATCH_LOOPEN		BIT2
#define PATCH_BIDIR			BIT3
#define PATCH_BACKWARD  	BIT4
#define PATCH_SUSTAIN   	BIT5
#define PATCH_ENVEN     	BIT6
#define PATCH_FAST_REL  	BIT7

/* flags for patch loading */
#define PATCH_LOAD_8_BIT 	BIT0

#define LOG_TAB_SIZE 12
extern long gf1_log_table[];

/*
* structure definitions 
*/
typedef struct
{
	BYTE		fractions;
	long		wave_size;
	long		start_loop;
	long		end_loop;
	WORD		sample_rate;
	long		low_frequency;
	long		high_frequency;
	long		root_frequency;
	short		tune;
	BYTE		balance;
	BYTE		envelope_rate[ ENVELOPES ];
	BYTE		envelope_offset[ ENVELOPES ];	
	BYTE		tremolo_sweep;
	BYTE		tremolo_rate;
	BYTE		tremolo_depth;
	BYTE		vibrato_sweep;
	BYTE		vibrato_rate;
	BYTE		vibrato_depth;
	char		modes;
	short		scale_frequency;
	WORD		scale_factor;		/* from 0 to 2048 or 0 to 2 */
} GF1_INFO;

typedef struct
{
	char		wave_name[ 7 ];
	GF1_INFO	info;
	char		reserved[ PATCH_DATA_RESERVED_SIZE ];
} GF1_PATCH_DATA;

typedef struct
{
	DWORD		mem;
	WORD		start_acc_low;
	WORD		start_acc_high;
	WORD		start_low;
	WORD		start_high;
	WORD		end_low;
	WORD		end_high;
	WORD		end_acc_low;
	WORD		end_acc_high;
	WORD		sample_ratio;
	GF1_INFO	info;
} GF1_WAVE;

typedef struct
{
	short		nlayers;
	GF1_WAVE	*layer_waves[ MAX_LAYERS ];
	short 		layer_nwaves[ MAX_LAYERS ];
} GF1_PATCH;

typedef struct
{
	short 		voices;
	short		forced_base_port;
	char		forced_gf1_irq;
	char		forced_midi_irq;
	char		forced_channel_in;
	char		forced_channel_out;
} GF1_OS;

/* digital playback callback reasons & return values */
#define	DIG_DONE			0
#define DIG_MORE_DATA	  	1
#define DIG_BUFFER_DONE   	2
#define DIG_PAUSE         	3

#endif
