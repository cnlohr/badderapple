#include <stdio.h>
#include "../common/bacommon.h"

uint32_t datastream[FRAMECT][RESY/BLOCKSIZE][RESX/BLOCKSIZE];

int main()
{
	FILE * f = fopen( "stream.dat", "rb" );
	uint32_t tr;
	int frame, y, x;
	for( frame = 0; frame < FRAMECT; frame++ )
	for( y = 0; y < RESY/BLOCKSIZE; y++ )
	for( x = 0; x < RESX/BLOCKSIZE; x++ )
	{
		if( fread( &tr, 4, 1, f ) != 1 )
		{
			fprintf( stderr, "error: wrong stream read\n" );
			exit( -5 );
		}
		datastream[frame][y][x] = tr;
	}
	fclose( f );
	f = fopen( "stream-original-small.dat", "wb" );
	for( y = 0; y < RESY/BLOCKSIZE; y++ )
	for( x = 0; x < RESX/BLOCKSIZE; x++ )
	for( frame = 0; frame < FRAMECT; frame++ )
	{
		tr = datastream[frame][y][x];
		fwrite( &tr, 1, 1, f );
	}
	fclose( f );

	f = fopen( "stream-reorder-small.dat", "wb" );
	for( frame = 0; frame < FRAMECT; frame++ )
	for( y = 0; y < RESY/BLOCKSIZE; y++ )
	for( x = 0; x < RESX/BLOCKSIZE; x++ )
	{
		tr = datastream[frame][y][x];
		fwrite( &tr, 1, 1, f );
	}
	fclose( f );

	f = fopen( "stream-original-big.dat", "wb" );
	for( frame = 0; frame < FRAMECT; frame++ )
	for( y = 0; y < RESY/BLOCKSIZE; y++ )
	for( x = 0; x < RESX/BLOCKSIZE; x++ )
	{
		tr = datastream[frame][y][x];
		fwrite( &tr, 1, 4, f );
	}
	fclose( f );

	f = fopen( "stream-reorder-big.dat", "wb" );
	for( y = 0; y < RESY/BLOCKSIZE; y++ )
	for( x = 0; x < RESX/BLOCKSIZE; x++ )
	for( frame = 0; frame < FRAMECT; frame++ )
	{
		tr = datastream[frame][y][x];
		fwrite( &tr, 1, 4, f );
	}
	fclose( f );
}
