#ifndef _GF1_H_
#define _GF1_H_

#ifndef _TYPES_H
#include "types.h"
#endif

#include "gf1api.h"

/* INTERNAL FLAGS */
#define F_OS_LOADED		(1 << 0)
#define F_REDO_INT		(1 << 1)
#define F_GEN_TC		(1 << 2)
#define F_REDO_MIDI_INT (1 << 3)
#define F_BLOCK_READY	(1 << 4)

/* REGISTERS */
#define	SET_CONTROL			0x00
#define	SET_FREQUENCY		0x01
#define	SET_START_HIGH		0x02
#define	SET_START_LOW		0x03
#define	SET_END_HIGH		0x04
#define	SET_END_LOW			0x05
#define	SET_VOLUME_RATE		0x06
#define	SET_VOLUME_START	0x07
#define	SET_VOLUME_END		0x08
#define	SET_VOLUME			0x09
#define	SET_ACC_HIGH		0x0a
#define	SET_ACC_LOW			0x0b
#define	SET_BALANCE			0x0c
#define	SET_VOLUME_CONTROL 	0x0d
#define	SET_VOICES			0x0e
#define DMA_CONTROL     	0x41
#define SET_DMA_ADDRESS		0x42
#define SET_DRAM_LOW    	0x43
#define SET_DRAM_HIGH   	0x44
#define ADLIB_CONTROL   	0x45
#define ADLIB_TIMER1		0x46
#define ADLIB_TIMER2		0x47
#define	SET_SAMPLE_RATE		0x48
#define SAMPLE_CONTROL  	0x49
#define SET_JOYSTICK		0x4B
#define MASTER_RESET		0x4C
#define	GET_CONTROL			0x80
#define	GET_FREQUENCY		0x81
#define	GET_START_HIGH		0x82
#define	GET_START_LOW		0x83
#define	GET_END_HIGH		0x84
#define	GET_END_LOW			0x85
#define	GET_VOLUME_RATE		0x86
#define	GET_VOLUME_START	0x87
#define	GET_VOLUME_END		0x88
#define	GET_VOLUME			0x89
#define	GET_ACC_HIGH		0x8a
#define	GET_ACC_LOW			0x8b
#define	GET_BALANCE			0x8c
#define	GET_VOLUME_CONTROL 	0x8d
#define	GET_VOICES			0x8e
#define	GET_IRQV			0x8f

/* REGISTER BITS */
/* mixer */
#define GF1_DISABLE_LINE	BIT0
#define GF1_DISABLE_OUT		BIT1
#define GF1_ENABLE_MIC		BIT2
#define GF1_ENABLE_IRQ_DMA	BIT3
#define GF1_CONTROL_REGISTER_SELECT	BIT6

/* voice control */
#define GF1_STOPPED		BIT0
#define GF1_STOP		BIT1
#define GF1_WT16		BIT2
#define GF1_LPE			BIT3
#define GF1_BLE			BIT4
#define GF1_IRQE		BIT5
#define GF1_DIR_DEC		BIT6
#define GF1_IRQP		BIT7

/* volume control */
#define GF1_STOPPED		BIT0
#define GF1_STOP		BIT1
#define GF1_ENPCM		BIT2
#define GF1_LPE			BIT3
#define GF1_BLE			BIT4
#define GF1_IRQE		BIT5
#define GF1_DIR_DEC		BIT6
#define GF1_IRQP		BIT7

/* IRQ control */
#define GF1_COMBINE_IRQ	BIT6
 
/* DMA control */
#define GF1_COMBINE_DMA	BIT6

/* IRQ status register */
#define	MIDI_IRQ_SEND	BIT0
#define	MIDI_IRQ_RCV	BIT1
#define	TIMER_1			BIT2
#define	TIMER_2			BIT3
#define ADLIB_DATA		BIT4
#define	GF1_WAVETABLE	BIT5
#define	GF1_ENVELOPE	BIT6
#define	DMA_TC_BIT		BIT7

/* frequency of gf1 clock */
#define CLOCK_RATE      9878400L

typedef struct
{
	char		header[ PATCH_HEADER_SIZE ];	
	char		gravis_id[ PATCH_ID_SIZE ];	/* Id = "ID#000002" */
	char		description[ PATCH_DESC_SIZE ];
	BYTE		instruments;
	char		voices;
	char		channels;
	WORD		wave_forms;
	WORD		master_volume;
	DWORD		data_size;
	char		reserved[ PATCH_HDR_RESERVED_SIZE ];
} GF1_PATCH_HDR;

typedef struct
{
	WORD		instrument;
	char		instrument_name[ INST_NAME_SIZE ];
	long		instrument_size;
	char		layers;
	char		reserved[ INST_RESERVED_SIZE ];	
} GF1_INST_DATA;

typedef struct
{
	GF1_PATCH_HDR	header;
	GF1_INST_DATA	idata;
} GF1_PATCH_INFO;

typedef struct
{
	char		layer_duplicate;
	char		layer;
	long		layer_size;
	char		samples;
	char		reserved[ LAYER_RESERVED_SIZE ];	
} GF1_LAYER;

#define SCALE_TABLE_SIZE	128

