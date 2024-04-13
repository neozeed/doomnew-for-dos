#ifndef _MVPAS_H
#define _MVPAS_H
typedef struct
{
	BYTE		_sysspkrtmr;   /*   42 System Speaker Timer Address */
	BYTE		_systmrctlr;   /*   43 System Timer Control	    	*/
	BYTE		_sysspkrreg;   /*   61 System Speaker Register		*/
	BYTE		_joystick;     /*  201 Joystick Register	    	*/
	BYTE		_lfmaddr;      /*  388 Left  FM Synth Address		*/
	BYTE		_lfmdata;      /*  389 Left  FM Synth Data	    	*/
	BYTE		_rfmaddr;      /*  38A Right FM Synth Address		*/
	BYTE		_rfmdata;      /*  38B Right FM Synth Data	    	*/
	BYTE		_dfmaddr;      /*  788 Dual  FM Synth Address		*/
	BYTE		_dfmdata;      /*  789 Dual  FM Synth Data	    	*/
	BYTE		_RESRVD1[1];   /*      reserved 		    		*/
	BYTE		_paudiomixr;   /*  78B Paralllel Audio Mixer Control*/
	BYTE		_audiomixr;    /*  B88 Audio Mixer Control          */
	BYTE		_intrctlrst;   /*  B89 Interrupt Status 			*/
	BYTE		_audiofilt;    /*  B8A Audio Filter Control			*/
	BYTE		_intrctlr;     /*  B8B Interrupt Control			*/
	BYTE		_pcmdata;      /*  F88 PCM Data I/O Register		*/
	BYTE		_RESRVD2;      /*      reserved 		    		*/
	BYTE		_crosschannel; /*  F8A Cross Channel				*/
	BYTE		_RESRVD3;      /*      reserved 		    		*/
	WORD		_samplerate;   /* 1388 Sample Rate Timer			*/
	WORD		_samplecnt;    /* 1389 Sample Count Register		*/
	WORD		_spkrtmr;      /* 138A Shadow Speaker Timer Count   */
	BYTE		_tmrctlr;      /* 138B Shadow Speaker Timer Control */
	BYTE		_mdirqvect;    /* 1788 MIDI IRQ Vector Register     */
	BYTE		_mdsysctlr;    /* 1789 MIDI System Control Register */
	BYTE		_mdsysstat;    /* 178A MIDI IRQ Status Register     */
	BYTE		_mdirqclr;     /* 178B MIDI IRQ Clear Register	    */
	BYTE		_mdgroup1;     /* 1B88 MIDI Group #1 Register	    */
	BYTE		_mdgroup2;     /* 1B89 MIDI Group #2 Register	    */
	BYTE		_mdgroup3;     /* 1B8A MIDI Group #3 Register	    */
	BYTE		_mdgroup4;     /* 1B8B MIDI Group #4 Register	    */
} MVState;

	/* Hardware associated with the product 				*/

#define DEFAULT_BASE	0x0388		/* default base I/O address		*/
#define ALT_BASE_1		0x0384		/* first alternate address		*/
#define ALT_BASE_2		0x038C		/* second alternate address		*/
#define ALT_BASE_3		0x0288		/* third alternate address		*/
#define USE_ACTIVE_ADDR 0x0000		/* uses what is currently active*/

#define DUALFM		1			/* Dual FM chips					*/
#define DUALMIXER	1			/* Dual mixers						*/
#define FILTER		1			/* Has filter after input mixer 	*/
#define VOLUME		1			/* Has total volume control			*/

#define TEXTVERSIONHI	'01'  	/* VERSION 01.00                    */
#define TEXTVERSIONLO	'00'

#define SYSSPKRTMR  	0x0042		/* System Speaker Timer Address */
#define SYSTMRCTLR		0x0043		/* System Timer Control Register*/
#define SYSSPKRREG		0x0061		/* System Speaker Register		*/
#define JOYSTICK		0x0201		/* Joystick Register			*/
#define LFMADDR 		0x0388		/* Left  FM Address Register	*/
#define LFMDATA 		0x0389		/* Left  FM Data Register		*/
#define RFMADDR 		0x038A		/* Right FM Address Register	*/
#define RFMDATA 		0x038B		/* Right FM Data Register		*/

