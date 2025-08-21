// This wants to be a forward LZSS VPX compression mode, but IT DOES NOT WORK, RECONSIDER SOMEDAY.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>

const int MinRL = 1;
#define OFFSET_MINIMUM 2
#define COMPCOSTTRADEOFF 10

#define MAX_LSBITS 10

#define VPXCODING_READER
#define VPXCODING_WRITER
#include "vpxcoding.h"

#include "probabilitytree.h"

// TODO: Can we also store substrings here?

struct DataTree
{
	int numUnique;
	int numList;
	uint32_t * uniqueMap;
	float * uniqueCount; // Float to play nice with the vpx tree code.
	uint32_t * fullList;
};


vpx_writer writer = { 0 };


int GetIndexFromValue( struct DataTree * dt, uint32_t value )
{
	int i;
	for( i = 0; i < dt->numUnique; i++ )
	{
		if( dt->uniqueMap[i] == value )
		{
			return i;
		}
	}
	fprintf( stderr, "Error: Requesting unknown value (%d)\n", value );
	return -1;
}

void AddValue( struct DataTree * dt, uint32_t value )
{
	int nu = dt->numList;
	dt->fullList = realloc( dt->fullList, (nu+1) * sizeof(dt->fullList[0]) );
	dt->fullList[nu] = value;
	dt->numList = nu + 1;
	int n;
	for( n = 0; n < dt->numUnique; n++ )
	{
		if( dt->uniqueMap[n] == value )
		{
			dt->uniqueCount[n]++;
			return;
		}
	}

	dt->uniqueMap = realloc( dt->uniqueMap, (n+1)*sizeof(dt->uniqueMap[0]) );
	dt->uniqueMap[n] = value;
	dt->uniqueCount = realloc( dt->uniqueCount, (n+1)*sizeof(dt->uniqueCount[0]) );
	dt->uniqueCount[n] = 1;
	dt->numUnique = n+1;
}

static inline int BitsForNumber( unsigned number )
{
	if( number == 0 ) return 0;
	number++;
#if (defined( __GNUC__ ) || defined( __clang__ ))
	return 32 - __builtin_clz( number - 1 );
#else
	int n = 32;
	unsigned y;
	unsigned x = number - 1;
	y = x >>16; if (y != 0) { n = n -16; x = y; }
	y = x >> 8; if (y != 0) { n = n - 8; x = y; }
	y = x >> 4; if (y != 0) { n = n - 4; x = y; }
	y = x >> 2; if (y != 0) { n = n - 2; x = y; }
	y = x >> 1; if (y != 0) return 32 - (n - 2);
	return 32 - (n - x);
#endif
}

int ExpGolombCost( int ib )
{
	ib++;
	int bitsemit = 0;
	int bits = (ib == 0) ? 1 : BitsForNumber( ib );
	int i;
	for( i = 1; i < bits; i++ )
	{
		bitsemit++;
	}

	if( bits )
	{
		for( i = 0; i < bits; i++ )
		{
			bitsemit++;
		}
	}

	return bitsemit;
}


void EmitBit( int b )
{
	vpx_write( &writer, b, 128);
}

int EmitExpGolomb( int ib )
{
	ib++;
	int bitsemit = 0;
	int bits = (ib == 0) ? 1 : BitsForNumber( ib );
	int i;
	for( i = 1; i < bits; i++ )
	{
		EmitBit( 0 );
		bitsemit++;
	}

	if( bits )
	{
		for( i = 0; i < bits; i++ )
		{
			EmitBit( ((ib)>>(bits-i-1)) & 1 );
			bitsemit++;
		}
	}

	return bitsemit;
}


// Like golomb encoding except purely vpx.
void WriteLogshift( vpx_writer * w, int v, int * bctable, int * bctableout )
{
	if( bctableout ) bctableout[0]++;

	int i;
	for( i = 0; i < MAX_LSBITS; i++ )
	{
		int b = v&1;
		if (bctable)
		{
			int prob = 
				255 - (bctable[i+1] * 255) / bctable[0];
				//128 + (i * 128 / MAX_LSBITS);
			vpx_write( &writer, b, prob );
		}
		if( bctableout )
		{
			bctableout[i+1] += b;
		}
		v>>=1;
	}
}


