#ifndef _BA_PLAY_AUDIO_H
#define _BA_PLAY_AUDIO_H

#include <stdint.h>
#include <math.h>
#include <string.h>

#ifndef BAS_DECORATOR
#define BAS_DECORATOR
#endif
#include "badapple_song_huffman_reverselzss.h"

#ifndef F_SPS
#define F_SPS (46875)
#endif

#define NUM_VOICES 4
#define TIMESCALE ((F_SPS*60)/(138*4)) // 138 BPM

#define START_AUDIO_AT_FRAME 46

// MUST BE POWER-OF-TWO
#ifndef AUDIO_BUFFER_SIZE
#define AUDIO_BUFFER_SIZE 1024
#endif

#ifndef CHECKPOINT
#define CHECKPOINT(x...)
#endif

#ifndef CHECKBITS_AUDIO
#define CHECKBITS_AUDIO(x...)
#endif

#define PRECISION_SHIFT_ADD 4
#define BASE_PITCH 440

// Note 0 in MIDI is -69 from A 440.  We are offset at 47, because the lowest note in our stream is note 47.
#define LOWEST_NOTE (47.0) // Could tune up or down to taste.

#ifndef WASM
#define SYFN( n ) \
	(uint16_t)((pow( 2, (((float)n + LOWEST_NOTE) - 69.0)/12.0 ) * BASE_PITCH * 65536.0 * (1<<PRECISION_SHIFT_ADD) * 2.0 / 2.0 / (float)F_SPS) + 0.5)
#else
// WASM (Clang)
// Thanks, @kayla
#define STATICPOW2(f) ((float)(1 << (int)(f)) * ( \
    1.+ (0.6931471806 * ((f) - (int)(f)) / 1.) + \
        (0.4804530139 * ((f) - (int)(f)) * ((f) - (int)(f)) / 2.) + \
        (0.3330246520 * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) / 6.) + \
        (0.2308350986 * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) / 24.) + \
        (0.1600026978 * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) * ((f) - (int)(f)) / 120.)))

#define SYFN( n ) \
	(uint16_t)((STATICPOW2( (((float)n + LOWEST_NOTE + 24) - 69.0)/12.0 ) * (1<<PRECISION_SHIFT_ADD) * BASE_PITCH/4.0 * 65536.0 * 2.0 / 2.0 / (float)F_SPS) + 0.5)
#endif

#define NOTE_RANGE  ( ESPBADAPPLE_SONG_HIGHEST_NOTE - ESPBADAPPLE_SONG_LOWEST_NOTE + 1 )

const uint16_t frequencies[NOTE_RANGE] = {
	SYFN( 0), SYFN( 1), SYFN( 2), SYFN( 3), SYFN( 4), SYFN( 5), SYFN( 6), SYFN( 7), SYFN( 8), SYFN( 9),
	SYFN(10), SYFN(11), SYFN(12), SYFN(13), SYFN(14), SYFN(15), SYFN(16), SYFN(17), SYFN(18), SYFN(19),
	SYFN(20), SYFN(21), SYFN(22), SYFN(23), SYFN(24), SYFN(25), SYFN(26), SYFN(27), SYFN(28), SYFN(29),
	SYFN(30), SYFN(31), SYFN(32), SYFN(33) };

typedef uint16_t notetype;

#ifndef WARNING
#define WARNING( x... ) fprintf( stderr, x );
#endif

/*
#define ESPBADAPPLE_SONG_MINRL 2
#define ESPBADAPPLE_SONG_OFFSET_MINIMUM 7
#define ESPBADAPPLE_SONG_MAX_BACK_DEPTH 18
*/

struct ba_audio_player_stack_element
{
	uint16_t offset;
	uint16_t remain;
};

struct ba_audio_player_t
{
	int nexttrel;
	int ending;
	int t;
	int sub_t_sample;
	int outbufferhead;
	int stackplace;
	uint16_t   noiselfsr;
	int16_t    noisetremain;
	int        noisesum;
	uint16_t   playing_freq[NUM_VOICES]; //NUM_VOICES = 4
	uint32_t   phase[NUM_VOICES];  // 32 bit to allow for more precision than 16-bit on the playing freq, otherwise it's too imprecise for good sound.
	int        tstop[NUM_VOICES];