#define AUXADDR 		0x0788		/* Auxiliary Register			*/
#define AUXDATA 		0x0789		/* Auxiliary Register			*/
#define DFMADDR 		0x0788		/* Dual  FM Address Register	*/
#define DFMDATA 		0x0789		/* Dual  FM Data Register		*/

#define PAUDIOMIXR		0x078B		/* Parallel interface Audio Mixer Reg	*/
#define AUDIOMIXR   	0x0B88		/* Audio Mixer Control Register */
#define AUDIOFILT		0x0B8A		/* Audio Filter Control Register*/
#define INTRCTLRST		0x0B89		/* Interrupt Control Register	*/
#define INTRCTLR		0x0B8B		/* Interrupt Control Register	*/
#define INTRCTLRRB		0x0B8B		/* Interrupt Control Register read back */
#define PCMDATA 		0x0F88		/* PCM data I/O register		*/
#define PCMDATAH		0x0F89		/* PCM data I/O register		*/
#define CROSSCHANNEL	0x0F8A		/* Cross Channel Register		*/
#define SAMPLERATE		0x1388		/* (t0) Sample Rate Timer Register	*/
#define SAMPLECNT		0x1389		/* (t1) Sample Count Register		*/
#define SPKRTMR 		0x138A		/* (t2) Local Speaker Timer Address	*/
#define TMRCTLR 		0x138B		/* Local Timer Control Register 	*/
#define MDIRQVECT		0x1788		/* MIDI IRQ Vector Register		*/
#define MDSYSCTLR		0x1789		/* MIDI System Control Register 	*/
#define MDSYSSTAT		0x178A		/* MIDI IRQ Status Register		*/
#define MDIRQCLR		0x178B		/* MIDI IRQ Clear Register		*/
#define MDGROUP4		0x1B88		/* MIDI Group #1 Register (MDGROUP1)	*/
#define MDGROUP5		0x1B89		/* MIDI Group #2 Register (MDGROUP2)	*/
#define MDGROUP6		0x1B8A		/* MIDI Group #3 Register (MDGROUP3)	*/
#define MDGROUP7		0x1B8B		/* MIDI Group #4 Register (MDGROUP4)	*/

/*										*/
/* Factory Default Settings				*/
/*										*/
#define DEFAULTDMA		0x01		/* DMA channel 1			*/
#define DEFAULTIRQ		0x07		/* IRQ channel 7			*/
#define DEFAULTINT		0x65		/* Interrupt # for software interface	*/

/*										*/
/* mixer select 						*/
/*										*/
#define OUTPUTMIXER		0x00		/* output mixer H/W select		*/
#define INPUTMIXER		0x40		/* input mixer select			*/
#define DEFMIXER		-1			/* use last mixer selected		*/
#define MIXERMAX		0x1f		/* maximum mixer setting		*/

#define MVVOLUMEMAX		0x3f		/* MVA508 maximum mixer setting 	*/
#define NSVOLUMEMAX		0x28		/* National maximum mixer setting	*/

#define EQUALIZERMAX	0x0c		/* maximum equalizer setting		*/
#define EQUALIZERMID	0x06		/* maximum mid setting			*/

/*                                  */
/* Filter register bits 			*/
/*									*/
#define fFIdatabits		0x1f		/* filter select and decode field bits	*/
#define fFImutebits		0x20		/* filter mute field bits		*/
#define fFIpcmbits		0xc0		/* filter sample rate field bits	*/
#define bFImute 		0x20		/* filter mute bit			*/
#define bFIsrate		0x40		/* filter sample rate timer mask	*/
#define bFIsbuff		0x80		/* filter sample buffer counter mask	*/
#define FILTERMAX		6			/* six possible settings		*/

