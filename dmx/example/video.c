#include <stdio.h>
#include <malloc.h>
#include <dos.h>

typedef struct
{
    int     row;
    int     column;
    int     rows;
    int     columns;
    int     cur_row;
    int     cur_column;
} SAVESIZE;

unsigned char		vidRows;
unsigned	 		vidCols;

/****************************************************************************
* vioGetMode - Get active video mode
*****************************************************************************/
int     /* Returns active mode */
vioGetMode( void )
{
    union REGS  r;

	vidCols = * ( unsigned  * )( 0x44AL );
	vidRows = * ( unsigned char * )( 0x484L );
	if ( vidRows == 0 )
		vidRows = 24;
	++vidRows;

    r.w.ax = 0x0F00;
    int386( 0x10, &r, &r );
    return ( r.h.al & 0x7F );
}

/****************************************************************************
* vioSetMode - Set video mode
*----------------------------------------------------------------------------
* Values for video mode:    (partial listing)
*       text/ text pixel   pixel  colors  disply  scrn  system
*       grph resol  box   resoltn         pages   addr
*  00h = T   40x25  8x8           16gray     8    B800 CGA,PCjr
*      = T   40x25  8x14          16gray     8    B800 EGA
*      = T   40x25  8x16            16       8    B800 MCGA
*      = T   40x25  9x16            16       8    B800 VGA
*  01h = T   40x25  8x8             16       8    B800 CGA,PCjr
*      = T   40x25  8x14            16       8    B800 EGA
*      = T   40x25  8x16            16       8    B800 MCGA
*      = T   40x25  9x16            16       8    B800 VGA
*  02h = T   80x25  8x8           16gray     4    B800 CGA,PCjr
*      = T   80x25  8x14          16gray     4    B800 EGA
*      = T   80x25  8x16            16       4    B800 MCGA
*      = T   80x25  9x16            16       4    B800 VGA
*  03h = T   80x25  8x8             16       4    B800 CGA,PCjr
*      = T   80x25  8x14            16       4    B800 EGA
*      = T   80x25  8x16            16       4    B800 MCGA
*      = T   80x25  9x16            16       4    B800 VGA
*  04h = G   40x25  8x8   320x200    4            B800 CGA,PCjr,EGA,MCGA,VGA
*  05h = G   40x25  8x8   320x200  4gray          B800 CGA,PCjr,EGA
*      = G   40x25  8x8   320x200    4            B800 MCGA,VGA
*  06h = G   80x25  8x8   640x200    2            B800 CGA,PCjr,EGA,MCGA,VGA
*  07h = T   80x25  9x14           mono     var   B000 MDA,Hercules,EGA
*      = T   80x25  9x16           mono           B000 VGA
*****************************************************************************/
int
vioSetMode(
    int     mode        /* INPUT: Video Mode - See Listing Above */
    )
{
    union REGS  r;

    r.h.ah = 0x00;
    r.h.al = mode;
    int386( 0x10, &r, &r );

	return vioGetMode();
}


/****************************************************************************
* vioSetCurPos - Set cursor position
*****************************************************************************/
void
vioSetCurPos(
    int     row,        /* INPUT: Row on screen     - 0 = Top   */
    int     column      /* INPUT: Column on screen  - 0 = Left  */
    )
{
    union REGS  r;

    r.h.ah = 0x02;
    r.h.bh = 0;
    r.h.dh = row;
    r.h.dl = column;
    int386( 0x10, &r, &r );
}

/****************************************************************************
* vioGetCurPos - Get cursor position
*****************************************************************************/
void
vioGetCurPos(
    int     *row,       /* OUTPUT: Row on screen    - 0 = Top   */
    int     *column     /* OUTPUT: Column on screen - 0 = Left  */
    )
{
    union REGS  r;

    r.h.ah = 0x03;
    r.h.bh = 0;
    int386( 0x10, &r, &r );

    if ( row )
        *row = r.h.dh;
    if ( column )
        *column = r.h.dl;
}



/****************************************************************************
* vioScrollUp - Scroll specified area (window) up.
*****************************************************************************/
void
vioScrollUp(
    int     t_row,      /* INPUT: Window Top Row - 0 = Top          */
    int     l_column,   /* INPUT: Window Left Column                */
    int     b_row,      /* INPUT: Window Bottom Row                 */
    int     r_column,   /* INPUT: Window Right Column               */
    int     rows,       /* INPUT: Number of rows to scroll          */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    )
{
    union REGS  r;

    r.h.ah = 0x06;
    r.h.al = rows;
    r.h.bh = attribute;

    r.h.ch = t_row;
    r.h.cl = l_column;
    r.h.dh = b_row;
    r.h.dl = r_column;

    int386( 0x10, &r, &r );
}