	struct ba_audio_player_stack_element stack[ESPBADAPPLE_SONG_MAX_BACK_DEPTH];
} ba_player;

static int ba_audio_pull_note( struct ba_audio_player_t * player );

#define BITPULL_START \
	int bpo = *optr; \
	int bpoo = bpo & 0x1f; \
	uint32_t bpr = espbadapple_song_data[bpo>>5]>>(bpoo); \
	int bit;

#define BITPULL \
	bit = (bpr&1); bpr>>=1; if( ++bpoo >= 32 ) { bpo += 32; bpoo = 0; bpr = espbadapple_song_data[bpo>>5]; }

#define BITPULL_END \
	*optr = (bpo & ~0x1f) | bpoo;

static int ba_audio_internal_pull_bit( struct ba_audio_player_t * player, uint16_t * optr )
{
	BITPULL_START;
	BITPULL; CHECKBITS_AUDIO( 1 );
	BITPULL_END;
	return bit;
}

int ba_audio_internal_pull_exp_golomb( struct ba_audio_player_t * player, uint16_t * optr )
{
	BITPULL_START;
	int exp = 0;
	CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = 0, audio_last_bitmode = 1, audio_golmb_exp = exp, audio_golmb_v = 0, audio_golmb = 0, audio_golmb_br = -1 );
	do
	{
		BITPULL; CHECKBITS_AUDIO( 1 );
		CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_golmb_exp = exp, audio_golmb_br = -1 );
		if( bit ) break;
		exp++;
	} while( 1 );

	int br;
	int v = 1;
	CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_golmb_exp = exp, audio_golmb_v = v, audio_golmb_br = 0 );
	for( br = 0; br < exp; br++ )
	{
		v = v << 1;
		BITPULL; CHECKBITS_AUDIO( 1 );
		v |= bit;
		CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_golmb_exp = exp, audio_golmb_v = v, audio_golmb_br = br+1 );
	}
	BITPULL_END;
	CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_golmb = v-1 );
	return v-1;
}

int ba_audio_internal_pull_huff( struct ba_audio_player_t * player, const uint16_t * htree, uint16_t * optr )
{
	BITPULL_START;
	int ofs = 0;
	do
	{
		uint16_t he = htree[ofs];
		BITPULL; CHECKBITS_AUDIO( 1 );
		he>>=bit*8;
		//CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_last_bitmode = 2, audio_last_ofs = ofs, audio_last_he = he );
		if( he & 0x80 )
		{
			BITPULL_END;
			CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_last_bitmode = 2, audio_last_he = he, audio_last_ofs = -1, audio_pullhuff = he & 0xff );
			return he & 0x7f;
		}
		he &= 0xff;
		ofs = he + 1 + ofs;
		CHECKPOINT( audio_bpr = (bpo & ~0x1f) | bpoo, audio_pullbit = *optr, audio_gotbit = bit, audio_last_bitmode = 2, audio_last_he = he, audio_last_ofs = ofs );
	} while( 1 );
}

static void ba_audio_setup()
{
	CHECKPOINT( decodephase = "AUDIO: Audio Setup" );
	struct ba_audio_player_t * player = &ba_player;
	memset( player, 0, sizeof( *player ) );
	player->stack[0].remain = ESPBADAPPLE_SONG_LENGTH + 1;
	player->noiselfsr = 1;
	CHECKPOINT( audio_stack_place = 0, audio_stack_remain = player->stack[0].remain, audio_stack_offset = player->stack[0].offset );
}