/*										*/
/* Cross Channel Bit definitions						*/
/*										*/
#define fCCcrossbits	0x0f		/* cross channel bit field		*/
#define fCCpcmbits		0xf0		/* pcm/dma control bit field		*/
#define bCCr2r			0x01		/* CROSSCHANNEL Right to Right		*/
#define bCCl2r			0x02		/* CROSSCHANNEL Left  to Right		*/
#define bCCr2l			0x04		/* CROSSCHANNEL Right to Right		*/
#define bCCl2l			0x08		/* CROSSCHANNEL Left  to Left		*/
#define bCCdac			0x10		/* DAC/ADC Control			*/
#define bCCmono 		0x20		/* PCM Monaural Enable			*/
#define bCCenapcm		0x40		/* Enable PCM state machine		*/
#define bCCdrq			0x80		/* Enable DRQ bit			*/

/*										*/
/* Interrupt Control Register Bits						*/
/*										*/
#define fICintmaskbits	0x1f		/* interrupt mask field bits		*/
#define fICrevbits		0xe0		/* revision mask field bits		*/
#define fICidbits		0xe0		/* Board revision ID field bits 	*/
#define bICleftfm		0x01		/* Left FM interrupt enable		*/
#define bICritfm		0x02		/* Right FM interrupt enable		*/
#define bICsamprate		0x04		/* Sample Rate timer interrupt enable	*/
#define bICsampbuff		0x08		/* Sample buffer timer interrupt enable */
#define bICmidi 		0x10		/* MIDI interrupt enable		*/
#define fICrevshr		0x05		/* rotate rev bits to lsb		*/

/*										*/
/* Interrupt Status Register Bits						*/
/*										*/
#define fISints 	0x1f		/* Interrupt bit field			*/
#define bISleftfm	0x01		/* Left FM interrupt active		*/
#define bISritfm	0x02		/* Right FM interrupt active		*/
#define bISsamprate	0x04		/* Sample Rate timer interrupt active	*/
#define bISsampbuff	0x08		/* Sample buffer timer interrupt active */
#define bISmidi 	0x10		/* MIDI interrupt active		*/
#define bISPCMlr	0x20		/* PCM left/right active		*/
#define bISActive	0x40		/* Hardware is active (not in reset)	*/
#define bISClip 	0x80		/* Sample Clipping has occured		*/

/*										*/
/*	 cross channel channel #s						*/
/*										*/
#define RIGHT2RIGHT	0x00		/* right to right			*/
#define LEFT2RIGHT	0x01		/* left  to right			*/
#define RIGHT2LEFT	0x02		/* right to left			*/
#define LEFT2LEFT	0x03		/* left  to left			*/

/*										*/
/* left/right mixer channel selection						*/
/*										*/

/*	left channel values							*/

#define L_LEFTFM	0x01
#define L_IMIXER	0x02
#define L_EXT		0x03
#define L_INT		0x04
#define L_MIC		0x05
#define L_PCM		0x06
#define L_SPEAKER	0x07
#define L_FREE		0x00
#define L_SBDAC         0x00

/*	right channel values							*/

#define R_RIGHTFM	0x08
#define R_IMIXER	0x09
#define R_EXT		0x0A
#define R_INT		0x0B
#define R_MIC		0x0C
#define R_PCM		0x0D
#define R_SPEAKER	0x0E
#define R_FREE		0x0F
#define R_SBDAC 	0x0F

/*	Mixer register bits							*/

#define fMImixerbits	0x17		/* mixer control bit fields		*/
#define fMIspkrbits	0x40		/* speaker integrator field bits	*/
#define bMIdata 	0x01		/* data bit				*/
#define bMIclock	0x02		/* clock strobe 			*/
#define bMImistrb	0x04		/* mixer output strobe			*/
#define bMIna1		0x08		/* not used				*/
#define bMIvol		0x10		/* total volume enabled 		*/
#define bMIna2		0x20		/* not used				*/
#define bMIspkrint	0x40		/* speaker integrator			*/
#define bMImonofm	0x80		/* make both FMs mono			*/