/****************************************************************************
* vioScrollDown - Scroll specified area (window) down.
*****************************************************************************/
void
vioScrollDown(
    int     t_row,      /* INPUT: Window Top Row - 0 = Top          */
    int     l_column,   /* INPUT: Window Left Column                */
    int     b_row,      /* INPUT: Window Bottom Row                 */
    int     r_column,   /* INPUT: Window Right Column               */
    int     rows,       /* INPUT: Number of rows to scroll          */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    )
{
    union REGS  r;

    r.h.ah = 0x07;
    r.h.al = rows;
    r.h.bh = attribute;

    r.h.ch = t_row;
    r.h.cl = l_column;
    r.h.dh = b_row;
    r.h.dl = r_column;

    int386( 0x10, &r, &r );
}



/****************************************************************************
* vioClearWindow - Clear specified area (window) to attribute.
*****************************************************************************/
void
vioClearWindow(
    int     t_row,      /* INPUT: Window Top Row - 0 = Top          */
    int     l_column,   /* INPUT: Window Left Column                */
    int     b_row,      /* INPUT: Window Bottom Row                 */
    int     r_column,   /* INPUT: Window Right Column               */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    )
{
    vioScrollUp( t_row, l_column, b_row, r_column, 0, attribute );
}


/****************************************************************************
* vioDrawCharAttr - Draws character/attribute 'n' times starting from cursor.
*****************************************************************************/
void
vioDrawCharAttr(
    char    character,  /* INPUT: Character to draw                 */
    int     attribute,  /* INPUT: Attribute (color) to fill with    */
    int     repeat      /* INPUT: Repeat count                      */
    )
{
    union REGS  r;

    r.h.ah = 0x09;
    r.h.al = character;
    r.h.bh = 0;
    r.h.bl = attribute;

    if ( repeat > 0 )
    {
        r.w.cx = repeat;
        int386( 0x10, &r, &r );
    }
}

/****************************************************************************
* vioDrawChar - Draws character 'n' times starting from cursor.
*****************************************************************************/
void
vioDrawChar(
    char    character,  /* INPUT: Character to draw                 */
    int     repeat      /* INPUT: Repeat count                      */
    )
{
    union REGS  r;

    r.h.ah = 0x0A;
    r.h.al = character;
    r.h.bh = 0;

    if ( repeat > 0 )
    {
        r.w.cx = repeat;
        int386( 0x10, &r, &r );
    }
}


/****************************************************************************
* vioDrawStr - Draws string of characters starting from cursor.
*****************************************************************************/
void
vioDrawStr(
    char    *string     /* INPUT: String to draw                    */
    )
{
    int     row, column, save;

    vioGetCurPos( &row, &column );
    save = column;
    while ( *string )
    {
        vioDrawChar( *string++, 1 );
        vioSetCurPos( row, ++column );
    }
    vioSetCurPos( row, save );
}


/****************************************************************************
* vioDrawStrAttr - Draws string of characters with attribute starting from
*                  cursor.
*****************************************************************************/
void
vioDrawStrAttr(
    char    *string,    /* INPUT: String to draw                    */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    )
{
    int     row, column, save;

    vioGetCurPos( &row, &column );
    save = column;
    while ( *string )
    {
        vioDrawCharAttr( *string++, attribute, 1 );
        vioSetCurPos( row, ++column );
    }
    vioSetCurPos( row, save );
}

/****************************************************************************
* vioReadCharAttr - Reads character/attribute at cursor.
*****************************************************************************/
void
vioReadCharAttr(
    char    *character, /* OUTPUT: Character                        */
    char    *attribute  /* OUTPUT: Attribute                        */
    )
{
    union REGS  r;

    r.h.ah = 0x08;
    r.h.bh = 0;
    int386( 0x10, &r, &r );

    if ( character )
        *character = r.h.al;
    if ( attribute )
        *attribute = r.h.ah;
}


