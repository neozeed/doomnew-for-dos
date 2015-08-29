// DOOM strings, by language.


#ifndef __DSTRINGS__
#define __DSTRINGS__


#include "d_englsh.h"

// Misc. other strings.
#define SAVEGAMENAME		"doomsav"


//
// File locations,
// relative to current position.
// Path names are OS-sensitive.
//
#define DEVMAPS			"devmaps"
#define DEVDATA			"devdata"

// QuitDOOM messages
#define NUM_QUITMESSAGES	8

extern char*			endmsg[];
extern char*			endmsg2[]; // FS: Fuck this shit
extern char*			endmsgchex[]; // FS: For Chex Quest

#if 0
extern char*			endmsgunused[]; // FS: John Romero's unused quit messages.
#endif


#endif
