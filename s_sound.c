// S_sound.c

#ifndef OLDSOUND

#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "i_sound.h"
#include "sounds.h"
#include "s_sound.h"

#include "z_zone.h"
#include "m_random.h"
#include "w_wad.h"

#include "doomdef.h"
#include "p_local.h"

#include "doomstat.h"

#include "oldnew.h" // FS

#define S_MAX_VOLUME		127

// when to clip out sounds
// Does not fit the large outdoor areas.
#define S_CLIPPING_DIST		(1200*0x10000)

// Distance tp origin when sounds should be maxed out.
// This should relate to movement clipping resolution
// (see BLOCKMAP handling).
// Originally: (200*0x10000).
#define S_CLOSE_DIST		(160*0x10000)


#define S_ATTENUATOR		((S_CLIPPING_DIST-S_CLOSE_DIST)>>FRACBITS)

// Adjustable by menu.
#define NORM_VOLUME			snd_MaxVolume

#define NORM_PITCH	 		128
#define NORM_PRIORITY		64
#define NORM_SEP			128

#define S_PITCH_PERTURB		1
#define S_STEREO_SWING		(96*0x10000)

// percent attenuation from front to back
#define S_IFRACVOL			30

#define NA					0
#define S_NUMCHANNELS		2

extern int			snd_MusicDevice;
extern int			snd_SfxDevice;
extern int			snd_DesiredMusicDevice;
extern int			snd_DesiredSfxDevice;
extern sfxinfo_t	S_sfx[]; // FS
extern int			snd_MaxVolume; // FS: Real volume


// the set of channels available
static channel_t*	channels;

// These are not used, but should be (menu).
// Maximum volume of a sound effect.
// Internal default is max out of 0-15.
int					snd_SfxVolume;

// Maximum volume of music. Useless so far.
extern int			snd_MusicVolume; 



// whether songs are mus_paused
static boolean		mus_paused;	

// music currently being played
static musicinfo_t*	mus_playing=0;

// following is set
//  by the defaults code in M_misc:
// number of channels available
extern int			snd_Channels; // FS
static int			nextcleanup;



//
// Internals.
//
int S_getChannel (void* origin, sfxinfo_t* sfxinfo);
int S_AdjustSoundParams (mobj_t* listener, mobj_t* source, int* vol, int* sep, int* pitch);
void S_StopChannel(int cnum);
void S_StartSong(int mnum); // FS

//
// Initializes sound stuff, including volume
// Sets channels, SFX and music volume,
//  allocates channel buffer, sets S_sfx lookup.
//
void S_Init (int sfxVolume, int musicVolume)
{  
	int	i;

	if (sfxVolume > 15)
	{
		mprintf("WARNING: Sound volume is invalid; defaulting to 8.\n");
		sfxVolume = snd_SfxVolume = 8;
	}
	if (musicVolume > 15)
	{
		mprintf("WARNING: Music volume is invalid; defaulting to 8.\n");
		musicVolume = snd_MusicVolume = 8;
	}

	I_StartupSound();
	I_SetChannels(snd_Channels);
  
	S_SetSfxVolume(sfxVolume);
	I_SetMusicVolume(musicVolume);

	// Allocating the internal channels for mixing
	// (the maximum numer of sounds rendered
	// simultaneously) within zone memory.
	channels = (channel_t *) Z_Malloc(snd_Channels*sizeof(channel_t), PU_STATIC, 0);
  
	// Free all channels for use
	for (i=0 ; i<snd_Channels ; i++)
		channels[i].sfxinfo = 0;
  
	// no sounds are playing, and they are not mus_paused
	mus_paused = 0;

	// Note that sounds have not been cached (yet).
	for (i=1 ; i<NUMSFX ; i++)
		S_sfx[i].lumpnum = S_sfx[i].usefulness = -1;
}

void S_FreeChannels(void)
{
	Z_Free(channels);
}


