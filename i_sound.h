#ifndef __SOUND__
#define __SOUND__

#define SND_TICRATE		140             // tic rate for updating sound
#define SND_MAXSONGS	40              // max number of songs in game
#define SND_SAMPLERATE	11025   // sample rate of sound effects

typedef enum
{
	snd_none,	// P
	snd_PC,		// P
	snd_Adlib,	// A
	snd_SB,		// S
	snd_PAS,	// S
	snd_GUS,	// S
	snd_MPU,	// M
	snd_MPU2,	// M
	snd_MPU3,	// M
	snd_AWE,	// S
	NUM_SCARDS
} cardenum_t;

#endif