/****************************************************************************
* vioSaveArea - Allocates memory and saves area of screen.
*****************************************************************************/
void *
vioSaveArea(
    int     t_row,      /* INPUT: Top row of area           */
    int     l_column,   /* INPUT: Left column of area       */
    int     rows,       /* INPUT: Rows in area 1..n         */
    int     columns     /* INPUT: Columns in area 1..n      */
    )
{
    int         y, x;
    char        *under;
    size_t      size;
    SAVESIZE    *ss;
    char        *ca;

    size = rows * columns * 2 + sizeof( SAVESIZE );
    if ( ( under = malloc( size ) ) != NULL )
    {
        ss = ( SAVESIZE * ) under;
        vioGetCurPos( &ss->cur_row, &ss->cur_column );

        ss->row     = t_row;
        ss->column  = l_column;
        ss->rows    = rows;
        ss->columns = columns;

        ca = under + sizeof( SAVESIZE );
        for ( y = t_row; y < ( t_row + rows ); y++ )
        {
            for ( x = l_column; x < ( l_column + columns ); x++ )
            {
                vioSetCurPos( y, x );
                vioReadCharAttr( ca, ca + 1 );
                ca += 2;
            }
        }
    }
    return under;
}

/****************************************************************************
* vioRestoreArea - Restore area that was saved before and releases memory.
*****************************************************************************/
void
vioRestoreArea(
    void    *save       /* INPUT: Saved memory area address */
    )
{
    int         y, x;
    SAVESIZE    *ss;
    char        *ca;

    if ( save == NULL )
        return;

    ss = ( SAVESIZE * ) save;
    if ( ss->rows == 0 )
        return;

    ca = ( char * )( ( char * ) save + sizeof( SAVESIZE ) );
    for ( y = 0; y < ( ss->rows ); y++ )
    {
        for ( x = 0; x < ss->columns; x++ )
        {
            vioSetCurPos( ss->row + y, ss->column + x );
            vioDrawCharAttr( ca[ 0 ], ( int )( ca[ 1 ] ), 1 );
            ca += 2;
        }
    }
    vioSetCurPos( ss->cur_row, ss->cur_column );
    ss->rows = 0;
    free( save );
}

/****************************************************************************
* vioDrawBox - Draws box with optional border onto screen with option of
*              saving the contents of the screen that the box overwrites.
*****************************************************************************/
void *
vioDrawBox(
    int     t_row,      /* INPUT: Top row of box            */
    int     l_column,   /* INPUT: Left column of box        */
    int     rows,       /* INPUT: Rows in box 1..n          */
    int     columns,    /* INPUT: Columns in box 1..n       */
    int     attribute,  /* INPUT: Attribute of box          */
    int     border,     /* INPUT: Flag to draw border       */
    int     saveunder   /* INPUT: Save what's under box     */
    )
{
    int         width, height;
    int         y;
    char        *under;

    if ( t_row < 0 )    /* center */
        t_row = ( 25 - rows ) / 2;

    if ( l_column < 0 ) /* center */
        l_column = ( 80 - columns ) / 2;

    if ( border )
    {
        if ( t_row > 0 )
            --t_row;
        if ( l_column > 0 )
            --l_column;

        width = columns + 2;
        height = rows + 2;
    }
    else
    {
        width = columns;
        height = rows;
    }
    under = NULL;
    if ( saveunder )
    {
        under = vioSaveArea( t_row, l_column, height, width );
    }
    vioClearWindow( t_row, l_column,
                    t_row + height - 1, l_column + width - 1, attribute );
    if ( border )
    {
        vioSetCurPos( t_row, l_column );
        vioDrawChar( 'Ú', 1 );
        vioSetCurPos( t_row, l_column + 1 );
        vioDrawChar( 'Ä', columns );
        vioSetCurPos( t_row, l_column + width - 1 );
        vioDrawChar( '¿', 1 );

        for ( y = 1; y <= rows; y++ )
        {
            vioSetCurPos( t_row + y, l_column );
            vioDrawChar( '³', 1 );

            vioSetCurPos( t_row + y, l_column + width - 1 );
            vioDrawChar( '³', 1 );
        }

        vioSetCurPos( t_row + height - 1, l_column );
        vioDrawChar( 'À', 1 );
        vioSetCurPos( t_row + height - 1, l_column + 1 );
        vioDrawChar( 'Ä', columns );
        vioSetCurPos( t_row + height - 1, l_column + width - 1 );
        vioDrawChar( 'Ù', 1 );
    }
    vioSetCurPos( ( border ) ? t_row + 1 : t_row,
                  ( border ) ? l_column + 1 : l_column
                );
    return under;
}