//
// Per level startup code.
// Kills playing sounds at start of level,
//  determines music if any, changes music.
//
void S_Start(void)
{
	int cnum;
	int mnum;

	// kill all playing sounds at start of level
	//  (trust me - a good idea)
	for (cnum=0 ; cnum<snd_Channels ; cnum++)
		if (channels[cnum].sfxinfo)
			S_StopChannel(cnum);
  
	// start new music for the level
	mus_paused = 0;
  
	if (gamemode == commercial)
		mnum = mus_runnin + gamemap - 1;
	else
	{
		int spmus[]=
		{
			// Song - Who? - Where?
	  
			mus_e3m4,	// American	e4m1
			mus_e3m2,	// Romero	e4m2
			mus_e3m3,	// Shawn	e4m3
			mus_e1m5,	// American	e4m4
			mus_e2m7,	// Tim 	e4m5
			mus_e2m4,	// Romero	e4m6
			mus_e2m6,	// J.Anderson	e4m7 CHIRON.WAD
			mus_e2m5,	// Shawn	e4m8
			mus_e1m9	// Tim		e4m9
		};
	
		if (gameepisode < 4)
			mnum = mus_e1m1 + (gameepisode-1)*9 + gamemap-1;
		else
			mnum = spmus[gamemap-1];
	}	

	S_ChangeMusic(mnum, true);
  
	nextcleanup = 15;
}	

void S_StartSoundAtVolume (void* origin_p, int sfx_id, int volume)
{
	int			rc;
	int			sep;
	int			pitch;
	int			priority;
	sfxinfo_t*	sfx;
	int			cnum;
  
	mobj_t*		origin = (mobj_t *) origin_p;


  
	// check for bogus sound #
	if (sfx_id < 1 || sfx_id > NUMSFX)
		I_Error("Bad sfx #: %d", sfx_id);
  
	sfx = &S_sfx[sfx_id];
  
	// Initialize sound parameters
	if (sfx->link)
	{
		pitch = sfx->pitch;
		priority = sfx->priority;
		volume += sfx->volume;
	
		if (volume < 1)
			return;
	
		if (volume > snd_MaxVolume)
			volume = snd_MaxVolume;
	}	
	else
	{
		pitch = NORM_PITCH;
		priority = NORM_PRIORITY;
	}


	// Check to see if it is audible,
	//  and if not, modify the params
	if (origin && origin != players[consoleplayer].mo)
	{
		rc = S_AdjustSoundParams(players[consoleplayer].mo, origin, &volume, &sep, &pitch);
	
		if ( origin->x == players[consoleplayer].mo->x && origin->y == players[consoleplayer].mo->y)
		{	
			sep = NORM_SEP;
		}
	
		if (!rc)
			return;
	}	
	else
	{
		sep = NORM_SEP;
	}

	// kill old sound
	S_StopSound(origin);

	// try to find a channel
	cnum = S_getChannel(origin, sfx);
  
	if (cnum<0)
		return;

	//
	// This is supposed to handle the loading/caching.
	// For some odd reason, the caching is done nearly
	//  each time the sound is needed?
	//
  
	// get lumpnum if necessary
	if (sfx->lumpnum < 0)
		sfx->lumpnum = I_GetSfxLumpNum(sfx);

#ifndef SNDSRV
	// cache data if necessary
	if (!sfx->data)
	{
		// DOS remains, 8bit handling
		sfx->data = (void *) W_CacheLumpNum(sfx->lumpnum, PU_SOUND);
#ifdef __WATCOMC__
		_dpmi_lockregion(sfx->data, lumpinfo[sfx->lumpnum].size);
#endif

	}
#endif
  
	// increase the usefulness
	if (sfx->usefulness++ < 0)
		sfx->usefulness = 1;
  
	// Assigns the handle to one of the channels in the
	//  mix/output buffer.
	channels[cnum].handle = I_StartSound(sfx_id, sfx->data, volume, sep, pitch, priority);
}	
	 
void S_StartSound (void* origin, int sfx_id)
{
	S_StartSoundAtVolume(origin, sfx_id, snd_MaxVolume);
}




void S_StopSound(void *origin)
{
	int cnum;

	for (cnum=0 ; cnum<snd_Channels ; cnum++)
	{
		if (channels[cnum].sfxinfo && channels[cnum].origin == origin)
		{
			S_StopChannel(cnum);
			break;
		}
	}
}


//
// Stop and resume music, during game PAUSE.
//
void S_PauseSound(void)
{
	if (mus_playing && !mus_paused)
	{
		I_PauseSong(mus_playing->handle);
		mus_paused = true;
	}
}

void S_ResumeSound(void)
{
	if (mus_playing && mus_paused)
	{
		I_ResumeSong(mus_playing->handle);
		mus_paused = false;
	}
}


