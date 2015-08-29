// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// Copyright(C) 2005 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
// 02111-1307, USA.
//
//-----------------------------------------------------------------------------
//
// Parses "Cheat" sections in dehacked files
//
//-----------------------------------------------------------------------------

#include <string.h>

#include "doomdef.h"
#include "doomtype.h"

#include "deh_defs.h"
#include "deh_io.h"
#include "deh_main.h"
// #include "am_map.h"
#include "m_cheat.h"

typedef struct 
{
	char *name;
	cheatseq_t *seq;
	int	length; // FS: Cheat lengths for Vanilla
} deh_cheat_t;

static deh_cheat_t allcheats[] =
{
    {"Change music",        &cheat_mus, 4 },
    {"Chainsaw",            &cheat_choppers, 9},
    {"God mode",            &cheat_god, 4},
    {"Ammo & Keys",         &cheat_ammo, 4},
    {"Ammo",                &cheat_ammonokey, 3},
    {"No Clipping 1",       &cheat_noclip, 9},
    {"No Clipping 2",       &cheat_commercial_noclip, 5},
    {"Invincibility",       &cheat_powerup[0], 8},
    {"Berserk",             &cheat_powerup[1], 8},
    {"Invisibility",        &cheat_powerup[2], 8},
    {"Radiation Suit",      &cheat_powerup[3], 8},
    {"Auto-map",            &cheat_powerup[4], 8},
    {"Lite-Amp Goggles",    &cheat_powerup[5], 8},
    {"BEHOLD menu",         &cheat_powerup[6], 7},
    {"Level Warp",          &cheat_clev, 5},
    {"Player Position",     &cheat_mypos, 6},
    {"Map cheat",           &cheat_amap, 3},
};

static deh_cheat_t *FindCheatByName(char *name)
{
    size_t i;
    
    for (i=0; i<arrlen(allcheats); ++i)
    {
        if (!strcasecmp(allcheats[i].name, name))
            return &allcheats[i];
    }

    return NULL;
}

static void *DEH_CheatStart(deh_context_t *context, char *line)
{
    return NULL;
}

static void DEH_CheatParseLine(deh_context_t *context, char *line, void *tag)
{
    deh_cheat_t *cheat;
    char *variable_name;
    char *value;
    unsigned char *unsvalue;
    unsigned int i;

    if (!DEH_ParseAssignment(line, &variable_name, &value))
    {
        // Failed to parse

        DEH_Warning(context, "Failed to parse assignment");
        return;
    }

    unsvalue = (unsigned char *) value;

    cheat = FindCheatByName(variable_name);

    if (cheat == NULL)
    {
        DEH_Warning(context, "Unknown cheat '%s'", variable_name);
        return;
    }

    // write the value into the cheat sequence

    i = 0;

    while (unsvalue[i] != 0 && unsvalue[i] != 0xff)
    {
        // If the cheat length exceeds the Vanilla limit, stop.  This
        // does not apply if we have the limit turned off.

        if (!deh_allow_long_cheats && i > cheat->length) // FS
        {
            printf("Cheat sequence longer than supported by Vanilla dehacked for cheat:\n%s\nPress a key to continue...\n", variable_name);
		getchar();
            break;
        }

	if (deh_apply_cheats)
	{
	    cheat->seq->sequence[i] = SCRAMBLE(unsvalue[i]);
	}
        ++i;


        // Absolute limit - don't exceed

        if (i >= MAX_CHEAT_LEN - arrlen(cheat->seq->p))
        {
            DEH_Error(context, "Cheat sequence too long!");
            return;
        }
    }

	if (deh_apply_cheats)
	{
		if(!strcasecmp("Change Music", variable_name)) // FS: Hack for these cheats the want a variable
		{
			cheat->seq->sequence[i] = 1;
			cheat->seq->sequence[i+1] = 0;
			cheat->seq->sequence[i+2] = 0;
			cheat->seq->sequence[i+3] = 0xff;
		}
		else if(!strcasecmp("Level Warp", variable_name))
		{
			cheat->seq->sequence[i] = 1;
			cheat->seq->sequence[i+1] = 0;
			cheat->seq->sequence[i+2] = 0;
			cheat->seq->sequence[i+3] = 0xff;
		}
		else
			cheat->seq->sequence[i] = 0xff;
	}
}

deh_section_t deh_section_cheat =
{
    "Cheat",
    NULL,
    DEH_CheatStart,
    DEH_CheatParseLine,
    NULL,
    NULL,
};
