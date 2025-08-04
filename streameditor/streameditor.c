#include <stdio.h>
#include <stdarg.h>

#define CNFG_IMPLEMENTATION
#define CNFGOGL
#define OVERRIDE_RDCALLBACKS

#include "rawdraw_sf.h"
#include "bacommon.h"
#include "os_generic.h"

#define TILEX (RESX/BLOCKSIZE)
#define TILEY (RESY/BLOCKSIZE)

int startTimeSeconds;
unsigned frameNo;

uint32_t streamData[FRAMECT][TILEY][TILEX];
float  * tileData;
int numTiles;

short screenw, screenh;

int32_t currentFrame;
int32_t currentTile;

bool bAlreadySaved;
bool bDataTainted;

char alertText[16384];
double alertTime;

bool bClickEvent = false;
int lastMouseX;
int lastMouseY;

int selTileX;
int selTileY;

int brush = 0;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// For validating logic.
//
// The authortative portion of this code is in playback.c
//
typedef uint16_t graphictype;

int EmitPartial( graphictype tgprev, graphictype tg, graphictype tgnext, int subframe )
{
	// This should only need +2 regs (or 3 depending on how the optimizer slices it)
	// (so all should fit in working reg space)
	graphictype A = tgprev >> 8;
	graphictype B = tgprev;      // implied & 0xff
	graphictype C = tgnext >> 8;
	graphictype D = tgnext;      // implied & 0xff
	graphictype E = tg >> 8;
	graphictype F = tg;          // implied & 0xff

	if( subframe )
		tg = (D&E) | (B&E) | (B&C&F) | (A&D&F);     // 8 bits worth of MSB of this+(next+prev+1)/2-1 (Assuming values of 0,1,3)
	else
		tg = E | C | A | (D&F) | (B&F) | (B&D);       // 8 bits worth of MSB|LSB of this+(next+prev+1)/2-1

	//KOut( tg );
	return tg;
}


