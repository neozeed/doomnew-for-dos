static char *
bytetohex(
	unsigned char	byte,
	char			*str
	)
{
	char			*p = str;

	*p = ( byte >> 4 ) + '0';
	if ( *p > '9' )
		*p += ( 'A' - ':' );
    ++p;
	*p = ( byte & 0x0F ) + '0';
	if ( *p > '9' )
		*p += ( 'A' - ':' );
    ++p;
    *p = '\0';
	return str;
}
static char *
wordtohex(
	unsigned short	word,
	char			*str
	)
{
	bytetohex( word >> 8, str );
	bytetohex( word & 0x00FF, str + 2 );
	return str;
}

static char *
dwordtohex(
	unsigned long   dword,
	char			*str
	)
{
	wordtohex( dword >> 16, str );
	wordtohex( dword & 0x0000FFFF, str + 4 );
	return str;
}