#define bMIfmreset	bMIdata 	/* OPL3 FM chip reset			*/
#define bMIdacreset	bMIclock	/* CODEC reset				*/
#define bMIsbreset	bMImistrb	/* SB microprocessor reset		*/
#define bMI508reset	bMIvol		/* MVA508 reset 			*/

/*	Volume control channel #s						*/

#define VOLMUTE 	0x40		/* MUTE button				*/
#define VOLLOUDENH	0x41		/* LOUDNESS and ENHANCED STEREO switch	*/
#define VOLBASS 	0x42		/* BASS level setting			*/
#define VOLTREBLE	0x43		/* TREBLE level setting 		*/
#define VOLLEFT 	0x44		/* MASTER LEFT LEVEL settting		*/
#define VOLRIGHT	0x45		/* MASTER RIGHT LEVEL settting		*/
#define VOLMODE 	0x46		/* Model Select Left/Stereo/Right	*/

#define bVOLEbass	0x01		/* enhanced bass bit			*/
#define bVOLEstereo	0x02		/* enhanced stereo bit			*/

/*	output control								*/

#define pmADDRSELECT	0x80		/* Parallel mixer addr select		*/
#define pmDATASELECT	0x00		/* Parallel mixer data select		*/

/*	mixer channel programming selection					*/

#define pmCHANNELLR	0x00		/* Left/Right channel select		*/
#define pmCHANNELL	0x20		/* Left  channel only select		*/
#define pmCHANNELR	0x40		/* Right channel only select		*/

/*	device select								*/

#define pmMIXERSELECT	0x10		/* Parallel Mixer device select 	*/
#define pmVOLUMESELECT	0x00		/* Parallel Volume device select	*/

/*	Volume Device selects							*/

#define pmVOLUMEA	0x01		/* Left/Right channel select		*/
#define pmVOLUMEB	0x02		/* Left/Right channel select		*/
#define pmVOLUMEBASS	0x03		/* Left/Right channel select		*/
#define pmVOLUMETREB	0x04		/* Left/Right channel select		*/
#define pmVOLUMEMODE	0x05		/* Left/Right channel select		*/

/*	mixer selection 							*/

#define pmINPUTMIXER	0x00		/* Mixer-A selection			*/
#define pmOUTPUTMIXER	0x20		/* Mixer-B selection			*/

/*	mixer channel swap							*/

#define pmCHSWAP	0x40		/* Mixer channel reroute		*/

/*	int 2F application ID codes						*/

#define INT2FCODE1	0x00BC		/* Bryan's initials                     */

/*	int 2F ID (func 0) return register values				*/

#define INT2FREGBX	0x6D00		/* 'm '                                 */
#define INT2FREGCX	0x0076		/* ' v'                                 */
#define INT2FREGDX	0x2020		/* UPPERCASE XOR MASK			*/

/* hardware specific equates for the PAS2/CDPC (digital ASIC)			*/