// one pixel at a time.
int PixelBlend( int tgprev, int tg, int tgnext )
{

	static uint8_t * halut;

	if( !halut )
	{
		halut = malloc( 3*3*3 );

		// x = center, y = left, z = right
		#define SETLUT( x, y, z ) halut[x+y*3+z*3*3]
		SETLUT(0, 0, 0) = 0.0;
		SETLUT(1, 0, 0) = 0.0;
		SETLUT(2, 0, 0) = 1.0;
		SETLUT(0, 0, 1) = 0.0;
		SETLUT(1, 0, 1) = 1.0;
		SETLUT(2, 0, 1) = 2.0;
		SETLUT(0, 0, 2) = 1.0;
		SETLUT(1, 0, 2) = 1.0;
		SETLUT(2, 0, 2) = 2.0;
		SETLUT(0, 1, 0) = 0.0;
		SETLUT(1, 1, 0) = 1.0;
		SETLUT(2, 1, 0) = 2.0;
		SETLUT(0, 1, 1) = 1.0;
		SETLUT(1, 1, 1) = 1.0;
		SETLUT(2, 1, 1) = 2.0;
		SETLUT(0, 1, 2) = 1.0;
		SETLUT(1, 1, 2) = 2.0;
		SETLUT(2, 1, 2) = 2.0;
		SETLUT(0, 2, 0) = 1.0;
		SETLUT(1, 2, 0) = 1.0;
		SETLUT(2, 2, 0) = 2.0;
		SETLUT(0, 2, 1) = 1.0;
		SETLUT(1, 2, 1) = 2.0;
		SETLUT(2, 2, 1) = 2.0;
		SETLUT(0, 2, 2) = 1.0;
		SETLUT(1, 2, 2) = 2.0;
		SETLUT(2, 2, 2) = 2.0;

		// test
		int testtg, testtgprev, testtgnext;
		for( testtg = 0; testtg < 3; testtg++ )
		for( testtgprev = 0; testtgprev < 3; testtgprev++ )
		for( testtgnext = 0; testtgnext < 3; testtgnext++ )
		{
			int evan = halut[testtg + testtgprev*3+testtgnext*9];


			int tg = testtg;
			int tgprev = testtgprev;
			int tgnext = testtgnext;
			if( tg == 2 ) tg = 3;
			if( tgprev == 2 ) tgprev = 3;
			if( tgnext == 2 ) tgnext = 3;
			// this+(next+prev+1)/2-1 assuming 0..3, skip 2.
			//printf( "%d\n", tg );

			tg = PixelBlendPlayback( tgprev, tg, tgnext );

			if( tg == 2 ) printf( "Illegal TG Output %d %d %d\n", tg, tgprev, tgnext );
			if( tg == 3 ) tg = 2;
/*			tg = (tg + (tgprev+tgnext+1)/2 - 1);
printf( "%d\n", (tgprev+tgnext+1)/2 );
			if( tg < 0 ) tg = 0;
			if( tg > 2 ) tg = 2;*/
			if( tg != evan )
			{
				printf( "Horizontal Disagree: t%d, p%d, n%d  mine:%d evan:%d\n", testtg, testtgprev, testtgnext, tg, evan );
			}

			tg = testtg;
			tgprev = testtgprev;
			tgnext = testtgnext;

			if( tgprev == 2 ) tgprev = 3;
			if( tg == 2 ) tg = 3;
			if( tgnext == 2 ) tgnext = 3;

			int tgprevbits = (tgprev & 1) | ((tgprev & 2)<<7);
			int tgnextbits = (tgnext & 1) | ((tgnext & 2)<<7);
			int tgbits = (tg & 1) | ((tg & 2)<<7);
			uint16_t v0 = EmitPartial( tgprevbits, tgbits, tgnextbits, 0 ) & 0xff;
			uint16_t v1 = EmitPartial( tgprevbits, tgbits, tgnextbits, 1 ) & 0xff;
			if( v1 && !v0 ) printf( "Illegal at %d %d %d [%d %d]\n", testtg, testtgprev, testtgnext, v0, v1 );
			int v = (v0 && v1) ? 2 : (v1||v0);
			//printf( "%d %d %d   %d %d -> %d\n", testtg, testtgprev, testtgnext, v0, v1, evan );
			if( v != evan )
				printf( "Vertical Disagree:   t%d, p%d, n%d  mine:%d [%d %d] evan:%d\n", testtg, testtgprev, testtgnext, v, v0, v1, evan );
		}
	}

	if( tg > 2 ) tg = 2;
	if( tgprev > 2 ) tgprev = 2;
	if( tgnext > 2 ) tgnext = 2;
	return halut[tg+tgprev*3+tgnext*9];
}

int GetTileByInternal( int x, int y )
{
	if( x < 0 ) x = 0;
	if( x >= RESX ) x = RESX - 1;
	if( y < 0 ) y = 0;
	if( y >= RESY ) y = RESY - 1 ;

	int tx = x / BLOCKSIZE;
	int ty = y / BLOCKSIZE;
	int tileId = streamData[currentFrame][ty][tx];
	int bx = x % BLOCKSIZE;
	int by = y % BLOCKSIZE;

	float f = ((float*)tileData)[tileId*BLOCKSIZE*BLOCKSIZE + by * BLOCKSIZE + bx ];

	int c = (int)(f * 2.999);
	if( c == 2 ) c = 3;
	return c;
}

int FtoC( float f )
{
	int c = (int)(f * 2.999);
	if( c == 2 ) c = 3;
	return c;
}

float GetTileByVertical( int x, int y )
{
	int bx = x % BLOCKSIZE;
	int by = y % BLOCKSIZE;

	int tg = GetTileByInternal( x, y );
	if( bx == 0 || bx == BLOCKSIZE-1 )
	{
		int tgprev = GetTileByInternal( (x>0)?(x-1):(x+1), y );
		int tgnext = GetTileByInternal( (x<RESX-1)?(x+1):(x-1), y );
		//printf( "%d %d %d -> %d\n", tgprev, tg, tgnext, PixelBlend( tgprev, tg, tgnext ) );
		tg = PixelBlend( tgprev, tg, tgnext );
	}

	return tg;
}

