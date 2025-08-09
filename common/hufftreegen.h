/*
 Public Domain Huffman Table Creator
  2022-2024 cnlohr
 You may license this under MIT/x11, NewBSD, CC0, MIT0 or Public Domain
 Pass in a bunch of data, and the count of that data.
 It will give you an optimal huffman tree.

 General approach:
  1. Generate a list of symbols, and their counts.
  2. If you want to use dynamic arrays, you can, just use HuffmanAppendHelper.
  3. Pass this to GenerateHuffmanTree.
  4. This generates a complete huffman tree, where each element will be a
     central node to the tree (which will have pair0/pair1 for if you get a
     0 or 1 in the bitstream) or a term node, where it means if you got here
     you are done.
  5. The huffelement * returned by GenerateHuffmanTree is how you DECODE a
     bitstream.
  6. You can then pass this to GenPairTable, and it will make a list of all
     symbols, and the needed bitstream to encode that symbol.  Note the char *
     is actually a seies of \000 and \001's.  Not ASCII, you can print them
     by adding '0' to each element.

 The workflow is:
  1. Generate your list of counts/symbols.
  2. Call GenerateHuffmanTree
  3. Call GenPairTable
  4. Write out in whatever way you choose the output of GenerateHuffmanTree
  5. Encode your data with the output from GenPairTable and write it out.

 To decode, load the huffman tree, however you choose.
 Read through the bitstream, using the huffman tree and maintaining an index
 everywhere along the way.

 Usage example:

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define HUFFER_IMPLEMENTATION
#include "hufftreegen.h"

#define NELEM 1000000

int data_to_compress[NELEM];

// Stored as individual 1's and 0's for clarity.
uint8_t compressed_bitstream[NELEM*16];

int main()
{
	int i, j, k;

	// Generate a random distribution of 5 unique values, weighted towards lower numbers.
	for( i = 0; i < NELEM; i++ )
	{
		data_to_compress[i] = (rand()%3)*(rand()%3)*(rand()%3);
	}

	hufftype * vals = 0;
	hufffreq * counts = 0;
	int nunique = 0;
	for( i = 0; i < NELEM; i++ )
	{
		nunique = HuffmanAppendHelper( &vals, &counts, nunique, data_to_compress[i] );
	}

	printf( "%d unique elements\n", nunique );

	int huff_decode_len = 0;
	huffelement * huff_decode = GenerateHuffmanTree( vals, counts, nunique, &huff_decode_len );

	printf( "Tree has %d entries (%p)\n", huff_decode_len, huff_decode );
	printf( " Index: Frequency -> If 0, jump to, If 1 jump to (Or terminal)\n" );
	for( i = 0; i < huff_decode_len; i++ )
	{
		huffelement * he = huff_decode + i;
		printf( " %2d: %6d -> ", i, he->freq );
		if( he->is_term )
			printf( "TERMINAL (Emit %d)\n", he->value );
		else
			printf( "if 0 goto %d / if 1 goto %d\n", he->pair0, he->pair1 );
	}


	int huff_encode_len = 0;
	huffup * huff_encode = GenPairTable( huff_decode, &huff_encode_len );

	printf( "Encode %d entries\n", huff_encode_len );
	printf( " Index: Value: Frequency -> Bitstream to encode this symbol\n" );
	for( i = 0; i < nunique; i++ )
	{
		huffup * hu = huff_encode + i;
		printf( " %2d: %3d: %6d -> ", i, hu->value, hu->freq );
		for( j = 0; j < hu->bitlen; j++ )
		{
			printf( "%c", hu->bitstream[j] + '0' );
		}
		printf( "\n" );
	}

	int bspos = 0;
	for( i = 0; i < NELEM; i++ )
	{
		for( j = 0; j < huff_encode_len; j++ )
		{
			huffup * hu = huff_encode + j;
			// Find the right symbol to output
			if( hu->value == data_to_compress[i] )
			{
				for( k = 0; k < hu->bitlen; k++ )
				{
					// Write data into stream
					compressed_bitstream[bspos++] = hu->bitstream[k];
				}
				break;
			}
		}
		assert( j != huff_encode_len );
	}

	printf( "Total Elements: %d, Stored as 8-bit values totaling %d bits\n", NELEM, NELEM*8 );
	printf( "Compressed data stream: %d bits (%.2f%% of original)\n", bspos, bspos*100.0/(NELEM*8) );

	j = 0;
	int place_in_tree = 0;
	for( i = 0; i < bspos; )
	{
		huffelement * he = huff_decode + place_in_tree;
		if( he->is_term )
		{
			int check = data_to_compress[j++];
			if( he->value != check )
			{
				printf( "Compression failed. %d received, expected %d at symbol %d\n", he->value, check, j-1 );
				assert( 0 );
			}
			place_in_tree = 0;
		}
		else
		{
			place_in_tree = compressed_bitstream[i++] ? he->pair1 : he->pair0;
		}
	}

	printf( "Validated OK\n" );

	return 0;
}


*/

#ifndef _HUFFTREEGEN_H
#define _HUFFTREEGEN_H

#include <string.h>
#include <stdint.h>

// Header-only huffman-table creator.

#ifndef hufftype
#define hufftype uint32_t
#endif

#ifndef hufffreq
#define hufffreq uint32_t
#endif


typedef struct
{
	hufffreq freq;
	uint8_t is_term;

	// valid if is_term is 0.
	int pair0;
	int pair1;

	// valid if is_term is 1.
	hufftype value;
} huffelement;

int HuffmanAppendHelper( hufftype ** vals, hufffreq ** counts, int elems, hufftype t );

huffelement * GenerateHuffmanTree( hufftype * data, hufffreq * frequencies, int numpairs, int * hufflen );