#define MASTERADDRP	0x9A01		/* Master Address Pointer    (w)	*/
#define MIDIPRESCALE	0x1788		/* MIDI prescale	     (r/w)	*/
#define MIDITIMER	0x1789		/* MIDI Timer		     (r/w)	*/
#define MIDIDATA	0x178A		/* MIDI Data		     (r/w)	*/
#define MIDICONTROL	0x178B		/* MIDI Control 	     (r/w)	*/
#define MIDISTATUS	0x1B88		/* MIDI Status		     (r/w)	*/
#define MIDIFIFOS	0x1B89		/* MIDI Fifo Status	     (r/w)	*/
#define MIDICOMPARE	0x1B8A		/* MIDI Compare Time	     (r/w)	*/
#define MIDITEST	0x1B8B		/* MIDI Test		     (w)	*/
#define MASTERCHIPR	0xFF88		/* Master Chip Rev	     (r)	*/
#define SLAVECHIPR	0xEF88		/* Slave Chip Rev	     (r)	*/
#define ENHANCEDSCSI	0x7f89		/* Enhanced SCSI detect port		*/
#define SYSCONFIG1      0x8388          /* System Config 1           (r/w)      */
#define SYSCONFIG2	0x8389		/* System Config 2	     (r/w)	*/
#define SYSCONFIG3	0x838A		/* System Config 3	     (r/w)	*/
#define SYSCONFIG4	0x838B		/* System Config 4	     (r/w)	*/
#define IOCONFIG1	0xF388		/* I/O Config 1 	     (r/w)	*/
#define IOCONFIG2	0xF389		/* I/O Config 2 	     (r/w)	*/
#define IOCONFIG3	0xF38A		/* I/O Config 3 	     (r/w)	*/
#define IOCONFIG4	0xF38B		/* I/O Config 4 	     (r/w)	*/
#define COMPATREGE	0xF788		/* Compatible Rgister Enable (r/w)	*/
#define EMULADDRP	0xF789		/* Emulation Address Pointer (r/w)	*/
#define WAITSTATE	0xBF88		/* Wait State		     (r/w)	*/
#define PUSHBUTTON	0xE388		/* Push Button (slave)	     (???)	*/
#define AXUINTSTAT	0xE38A		/* Aux Int Status	     (???)	*/
#define AUXINTENA	0xE38B		/* Aux Int Enable	     (???)	*/
#define OVRSMPPRE	0xBF8A		/* Over Sample Prescale      (r/w)	*/
#define ANALSERD	0xBF89		/* Analog Chip Serial Data   (w)	*/
#define MASTERMODRD	0xFF8B		/* Master Mode Read	     (r)	*/
#define SLAVEMODRD	0xEF8B		/* Slave Mode Read	     (r)	*/
#define INTWATCHDOG	0xFB8B		/* Interrupt Watch Dog	     (???)	*/
#define MASTERuPDATA	0xFB88		/* Master uP Data	     (???)	*/
#define MASTERuPCMD	0xFB89		/* Master uP Command/Status  (???)	*/
#define MASTERuPRST	0xFB8A		/* Master uP Restart	     (???)	*/
#define SLAVEuPDATA	0xEB88		/* Slave uP Data	     (???)	*/
#define SLAVEuPCMD	0xEB88		/* Slave uP Command/Status   (???)	*/
#define SLAVEuPRST	0xEB88		/* Slave uP Restart	     (???)	*/
#define CDTOCOUNTER	0x4388		/* CD-ROM timeout counter    (r/w)	*/
#define CDTOSTAT	0x4389		/* CD-ROM timeout status     (r/w)	*/
#define LEFTVURD	0x2388		/* Left VU Read 	     (r)	*/
#define RITVURD 	0x2389		/* Right VU Read	     (r)	*/

#define SBRST		0x0206		/* SB Reset		     (w)	*/
#define SBDATA		0x020A		/* SB Data Read 	     (r)	*/
#define SBCMD		0x020C		/* SB CMD Write/Status Read  (r/w)	*/
#define SBSTAT		0x020E		/* SB Data Status	     (r)	*/
#define MPUDATA 	0x0300		/* MPU-401 data reg	     (r/w)	*/
#define MPUCMD		0x0301		/* MPU-401 command reg	     (r/w)	*/

/* Sys Config 1 								*/

#define bSC1timena	0x01		/* shadow enable			*/
#define bSC1pcmemu	0x02		/* PCM Emulation of PAS1		*/
#define bSC128mhz	0x04		/* 28mhz clock divisor			*/
#define bSC1invcom	0x08		/* invert COM port interrupt input	*/
#define bSC1stspea	0x10		/* stereoize pc speaker 		*/
#define bSC1realsnd	0x20		/* smart real sound emulatio		*/
#define bSC1d6		0x40
#define bSC1mstrst	0x80		/* master chip reset			*/

/* Sys Config 2 								*/

