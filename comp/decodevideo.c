
#include "ffmdecode.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int targw, targh;

uint8_t * data;
int frames;
FILE * f ;
void got_video_frame( unsigned char * rgbbuffer, int linesize, int width, int height, int frame )
{
	//else data = realloc( data, width*height*(++frames));
	uint8_t * fd = data;
	int x;
	int y;
	printf( "%d %d %d %p %p %d\n", width, height, linesize, fd, fd+width*height, frame );
	for( y = 0; y < targh; y++ )
	{
		for( x = 0; x < targw; x++ )
		{
			int div = 0;
			int sum = 0;
			int xa, ya;
			for( ya = height * y / targh; ya < height * (y+1) / targh; ya++ )
			for( xa = width  * x / targw; xa < width  * (x+1) / targw; xa++ )
			{
				sum += (rgbbuffer[xa*3+0+ya*linesize] + rgbbuffer[xa*3+1+ya*linesize] + rgbbuffer[xa*3+2+ya*linesize]);
				div += 3;
			}
			*(fd++) = sum / div;
		}
	}
	fwrite( data, targw, targh, f );
	frames++;	
}


int main( int argc, char ** argv )
{
	if( argc != 4 )
	{
		fprintf( stderr, "Usage: [tool] [video] [out x] [out y]\n" );
		return -9;
	}
	targw = atoi( argv[2] );
	targh = atoi( argv[3] );
	data = malloc( targw*targh );
	f = fopen( "videoout.dat", "wb" );
	fprintf( f, "%d %d\n", targw, targh );
	setup_video_decode();
	video_decode( argv[1], targw, targh );
	fclose( f );
	return 0;
}

