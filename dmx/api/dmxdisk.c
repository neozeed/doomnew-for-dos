/*===========================================================================
* DMXDISK.C - Cached read from disk
*----------------------------------------------------------------------------
*                CONFIDENTIAL & PROPRIETARY INFORMATION 			     
*----------------------------------------------------------------------------
*             Copyright (C) 1994 Digital Expressions, Inc. 		     
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
*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <dos.h>

#include "dmx.h"

typedef struct
{
    BYTE            *buffer;
    UINT            index;
    int             file_handle;
    UINT	        size;
    UINT            cached;
} FCB;

/****************************************************************************
* DmxOpenFile - Open DOS file for buffered reads
****************************************************************************/
PUBLIC VOID *       // Returns: File handle
DmxOpenFile(
    char            *filename,      // INPUT: Filename to open
    BYTE            *buffer,        // INPUT: Cache buffer
    UINT            size            // INPUT: Cache size    
    )
{
    FCB             *fcb;

    fcb = malloc( sizeof( FCB ) );
    if ( fcb != NULL )
    {
        memset( fcb, 0, sizeof( FCB ) );
    	if ( _dos_open( filename, O_RDWR, &fcb->file_handle ) != 0 )
        {
            free( fcb );
            return NULL;
        }
        fcb->buffer = buffer;
        fcb->size   = size;
        fcb->index	= 0;
    }
    return ( void * ) fcb;
}

/****************************************************************************
* DmxReadFile - Read from file.
****************************************************************************/
PUBLIC UINT			// Returns: number of bytes read
DmxReadFile(
    void            *fh,        // INPUT: File Handle from DmxOpenFile
    VOID            *data,    // INPUT: Buffer to read into    
    UINT	        amount      // INPUT: Amount of data to read
    )
{
    FCB             *fcb = ( FCB * ) fh;
    UINT            len;
    UINT            ar;
	BYTE			*buffer = ( BYTE * ) data;

    ar = amount;
    while ( ar > 0 )
    {
        if ( fcb->cached )
        {
            len = ( fcb->cached >= ar ? ar : fcb->cached );
            memcpy( buffer, fcb->buffer + fcb->index, len );

            fcb->index    += len;
            buffer        += len;
            fcb->cached   -= len;
            ar            -= len;
			continue;
        }
        if ( ar >= fcb->size )
        {
    	    _dos_read( fcb->file_handle, buffer, ar, &len );
            ar -= len;
            break;
        }
        _dos_read( fcb->file_handle, fcb->buffer, fcb->size, &fcb->cached );
        fcb->index = 0;
		if ( fcb->cached == 0 )
			break;
    }
    return amount - ar;
}

/****************************************************************************
* DmxCloseFile - ReadFile - Read from file.
****************************************************************************/
VOID
DmxCloseFile(
    void            *fh
    )
{
    FCB             *fcb = ( FCB * ) fh;

    _dos_close( fcb->file_handle );
    free( fcb );
}