//
// Updates music & sounds
//
void S_UpdateSounds(void* listener_p)
{
	int			audible;
	int			cnum;
	int			volume;
	int			sep;
	int			pitch;
	int			i; // FS: For DMX
	sfxinfo_t*	sfx;
	channel_t*	c;
	
	mobj_t*		listener = (mobj_t*)listener_p;


	
	// Clean up unused data.
	// This is currently not done for 16bit (sounds cached static).
	// DOS 8bit remains. 
	if (gametic > nextcleanup)
	{
		for (i=1 ; i<NUMSFX ; i++)
		{
			if (S_sfx[i].usefulness < 1 && S_sfx[i].usefulness > -1 && S_sfx[i].data) // FS
			{
				if (--S_sfx[i].usefulness == -1)
				{
					Z_ChangeTag(S_sfx[i].data, PU_CACHE);
#ifdef __WATCOMC__
					_dpmi_unlockregion(S_sfx[i].data, lumpinfo[S_sfx[i].lumpnum].size);
#endif
					S_sfx[i].usefulness = 1; // FS
					S_sfx[i].data = NULL; // FS
				}
			}
		}
		nextcleanup = gametic + 15;
	}
	
	for (cnum=0 ; cnum<snd_Channels ; cnum++)
	{
		c = &channels[cnum];
		sfx = c->sfxinfo;

		if (c->sfxinfo)
		{
			if (I_SoundIsPlaying(c->handle))
			{
				// initialize parameters
				volume = snd_MaxVolume;
				pitch = NORM_PITCH;
				sep = NORM_SEP;

				if (sfx->link)
				{
					pitch = sfx->pitch;
					volume += sfx->volume;
					if (volume < 1)
					{
						S_StopChannel(cnum);
						continue;
					}
					else if (volume > snd_MaxVolume)
					{
						volume = snd_MaxVolume;
					}
				}

				// check non-local sounds for distance clipping
				//  or modify their params
				if (c->origin && listener_p != c->origin)
				{
					audible = S_AdjustSoundParams(listener, c->origin, &volume, &sep, &pitch);
			
					if (!audible)
					{
						S_StopChannel(cnum);
					}
					else
						I_UpdateSoundParams(c->handle, volume, sep, pitch);
				}
			}
			else
			{
				// if channel is allocated but sound has stopped,
				//  free it
				S_StopChannel(cnum);
			}
		}
	}
	// FS: kill music if it the volume becomes 0
	if (mus_playing && snd_MusicVolume == 0 && !mus_paused )
		S_PauseSound();
}

void S_SetMusicVolume(int volume)
{
	if (volume < 0 || volume > 127)
	{
		snd_MusicVolume = 12; // FS: Cap it before bomb
		I_Error("Attempt to set music volume at %d", volume);
	}	

	I_SetMusicVolume(127);
	I_SetMusicVolume(volume);
	snd_MusicVolume = volume;

	if(snd_MusicVolume == 0) // FS: Need to stop the song, mang.
	{
		I_PauseSong(mus_playing->handle);
		mus_paused = true;
	}
	else if(mus_paused)
	{
		mus_paused = false;
		I_ResumeSong(mus_playing->handle);
	}
}



void S_SetSfxVolume(int volume)
{
	if (volume < 0 || volume > 127)
	{
		snd_SfxVolume = 12; // FS: Cap it before bomb
		I_Error("Attempt to set sfx volume at %d", volume);
	}

	snd_MaxVolume = volume*8;
}

//
// Starts some music with the music id found in sounds.h.
//
void S_StartMusic(int m_id)
{
	S_ChangeMusic(m_id, false);
}
void S_StartSong(int m_id)
{
	S_ChangeMusic(m_id, false);
}

void S_ChangeMusic (int musicnum, int looping)
{
	musicinfo_t*	music;
	char			namebuf[9];

	if ( (musicnum <= mus_None) || (musicnum >= NUMMUSIC) )
	{
		I_Error("Bad music number %d", musicnum);
	}
	else
		music = &S_music[musicnum];

	if (mus_playing == music)
		return;

	// shutdown old music
	S_StopMusic();

	// get lumpnum if neccessary
	if (!music->lumpnum)
	{
		sprintf(namebuf, "d_%s", DEH_String(music->name));
		music->lumpnum = W_GetNumForName(namebuf);
	}

	// load & register it
	music->data = (void *) W_CacheLumpNum(music->lumpnum, PU_MUSIC);
	music->handle = I_RegisterSong(music->data);
#ifdef __WATCOMC__
	_dpmi_lockregion(music->data, lumpinfo[music->lumpnum].size);
#endif
	// play it
	I_PlaySong(music->handle, looping);

	mus_playing = music;
}


