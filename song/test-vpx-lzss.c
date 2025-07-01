#include <stdint.h>
#include <stdio.h>

#include "attic/ba_play_audio_vpx.h"

int main()
{
	ba_audio_setup( &ba_player );
	int nindex = 0;
	do
	{
		notetype nt = ba_audio_pull_note( &ba_player );


		int endurement = ((nt) & 0x7);
		if( endurement == 7 ) { endurement = 8; } // XXX Special case scenario at ending.

		int duration = ((nt >> 3) & 0x1f);

		printf( "%d: note:%d edurement:%d duration:%d\n", nindex,  nt>>8, endurement, duration );
		nindex++;
	}
	while( nindex < bas_songlen_notes );
	printf( "Total Notes: %d/%d\n",nindex, bas_songlen_notes ); 
	return 0;
}


