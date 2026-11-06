
#include "bacommon.h"

uint8_t rawframedata[FRAMECT*RESY*RESX];


#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int w;
int h;
int ch;
uint8_t * data;
uint8_t * datao;

int main(int argc, char**argv)
{
	FILE * frd = fopen( "rawframeoutdata.dat", "rb" );
	if( fread( rawframedata, sizeof(rawframedata), 1, frd ) != 1 )
	{
		fprintf( stderr, "Error: need rawframeoutdata.dat, expected %d bytes\n", (int)sizeof(rawframedata) );
		return -6;
	}
	fclose( frd );


	if( argc < 2 )
	{
		fprintf( stderr, "Usage: [tool] source image\n" );
		return 0;
	}

	data = stbi_load( argv[1], &w, &h, &ch, 4 );
	datao = malloc( w * h * 4 );
	memcpy( datao, data, w*h*4 );

	int ofsx = 0;
	int ofsy = 220;
	int tw = 960;
	int th = 336;

	int patx = tw / RESX;
	int paty = th / RESY;
//printf( "PATTY: %d %d\n", patx, paty );
	int x, y;
	for( y = 0; y < paty; y++ )
	for( x = 0; x < patx; x++ )
	{
		int bestscore = 1000000000;
		int f;
		int invert = 0;
		for( invert = 0; invert < 2; invert++ )
		for( f = 0; f < FRAMECT; f++ )
		{
			int ix, iy;
			int score = 0;
			for( iy = 0; iy < RESY; iy++ )
			for( ix = 0; ix < RESX; ix++ )
			{
				int pv = rawframedata[f*RESX*RESY + iy * RESX + ix];
				// PV = the bad apple portion.  0 for black, 2 for grey, 3 for white.

				if( invert ) { if( pv == 0 ) pv = 3; else if( pv > 1 ) pv = 0; }

				int tv = data[(ofsx+ix+x*RESX+(ofsy+iy+y*RESY)*w)*4];
				if( tv > 180 ) tv = 3;
				else if( tv > 90 ) tv = 2;
				else tv = 0;

				int diff = tv - pv;
				if( diff < 0 ) diff = -diff;

				if( pv == 1 ) diff = 0; // Grey is ambiguous.
				score += diff;
			}

			score += rand()%500;

			if( score < bestscore )
			{
				bestscore = score;
				int v = 0;

				for( iy = 0; iy < RESY; iy++ )
				for( ix = 0; ix < RESX; ix++ )
				{

					int pv = rawframedata[f*RESX*RESY+iy*RESX+ix];
					int tv = data[(ix+ofsx+x*RESX+(ofsy+iy+y*RESY)*w)*4];

					if( invert ) { if( pv == 0 ) pv = 3; else if( pv > 1 ) pv = 0; }

					if( pv == 0 ) v = 0;
					else if( pv > 1 ) v = 255;
					else
					{
						if( tv > 128 ) v = 255;
						else v = 0;
					}

					datao[(ix+ofsx+x*RESX+(ofsy+iy+y*RESY)*w)*4+0] = v;
					datao[(ix+ofsx+x*RESX+(ofsy+iy+y*RESY)*w)*4+1] = v;
					datao[(ix+ofsx+x*RESX+(ofsy+iy+y*RESY)*w)*4+2] = v;
					datao[(ix+ofsx+x*RESX+(ofsy+iy+y*RESY)*w)*4+3] = 255;
				}
			}
		}
	}

    stbi_write_png("helloiam.png", w, h, 4, datao, w*4);

	return 0;
}