static int ba_audio_pull_note( struct ba_audio_player_t * player )
{
	int stackplace = player->stackplace;
	struct ba_audio_player_stack_element * stack = player->stack;
	struct ba_audio_player_stack_element * stackpl = stack + stackplace;

	stackpl->remain--;

	CHECKPOINT( audio_stack_place = stackplace, audio_stack_remain = stackpl->remain, decodephase = "AUDIO: Pulling Note" );
	while( stackpl->remain == 0 )
	{
		CHECKPOINT( audio_stack_place = stackplace, audio_stack_remain = stackpl->remain, audio_stack_offset = stackpl->offset, decodephase = "AUDIO: Popping back stack" );
		stackplace--;
		if( stackplace < 0 ) return -5;
		stackpl = stack + stackplace;
	}

	uint16_t * optr;

	do
	{
		optr = &stackpl->offset;
		CHECKPOINT( audio_stack_place = stackplace, audio_stack_remain = stackpl->remain, audio_stack_offset = stackpl->offset, decodephase = "AUDIO: Reading Next" );
		int bpstart = *optr;
		int is_backtrace = ba_audio_internal_pull_bit( player, optr );
		CHECKPOINT( audio_bpr = *optr, audio_pullbit = *optr, audio_gotbit = is_backtrace, audio_last_bitmode = 0, audio_backtrace = is_backtrace, decodephase = is_backtrace ? "AUDIO: Backtrack" : "AUDIO: No Backtrack" );
		if( !is_backtrace ) break;

		CHECKPOINT( decodephase = "AUDIO: Reading Backtrack Run Length", audio_golmb_exp = -1, audio_golmb_v = 0, audio_golmb = 0, audio_golmb_br = -1  );
		int runlen = ba_audio_internal_pull_exp_golomb( player, optr );
		CHECKPOINT( decodephase = "AUDIO: Reading Backtrack Offset", audio_golmb_exp = -1, audio_golmb_v = 0, audio_golmb = 0, audio_golmb_br = -1  );
		int offset = ba_audio_internal_pull_exp_golomb( player, optr );

		int tremain = runlen + ESPBADAPPLE_SONG_MINRL + 1;
		if( tremain > stackpl->remain ) tremain = stackpl->remain;  //TODO: Fold tremain into remain logic below.
		stackpl->remain -= tremain;
		//printf( "RATCHET IN: %d / LEFT: %d\n", tremain, stack[stackplace].remain );
		stackplace++;
		stackpl = stack + stackplace;
		CHECKPOINT( audio_stack_place = stackplace, audio_stack_remain = stackpl->remain, audio_stack_offset = stackpl->offset, audio_backtrack_remain = tremain, audio_backtrack_runlen = runlen, audio_backtrack_offset = offset, decodephase = "AUDIO: Committed Backtrack" );

		*stackpl = (struct ba_audio_player_stack_element)
		{ 
			bpstart - ( offset + ESPBADAPPLE_SONG_OFFSET_MINIMUM + runlen + ESPBADAPPLE_SONG_MINRL + 1 ),
			tremain
		};
		//printf( "RTK: %d %d\n", stack[stackplace].offset, stack[stackplace].remain );
	} while( 1 );

	CHECKPOINT( audio_gotbit = -1, decodephase = "AUDIO: Reading Note", audio_last_ofs = 0, audio_last_he = 0, audio_pullhuff = 0 );
	int note = ba_audio_internal_pull_huff( player, espbadapple_song_huffnote, optr );
	CHECKPOINT( audio_gotbit = -1, audio_newnote = note, decodephase = "AUDIO: Reading Length and Run", audio_last_ofs = 0, audio_last_he = 0, audio_pullhuff = 0 );
	int lenandrun = ba_audio_internal_pull_huff( player, espbadapple_song_hufflen, optr );
	CHECKPOINT( audio_gotbit = -1, audio_lenandrun = lenandrun, decodephase = "AUDIO: Read Len And Run" );

	//printf( "[%02x %02x, %d, %d]\n", note, lenandrun, stack[stackplace].remain, stackplace );
	player->stackplace = stackplace;
	return (note<<8) | lenandrun;
}