#define bSC2ovrsmp	0x03		/* oversampling 0,1,2,4 		*/
#define bSC216bit	0x04		/* 16 bit audio 			*/
#define bSC212bit	0x08		/* 12 bit interleaving (d2 must be set) */
#define bSC2msbinv	0x10		/* invert MSB from standard method	*/
#define bSC2slavport	0x60		/* slave port bits			*/
#define bSC2vcolock	0x80		/* VCO locked (Sample Rate Clock Valid) */

/* Sys Config 3	*/

#define bSC328mhzfil	0x01		/* PCM Rate uses 28mhz			*/
#define bSC31mhzsb	0x02		/* 1mhz timer for SB sample rate	*/
#define bSC3vcoinv	0x04		/* invert VCO output			*/
#define bSC3bclkinv	0x08		/* invert BCLK form 16 bit DAC		*/
#define bSC3lrsync	0x10		/* 0=L/R, 1 = Sync Pulse		*/
#define bSC3d5		0x20
#define bSC3d6		0x40
#define bSC3d7		0x80

/* Sys Config 4 								*/

#define bSC4drqahi	0x01		/* DRQ from drive active high		*/
#define bSC4dackahi	0x02		/* DRQ from drive active high		*/
#define bSC4intahi	0x04		/* INT from drive active high		*/
#define bSC4drqvalid	0x08		/* DRQ line valid from drive		*/
#define bSC4comena	0x10		/* enable COM interrupt 		*/
#define bSC4enascsi	0x20		/* enable SCSI interrupt		*/
#define bSC4drqptr	0xc0		/* DRQ timing pointer bits		*/

/* I/O Config 1 								*/

#define bIC1ps2ena	0x01		/* Enable Chip (PS2 only)		*/
#define bIC1comdcd	0x06		/* COM port decode pointer		*/
#define bIC1comint	0x38		/* COM port interrupt pointer		*/
#define bIC1joyena	0x40		/* Enable joystick read 		*/
#define bIC1wporena	0x80		/* Enable warm boot reset		*/

/* I/O Config 2 								*/

#define bIC2dmaptr	0x03		/* DMA channel select			*/

#define bIC28dmaptr	0x0f		/*  8 bit DMA channel select		*/
#define bIC216dmaptr	0xf0		/* 16 bit DMA channel select		*/

/* I/O Config 3 								*/

#define bIC3pcmint	0x0f		/* pcm IRQ channel select		*/
#define bIC3cdint	0xf0		/* cd  IRQ channel select		*/

/* Compatibility Register							*/

#define cpMPUEmulation	0x01		/* MPU emuation is on bit		*/
#define cpSBEmulation	0x02		/* SB emuation is on bit		*/

/* Emulation Address Pointer							*/

#define epSBptr 	0x0F		/* bit field for SB emulation		*/
#define epMPUptr	0xF0		/* bit field for MPU emulation		*/

/* Slave Mode Read								*/

#define bSMRDdrvtyp	0x03		/* drive interface type 		*/
#define bSMRDfmtyp	0x04		/* FM chip type 			*/
#define bSMRDdactyp	0x08		/* 16 bit dac (1) or 8 bit dac (0)	*/
#define bSMRDimidi	0x10		/* use internal MIDI			*/
#define bSMRDswrep	0x80		/* switch is auto repeating		*/

/* Master Mode Read								*/

#define bMMRDatps2	0x01		/* AT(1) or PS2(0) bus			*/
#define bMMRDtmremu	0x02		/* timer emulation enabled		*/
#define bMMRDmsmd	0x04		/* master/slave mode			*/
#define bMMRDslave	0x08		/* slave power on or device present	*/
#define bMMRDattim	0x10		/* xt/at timing 			*/
#define bMMRDmstrev	0xe0		/* master  rev level			*/

/* MIDI Control Register							*/