float GetTileBy( int x, int y )
{
	int bx = x % BLOCKSIZE;
	int by = y % BLOCKSIZE;

	int tg = GetTileByVertical( x, y );

	if( by == 0 || by == BLOCKSIZE-1 )
	{
		int tgprev = GetTileByVertical( x, (y>0)?(y-1):(y+1) );
		int tgnext = GetTileByVertical( x, (y<RESY-1)?(y+1):(y-1) );
		tg = PixelBlend( tgprev, tg, tgnext );
	}

	return tg;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int DrawInBoxAndTest( int c, int x1, int y1, int x2, int y2 )
{
	if( c < 1 ) CNFGColor( 0x000000ff );
	else if( c < 2 ) CNFGColor( 0x808080ff );
	else CNFGColor( 0xffffffff );
	CNFGTackRectangle( x1, y1, x2, y2 );	
	return lastMouseX >= x1 && lastMouseX <= x2 && lastMouseY >= y1 && lastMouseY <= y2;
}

void Alert( const char * fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(alertText, sizeof(alertText)-1, fmt, args);
	va_end(args);
	alertText[sizeof(alertText)-1] = 0;
	alertTime = OGGetAbsoluteTime();
}

int SaveData();
void AssignTile();

void HandleKey( int keycode, int bDown )
{
	if( !bDown ) return;
	switch( keycode )
	{
	case 65360: currentFrame = 0; break; // Home
	case 65367: currentFrame = FRAMECT-1; break; // End
	case 65366: // Page dn
		currentFrame += 100;
		if( currentFrame >= FRAMECT ) currentFrame = FRAMECT-1;
		break;
	case 65361: // Left
		if( currentFrame > 0 ) currentFrame--;
		break;
	case 65365: // Page up
		currentFrame -= 100;
		if( currentFrame < 0 ) currentFrame = 0;
		break;
	case 65363: // Right
		if( currentFrame < FRAMECT - 1 ) currentFrame++;
		break;
	case 65362: // Up
		if( currentTile < numTiles - 1 ) currentTile++;
		break;
	case 65364: // Down
		if( currentTile > 0 ) currentTile--;
		break;
	case 49: brush = 0; break;
	case 50: brush = 1; break;
	case 51: brush = 2; break;
	case 97: AssignTile(); break;
	case 115:
		SaveData();
		break;
	default:	
		printf( "Keycode %d unmapped\n", keycode );
		break;
	}
}

void AssignTile()
{
	streamData[currentFrame][selTileY][selTileX] = currentTile;
	bDataTainted = true;
}

void HandleButton( int x, int y, int button, int bDown ) {lastMouseX = x; lastMouseY = y; if( bDown ) bClickEvent = true; }
void HandleMotion( int x, int y, int mask ) { lastMouseX = x; lastMouseY = y; }

int HandleDestroy() { return 0; }

int SaveData()
{
	if( !bDataTainted ) { Alert( "No file save needed." );  return -1; }

	char fname[1024];
	sprintf( fname, "../comp2/stream-%dx%dx8.dat", RESX, RESY );
	FILE * f = fopen( fname, "wb" );
	if( !f || fwrite( streamData, sizeof(streamData), 1, f ) != 1 )
	{
		fprintf( stderr, "Error: can't open backup file\n" );
		return -7;
	}
	fclose( f );

	sprintf( fname, "../streamrecomp/stream_stripped.dat" );
	f = fopen( fname, "wb" );
	if( !f || fwrite( streamData, sizeof(streamData), 1, f ) != 1 )
	{
		fprintf( stderr, "Error: can't open backup file\n" );
		return -7;
	}
	fclose( f );

	sprintf( fname, "../comp2/tiles-%dx%dx8.dat", RESX, RESY );
	f = fopen( fname, "wb" );
	if( !f || fwrite( tileData, numTiles*BLOCKSIZE*BLOCKSIZE*4, 1, f ) != 1 )
	{
		fprintf( stderr, "Error: can't open backup file\n" );
		return -7;
	}
	fclose( f );

	Alert( "File saved." );

	bDataTainted = false;
	return 0;
}

void RenderFrame()
{
	int rw = screenw/3;
	int bw = rw / RESX;
	int xofs = 0;
	int yofs = 20;
	int x, y;

	CNFGColor( (frameNo&1)?0xffffffff:0 );
	CNFGTackRectangle( (selTileX)*bw*BLOCKSIZE+xofs-1, (selTileY)*bw*BLOCKSIZE+yofs-1, (selTileX+1)*bw*BLOCKSIZE+xofs, (selTileY+1)*bw*BLOCKSIZE+yofs );	

	for( y = 0; y < RESY; y++ )
	{
		for( x = 0; x < RESX; x++ )
		{
			int tx = x / BLOCKSIZE;
			int ty = y / BLOCKSIZE;
			int tileId = streamData[currentFrame][ty][tx];
			int bx = x % BLOCKSIZE;
			int by = y % BLOCKSIZE;

			int c = GetTileBy( x, y );

			if( DrawInBoxAndTest( c, x * bw+xofs, y * bw+yofs, (x+1)*bw-1+xofs, (y+1)*bw-1+yofs ) && bClickEvent )
			{
				selTileX = tx;
				selTileY = ty;
				currentTile = tileId;
			}
		}
	}
}

void RenderTileSet()
{
	int rw = screenw/3;
	int bw = rw / (16*BLOCKSIZE+16);
	int xofs = rw*1;
	int yofs = 20;
	int x, y;


	CNFGColor( (frameNo&1)?0xffffffff:0 );
	int stx = currentTile % 16;
	int sty = currentTile / 16;
	//CNFGTackRectangle( ((stx)*BLOCKSIZE+3)*bw+xofs-1, ((sty)*BLOCKSIZE+3)*bw+yofs-1, ((stx+1)*BLOCKSIZE+3)*bw+xofs, ((sty+1)*BLOCKSIZE+3)*bw+yofs );
	int xox = stx*3;
	int xoy = sty*3;
	CNFGTackRectangle( ((stx)*bw)*BLOCKSIZE+xofs-2+xox, ((sty)*bw)*BLOCKSIZE+yofs-2+xoy, ((stx+1)*bw)*BLOCKSIZE+xofs+xox+2, ((sty+1)*bw)*BLOCKSIZE+yofs+xoy+2 );


	for( y = 0; y < 16*BLOCKSIZE; y++ )
	{
		for( x = 0; x < 16*BLOCKSIZE; x++ )
		{
			int tx = x / BLOCKSIZE;
			int ty = y / BLOCKSIZE;
			int tileId = tx+ty*16;

			int bx = x % BLOCKSIZE;
			int by = y % BLOCKSIZE;

			float tval = ((float*)tileData)[tileId*BLOCKSIZE*BLOCKSIZE + by * BLOCKSIZE + bx ];

			int xofsi = xofs + tx*3;
			int yofsi = yofs + ty*3;

			int c = FtoC( tval );

			if( DrawInBoxAndTest( c, x * bw+xofsi, y * bw+yofsi, (x+1)*bw+xofsi, (y+1)*bw+yofsi ) && bClickEvent )
			{
				currentTile = tx + ty * 16;
			}
		}
	}
}

void RenderTile()
{
	int rw = screenw/3;

	int bw = rw / BLOCKSIZE;

	int xofs = rw*2;
	int yofs = 20;

	int x, y;
	for( y = 0; y < BLOCKSIZE; y++ )
	{
		for( x = 0; x < BLOCKSIZE; x++ )
		{
			float * fp = &((float*)tileData)[currentTile*BLOCKSIZE*BLOCKSIZE + y * BLOCKSIZE + x ];
			float tval = *fp;

			int c = FtoC( tval );

			if( DrawInBoxAndTest( c, x * bw+xofs, y * bw+yofs, (x+1)*bw-1+xofs, (y+1)*bw-1+yofs ) && bClickEvent )
			{
				// Edit tile.
				*fp = brush / 2.0;
				printf( "Tile: %f\n", *fp );
				bDataTainted = true;
			}
		}
	}
}

int InitialLoadFile( const char * sType, uint8_t ** ppData, int expectedSize )
{
	char fname[1024];
	sprintf( fname, "../comp2/%s-%dx%dx8.dat", sType, RESX, RESY );
	FILE * f = fopen( fname, "rb" );
	if( !f || ferror( f ) )
	{
		fprintf( stderr, "Error: can't open stream file %s\n", fname );
		return -5;
	}
	fseek( f, 0, SEEK_END );
	int slen = ftell( f );
	fseek( f, 0, SEEK_SET );

	if( expectedSize == 0 )
	{
		expectedSize = slen;
	}
	else if( slen != expectedSize )
	{
		fprintf( stderr, "Error: stream data sizes don't match.\n" );
		return -5;
	}

	if( !*ppData ) *ppData = malloc( expectedSize );

	if( fread( *ppData, expectedSize, 1, f ) != 1 )
	{
		fprintf( stderr, "Error: can't read out stream data %s\n", fname );
		return -6;
	}

	fclose( f );
	mkdir( "backups", 0777 );
	sprintf( fname, "backups/%s-%dx%dx8-backup%d.dat", sType, RESX, RESY, startTimeSeconds );
	f = fopen( fname, "wb" );
	if( !f || fwrite( *ppData, expectedSize, 1, f ) != 1 )
	{
		fprintf( stderr, "Error: can't open backup file\n" );
		return -7;
	}
	fclose( f );
	return expectedSize;
}

void DrawTextAt( int x, int y, int s, char * fmt, ... )
{
	CNFGPenX = x; CNFGPenY = y;
	char buff[16384];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, sizeof(buff)-1, fmt, args);
	va_end(args);
	buff[sizeof(buff)-1] = 0;
	CNFGDrawText( buff, s );
}