void S_StopMusic(void)
{
	if (mus_playing)
	{
		if (mus_paused)
			I_ResumeSong(mus_playing->handle);

		I_StopSong(mus_playing->handle);
		I_UnRegisterSong(mus_playing->handle);
		Z_ChangeTag(mus_playing->data, PU_CACHE);
#ifdef __WATCOMC__
		_dpmi_unlockregion(mus_playing->data, lumpinfo[mus_playing->lumpnum].size);
#endif	
		mus_playing->data = NULL; // FS
		mus_playing = 0;
	}
}




void S_StopChannel(int cnum)
{

	int			i;
	channel_t*	c = &channels[cnum];

	if (c->sfxinfo)
	{
		// stop the sound playing
		if (I_SoundIsPlaying(c->handle))
		{
#ifdef SAWDEBUG
			if (c->sfxinfo == &S_sfx[sfx_sawful])
				fprintf(stderr, "stopped\n");
#endif
		I_StopSound(c->handle);
		}

		// check to see
		//  if other channels are playing the sound
		for (i=0 ; i<snd_Channels ; i++)
		{
			if (cnum != i && c->sfxinfo == channels[i].sfxinfo)
			{
				break;
			}
		}
	
		// degrade usefulness of sound data
		c->sfxinfo->usefulness--;

		c->sfxinfo = 0;
	}
}



//
// Changes volume, stereo-separation, and pitch variables
//  from the norm of a sound effect to be played.
// If the sound is not audible, returns a 0.
// Otherwise, modifies parameters and returns 1.
//
int
S_AdjustSoundParams
( mobj_t*	listener,
  mobj_t*	source,
  int*		vol,
  int*		sep,
  int*		pitch )
{
	fixed_t	approx_dist;
	fixed_t	adx;
	fixed_t	ady;
	angle_t	angle;

	// calculate the distance to sound origin
	//  and clip it if necessary
	adx = abs(listener->x - source->x);
	ady = abs(listener->y - source->y);

	// From _GG1_ p.428. Appox. eucledian distance fast.
	approx_dist = adx + ady - ((adx < ady ? adx : ady)>>1);
	
	if (gamemap != 8
	&& approx_dist > S_CLIPPING_DIST)
	{
	return 0;
	}
	
	// angle of source to listener
	angle = R_PointToAngle2(listener->x,
				listener->y,
				source->x,
				source->y);

	if (angle > listener->angle)
	angle = angle - listener->angle;
	else
	angle = angle + (0xffffffff - listener->angle);

	angle >>= ANGLETOFINESHIFT;

	// stereo separation
	*sep = 128 - (FixedMul(S_STEREO_SWING,finesine[angle])>>FRACBITS);

	// volume calculation
	if (approx_dist < S_CLOSE_DIST)
	{
	*vol = snd_MaxVolume;
	}
	else if (gamemap == 8)
	{
	if (approx_dist > S_CLIPPING_DIST)
		approx_dist = S_CLIPPING_DIST;

	*vol = 15+ ((snd_MaxVolume-15)
			*((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
		/ S_ATTENUATOR;
	}
	else
	{
	// distance effect
	*vol = (snd_MaxVolume
		* ((S_CLIPPING_DIST - approx_dist)>>FRACBITS))
		/ S_ATTENUATOR; 
	}
	
	return (*vol > 0);
}




//
// S_getChannel :
//   If none available, return -1.  Otherwise channel #.
//
int S_getChannel (void* origin, sfxinfo_t* sfxinfo)
{
	// channel number to use
	int			cnum;
	
	channel_t*	c;

	// Find an open channel
	for (cnum=0 ; cnum<snd_Channels ; cnum++)
	{
			if (!channels[cnum].sfxinfo)
				break;
			else if (origin &&  channels[cnum].origin ==  origin)
			{
				S_StopChannel(cnum);
				break;
			}
	}

	// None available
	if (cnum == snd_Channels)
	{
		// Look for lower priority
		for (cnum=0 ; cnum<snd_Channels ; cnum++)
			if (channels[cnum].sfxinfo->priority >= sfxinfo->priority)
				break;

		if (cnum == snd_Channels)
		{
			// FUCK!  No lower priority.  Sorry, Charlie.	
			return -1;
		}
		else
		{
			// Otherwise, kick out lower priority.
			S_StopChannel(cnum);
		}
	}

	c = &channels[cnum];

	// channel is decided to be cnum.
	c->sfxinfo = sfxinfo;
	c->origin = origin;

	return cnum;
}

#endif