extern WORD gf1_freq_divisor[ 19 ];
extern WORD gf1_mix_control;
extern WORD gf1_status_register;
extern WORD gf1_timer_control;
extern WORD gf1_timer_data;
extern WORD gf1_irqdma_control;
extern WORD gf1_reg_control;
extern WORD gf1_midi_control;
extern WORD gf1_midi_data;
extern WORD gf1_page_register;
extern WORD gf1_register_select;
extern WORD gf1_data_low;
extern WORD gf1_data_high;
extern WORD gf1_dram_io;
extern WORD gf1_voices;
extern WORD gf1_freq_divisor[];
extern long	gf1_scale_table[];

/****************************************************************************
* gf1Poke - Poke a byte into GF1 DRAM
*****************************************************************************/
PUBLIC void
gf1Poke(
    DWORD       address,	// INPUT: GF1 DRAM address to write to
    BYTE        data		// INPUT: Byte to write
    );

/****************************************************************************
* gf1SetTC - Sets DMA Terminal Count Flag
*****************************************************************************/
PUBLIC void
gf1SetTC(
	void
	);

/****************************************************************************
* gf1InitMem - Initialize GF1 memory
*****************************************************************************/
PUBLIC int		// Returns: number of memory banks installed.
gf1InitMem(
	void
    );

/****************************************************************************
* gf1GetMemAvail - Returns number of contiguous bytes of GF1 DRAM available.
*****************************************************************************/
DWORD		// Returns: number of available bytes minus reserve.
gf1GetMemAvail(
    void
    );


/****************************************************************************
* gf1InitVoiceMgr - Initialize Voice Manager
*****************************************************************************/
PUBLIC void
gf1InitVoiceMgr(
	void
	);

/****************************************************************************
* gf1AllocVoice - Allocate voice for usage
*****************************************************************************/
PUBLIC int		// Returns: NO_MORE_VOICES(-1) or 0..MAX_VOICES-1
gf1AllocVoice(
    int     priority,		// INPUT : 0 = Highest, >0 = Lower
    void    (*steal_notify)(int)
    );

/****************************************************************************
* gf1AllocVoice - Allocate voice for usage
*****************************************************************************/
PUBLIC void
gf1FreeVoice(
	int			voice_id	// INPUT : Voice ID 0..MAX_VOICES-1
	);

/****************************************************************************
* gf1AdjustPriority - Adjust voice priority.
*****************************************************************************/
PUBLIC void
gf1AdjustPriority(
	int			voice_id,	// INPUT : Voice ID 0..MAX_VOICES-1
	int 		priority	// INPUT : New priority 0..MAX_INT
	);

/****************************************************************************
* gf1Yield - Yield control to GF1
*****************************************************************************/
PUBLIC void
gf1Yield(
	void
	);



/****************************************************************************
* gf1DramXfer - Write to GF1 DRAM's
*****************************************************************************/
PUBLIC int		// Returns: DMA_BUSY, or OK
gf1DramXfer(
	BYTE		*ptr,
	DWORD		size,
	DWORD		dram_address,
	BYTE		dma_control
	);

/****************************************************************************
* gf1WaitDma - Wait for DMA to complete.
*****************************************************************************/
PUBLIC int		// Returns: DMA_HUNG or OK
gf1WaitDma(
	void
	);

/****************************************************************************
* gf1DmaReady - Test if DMA channel is ready (idle).
*****************************************************************************/
PUBLIC BOOL		// Returns: TRUE - DMA Idle, FALSE - DMA Active
gf1DmaReady(
	void
	);

/****************************************************************************
* gf1StopDma - Stop DMA Transfer
*****************************************************************************/
PUBLIC void
gf1StopDma(
	void
	);

/****************************************************************************
* gf1DmaInit - Initialize DMA Mangager
*****************************************************************************/
PUBLIC int
gf1DmaInit(
	WORD		dma_channel
	);

/****************************************************************************
* gf1DmaDeInit - Deinit DMA Manager
*****************************************************************************/
PUBLIC void
gf1DmaDeInit(
	void
	);


/****************************************************************************
* gf1DigitalInit - Initialize Digital Interface
*****************************************************************************/
PUBLIC int
gf1DigitalInit(
	void
	);

/****************************************************************************
* gf1MidiInit - Initialize MIDI interface
*****************************************************************************/
PUBLIC int
gf1MidiInit(
	void
	);

/****************************************************************************
* gf1EnableTimer2 - Enable Timer 2
*****************************************************************************/
PUBLIC int
gf1EnableTimer2(
	int 		( *callback )( void ),
	int 		resolution
	);

/****************************************************************************
* gf1EnableTimer1 - Enable timer 1
*****************************************************************************/
PUBLIC int
gf1EnableTimer1(
	int 			( *callback )(),
	int 			resolution
	);

/****************************************************************************
* gf1DisableTimer1 - Disable the 1st timer
*****************************************************************************/
PUBLIC void
gf1DisableTimer1(
	void
	);

/****************************************************************************
* gf1DisableTimer2 - Disable the 2nd timer
*****************************************************************************/
PUBLIC void
gf1DisableTimer2(
	void
	);

#endif