#define bMCRenatstmp	0x01		/* MIDI enable time stamp interrupt	*/
#define bMCRenacmptm	0x02		/* MIDI enable compare time interrupt	*/
#define bMCRenafifoi	0x04		/* MIDI enable FIFO input interrupt	*/
#define bMCRenafifoo	0x08		/* MIDI enable FIFO output interrupt	*/
#define bMCRenafifooh	0x10		/* MIDI enable FIFO output half int	*/
#define bMCRrstfifoi	0x20		/* MIDI reset Input FIFO pointer	*/
#define bMCRrstfifoo	0x40		/* MIDI reset Output FIFO pointer	*/
#define bMCRechoio	0x80		/* MIDI echo input to output (THRU)	*/

/* MIDI Status Register 							*/

#define bMSRtimstamp	0x01		/* MIDI time stamp interrupt		*/
#define bMSRcmptime	0x02		/* MIDI compare time interrupt		*/
#define bMSRififo	0x04		/* MIDI input FIFO data avail interrupt */
#define bMSRofifo	0x08		/* MIDI output FIFO empty interrupt	*/
#define bMSRofifohalf	0x10		/* MIDI output FIFO half empty interrupt*/
#define bMSRififoovr	0x20		/* MIDI input FIFO overrun error	*/
#define bMSRofifoovr	0x40		/* MIDI output FIFO overrun error	*/
#define bMSRframeerr	0x80		/* MIDI frame error			*/

/* MIDI FIFO count								*/

#define bMFCififo	0x0F		/* MIDI input FIFO count		*/
#define bMFCofifo	0xF0		/* MIDI output FIFO count		*/

/* Aux interrupt status/enable                                                  */

#define bAUfmrit	0x01		/* FM right interrupt			*/
#define bAUpushb	0x02		/* push button active			*/
#define bAUslavecpu	0x04		/* slave coprocessor			*/
#define bAUaux0int	0x08		/* aux 0 interrupt			*/
#define bAUaux1int	0x10		/* aux 1 interrupt			*/
#define bAUaux2int	0x20		/* aux 2 interrupt			*/
#define bAUaux3int	0x40		/* aux 3 interrupt			*/
#define bAUmastrcpu	0x80		/* master coprocessor or emulation activ*/

/* Push Buttons on the Front Panel						*/

#define bPSHuparrow	0x01		/* up arrow on the front panel		*/
#define bPSHdnarrow	0x02		/* down arrow on the front panel	*/
#define bPSHmute	0x04		/* mute on the front panel		*/
#define bPSauxbit1	0x08		/* unused bit...			*/
#define bPSauxbit2	0x10		/* unused bit...			*/
#define bPSauxbit3	0x20		/* unused bit...			*/
#define bPSauxbit4	0x40		/* unused bit...			*/
#define bPSauxbit5	0x80		/* unused bit...			*/

/*
* Each product will some/all of of these features
*/
#define bMVA508 	0x0001	// MVA508(1) or National(0)
#define bMVPS2		0x0002	// PS2 bus stuff
#define bMVSLAVE	0x0004	// CDPC Slave device is present
#define bMVSCSI 	0x0008	// SCSI interface
#define bMVENHSCSI	0x0010	// Enhanced SCSI interface
#define bMVSONY 	0x0020	// Sony 535 interface
#define bMVDAC16	0x0040	// 16 bit DAC
#define bMVSBEMUL	0x0080	// SB h/w emulation
#define bMVMPUEMUL	0x0100	// MPU h/w emulation
#define bMVOPL3     0x0200	// OPL3(1) or 3812(0)
#define bMV101		0x0400	// MV101 ASIC
#define bMV101_REV	0x7800	// MV101 Revision
#define bMV101_MORE	0x8000	// more bits in BX

/*
* Define the ASIC versions
*/

#define ASIC_VERSION_B	0x0002	// revision B
#define ASIC_VERSION_C	0x0003	// revision C
#define ASIC_VERSION_D	0x0004	// revision D
#define ASIC_VERSION_E	0x0005	// revision E
#define ASIC_VERSION_F	0x0006	// revision F


#endif