typedef struct
{
	hufftype value;
	int bitlen;
	uint8_t * bitstream;
	hufffreq freq;
} huffup;

huffup * GenPairTable( huffelement * table, int * htlen );

#ifdef HUFFER_IMPLEMENTATION

/*
static void PrintHeap( int * heap, int len, huffelement * tab )
{
	int i;
	printf( "Len: %d\n", len );
	for( i = 0; i < len; i++ )
		printf( "%d %d  %d\n", i, heap[i], tab[heap[i]].freq );
}
*/

static void PercDown( int p, int size, huffelement * tab, int * heap )
{
	hufffreq f = tab[heap[p]].freq;
	do
	{
		int left = 2*p+1;
		int right = left+1;
		int smallest = p;
		hufffreq tf = f;
		hufffreq fcomp = f;
		if( left < size && (tf = tab[heap[left]].freq ) < fcomp )
		{
			smallest = left;
			fcomp = tf;
		}
		if( right < size && (tf = tab[heap[right]].freq ) < fcomp )
		{
			smallest = right;
		}
		if( smallest == p ) return;
		// Otherwise, swap.
		int temp = heap[p];
		heap[p] = heap[smallest];
		heap[smallest] = temp;
		p = smallest;
	} while( 1 );
}

huffelement * GenerateHuffmanTree( hufftype * data, hufffreq * frequencies, int numpairs, int * hufflen )
{
	int e, i;

	if( numpairs < 1 )
	{
		if( hufflen ) *hufflen = 0;
		return 0;
	}

	int oecount = 0;

	for( i = 0; i < numpairs; i++ )
	{
		// Cull out any zero frequency elements.
		if( frequencies[i] ) oecount++;
	}

	// We actually want a min-heap of all of the possible symbols
	// to accelerate the generation of the tree.

	int hl = *hufflen = oecount * 2 - 1;
	huffelement * tab = calloc( sizeof( huffelement ), hl );
	int * minheap = calloc( sizeof( int ), oecount );
	int minhlen = 0;

	for( i = oecount-1, e = 0; i < hl; i++, e++ )
	{
		while( frequencies[e] == 0 ) e++;
		huffelement * t = &tab[i];
		t->is_term = 1;
		t->value = data[e];
		int tf = t->freq = frequencies[e];

		// Append value to heap.
		int h = i;
		int p = minhlen++;
		minheap[p] = i;

		while( p )
		{
			int parent = (p-1)/2;
			int ph = minheap[parent];
			int pf = tab[ph].freq;
			if( pf <= tf ) // All is well.
				break;

			// Otherwise, flip.
			minheap[parent] = h;
			minheap[p] = ph;
			//tf = pf;
			//h = ph;
			p = parent;
		}


/*printf( "***HEAP*** %d\n", minhlen );
int tmp;
for( tmp=0; tmp<minhlen;tmp++)
{
	printf( "%d - FRQ: %d\n", minheap[tmp], tab[minheap[tmp]].freq );
}
*/

	}

	for( i = oecount-2; i >= 0; i-- )
	{
		huffelement * e = &tab[i];
		// Filling in the huffman tree from the back, forward.
		e->pair0 = minheap[0];
		minheap[0] = minheap[--minhlen];

		// Percolate-down.
		PercDown( 0, minhlen, tab, minheap );

		e->pair1 = minheap[0];

		e->is_term = 0;

		e->freq = tab[e->pair0].freq + tab[e->pair1].freq;

		minheap[0] = i;
		PercDown( 0, minhlen, tab, minheap );

	//	printf( "SECOND!!!\n" );
	//	PrintHeap( minheap, minhlen, tab );
	}


	return tab;
}

void InternalHuffT( huffelement * tab, int p, huffup ** out, int * huffuplen, uint8_t ** dstr, int * dstrlen )
{
	huffelement * ths = &tab[p];
	if( ths->is_term )
	{
		int newlen = ++(*huffuplen);
		*out = realloc( *out, sizeof( huffup ) * ( newlen ) );
		huffup * e = &(*out)[newlen-1];
		e->value = ths->value;

		e->bitlen = *dstrlen;
		e->bitstream = malloc( *dstrlen + 1 );
		memcpy( e->bitstream, *dstr, *dstrlen );
		e->freq = ths->freq;
	}
	else
	{
		int dla = *dstrlen;
		*dstr = realloc( *dstr, ++(*dstrlen) );
		(*dstr)[dla] = 0;
		InternalHuffT( tab, tab[p].pair0, out, huffuplen, dstr, dstrlen );
		(*dstr)[dla] = 1;
		InternalHuffT( tab, tab[p].pair1, out, huffuplen, dstr, dstrlen );
		(*dstrlen)--;
	}
}

huffup * GenPairTable( huffelement * table, int * htlen )
{
	huffup * ret = 0;
	int huffuplen = 0;

	uint8_t * dstr = malloc( 1 );
	int dstrlen = 0;
	dstr[0] = 0;

	// Generate the table from a tree, recursively.
	InternalHuffT( table, 0, &ret, &huffuplen, &dstr, &dstrlen );
	if( htlen ) *htlen = huffuplen;
	return ret;
}

int HuffmanAppendHelper( hufftype ** vals, hufffreq ** counts, int elems, hufftype t )
{
	int j;
	for( j = 0; j < elems; j++ )
	{
		if( (*vals)[j] == t )
		{
			(*counts)[j]++;
			return elems;
		}
	}
	elems++;
	*vals = realloc( *vals, (elems)*sizeof(hufftype) );
	*counts = realloc( *counts, (elems)*sizeof(hufffreq) );
	(*vals)[elems-1] = t;
	(*counts)[elems-1] = 1;
	return elems;
}

#endif

#endif

