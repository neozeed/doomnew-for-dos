/***************************************************************************
**	NAME:  GF1SCALE.H
**	COPYRIGHT:
**	"Copyright (c) 1991,1992, by Advanced Gravis Computer Technology LTD
**
**       "This software is furnished under a license and may be used,
**       copied, or disclosed only in accordance with the terms of such
**       license and with the inclusion of the above copyright notice.
**       This software or any other copies thereof may not be provided or
**       otherwise made available to any other person. No title to and
**       ownership of the software is hereby transfered."
****************************************************************************
*  CREATION DATE: 07/01/92
*--------------------------------------------------------------------------*
*     VERSION	DATE	   NAME		DESCRIPTION
*>	1.0	07/01/92		Original
*>	1.1	09/15/92		More precision
***************************************************************************/

#define SCALE_TABLE_SIZE	128

#ifdef GF1_KERNEL
long	gf1_scale_table[ SCALE_TABLE_SIZE ] =
{
    8175L,     8661L,     9177L,     9722L,    10300L,    10913L,    11562L,    12249L,    12978L,    13750L,    14567L,    15433L,
   16351L,    17323L,    18354L,    19445L,    20601L,    21826L,    23124L,    24499L,    25956L,    27500L,    29135L,    30867L,
   32703L,    34647L,    36708L,    38890L,    41203L,    43653L,    46249L,    48999L,    51913L,    55000L,    58270L,    61735L,
   65406L,    69295L,    73416L,    77781L,    82406L,    87307L,    92498L,    97998L,   103826L,   110000L,   116540L,   123470L,
  130812L,   138591L,   146832L,   155563L,   164813L,   174614L,   184997L,   195997L,   207652L,   220000L,   233081L,   246941L,
  261625L,   277182L,   293664L,   311126L,   329627L,   349228L,   369994L,   391995L,   415304L,   440000L,   466163L,   493883L,
  523251L,   554365L,   587329L,   622253L,   659255L,   698456L,   739988L,   783990L,   830609L,   880000L,   932327L,   987766L,
 1046502L,  1108730L,  1174659L,  1244507L,  1318510L,  1396912L,  1479977L,  1567981L,  1661218L,  1760000L,  1864655L,  1975533L,
 2093004L,  2217461L,  2349318L,  2489015L,  2637020L,  2793825L,  2959955L,  3135963L,  3322437L,  3520000L,  3729310L,  3951066L,
 4186009L,  4434922L,  4698636L,  4978031L,  5274040L,  5587651L,  5919910L,  6271926L,  6644875L,  7040000L,  7458620L,  7902132L,
 8372018L,  8869844L,  9397272L,  9956063L, 10548081L, 11175303L, 11839821L, };
#else
extern long gf1_scale_table[];
#endif
