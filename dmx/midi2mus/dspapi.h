typedef enum
{
    DSP_MONO_8,         // 8 Bit Mono
    DSP_STEREO_LR8,     // 8 Bit Stereo Left-Right ordering
    DSP_STEREO_RL8,     // 8 Bit Stereo Right-Left ordering
	DSP_STEREO_LR8S		// 8 Bit Stereo Left-Right-Seperate Channels
} CHNL_TYPE;


/****************************************************************************
* dspInit - Initialize DSP Utilities
*****************************************************************************/
PUBLIC int      // returns: 0=Success, 1=Fail, Not enough memory
dspInit(
    CHNL_TYPE   eType,          // INPUT : Physical properties of channel
    WORD        SampleRate,     // INPUT : Sample Rate for Playback
    int         Channels,       // INPUT : Max # channels allowed
    WORD        (*GetPlayCnt)(  // INPUT : Get current playback position
                    WORD            // PASSBACK: DMA Buffer Size
                    ),
    VOID        (*MixNotify)(   // INPUT : Mix Complete Notification
                    BYTE *,         // PASSBACK; Mixed Data
                    WORD,           // PASSBACK: amount mixed.
                    WORD            // PASSBACK: position
                    ),
	BOOL		SyncPages	    // INPUT : T/F Sync. Pages with DMA Int.
    );

/****************************************************************************
* dspDeInit - Deinitialize DSP Utilities
*****************************************************************************/
PUBLIC void
dspDeInit(
    void
    );

/****************************************************************************
* dspGetDmaBuffer - Get DMA Buffer Address
*****************************************************************************/
PUBLIC BYTE *
dspGetDmaBuffer(
	void
	);

/****************************************************************************
* dspGetPageSize - Get DMA Page Size
*****************************************************************************/
PUBLIC WORD
dspGetPageSize(
	void
	);

/****************************************************************************
* dspGetDmaSize - Get Size of DMA Buffer
*****************************************************************************/
PUBLIC WORD
dspGetDmaSize(
	void
	);

/****************************************************************************
* dspNextPage - Advance to next mixing page
*****************************************************************************/
PUBLIC void
dspNextPage(
	void
	);

/****************************************************************************
* dspStopPatch - Stop playing patch associated with handle.
*****************************************************************************/
PUBLIC VOID
dspStopPatch(
	int			handle      // INPUT : Handle to Active Patch
	);

/****************************************************************************
* dspIsPlaying - Test if patch is still playing.
*****************************************************************************/
PUBLIC int	/* Returns: 1=Active, 0=Inactive	*/
dspIsPlaying(
	int			handle      // INPUT : Handle to Active Patch
	);

/****************************************************************************
* dspAdvlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
dspAdvPlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			left,    	// INPUT : 0=Silent..127=Full Volume
	int			right,   	// INPUT : 0=Silent..127=Full Volume
	int			phase,		// INPUT : Sample Delay -16=L..0=C..16=R
	int			flags,		// INPUT : Flags
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	);

/****************************************************************************
* dspPlayPatch - Play Patch.
*****************************************************************************/
PUBLIC int		// Returns: -1=Failure, else good handle
dspPlayPatch(
	void		*patch,		// INPUT : Patch to play
	int			pitch,		// INPUT : 0..128..255  -1Oct..C..+1Oct
	int			x,     		// INPUT : X (l-r) 0=Left,127=Center,255=Right
	int			v,    		// INPUT : 0=Silent..127=Full Volume
	int			flags,		// INPUT : Flags
	int			priority	// INPUT : Priority, 0=No Stealing 1..MAX_INT
	);

/****************************************************************************
* dspAdvSetOrigin - Change origin of active patch.
* ---------------------------------------------------------------------------
* NOTE: for each X,VOLUME and PITCH the parameter will be ignored if it's
*       value == -1
*****************************************************************************/
PUBLIC VOID
dspAdvSetOrigin(
	int			handle,     // INPUT : Handle to Active Patch
	int			pitch,		// INPUT : 0..127..255
	int			left,     	// INPUT : 0=Silent..127=Full Volume
	int			right,    	// INPUT : 0=Silent..127=Full Volume
	int			phase		// INPUT : Phase Shift
	);

/****************************************************************************
* dspSetOrigin - Change origin of active patch.
* ---------------------------------------------------------------------------
* NOTE: for each X,VOLUME and PITCH the parameter will be ignored if it's
*       value == -1
*****************************************************************************/
PUBLIC VOID
dspSetOrigin(
	int			handle,     // INPUT : Handle to Active Patch
	int			pitch,		// INPUT : 0..127..255
	int			x,     		// INPUT : X (l-r) 0=Left,127=Center,255=Right
	int			v    		// INPUT : 0=Silent..127=Full Volume
	);

/****************************************************************************
* dspExpandedStereo - Sets Expanded Stereo ON/OFF
*****************************************************************************/
PUBLIC int		// returns: previous setting 0=Off, 1=On
dspExpandedStereo(
	int			mode			// INPUT : 1=On,0=Off
	);