static inline void perform_16th_note( struct ba_audio_player_t * player )
{
	int i;
	static int sixteenthnote;
	CHECKPOINT( decodephase = "AUDIO: Perform 16th Note", audio_sixteenth = ++sixteenthnote );
	for( i = 0; i < NUM_VOICES; i++ )
	{
		if( player->playing_freq[i] && player->tstop[i] <= player->t )
		{
			//CHECKPOINT( decodephase = "AUDIO: Note Complete" );
			player->playing_freq[i] = 0;
		}
	}

	while( player->t >= player->nexttrel && !player->ending )
	{
		int note = ba_audio_pull_note( player );

		if( note < 0 )
		{
			CHECKPOINT( decodephase = "AUDIO: Ending" );
			player->ending = 1;
			break;
		}

		int endurement = ((note) & 0x7);
		if( endurement == 7 ) { endurement = 8; } // XXX Special case scenario at ending.

		int duration = ((note >> 3) & 0x1f);

		if( (note>>8) == 33 )
		{
			CHECKPOINT( decodephase = "AUDIO: Processed Noise" );
			player->nexttrel = player->t + endurement;
			player->noisetremain = duration << 10;
		}
		else
		{
			for( i = 0; i < NUM_VOICES; i++ )
			{
				if( player->playing_freq[i] == 0 ) break;
			}

			if( i == NUM_VOICES )
			{
				WARNING( "WARNING: At time %d, trying to add note %d, too many voices\n", player->t, note>>8 );
			}
			else
			{
				player->playing_freq[i] = frequencies[note >> 8];
				player->phase[i] = 0;
				player->tstop[i] = duration + player->t + 1;
				//player->phase[i] = 16384; // TODO: Do we want to randomize this to get some variety?  For now, let's try it in the center?  We could randomize by just not setting it.  TODO Try this.
				//printf( "%d %d %d STOP: %d  ENDURE: %d\n", ((note >> 3) & 0x1f), note & 7, note>>8, player->tstop[i], endurement );
				player->nexttrel = player->t + endurement;
				//printf( "%d\n", player->nexttrel );
				//fprintf( stderr, "NEXT: LEN %d -> %d -> %d (%04x)\n", endurement, t, nexttrel, next_note );
				CHECKPOINT( decodephase = "AUDIO: Processed Note" );
			}
		}
	}
}

int ba_audio_fill_buffer( volatile uint8_t * outbuffer, int outbuffertail ) __attribute__((noinline));
int ba_audio_fill_buffer( volatile uint8_t * outbuffer, int outbuffertail )
{
	int i;
	struct ba_audio_player_t * player = &ba_player;

	int outbufferhead = player->outbufferhead;
	if( outbufferhead == outbuffertail ) return 1;

	while( outbufferhead != outbuffertail )
	{
		// By default, 5095 (at 46875 SPS) or 5217 at 48000 SPS
		if( player->sub_t_sample-- < 0 )
		{
			player->t++;
			perform_16th_note( player );
			player->sub_t_sample += TIMESCALE;
		}
		
		uint32_t sample = 0;
		for( i = 0; i < NUM_VOICES; i++ )
		{
			int pn = player->playing_freq[i];
			if( pn == 0 )
			{
				sample += 16384;
			}
			else
			{
				//int pitch = pn;
				int phase = player->phase[i];
				phase += pn;
				player->phase[i] = phase;
				uint16_t ts = (uint16_t)(phase>>PRECISION_SHIFT_ADD);
				if( ts >= 32768 ) ts = 65536-ts;
				sample += ts;
			}
		}

		int ntr = player->noisetremain;
		if( ntr > 0 )
		{
#if 1
			int l = player->noiselfsr;
			if( ( outbufferhead & 3 ) == 0 ) // Only update LFSR every 4th sample to avoid very high frequency stuff
			{
				int bit = ((l >> 0) ^ (l >> 2) ^ (l >> 3) ^ (l >> 5)) & 1u;
				l = player->noiselfsr = (l>>1) | (bit<<15);
				player->noisetremain = ntr - 32;
				if( ntr > 2048 ) ntr = 2048;
				player->noisesum = (l * ntr)>>14;
			}
			sample += player->noisesum;
#endif
		}
		sample >>= (1+8); // Reduce to 8-bit range.

		// Run through song once to make sure we don't overflow (somehow we don't, even with 4 notes and noise) (??)
		//if(((uint32_t)sample)>511) while(1);

		outbuffer[outbufferhead] = (volatile uint32_t)((sample));
		//asm volatile( "nop" : : [dirty]"r"(sample) : "memory" );
		outbufferhead = ( outbufferhead + 1 ) & ( AUDIO_BUFFER_SIZE - 1);
	}
	//PrintHex( outbuffer[outbufferhead] );
	player->outbufferhead = outbufferhead;
	return 0;
}

#endif