int RLMakesSense( int rl, int ras, int i )
{
	int offset = i-ras-rl;

	if( (offset - OFFSET_MINIMUM) >= 1<<MAX_LSBITS )
	{
		// Can't fit.
		return 0;
	}

	if( rl > ExpGolombCost(offset)/COMPCOSTTRADEOFF )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


int main()
{
	int i, j, k, l;
	FILE * f = fopen( "fmraw.dat", "rb" );

	struct DataTree dtNotes = { 0 };
	struct DataTree dtLenAndRun = { 0 };

	struct DataTree dtNotesComp = { 0 };
	struct DataTree dtLenAndRunComp = { 0 };

	int bctableoffset[MAX_LSBITS+1] = { 0 };
	int bctablerunlen[MAX_LSBITS+1] = { 0 };

	int numNotes = 0;
	uint32_t * completeNoteList = 0;

	while( !feof( f ) )
	{
		uint16_t s;
		int r = fread( &s, 1, 2, f );
		if( r == 0 ) break;
		if( r != 2 )
		{
			fprintf( stderr, "Error: can't read symbol.\n" );
			return -6;
		}

		//usedmask |= s;
		int note = (s>>8)&0xff; // ( (next_note >> 8 ) & 0xff) + 47;
		int len = (s>>3)&0x1f;       // ( (next_note >> 3 ) & 0x1f) + t + 1;
		int run = (s)&0x07;

		// Combine note and run.
		len |= run<<8;

		AddValue( &dtNotes, note );
		AddValue( &dtLenAndRun, len );

		completeNoteList = realloc( completeNoteList, sizeof(completeNoteList[0]) * (numNotes+1) );
		completeNoteList[numNotes] = s;

		numNotes++;
	}

	for( i = 0; i < dtNotesComp.numUnique; i++ )
	{
		AddValue( &dtNotesComp, i );
	}

	// STAGE 1: DRY RUN.
	int numComp = 0;
	int numRev = 0;

	int highestNoteCnt = 0;

	for( i = 0; i < numNotes; i++ )
	{
		// Search for repeated sections.
		int searchStart = 0; //i - MaxREV - MaxRL - MinRL;
		if( searchStart < 0 ) searchStart = 0;
		int s;
		int bestcompcost = 0, bestrl = 0, bestrunstart = 0;
		for( s = searchStart; s < i - OFFSET_MINIMUM; s++ )
		{
			int ml;
			int mlc;
			int rl;
			int compcost;

			for( 
				ml = s, mlc = i, rl = 0;
				ml < numNotes && mlc < numNotes; //&& rl < MaxRL;
				ml++, mlc++, rl++ )
			{
				if( completeNoteList[ml] != completeNoteList[mlc] ) break;
			}

#if 0
			compcost = ExpGolombCost( rl ) + ExpGolombCost( i-s );

			if( rl*COMPCOSTTRADEOFF - compcost > bestrl*COMPCOSTTRADEOFF - bestcompcost )
			{
				bestrl = rl;
				bestcompcost = compcost;
				bestrunstart = s;
			}
#endif
			if( rl >= bestrl && RLMakesSense( rl, s, i ) )// && // Make sure it's best
				//s + MinRL + rl + MaxREV >= i )
			{
				bestrl = rl;
				bestrunstart = s;
			}

		}
		if( bestrl > MinRL )

		{
			//printf( "Found Readback at %d (%d %d) (D: %d)\n", i, i-bestrunstart, bestrl, i-bestrunstart-bestrl );
			i += bestrl-1;

			//printf( "AT: %d BEST: LENG:%d  START:%d\n", i, bestrl, bestrunstart );
			int offset = i-bestrunstart-bestrl;
			//printf( "Emitting: %d %d (%d %d %d %d)\n", bestrl, offset, i, bestrunstart, bestrl, MinRL );

			int emit_best_rl = bestrl - MinRL - 1;

			// Output emit_best_rl, RLBits, Prob1RL
			// Output offset, MRBits, Prob1MR

			// Output emit_best_rl
			// Output offset

			// Using 11 bits for both offset and emit_best_rl
			if( offset >= (1<<MAX_LSBITS) ) { fprintf( stderr, "offset TOO BIG (%d)\n", offset ); }
			if( emit_best_rl >= (1<<MAX_LSBITS) ) { fprintf( stderr, "emit_best_rl TOO BIG (%d)\n", emit_best_rl ); }

			offset -= OFFSET_MINIMUM;

			WriteLogshift( 0, offset, 0, bctableoffset );
			WriteLogshift( 0, emit_best_rl, 0, bctablerunlen );

			numRev++;
		}
		else
		{
			// No readback found, we will have toa ctaully emit encode this note.
			//AddValue( &dtNotes, note );
			//AddValue( &dtLenAndRun, len );

			//regNoteList = realloc( regNoteList, sizeof(regNoteList[0]) * (numComp+1) );
			//regNoteList[numComp] = completeNoteList[i];

			AddValue( &dtNotesComp, dtNotes.fullList[i] );
			AddValue( &dtLenAndRunComp, dtLenAndRun.fullList[i] );

			numComp++;
		}
	}

	printf( "Comp/Rev: %d/%d\n", numComp, numRev );
	int chanceRL = numRev * 255 / (numComp+numRev);

	int nHighestNote = 0;
	int nLowestNote = INT_MAX;
	// this is only a rough approximation of the distribution that will be used.
	for( int r = 0; r < numComp; r++ )
	{
		int nv = dtNotesComp.fullList[r];
		int pitch = nv>>8;
		if( pitch > nHighestNote ) nHighestNote = pitch;
		if( pitch < nLowestNote ) nLowestNote = pitch;

		int note = nv>>8;
		int len = nv&0xff;

		if( note >= highestNoteCnt ) highestNoteCnt = note+1;
		//numsym = HuffmanAppendHelper( &symbols, &symcounts, numsym, note );
		//numlens = HuffmanAppendHelper( &lenss, &lencountss, numlens, len );
	}

	// We just need these for later.
	int bitsForNotes = ProbabilityTreeBitsForMaxElement( dtNotesComp.numUnique );
	int bitsForLenAndRun = ProbabilityTreeBitsForMaxElement( dtLenAndRunComp.numUnique );

	int treeSizeNotes = ProbabilityTreeGetSize( dtNotesComp.numUnique, bitsForNotes );
	int treeSizeLenAndRun = ProbabilityTreeGetSize( dtLenAndRunComp.numUnique, bitsForLenAndRun );

	uint8_t probabilitiesNotes[treeSizeNotes];
	uint8_t probabilitiesLenAndRun[treeSizeLenAndRun];

	ProbabilityTreeGenerateProbabilities( probabilitiesNotes, treeSizeNotes, dtNotesComp.uniqueCount, dtNotesComp.numUnique, bitsForNotes );
	ProbabilityTreeGenerateProbabilities( probabilitiesLenAndRun, treeSizeLenAndRun, dtLenAndRunComp.uniqueCount, dtLenAndRunComp.numUnique, bitsForLenAndRun );

	uint8_t vpxbuffer[1024*16];
	vpx_start_encode( &writer, vpxbuffer, sizeof(vpxbuffer) );

	printf( "Bits: %d/%d\n", bitsForNotes, bitsForLenAndRun );

	for( i = 0; i < dtNotesComp.numUnique; i++ )
	{
		printf( "%2d %02x %f\n", i, dtNotesComp.uniqueMap[i], dtNotesComp.uniqueCount[i] );
	}

	for( i = 0; i < treeSizeNotes; i++ )
	{
		printf( "%d ", probabilitiesNotes[i] );
	}
	printf( "\n" );

	for( i = 0; i < dtLenAndRunComp.numUnique; i++ )
	{
		printf( "%2d %04x %f\n", i, dtLenAndRunComp.uniqueMap[i], dtLenAndRunComp.uniqueCount[i] );
	}

	for( i = 0; i < treeSizeLenAndRun; i++ )
	{
		printf( "%d ", probabilitiesLenAndRun[i] );
	}
	printf( "\n" );

	printf( "Theoretical:\n" );

	float fBitsNotes = 0;
	float fBitsLens = 0;

	for( i = 0; i < dtNotesComp.numUnique; i++ )
	{
		float fPortion = (dtNotesComp.uniqueCount[i] / numComp);
		if( dtNotesComp.uniqueCount[i] == 0 ) continue;
		float fBitContrib = -log(fPortion)/log(2.0);
		fBitsNotes += fBitContrib * dtNotesComp.uniqueCount[i];
	}

	for( i = 0; i < dtLenAndRunComp.numUnique; i++ )
	{
		float fPortion = (dtLenAndRunComp.uniqueCount[i] / numComp);
		if( dtLenAndRunComp.uniqueCount[i] == 0 ) continue;
		float fBitContrib = -log(fPortion)/log(2.0);
		fBitsLens += fBitContrib * dtLenAndRunComp.uniqueCount[i];
	}

	printf( "Notes: %.1f bits\n", fBitsNotes );
	printf( " Lens: %.1f bits\n", fBitsLens );
	printf( "Total: %.1f bits\n", fBitsLens + fBitsNotes );

	int finalRun = 0, finalNote = 0;
	for( i = 0; i < numNotes; i++ )
	{
		// Search for repeated sections.
		int searchStart = 0;//i - MaxREV - MaxRL - MinRL;
		if( searchStart < 0 ) searchStart = 0;
		int s;
		int bestrl = 0, bestrunstart = 0;
		for( s = searchStart; s < i - OFFSET_MINIMUM; s++ )
		{
			int ml;
			int mlc;
			int rl;
			for( 
				ml = s, mlc = i, rl = 0;
				ml < numNotes && mlc < numNotes; // && rl < MaxRL;
				ml++, mlc++, rl++ )
			{
				if( completeNoteList[ml] != completeNoteList[mlc] ) break;
			}

			if( rl >= bestrl && RLMakesSense( rl, s, i ) )// && // Make sure it's best
				//s + MinRL + rl + MaxREV >= i )
			{
				bestrl = rl;
				bestrunstart = s;
			}
		}
		//printf( "Byte: %d / MRL: %d\n", i, bestrl );
		if( bestrl > MinRL )
		{
			//printf( "Found Readback at %d (%d %d) (D: %d)\n", i, i-bestrunstart, bestrl, i-bestrunstart-bestrl );
			i += bestrl-1;
			numRev++;

			//printf( "AT: %d BEST: LENG:%d  START:%d\n", i, bestrl, bestrunstart );
			int offset = i-bestrunstart-bestrl;
			//printf( "Emitting: %d %d (%d %d %d %d)\n", bestrl, offset, i, bestrunstart, bestrl, MinRL );

			int emit_best_rl = bestrl - MinRL - 1;

			// Output emit_best_rl, RLBits, Prob1RL
			// Output offset, MRBits, Prob1MR

			// Output emit_best_rl
			// Output offset

			vpx_write( &writer, 0, chanceRL);

			// Using 11 bits for both offset and emit_best_rl
			if( offset >= (1<<MAX_LSBITS) ) { fprintf( stderr, "offset TOO BIG (%d)\n", offset ); }
			if( emit_best_rl >= (1<<MAX_LSBITS) ) { fprintf( stderr, "emit_best_rl TOO BIG (%d)\n", emit_best_rl ); }

			offset -= OFFSET_MINIMUM;

			WriteLogshift( &writer, offset, bctableoffset, 0 );
			WriteLogshift( &writer, emit_best_rl, bctablerunlen, 0 );

			printf( "Reference: ofs:%d run:%d\n", offset, emit_best_rl );

			finalRun++;

			//printf( "Emit callback\n" );
		}
		else
		{
			// No readback found, we will have toa ctaully emit encode this note.
			//AddValue( &dtNotes, note );
			//AddValue( &dtLenAndRun, len );

			//regNoteList = realloc( regNoteList, sizeof(regNoteList[0]) * (numComp+1) );
			//regNoteList[numComp] = completeNoteList[i];

			//numComp++;

			uint32_t note = dtNotes.fullList[i];
			uint32_t lenAndRun = dtLenAndRun.fullList[i];
			vpx_write( &writer, 1, chanceRL );

			ProbabilityTreeWriteSym( &writer, /*GetIndexFromValue( &dtNotesComp, note )*/ note,
				probabilitiesNotes, treeSizeNotes, bitsForNotes );

			ProbabilityTreeWriteSym( &writer, GetIndexFromValue( &dtLenAndRunComp, lenAndRun ),
				probabilitiesLenAndRun, treeSizeLenAndRun, bitsForLenAndRun );

			printf( "Write Note: %d %d\n", note, lenAndRun );

			finalNote++;
		}
	}


	printf( "bctableoffset: " );
	for( i = 0; i < MAX_LSBITS; i++ )
		printf( "%d ", bctableoffset[i+1] );
	printf( "\nbctablerunlen: " );
	for( i = 0; i < MAX_LSBITS; i++ )
		printf( "%d ", bctablerunlen[i+1] );
	printf( "\n" );


	printf( "Final Note/Run: %d %d (%d)\n", finalNote, finalRun, chanceRL );

	printf( "Notes: %d\n", numComp );
	uint32_t sum = writer.pos;
	printf( "Data: %d bytes\n", writer.pos );

	printf( "bctable size: %d\n", MAX_LSBITS*2 );
	sum += MAX_LSBITS;

	printf( "Notes: %d + ~~%d~~\n", treeSizeNotes, dtNotesComp.numUnique * 1 );
	sum += treeSizeNotes;
//	sum += dtNotesComp.numUnique * 1;

	printf( "LenAndRun: %d + %d\n", treeSizeLenAndRun, dtLenAndRunComp.numUnique * 2 );
	sum += treeSizeLenAndRun;
	sum += dtLenAndRunComp.numUnique * 2;

	printf( "Total bytes:\n" );
	printf( "%d\n", sum );
	//unsigned int pos;
	//unsigned int size;

	return 0;
}

















#if 0








uint32_t runword = 0;
uint8_t runwordplace = 0;
int total_bytes = 0;
int bitcount = 0;

char * bitlist = 0; // size = bitcount

void EmitBit( int ib )
{
	runword |= ib << runwordplace;
	runwordplace++;
	if( runwordplace == 32 )
	{
		if( fData ) fprintf( fData, "0x%08x%s", runword, ((total_bytes%32)!=28)?", " : ",\n\t" );
		total_bytes+=4;
		if( fD ) fwrite( &runword, 1, 4, fD );
		runword = 0;
		runwordplace = 0;
	}
	bitlist = realloc( bitlist, bitcount+1 );
	bitlist[bitcount] = ib;
	bitcount++;
}


int PullBit( int * bp )
{
	if( *bp >= bitcount ) return -2;
	return bitlist[(*bp)++];
}

int PullExpGolomb( int * bp )
{
	int exp = 0;
	do
	{
		int b = PullBit( bp );
		if( b < 0 ) return b;
		if( b != 0 ) break;
		exp++;
	} while( 1 );

	int br;
	int v = 1;
	for( br = 0; br < exp; br++ )
	{
		v = v << 1;
		int b = PullBit( bp );
		if( b < 0 ) return b;
		v |= b;
	}
	return v-1;
}

int PullHuff( int * bp, huffelement * he )
{
	int ofs = 0;
	huffelement * e = he + ofs;
	do
	{
		if( e->is_term ) return e->value;

		int b = PullBit( bp );
		if( b < 0 ) return b;
//printf( "%d (%d)", b, *bp );

		//  ofs + 1 + if encoded in table
		ofs = (b ? (e->pair1) : (e->pair0) );
		e = he + ofs;
	} while( 1 );
}

#if 0
int DecodeMatch( int startbit, uint32_t * notes_to_match, int length_max_to_match, int * depth )
{
//	printf( "RMRS: %d\n", startbit );
	// First, pull off 
	// Read from char * bitlist = 0; // size = bitcount
	int matchno = 0;
	int bp = startbit;
	//printf( "Decode Match Check At %d\n", startbit );
	do
	{
		//printf( "CHECKP @ bp = %d\n", bp );
		int bpstart = bp;
		int class = PullBit( &bp );
		if( class < 0 )
		{
			//printf( "Class fail at %d\n", bp );
			return matchno;
		}
		//printf( "   Class %d @ %d\n", class, bp-1 );
		if( class == 0 )
		{
			// Note + Len
			int note = PullHuff( &bp, he );
			int lenandrun = PullHuff( &bp, hel );
			int cv = (note<<8) | lenandrun;
			//printf( "   CV %04x == %04x (matchno = %d)  (Values: %d %d)\n", cv, notes_to_match[matchno], matchno, note, lenandrun );
			if( cv != notes_to_match[matchno] || note < 0 || lenandrun < 0 )
			{
				//printf( "Breakout A %d != %d  (%d %d)\n", cv, notes_to_match[matchno], note, lenandrun );
				return matchno;
			}
			// Otherwise we're good!
			matchno++;
		}
		else
		{
			// Rewind
			if( (*depth)++ > MAX_BACK_DEPTH ) return matchno;
			int runlen = PullExpGolomb( &bp );
			int offset = PullExpGolomb( &bp );

			if( runlen < 0 || offset < 0 ) return matchno;
			//printf( "DEGOL %d %d BPIN: %d\n", runlen, offset, bp );
			runlen = runlen + MinRL + 1;
			offset = offset + OFFSET_MINIMUM;
			offset += runlen*INCLUDE_RUN_LENGTH_IN_BACK_TRACK_OFFSET;
			int bpjump = bpstart - offset;
			// Check for end of sequence or if bp points to something in the past.
			if( bpjump < 0 || bpjump >= bitcount-1 ) return matchno;
			//printf( "   OFFSET %d, %d (BP: %d)\n", runlen, offset, bp );
			int dmtm = length_max_to_match - matchno;
			if( dmtm > runlen ) dmtm = runlen;
			int dm = DecodeMatch( bpjump, notes_to_match + matchno, dmtm, depth );
			//printf( "DMCHECK %d %d @ BP = %d\n", dm, dmtm, bp );
			if( dm != dmtm )
			{
				//printf( "DM Disagree: %d != %d\n", dm, dmtm );
				return matchno + dm;
			}

			matchno += dm;
		}
	} while( matchno < length_max_to_match );
	//printf( "Match End: %d >= %d\n", matchno, length_max_to_match );
	return matchno;
}
#endif

int main()
{
	int i, j, k, l;
	FILE * f = fopen( "fmraw.dat", "rb" );

	// Notes
	hufftype * symbols = 0;
	hufffreq * symcounts = 0;
	int numsym = 0;

	// The play length of the note
	hufftype * lenss = 0;
	hufffreq * lencountss = 0;
	int numlens = 0;

	uint16_t usedmask = 0;
	int numNotes = 0;


	uint32_t * completeNoteList = 0;
	uint32_t * regNoteList = 0;

	while( !feof( f ) )
	{
		uint16_t s;
		int r = fread( &s, 1, 2, f );
		if( r == 0 ) break;
		if( r != 2 )
		{
			fprintf( stderr, "Error: can't read symbol.\n" );
			return -6;
		}

		usedmask |= s;
		int note = (s>>8)&0xff; // ( (next_note >> 8 ) & 0xff) + 47;
		int len = (s>>3)&0x1f;       // ( (next_note >> 3 ) & 0x1f) + t + 1;
		int run = (s)&0x07;


		completeNoteList = realloc( completeNoteList, sizeof(completeNoteList[0]) * (numNotes+1) );
		completeNoteList[numNotes] = s;

		//printf( "Append: %d %d\n", note, len );

		numNotes++;
	}










	
	int hufflen;
	he = GenerateHuffmanTree( symbols, symcounts, numsym, &hufflen );

	int htlen = 0;
	hu = GenPairTable( he, &htlen );

	int hufflenl;
	hel = GenerateHuffmanTree( lenss, lencountss, numlens, &hufflenl );

	int htlenl = 0;
	hul = GenPairTable( hel, &htlenl );

	float principal_length_note = 0;
	float huffman_length_note = 0;
	float principal_length_rl = 0;
	float huffman_length_rl = 0;

	printf( "Emitted syms/lens: %d/%d  Num Reg:%d Num rev:%d\n", numsym, numlens, numComp,numRev );
	// Total notes = numComp
	printf( "NOTES:\n" );

	for( i = 0; i < htlen; i++ )
	{
		huffup * thu = hu + i;
		printf( "%3d: %04x :%5d : ", i, thu->value, thu->freq );

		for( k = 0; k < thu->bitlen; k++ )
			printf( "%c", thu->bitstream[k]+'0' );

		huffman_length_note += thu->freq * thu->bitlen;
		principal_length_note += thu->freq * -log( thu->freq / (float)numComp ) / log(2);

		printf( "\n" );
	}

	printf( "LENS:\n" );
	for( i = 0; i < htlenl; i++ )
	{
		huffup * thu = hul + i;
		printf( "%3d: %04x :%5d : ", i, thu->value, thu->freq );

		for( k = 0; k < thu->bitlen; k++ )
			printf( "%c", thu->bitstream[k]+'0' );

		huffman_length_rl += thu->freq * thu->bitlen;
		principal_length_rl += thu->freq * -log( thu->freq / (float)numComp ) / log(2);

		printf( "\n" );
	}

	printf( "Expected Huffman Length Note: %.0f bits\n", huffman_length_note );
	printf( "Principal Length Note: %.0f bits\n", principal_length_note );
	printf( "Expected Huffman Length RL: %.0f bits\n", huffman_length_rl );
	printf( "Principal Length RL: %.0f bits\n", principal_length_rl );

	printf( "Expected Huffman Length: %.0f bits / %.0f bytes\n", (huffman_length_note+huffman_length_rl),(huffman_length_note+huffman_length_rl)/8.0 );
	printf( "Principal Length: %.0f bits / %.0f bytes\n", (principal_length_note+principal_length_rl),(principal_length_note+principal_length_rl)/8.0 );


	FILE * fTN = fopen( "huffTN_fmraw.dat", "wb" );
	FILE * fTL = fopen( "huffTL_fmraw.dat", "wb" );
	fD = fopen( "huffD_fmraw.dat", "wb" );

	fData = fopen( "../playback/badapple_song_huffman_forwardlzss.h", "wb" );

	fprintf( fData, "#ifndef ESPBADAPPLE_SONG_H\n" );
	fprintf( fData, "#define ESPBADAPPLE_SONG_H\n\n" );
	fprintf( fData, "#include <stdint.h>\n\n" );

	fprintf( fData, "#define ESPBADAPPLE_SONG_MINRL %d\n", MinRL );
	fprintf( fData, "#define ESPBADAPPLE_SONG_OFFSET_MINIMUM %d\n", OFFSET_MINIMUM );
	fprintf( fData, "#define ESPBADAPPLE_SONG_HIGHEST_NOTE %d\n", nHighestNote );
	fprintf( fData, "#define ESPBADAPPLE_SONG_LOWEST_NOTE %d\n", nLowestNote );
	fprintf( fData, "#define ESPBADAPPLE_SONG_LENGTH %d\n", numNotes );

	fprintf( fData, "\n" );
	fprintf( fData, "BAS_DECORATOR uint16_t espbadapple_song_huffnote[%d] = {\n\t", hufflen - numsym );


	int emit_bits_class = 0;
	int emit_bits_backtrack = 0;
	int actualRev = 0;
	int actualReg = 0;
	int emit_bits_data = 0;

	int bitmaplocation[numNotes];


	// Emit tables
	int maxpdA = 0;
	int maxpdB = 0;
	int htnlen = 0;
	for( i = 0; i < hufflen; i++ )
	{
		huffelement * h = he + i;
		if( h->is_term )
		{
			//uint32_t sym = h->value;
			//fwrite( &sym, 1, 2, fTN );
			//fprintf( fData, "0x%04x%s", sym, ((i%12)!=11)?", " : ",\n\t" );
			//htnlen += 2;
		}
		else
		{
			int pd0 = h->pair0 - i - 1;
			int pd1 = h->pair1 - i - 1;

			huffelement * h0 = he + h->pair0;
			huffelement * h1 = he + h->pair1;

			if( pd0 < 0 || pd1 < 0 )
			{
				fprintf( stderr, "Error: Illegal pd\n" );
				return -5;
			}
			if( pd0 > maxpdA ) maxpdA = pd0;
			if( pd1 > maxpdB ) maxpdB = pd1;


			if( h0->is_term )
				pd0 = h0->value | 0x80;
			if( h1->is_term )
				pd1 = h1->value | 0x80;


			uint32_t sym = (pd0) | (pd1<<8);
			fwrite( &sym, 1, 2, fTN );
			fprintf( fData, "0x%04x%s", sym, ((i%12)!=11)?", " : ",\n\t" );
			htnlen += 2;
		}
	}
	fprintf( fData, "};\n\n" );

	printf( "max pd %d / %d\n", maxpdA, maxpdB );

	int htnlen2 = 0;

	fprintf( fData, "BAS_DECORATOR uint16_t espbadapple_song_hufflen[%d] = {\n\t", hufflenl - numlens );

	maxpdA = 0;
	maxpdB = 0;
	for( i = 0; i < hufflenl; i++ )
	{
		huffelement * h = hel + i;
		if( h->is_term )
		{
			//uint32_t sym = h->value;
			//fwrite( &sym, 1, 1, fTL );
			//htnlen2 += 1;
			//fprintf( fData, "0x%04x%s", sym, ((i%12)!=11)?", " : ",\n\t" );
		}
		else
		{
			int pd0 = h->pair0 - i - 1;
			int pd1 = h->pair1 - i - 1;

			huffelement * h0 = hel + h->pair0;
			huffelement * h1 = hel + h->pair1;

			if( pd0 < 0 || pd1 < 0 )
			{
				fprintf( stderr, "Error: Illegal pd\n" );
				return -5;
			}
			if( pd0 > maxpdA ) maxpdA = pd0;
			if( pd1 > maxpdB ) maxpdB = pd1;

			//printf( "%d %d  %02x %02x  %02x %02x\n", h0->is_term, h1->is_term, pd0, pd1, h0->value, h1->value );

			if( h0->is_term )
				pd0 = h0->value | 0x80;
			if( h1->is_term )
				pd1 = h1->value | 0x80;

			uint32_t sym = (pd0) | (pd1<<8);
			fwrite( &sym, 1, 2, fTL );
			htnlen2 += 2;
			fprintf( fData, "0x%04x%s", sym, ((i%12)!=11)?", " : ",\n\t" );
		}
	}

	fprintf( fData, "};\n\n" );




	// Pass 2 - actually emit.
	for( i = 0; i < numNotes; i++ )
	{
		// Search for repeated sections.
		int searchStart = 0; //i - MaxREV - MaxRL - MinRL;
		if( searchStart < 0 ) searchStart = 0;
		int s;
		int bestrl = 0, bestcompcost = 0, bestrunstart = 0;
		for( s = searchStart; s <= i; s++ )
		{
			int ml;
			int mlc;
			int rl;
			int compcost;

			for( 
				ml = s, mlc = i, rl = 0;
				ml < i && mlc < numNotes; //&& rl < MaxRL;
				ml++, mlc++, rl++ )
			{
				if( completeNoteList[ml] != completeNoteList[mlc] ) break;
			}

			compcost = ExpGolombCost( rl ) + ExpGolombCost( i-s );

			if( rl*COMPCOSTTRADEOFF - compcost > bestrl*COMPCOSTTRADEOFF - bestcompcost )
			{
				bestrl = rl;
				bestcompcost = compcost;
				bestrunstart = s;
			}
		}

		if( bestrl > MinRL )
		{
			emit_bits_class++;
			int startplace = i;
			printf( "OUTPUT   CB @ bp =%5d bestrl=%3d bests=%3d ", bitcount, bestrl, bestrunstart );
			EmitBit( 1 );
			i += bestrl - 1;
			int offset = startplace - bestrunstart - OFFSET_MINIMUM;
			//offset -= bestrl*INCLUDE_RUN_LENGTH_IN_BACK_TRACK_OFFSET;

			if( offset < 0 )
			{
				fprintf( stderr, "Error: OFFSET_MINIMUM is too large (%d - %d - %d - %d = %d)\n", startplace, bestrunstart, bestrl, OFFSET_MINIMUM, offset );
				exit ( -5 );
			}
			int emit_best_rl = bestrl - MinRL - 1;
			//printf( "WRITE %d %d\n", emit_best_rl, offset );
			printf( "Write: %d %d\n", emit_best_rl, offset );
			emit_bits_backtrack += EmitExpGolomb( emit_best_rl );
			emit_bits_backtrack += EmitExpGolomb( offset );
			actualRev++;
		}
		else
		{
			// No readback found, we will have toa ctaully emit encode this note.
			//AddValue( &dtNotes, note );
			//AddValue( &dtLenAndRun, len );

			uint32_t note_to_emit = completeNoteList[i];

			emit_bits_class++;
			printf( "OUTPUT DATA @ bp = %d (Values %02x %02x (index %d))\n", bitcount,  completeNoteList[i]>>8,  completeNoteList[i]&0xff, i );
			EmitBit( 0 );
#ifndef SINGLETABLE
			int n = completeNoteList[i] >> 8;
#else
			int n = completeNoteList[i];
#endif
			int bitcountatstart = bitcount;
			bitmaplocation[i] = bitcount;

			for( k = 0; k < htlen; k++ )
			{
				huffup * thu = hu + k;
				if( thu->value == n )
				{
					//printf( "Emitting NOTE %04x at %d\n", n, bitcount );
					int ll;
					emit_bits_data += thu->bitlen;
					for( ll = 0; ll < thu->bitlen; ll++ )
					{
						EmitBit( thu->bitstream[ll] );
					}
					break;
				}
			}
			if( k == htlen )
			{
				fprintf( stderr, "Fault: Internal Error (%04x not in map)\n", n );
				return -4;
			}

			int lev =  completeNoteList[i] & 0xff;
			for( k = 0; k < htlenl; k++ )
			{
				huffup * thul = hul + k;
				if( thul->value == lev )
				{
					int ll;
					//printf( "Emitting LEN %04x at %d\n", lev, bitcount );
					emit_bits_data += thul->bitlen;
					for( ll = 0; ll < thul->bitlen; ll++ )
					{
						EmitBit( thul->bitstream[ll] );
					}
					break;
				}
			}
			if( k == htlenl )
			{
				fprintf( stderr, "Fault: Internal Error (run %d not in map)\n", l );
				return -4;
			}
			//printf( "Write: %d\n", bitcount, bitcountatstart );
			actualReg++;
		}
	}
#if 0
	printf( "emit_bits_data: %d\n", emit_bits_data );
	printf( "actualRev: %d\n", actualRev );
	printf( "actualReg: %d\n", actualReg );

	printf( "Class: %d bits\n", emit_bits_class );
	printf( "Backtrack: %d bits\n", emit_bits_backtrack );

	printf( "HNTLEN: %d\n", htnlen );
	printf( "HNTLEN2: %d\n", htnlen2 );
	printf( "Data Bits: %d\n", bitcount);
	printf( "Total Bytes:\n" );
	printf( "%d\n", (bitcount+7)/8 + htnlen2 + htnlen );
#endif
#if 0


	fprintf( fData, "BAS_DECORATOR uint32_t espbadapple_song_data[] = {\n\t" );

	printf( "max pd %d / %d\n", maxpdA, maxpdB );

	printf( "Rev/Reg: %d %d\n", numRev, numComp );

	printf( "NOTES: %d\n", numNotes );

	for( i = 0; i < numNotes; i++ )
	{
		// Search for repeated sections.
		int searchStart = 0;//i - MaxREV - MaxRL - MinRL;
		if( searchStart < 0 ) searchStart = 0;
		int s;
		int bestrl = 0, bestrunstart = 0;
		for( s = searchStart; s <= i; s++ )
		{
			int ml;
			int mlc;
			int rl;
			for( 
				ml = s, mlc = i, rl = 0;
				ml < i && mlc < numNotes; // && rl < MaxRL;
				ml++, mlc++, rl++ )
			{
				if( completeNoteList[ml] != completeNoteList[mlc] ) break;
			}

			if( rl > bestrl )// && // Make sure it's best
				//s + MinRL + rl + MaxREV >= i )
			{
				bestrl = rl;
				bestrunstart = s;
			}
		}
		printf( "Byte: %d / MRL: %d\n", i, bestrl );
		if( bestrl > MinRL )
		{
			//printf( "Found Readback at %d (%d %d) (D: %d)\n", i, i-bestrunstart, bestrl, i-bestrunstart-bestrl );
			i += bestrl-1;
			numRev++;

			//printf( "AT: %d BEST: LENG:%d  START:%d\n", i, bestrl, bestrunstart );
			int offset = i-bestrunstart-bestrl;
			//printf( "Emitting: %d %d (%d %d %d %d)\n", bestrl, offset, i, bestrunstart, bestrl, MinRL );

			int emit_best_rl = bestrl - MinRL - 1;

			// Output emit_best_rl, RLBits, Prob1RL
			// Output offset, MRBits, Prob1MR

			// Output emit_best_rl
			// Output offset
		}
		else
		{
			// No readback found, we will have toa ctaully emit encode this note.
			//AddValue( &dtNotes, note );
			//AddValue( &dtLenAndRun, len );

			regNoteList = realloc( regNoteList, sizeof(regNoteList[0]) * (numComp+1) );
			regNoteList[numComp] = completeNoteList[i];

			numComp++;
		}
	}





	int emit_bits_data = 0;
	int emit_bits_backtrack = 0;
	int emit_bits_class = 0;

	int actualReg = 0, actualRev = 0;
	for( i = 0; i < numNotes; i++ )
	{

#if 0
		int bestrl = -1;
		int bests = -1;
		int s = 0;
		for( s = bitcount - 1; s >= 0; s-- )
		{
			int depth = 0;
			int dm = DecodeMatch( s, completeNoteList + i, numNotes - i, &depth );
			//printf( "Check [at byte %d]: %d -> %d -> %d\n", i, s, dm, depth );
			int sderate = (log(bitcount-s+1)/log(2)) / 9;
			if( dm - sderate > bestrl && 
				// Tricky - make sure that for our decided OFFSET_MINIMUM, we can emit it.
				bitcount - s - OFFSET_MINIMUM - dm*INCLUDE_RUN_LENGTH_IN_BACK_TRACK_OFFSET >= 0
			)
			{
				bestrl = dm;
				bests = s;
			}
		}
#endif
		int bestback = -1;
		int bestsbk = -1;
		int sbk;
		for( sbk = i-OFFSET_MINIMUM; sbk >= 0; sbk-- )
		{
			int matching;
			for( matching = 1; matching+sbk < i; matching++ )
			{
				if( 
			}
		}

const int MinRL = 2;
#define OFFSET_MINIMUM 4
#define MAX_BACK_DEPTH 64



		if( bestrl > MinRL )
		{
			emit_bits_class++;
			int startplace = bitcount;
			printf( "OUTPUT   CB @ bp =%5d bestrl=%3d bests=%3d ", bitcount, bestrl, bests );
			EmitBit( 1 );
			i += bestrl - 1;
			int offset = startplace - bests - OFFSET_MINIMUM;
			offset -= bestrl*INCLUDE_RUN_LENGTH_IN_BACK_TRACK_OFFSET;

			if( offset < 0 )
			{
				fprintf( stderr, "Error: OFFSET_MINIMUM is too large (%d - %d - %d - %d = %d)\n", startplace, bests, bestrl, OFFSET_MINIMUM, offset );
				exit ( -5 );
			}
			int emit_best_rl = bestrl - MinRL - 1;
			//printf( "WRITE %d %d\n", emit_best_rl, offset );
			printf( "Write: %d %d\n", emit_best_rl, offset );
			emit_bits_backtrack += EmitExpGolomb( emit_best_rl );
			emit_bits_backtrack += EmitExpGolomb( offset );
			actualRev++;
		}
		else
		{
			emit_bits_class++;
			printf( "OUTPUT DATA @ bp = %d (Values %02x %02x (index %d))\n", bitcount,  completeNoteList[i]>>8,  completeNoteList[i]&0xff, i );
			EmitBit( 0 );
#ifndef SINGLETABLE
			int n = completeNoteList[i] >> 8;
#else
			int n = completeNoteList[i];
#endif
			int bitcountatstart = bitcount;
			bitmaplocation[i] = bitcount;

			for( k = 0; k < htlen; k++ )
			{
				huffup * thu = hu + k;
				if( thu->value == n )
				{
					//printf( "Emitting NOTE %04x at %d\n", n, bitcount );
					int ll;
					emit_bits_data += thu->bitlen;
					for( ll = 0; ll < thu->bitlen; ll++ )
					{
						EmitBit( thu->bitstream[ll] );
					}
					break;
				}
			}
			if( k == htlen )
			{
				fprintf( stderr, "Fault: Internal Error (%04x not in map)\n", n );
				return -4;
			}
#ifndef SINGLETABLE
			int lev =  completeNoteList[i] & 0xff;
			for( k = 0; k < htlenl; k++ )
			{
				huffup * thul = hul + k;
				if( thul->value == lev )
				{
					int ll;
					//printf( "Emitting LEN %04x at %d\n", lev, bitcount );
					emit_bits_data += thul->bitlen;
					for( ll = 0; ll < thul->bitlen; ll++ )
					{
						EmitBit( thul->bitstream[ll] );
					}
					break;
				}
			}
			if( k == htlenl )
			{
				fprintf( stderr, "Fault: Internal Error (run %d not in map)\n", l );
				return -4;
			}
			//printf( "Write: %d\n", bitcount, bitcountatstart );
#endif
			actualReg ++;
		}
	}

#endif
	printf( "Actual Rev/Reg: %d/%d\n", actualRev, actualReg );
	printf( "Data Usage: %d bits / %d bytes\n", emit_bits_data, emit_bits_data/8 );
	printf( "Backtrack Usage: %d bits / %d bytes\n", emit_bits_backtrack, emit_bits_backtrack/8 );
#ifndef SINGLETABLE
	printf( "Class Usage: %d bits / %d bytes\n", emit_bits_class, emit_bits_class/8 );
#endif
	printf( "Total: %d bits / %d bytes\n", bitcount, (bitcount+7)/8  );

	if( runwordplace )
	{
		fwrite( &runword, 1, 4, fD );
		total_bytes+=4;
		fprintf( fData, "0x%08x%s", runword, ((total_bytes%8)!=7)?", " : ",\n\t" );
	}

	fprintf( fData, " };\n\n" );
	fprintf( fData, "#endif" );
	fclose( fData );
	printf( "Used mask: %04x\n", usedmask );
	printf( "Huff Tree (N): %d bytes\n", htnlen );
#ifndef SINGLETABLE
	printf( "Huff Tree (D): %d bytes\n", htnlen2 );
#endif
	printf( "Written Data: %d bytes\n", total_bytes );
	printf( "TOTAL bytes:\n" );
	printf( "%d\n", htnlen + htnlen2 + total_bytes );
	return 0;
}
#endif

