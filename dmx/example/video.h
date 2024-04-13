#ifndef _VIDEO_H_
#define _VIDEO_H_

extern unsigned char	vidRows;
extern unsigned	 		vidCols;

/****************************************************************************
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
*----------------------------------------------------------------------------
* Values for attribute:
*   bit 7: blink
*   bits 6-4: background color
*       000 black
*       001 blue
*       010 green
*       011 cyan
*       100 red
*       101 magenta
*       110 brown
*       111 white
*   bits 3-0: foreground color
*       0000 black       1000 dark gray
*       0001 blue        1001 light blue
*       0010 green       1010 light green
*       0011 cyan        1011 light cyan
*       0100 red         1100 light red
*       0101 magenta     1101 light magenta
*       0110 brown       1110 yellow
*       0111 light gray  1111 white
*****************************************************************************/
#define VA_BLINK        0x80
#define VA_ON_BLACK     0x00
#define VA_ON_BLUE      0x10
#define VA_ON_GREEN     0x20
#define VA_ON_CYAN      0x30
#define VA_ON_RED       0x40
#define VA_ON_MAGENTA   0x50
#define VA_ON_BROWN     0x60
#define VA_ON_GRAY      0x70
#define VA_BLACK        0x00
#define VA_BLUE         0x01
#define VA_GREEN        0x02
#define VA_CYAN         0x03
#define VA_RED          0x04
#define VA_MAGENTA      0x05
#define VA_BROWN        0x06
#define VA_GRAY         0x07
#define VA_DARK_GRAY    0x08
#define VA_LT_BLUE      0x09
#define VA_LT_GREEN     0x0A
#define VA_LT_CYAN      0x0B
#define VA_LT_RED       0x0C
#define VA_LT_MAGENTA   0x0D
#define VA_YELLOW       0x0E
#define VA_WHITE        0x0F

/****************************************************************************
* vioGetMode - Get active video mode
*****************************************************************************/
int     /* Returns active mode */
vioGetMode( void );

/****************************************************************************
* vioSetMode - Set video mode
*****************************************************************************/
int     /* Returns new mode */
vioSetMode(
    int     mode        /* INPUT: Video Mode - See Listing Above */
    );

/****************************************************************************
* vioSetCurPos - Set cursor position
*****************************************************************************/
void
vioSetCurPos(
    int     row,        /* INPUT: Row on screen     - 0 = Top   */
    int     column      /* INPUT: Column on screen  - 0 = Left  */
    );

/****************************************************************************
* vioGetCurPos - Get cursor position
*****************************************************************************/
void
vioGetCurPos(
    int     *row,       /* OUTPUT: Row on screen    - 0 = Top   */
    int     *column     /* OUTPUT: Column on screen - 0 = Left  */
    );

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
    );

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
    );

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
    );

/****************************************************************************
* vioDrawCharAttr - Draws character/attribute 'n' times starting from cursor.
*****************************************************************************/
void
vioDrawCharAttr(
    char    character,  /* INPUT: Character to draw                 */
    int     attribute,  /* INPUT: Attribute (color) to fill with    */
    int     repeat      /* INPUT: Repeat count                      */
    );

/****************************************************************************
* vioDrawChar - Draws character 'n' times starting from cursor.
*****************************************************************************/
void
vioDrawChar(
    char    character,  /* INPUT: Character to draw                 */
    int     repeat      /* INPUT: Repeat count                      */
    );

/****************************************************************************
* vioDrawStr - Draws string of characters starting from cursor.
*****************************************************************************/
void
vioDrawStr(
    char    *string     /* INPUT: String to draw                    */
    );

/****************************************************************************
* vioDrawStrAt - Draws string of characters starting from cursor.
*****************************************************************************/
void
vioDrawStrAt(
    int     row,        /* INPUT: Row to draw                       */    
    int     column,     /* INPUT: Column to draw                    */
    char    *string     /* INPUT: String to draw                    */
    );

/****************************************************************************
* vioDrawStrAttr - Draws string of characters with attribute starting from
*                  cursor.
*****************************************************************************/
void
vioDrawStrAttr(
    char    *string,    /* INPUT: String to draw                    */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    );

/****************************************************************************
* vioDrawStrAttrAt - Draws string of characters with attribute starting from
*                    cursor.
*****************************************************************************/
void
vioDrawStrAttrAt(
    int     row,        /* INPUT: Row to draw                       */    
    int     column,     /* INPUT: Column to draw                    */
    char    *string,    /* INPUT: String to draw                    */
    int     attribute   /* INPUT: Attribute (color) to fill with    */
    );

/****************************************************************************
* vioReadCharAttr - Reads character/attribute at cursor.
*****************************************************************************/
void
vioReadCharAttr(
    char    *character, /* OUTPUT: Character                        */
    char    *attribute  /* OUTPUT: Attribute                        */
    );

/****************************************************************************
* vioSaveArea - Allocates memory and saves area of screen.
*****************************************************************************/
void *
vioSaveArea(
    int     t_row,      /* INPUT: Top row of area           */
    int     l_column,   /* INPUT: Left column of area       */
    int     rows,       /* INPUT: Rows in area 1..n         */
    int     columns     /* INPUT: Columns in area 1..n      */
    );

/****************************************************************************
* vioRestoreArea - Restore area that was saved before and releases memory.
*****************************************************************************/
void
vioRestoreArea(
    void    *save       /* INPUT: Saved memory area address */
    );

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
    );

#endif  /* _VIDEO_H_ */