int main()
{
	startTimeSeconds = (int)OGGetAbsoluteTime();
	int r;
	uint32_t * sdptr = (uint32_t*)streamData;
	r = InitialLoadFile( "stream", (uint8_t**)&sdptr, sizeof( streamData ) );
	if( r < 0 ) return r;
	r = InitialLoadFile( "tiles", (uint8_t**)&tileData, 0 );
	if( r < 0 ) return r;

	numTiles = r / BLOCKSIZE / BLOCKSIZE / 4;

	CNFGSetup( "Badder Apple Editor", 1920, 768 );

	Alert( "Loaded." );

	while(CNFGHandleInput())
	{
		double Now = OGGetAbsoluteTime();
		CNFGSetLineWidth( 2 );

		CNFGBGColor = 0x202020ff;

		CNFGClearFrame();
		CNFGGetDimensions( &screenw, &screenh );

		//Change color to white.
		CNFGColor( 0xffffffff );

		DrawTextAt( 1, 1, 3, "Frame %d/%d", currentFrame, FRAMECT );
		DrawTextAt( (screenw/2), 1, 3, "Tile %3d (%02x)/%d Brush: %d", currentTile, currentTile, numTiles, brush );

		double alertDeltaTime = Now - alertTime;
		if( alertDeltaTime < 10 )
		{
			double deltaUp = alertDeltaTime - 4;
			if( deltaUp < 0 ) deltaUp = 0;

			int tw, th;
			CNFGGetTextExtents( alertText, &tw, &th, 3 );
			CNFGPenX = screenw - tw - 3;
			CNFGPenY = 3 - deltaUp*10.0;
			CNFGDrawText( alertText, 3 );
		}

		RenderFrame();
		RenderTileSet();
		RenderTile();


		const char * help =  "Home: go to beginning, End: Go to end, PgDn = advance 100 frames, PgUp = advance 100 frames\nleft = go back one frame, right = go forward one frame, up = advance tile, down = retract tile\n0 = brush black, 1 = brush grey, 2 = brush white\na = assign tile, s = save tile+glyphs";

		{
			int tw, th;
			CNFGGetTextExtents( help, &tw, &th, 3 );
			CNFGPenX = 1;
			CNFGPenY = screenh - th - 3;
			CNFGColor( 0xffffffff );
			CNFGDrawText( help, 3 );
		}

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();
		bClickEvent = false;
		frameNo++;
	}
	return 0;
}

