main()
{
	long		j;
	unsigned 	mog1, mog2, q;
	long		p;

	mog1 = 0xEA >> 1;
	mog2 = 0xEA;
	q = 0x7;
	p = 0L;
	for ( j = 0; j < 15532; j++ )
	{
		if ( j % 8 == 0 )
			printf( "\n" );
		printf( "%6ld,", p );
		mog1 += mog2;
		p += ( q + ( mog1 >> 8 ) );
		mog1 &= 0x00FF;
	}
}
