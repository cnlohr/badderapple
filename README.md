# badder apple

# This writeup is incomplete, please do not share it on social media yet.

What started as my shot at bad apple on an ESP8266 ended in the biggest spiral into my longest running project. The final outcome from this project was all 6570 frames, at 64x48 pixels, with sound and code for playback in 64.5kB, running on a 10-cent ch32v006. This is the story of bad*der* apple.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/5c77bf51-2895-4764-a540-fee0bc53da5a WIDTH=50%>
</P>

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       62976 B        62 KB     99.19%
      BOOTLOADER:        3324 B       3328 B     99.88%
             RAM:        8064 B         8 KB     98.44%
```

If you are interested in the web viewer of the bitstream explaining what every bit means (the image below) you can click [here](https://cnvr.io/dump/badderapple.html)

![Web Viewer](https://github.com/user-attachments/assets/cac9ab4e-9ecf-43d7-9ec4-cad37b45c0f1)


If you're interested in how to do [setup and running](#setup-and-running) instructions or [previous and future work](#previous-and-future-work), feel free to click there.

## History

Originally, in 2016, I wanted to use the new-at-the-time [ESP8285](https://www.espressif.com/en/pcn-product/esp8285), an [ESP8266](https://www.espressif.com/en/products/socs/esp8266) with integrated flash.  I wanted to make the smallest bad apple.  I wanted to try to dead bug a crystal on the ground plane, and use my [NTSC Broadcast Television from an ESP8266 project](https://github.com/cnlohr/channel3) and make the (physically) smallest bad apple.

My first attempt was with RLE on the raster image, and I tried a number of other tricks, like variable width integers, etc.  But nothing could get it compress all that much.  With the 512x384 res, it took about 1kB per frame to store.  Total size was about 11,602,173 bytes.  I tried other compression mechanisms on top of that, getting it down to ~4MB, but nothing really stuck.  I eventually had to put the project back on the shelf.  There were no tradeoffs I was willing to make.

Then, one day in 2019, [Inverse Phase](https://inversephase.bandcamp.com/) showed me [Onslaught - Bad Apple 64 - C64 Demo](https://www.youtube.com/watch?v=OsDy-4L6-tQ), which was able to compress bad apple down to 170kB, or 70 bytes/frame, at 12fps.  This seemed truly impossible to me,  this was massively smaller than what I was able to achieve.  Granted, at a lower resolution and framerate (312x184, at 12FPS)... It blew my mind.

I didn't do anything serious here because I had always struggled to find any good way to generate glyphs (or 8x8 groups of pixels).  I tried a few different techniques but it kind of petered out, since all of my techniques produced lackluster output.  I would get occasional spirts of inspiration but no majorbreakthroughs.

But, then, in early 2024, things really got into high gear again, because WCH, the creators of the CH32V003, announced other chips in the series with FLASH ranging from 32kB to 62kB, so it was time for the rubber to hit the road again.  I would target the ch32v006, with 62kB flash + 2.5kB Bootloader.  Also, Ronboe just sent me some of their 64x48 pixel OLED displays that cost less than $1/ea.

A friend pointed out that we should get a baseline¹.  Let's see what the state-of-the art is in video encoding, let's try encoding our video with h265 (HEVC), a codec with the full force of millions of dollars of development?  How much can it encode Bad Apple down to? Let's just see what we're going to be up against."

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/e645dcee-80ec-4da6-8c78-48bd2ba74ce0 WIDTH=50%>
</P>

...Oof..  this is going to be one doozie.

By March I hit a wall. I couldn't get the video to a quality I was happy with, and I couldn't find very good tiles.  The video was hovering around 80kB, with lackluster quality due to the poor tile selection.

Then, while visiting my brother and sister in law, both PhD's in math, and they introduced me to the K-means algorithm and were rather perplexed I didn't know about it. 

As soon as I got a break, I implemented a k-means approach, and wowee! The tiles that came out of k-means was AMAZING!!! With these great tiles, the quality was solved  so the shift went to various compression for the tile sets + describing the glyphs.

The problem started to form clearly.  There were just a few things that needed to be compressed here:
1. The song, made up of notes, with a frequency, duration and time-to-next-note. Each note could be expressed as two bytes. At 1910 notes, it would take about 3820 bytes.
2. The glyphs, which make up tiles, about 256 qty, of 8x8 pixels, each pixel 3 varying intensities of grey. If naively encoded, would be 4096 bytes.
3. The stream, information to convery which which glpyh is used in each tile for each frame of the video.  For 64x48 pixels, or 8x6 tiles of 8x8 glyphs x 6570 frames.  If naively encoded, would be 315360 bytes.

All in all, I had to find some way to compress 323276 bytes worth of data, that was already pretty tight and an unknown quantity of code down into 64.5kB... and I knew it would be a challenge.

I gave it a solid go, using huffman trees to select my tiles and runlengthsm trying different runtime schemes, I was able to get it to fit.  I had to cut a lot of corners, and the quality was not quite where I wanted.  I was ready to, after 7 years just make the video and publish... Until I ran this whole thing by Lyuma and their brother, Hellcatv who pointed out I should be using arithmatic, range, or other better coding techniques.  I didn't believe that it would offer anything, but upon giving it a try, I knew I had to delay publushing for a year.  A decision I feel was a wise one.

I think instead of telling the story, I'd rather go into what some of the compression techniques that constitute these solutions.

# Primary Compression Techniques

Symbols: In general, for compression techniques, you want to break your data into "symbols" instead of bytes.  Because maybe you don't have 256 possible values, but instead 7, 143, or 258.  By storing the data as symbols, it helps us compress things one-symbol-at-a-time instead of trying to pick apart a data stream.

Minimizing Entropy: There are multiple ways to represent information, some ways compress much better than others.

General Compression: You can take a bitstream and put it through existing compression schemes, which will give you a ballpark guess of how much a given data stream can be compressed.

RLE: Run-length encoding, or say "nothing changes for n samples"

Run-lengths as symbols: Run lengths can instead of being encoded as a number, 

Symbol Compression: Using Huffman coding, or VPX coding to compress symbols.

Pattern Matching: You can reference earler parts of the decoded (or encoded) stream to encode repeated segments by reference instead of needing to re-encode entire sections. Systems like [LZSS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski) are used in regular compression algorithms to find, and reference these earlier parts of the stream. 

You can pair symbol compression with pattern matching, but you would not want to apply multiple symbol compression mechanisms together. For instance, it does not make sense to use LZSS and huffman trees, and it does not make sense to use LZSS and RLE.

## Minimizing Entropy

There are a lot of different ways to encode information, before compression.  This is illustrated by the chart below.  I took the bad apple song and represented the data in a variety of ways.  While all the ways convey the same information and can theoretically be compressed to similar levels, by representing information in binary, there are typically massive gains, and by simplifying the representation, it can be compressed greatly.  This is why you shouldn't necessarily encode everything as JSON, slap zstd or gzip ontop of it and call it good.

| Compression | .xml | .json (jq formatted) | .json.min | .bson (binary JSON) | .mid | .dat | 
| -- | -- | -- | -- | -- | -- | -- |
| uncompressed | 217686 (5698%) | 150802 (3947%)| 85855 (2247%) | 88677 (2321%) | 17707 (463%) | **3820 (100%)** |
| gzip -9      | 12001 (314%) | 11458 (300%) | 10590 (277%) | 13720 (359%) | 1329 (34%) | 682 (17.8%) |
| zstd -9      | 10052 (263%) | 9480 (248%) | 8877 (232%) | 12182 (319%)  | 1332 (35%) | 724 (19.0%) |

![Graph of above table](https://github.com/user-attachments/assets/ffe9bd12-6028-4c8c-86af-d76ef2695ae3)

Originally, I used a scheme of where I would encode each tile one at a time through the length of the video.  And I would compress it as `[ run-length | symbol-to-switch-to ]` before another tile would change.  I then tried doing exactly the same, but, scanned left-to-right and top-top-bottom, and looked for changes over time.  I was using exactly the same binary representation for both. And the binary data takes exactly the same amount of data to encode each way. YET! Across the board, it was a serious win to represent the data the other way.

In an example datastream I am using 256 tile, I can represent tile IDs as uint8_t's, so the stream can be represented as 64/8 * 48/8 * 6570 (Frames) = 315360 bytes.

| Type | Data Length (32-bit tiles) | Data Length (8-bit tiles) |
| -- | -- | -- |
| Tile IDs Data                 | 1261440 (400%) | 315360 (100%) |
| Original Ordering, gzip -9    | 146191 (46.4%) | **94440 (29.9%)** | 
| Original Ordering, zstd -9    | **130802 (41.5%)** | 95240 (30.2%) |
| Reordered, gzip -9            | 119046 (37.7%) | 114779 (36.4%) |
| Reordered, zstd -9            | **110001 (34.9%)** | **110119 (34.9%)** |

![Graph of above data](https://github.com/user-attachments/assets/8c70357b-729a-42e9-8785-6d9e022173d9)

Either way we look at this, this is going to be a sad uphill batter.  We are going to have to get down to about 58kB in order to fit the tile data, startup code and song.  If gzip and zstd can only get it down to 92kB this is going to be rough.

Wait ... 💭This is not the result I expected.  Why does this happen?  Why would the size of each tile being encoded (in 32-bit or 8-bit) change how effective gzip / zstd can compress the stream at a high level.

Just looking at the gzip compression only.

| Type | Data Length (32-bit tiles) | Data Length (8-bit tiles) |
| -- | -- | -- |
| Original Ordering, gzip -9    | 146191 (46.4%) | **94440 (29.9%)** | 
| Reordered, gzip -9            | **119046 (37.7%)** | 114779 (36.4%) |

💭I have no explaination for this... Well, what I can say is at least with *my* compression algorithms, the left-to-right, top-to-bottom, frame-at-a-time ordering provides the most compression by a reasonable clip.

I have to stress that both the original data and reordered data expresses the **exact** same data, it is just stored in `[frame][y][x]` verses `[y][x][frame]`.

I included both zstd and zlib here to illustrate that the best compression mechanism is content-sensitive.  From what I've found, gzip compresses better in areas where data is not particularly verbose, and zstd compresses better if your data is a bread sandwich.  If speed is a concern and your data sets are reasonably sized, zstd is a safe bet, for the 32-bit data, gzip took 45ms, and zstd took about 8ms, zstd being over 5x faster (For larger data sets, it worked out to be as much as 20x faster).

## Huffman Coding

If you are trying to represent a stream of one of say 4 possible values, to represent each value, you would need to use two bits for each possible outcome.  For instance, if we wanted to use the following for values, we could use this table to encode the data:

If our data was 32 symbols long as follows:

`ddddbddddcddcdddddcdddddaddddccc`

We could encode the data in ASCII, and it would take up 32 bytes (256 bits).  We could just store a string.  But we can do better.

We could represent it using the minimum number of binary digits.

Symbol | Bitstream
--- | ---
a | 00
b | 01
c | 10
d | 11

Then we could encode it as the following binary stream:

`11111111011111111110111110111111111110111111111100111111`

So, to represent 32 values, we would need to use 64 bits. 

But, Huffman realized we could do better.  By looking at how frequently we use each bit, we could use less data.

Symbol | Frequency
 --- | ---
a | 1
b | 1
c | 6
d | 24

If we could somehow make encoding a 3 use less bits than a 0, we could save space because there are so few 0's in the input data stream. The way we do this is we scan our table for the two most fewly used symbols, and combine them to make a new symbol.

And we update our symbol frequency table.

Symbol | Frequency
 --- | ---
 (a or b) | 2
 c | 6
 d | 24

We then re-scan the table, seaching for the next two fewest used symbols (or groups of symbols).  In this case, we combine 2 and (0 or 1).

So our new table is:

Symbol | Frequency
 --- | ---
 (a or b) or c | 8
 d | 24

And we repeat until we only have one entry in our table that contains all symbols.

Symbol | Frequency
 --- | ---
 ((a or b) or c) or d | 32

This can be redrawn as a tree.

```
       root
    /0      \1
 (a|b)|c     d
```

```
        root
     /0      \1
  (a|b)|c     d
  /0   \1
 (a|b)   c
 /0 \1
a    b 
```

By looking at this tree, we can make a bit of a table to let us know how we can encode a, b, c and d.

Symbol | Encoding
 --- | ---
 a     | 000
 b     | 001
 c     | 01
 d     | 1

Any time we see an `a` we emit `000`, any time we see a `b`, emit `001` any time we see a `c`, emit `01` and any time we see a d, simply emit `1`.

So, to encode our original stream, `ddddbddddcddcdddddcdddddaddddccc`, we could do it as follows: `1111000111110111011111101111110001111010101` or only 43 bits long, a 32% savings over our regular binary encoding, and an 83% savings over raw ASCII encoding!  

But admittedly, you do need to store the tree somehow. For small amounts of data like 32 bytes, it's likely this would eat any of the possible savings.  But for larger data sets, the size of the tree is miniscule compared to the data.

Huffman coding can be very fast and extremely simple to decode, since you just need to encode a tree... and it's provably optimal.  In that for fixedindiviual symbols, you can mathematically prove that it the most compressed you can make a data stream.  And for most of my life, I thought that's where the story stopped.  The thing is... It turns out there's a different theoretical limit.  I hadn't learned about until my friend Lyuma brought to my attention arithematic coding. Which could approach this other theoretical limit.

In the companion simulator for badder apple, you can see the huffman trees that are used for decoding all the notes, and their lengths, and time to next note in the song. Anywhere there's an uptick in the scrub bar, you can click on it then watch as the huffman tree is navigated.  Starting at the center of the tree, each bit is read from the input stream and if it's a 1 it goes down the white edge, if it's a 0 it goes down the black edge.  Once a leaf node is detected, the traversal stops.

![Huffman Tree Explainer](https://github.com/user-attachments/assets/29c49e9e-5aa0-49cf-a104-e5b403939f8e)

Additionally, if you would like to use huffman trees, you can use the [header-only huffman tree library](https://github.com/cnlohr/cntools/blob/master/hufftreegen_sf/hufftreegen.h) I used in this project.  It is just `hufftreegen.h` where you start with a list of values you want to encode.  For each value, you can call `HuffmanAppendHelper`.  This generates a table with the different symbols to be used, and their frequencies.  Then, you can call `GenerateHuffmanTree` which will use those frequencies to build a tree, which provides you the table to do bitstream decoding, as well as what's needed for `GenPairTable` To provide what's needed for encoding a bitstream.

The output from the sample program found at the top of hufftreegen.h is as follows:

```
5 unique elements
Tree has 9 entries (0x579aa1cc36f0)
 Index: Frequency -> If 0, jump to, If 1 jump to (Or terminal)
  0: 1000000 -> if 0 goto 1 / if 1 goto 4
  1: 296250 -> if 0 goto 6 / if 1 goto 2
  2: 184679 -> if 0 goto 3 / if 1 goto 5
  3:  73636 -> if 0 goto 8 / if 1 goto 7
  4: 703750 -> TERMINAL (Emit 0)
  5: 111043 -> TERMINAL (Emit 2)
  6: 111571 -> TERMINAL (Emit 4)
  7:  36877 -> TERMINAL (Emit 8)
  8:  36759 -> TERMINAL (Emit 1)
Encode 5 entries
 Index: Value: Frequency -> Bitstream to encode this symbol
  0:   4: 111571 -> 00
  1:   1:  36759 -> 0100
  2:   8:  36877 -> 0101
  3:   2: 111043 -> 011
  4:   0: 703750 -> 1
Total Elements: 1000000, Stored as 8-bit values totaling 8000000 bits
Compressed data stream: 1554565 bits (19.43% of original)
Validated OK
```

You can see that the most popular value (0) only needs one bit to define it, a single 1.  But if there's a prefix of 0, then it could be one of the other values.

A key portion of this is the simplicity of the decode.  You'll notice the decode table at the top can be easily bit packed into a concise table.  The frequency isn't needed, only the "this element is a terminal element or a go-to element", a "where-to-go-if-0/1" and "value."  Because the where-to-go and value are mutually exclusive they can overlap in the data they take up.  Additionally because the table can only ever look forward, you can encode the table as deltas.  I.e. Skip forward _this_ amount if a 0 or 1.

We use these tables to perform the huffman decoding in espbadapple.

### Huffman limitations

Because the huffman tree requires whole bits to decide which way to take in a tree, we are leaving something on the table. Imagine if the optimal entropy would result in say 2.5 bits should get spent on a symbol.  Huffman would need to either spend 2 or 3 bits.  Even though 2.5 bits is possible and could compress better.

## Arithmatic coding

We don't use arithmatic coding, but it is an important mention because it is what seeded the ground to alert mathematicians that there could be something **better than huffman**. Arithmatic coding is a mechanism to lay out every possible symbol from 0 to 1, with the ratios of how likely each symbol is.  Then, we can, bit-at-a-time partition that space to become more and more narrow. 

The general idea is you can split your space up into all of the symbols you want to represent. Then, figure out what percentageg chance each symbol is, and then draw a line back through the list of symbols. Each time, shrinking (or expanding) the space from (or to) the next symbol.

Then record the value that makes it so if you traversed the other direction, you would pass "through" each symbol. (See black line below).

![Arithmatic coding](https://github.com/user-attachments/assets/50e75aaf-ab30-49a3-9423-04b060c13248)

You can imagine if the ratios between the chances for `A`, `B` and `C` were the same you could theoretically hit the true limit of entropy with your compression, being no longer held into rigid minimums like huffman where you needed to spend a whole bit on a symbol output minimum.

There is nothing that says the chances need to remain fixed for each of your symbols from one stage to the next, they could be all different symbols, but, the critical part is you have to know the chance that a given symbol will be A, B or C at each stage.  The encoder and decoder must agree on that chance.

Raw Arithmatic coding is quite annoying to deal with, because the numbers get very s̸̤̺̲͂͛͑̍̉m̶̹̪͙̔̈͑a̴̰̾͋͒̉̔ļ̸̘̝̯͐̉ļ̷͕̖̬̻́̾͊̌ ̷̮̿a̸̻͋̍͗̃ͅn̴̪̺̒̀̒̆͜ḓ̴̹̣̙̳͉͒ ̸̡̟͚͕͉͓͌̎̿͒̒̈́e̸͎̳̎́̉v̸͓̭̟̯͑̆̌́̂͝i̴̛̭̓̃͒͛̈́l̸͓̞̫͍̜͊͂̈́͋ after a few symbols, but there's a special case of arithmatic coding called [range coding](https://en.wikipedia.org/wiki/Range_coding).  This is a simplification of the general ideal field of arithmatic coding, where you limit everything to two symbols a `0` and a `1`.

So instead of tracking many different symbols, and their chances, you only need to track 2.

This really is as optimal as described above. For instance, with `A`, `B` and `C`, if you map them to `00`, `01`, and `10`, there's no `11`.  So you can figure out the percentage of (`A` or `B` vs `C`) and if you get a `1` for the first bit, you know it's a 100% chance the next bit will be a `0` since there is no `11`.

## VPX (range) Coding

While arithmatic coding is particularly difficult to do in practice with bitstreams, there is a different take on it called range coding. This is the coding scheme implementation for range coding used in Google's VP7, VP8, VP9 video encoding engines to encode symbols. It maintains virtually all of the upsides of Arithmatic coding, and to show the effectiveness of range coding, we can compare the compression ratio to something aclled the "Expected Surprisal." Expected surprisal says the chance of getting a random upset given a series of heads/tails where there's a non-50/50 chance for each. The equation for this is:

```
-p*log₂(p)-q*log₂(q) where q = 1.0-p
```

This is the chance for an unexpected outcome.  And what's wild is it's also mathematically the same as the maximum compression ratio for bitstreams.  When we overlay this equation on actual performance of VPX coding bitstreams we get virtually the same performance.

![Optimal Compression Ratio](https://github.com/user-attachments/assets/02b9d48f-497c-4633-87b8-42a0e345aeaa)

VPX coding just exposes the ability to read/write 1 and 0 bits given a % chance of each.

I worked in the [web tool](https://cnvr.io/dump/badderapple.html) to vizualize how each bit in and each bit out are used.  In the image below you can see the squares indicate reading bits in, the X's indicate getting bits out. So you can see here it only took 3 bits to encode 8 bits of output data.

![VPX Coding Portion of demo](https://github.com/user-attachments/assets/248ca291-f478-44ab-8c4e-4db87a6a65b8)

The general idea is for every bit coming in, the decoder considers the `range` of possibilities left in the current value that's been decoded.  And it determines a `split` based on the probability of the outcome being a 0 or a 1 and that `range`. It also keeps track of a `value` sort of like a cursor.  If the current `value` is >= the `split` then the bit is a `1` otherwise it's a `0`.  If it is a `1` then the new `value` and `range` update from that `split`.  Then, if the `range` is now less than 1/2 the possible range, new data gets shifted into `value` to keep feeding the system.

In the image above, you can see the split, where gradually, the value is drained down because the split is so predominantly 1, and just before the second bit read, because the probability of it being a 1 is so much lower, it significantly reduces the porportion of value to range.

To speed things up, vpx coder keeps 24 (or 56 if 64-bit) bits worth of immediate to pull from, when this is drained, it reads another 3 (or 7) bytes from the buffer and refills it.

Whenever an unlikely path is taken, for instance, if a 0 is very unlikely, but is selected none the less, it can take several bits to encode that 0.  But when a likely path is taken, you can get bit after bit out without it consuming even one bit. You just have to know the chance on both the encoder and decoder so each side can make sense of the bitstream.

If you are curious to use vpx coding yourself for a project, for instance if you have a situation where you have a series of symbols, or numbers you want to express, or really, anything that can boil down to a bitstream of 0's and 1's, where you know how likely the next bit is to be a 0 or 1 is, you can use my project [vpxcoding](https://github.com/cnlohr/vpxcoding), that has an encoder, and a decoder.  And, the decoder is a header-only C library that works well for microcontrollers.  It's a little bit heavier than huffman to decode, but not by much!  To efficiently decode VPX you need a log₂ table, which takes up 256 bytes (could be RAM or ROM), and only 364 bytes of x86 code to decode.  And for a decoding context, it only takes a 33-byte structure in RAM.

You can see an example application below:
```c
#include <stdlib.h>
#include <stdio.h>

#define VPXCODING_READER
#define VPXCODING_WRITER
#define VPXCODING_NOTABLE
#include "vpxcoding.h"


#define NELEM 1000000

int data_to_compress[NELEM];
uint8_t compressed_data[NELEM];

// Stored as individual 1's and 0's for clarity.
uint8_t compressed_bitstream[NELEM*16];

int main()
{
	int i, j, k;

	// Generate a string of 0's and 1's, but mostly 0's
	for( i = 0; i < NELEM; i++ )
		data_to_compress[i] = ((rand()%12)==0) ? 1 : 0;

	int probability_of_0 = 256 - (int)( 1.0 * 255.0 / 12.0 );

	vpx_writer w;
	vpx_start_encode( &w, compressed_data, sizeof(compressed_data) );
	for( i = 0; i < NELEM; i++ )
		vpx_write( &w, data_to_compress[i], probability_of_0 );
	vpx_stop_encode( &w );

	int encode_length = w.pos;
	printf( "Compressed size: %d\n", encode_length );

	vpx_reader r;
	vpx_reader_init( &r, compressed_data, encode_length, 0, 0 );
	for( i = 0; i < NELEM; i++ )
		if( data_to_compress[i] != vpx_read( &r, probability_of_0 ) )
			printf( "Compression failed\n" );
	// No need to cleanup.

	printf( "Done\n" );

	return 0;
}
```

The one thing that's critical to remember here for later is that you have to figure out what the probability of each bit bing a 0 is, and, you have to make sure both the encoder and decoder know what that is so they can encode and decode properly.

The way that [vpxcoding](https://github.com/cnlohr/vpxcoding) represents that chance is the chance from 0 to 255 that the next symbol will be a 0. To denote this in this document, we will use ※.

| Symbol | ※ |
| --- | --- |
| `1` is definitely next bit | 0 |
| 50/50 chance for `0` or `1` next | 128 |
| `0` is definitely next bit | 255 |


We can graze theoretical limits with VPX coding, but there's still far more entropy that can be removed and compression that can be realized!  We must press on!

## LZSS/LZ77

Like RLE encoding mentioned earlier, [LZSS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski), Lempel–Ziv–Storer–Szymanski compression operates on whole symbols and is a type of compression where its goal is to find repeated segments of output data, and, instead of needing to re-encode that segment constantly, it can simply reference the earlier string, and with a length, simply repeat it in the current location.

![First 6 bars of my version of bad apple](https://github.com/user-attachments/assets/301533b0-5cee-4dbe-9438-0e49576cccb8)

Here's an example, using the first 6 bars of my version of the bad apple MIDI (rendered by rosegarden).  You can notice within the first 2 bars, there's a common section, so we only need to encode the first 7 notes, then we can reference the first 3 notes.  And then render the last note.

Then next, we can realize the next two bars are the same so we can say "just repeat the beginning, and keep going."  One clever trick of LZSS is that it is allowed to reference bytes that haven't been emitted yet, because it can read from references as it is writing new data! (This property is maintained by my later reverse-LZSS)

This is a common technique used in an enormous number of compression schemes to compress data because it is so effective. One of the major assumptions LZSS makes is that you have lots of memory, and can be tricky to implement.

## Heatshrink

There is an embedded compression tool called [heatshrink](https://github.com/atomicobject/heatshrink) that implements an exceedingly easy-to-decode version of LZSS where it uses fixed-size callback windows and lengths to encode references. One major assumption of heatshrink is that it assumes it can reference windows of decoded data, which means we need to keep decoded data in RAM.  For us, the RAM usage was a no-go.  Because we had at least some code to work with, we could do something a little more sophisticated.

## Exponential-Golomb coding

[Exponential-Golomb coding](https://en.wikipedia.org/wiki/Exponential-Golomb_coding) or Golomb encoding is a way of encoding numbers with variable numbers of bits. Much like huffman can, except, in situations where you don't have context, and numbers could be arbitrarily large.

It's used all over the place in the h.264 and h.265 video specifications to store numbers.  If you see `ue` as a type specifier in the spec, that means that's a Golomb encoded number.

```
 0₁₀ -> 1 -> 1
 1₁₀ -> 10 -> 010
 2₁₀ -> 11 -> 011
 3₁₀ -> 100 -> 00100
 4₁₀ -> 101 -> 00101
 5₁₀ -> 110 -> 00110
 6₁₀ -> 111 -> 00111
 7₁₀ -> 1000 -> 0001000
 8₁₀ -> 1001 -> 0001001
```

Note that the number of leading 0's indicates how many bits to read.  With each 0, increment the number of bits you will read. As soon as you hit a `1`, start subtracting from that number of bits until it's back at 0.

Because there's no way to naturally represent a 0₁₀, so Golomb coding shifts the read number down by 1.

While this can't have different compression depending on the prevealnce of each symbol, it does mean smaller numbers encode with less bits than bigger numbers.  The amount of bits needed to encode each new symbol is:

```
no of bits = log₂(n+1)*2+1
```

This offered a significant benefit over heatshrink for encoding the distance back and length of the run.

You also may note that it has more 0's than 1's, so there's somewhere else it's not optimal.  But overall, it's surprisingly effective.

Some basic code for reading Golomb coded is as follows:
```c
	int exp = 0;
	do
	{
		if( bit() ) break;
		exp++;
	} while( 1 );

	int br;
	int v = 1;
	for( br = 0; br < exp; br++ )
	{
		v = v << 1;
		v |= bit();
	}
	return v-1;
```

And encoding:
```c
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

// Return the number of bits needed to encode a number, i.e. 
// 0 = 0, 1 = 1, 2,3 = 2, 4,5,6,7 = 3
static inline int BitsForNumber( unsigned number )
{
	if( number == 0 ) return 0;
	number++;
	return 32 - __builtin_clz( number - 1 );
}
```

## Reverse-LZSS

Because we are heavily lacking in the RAM department, I turned LZSS upside-down, and instead of backtracking in the decoded stream, I decided to decode in the original data stream.  This _is_ less effective, because many of the references backwards in time do not align to useful bit boundaries, but overall, it is not that bad.

Instead of references being locations (byte-wise) in the output stream, this gives references, bit-wise in the input stream.

The idea is that each bit in the stream either indicates a literal note is being emitted, or, a refernce to somewhere earlier in the stream is being created. When a reference is created, it pushes the current decode location and number of notes to decode onto a stack, and the encoder begins decoding the incoming bitstream starting at that previous reference.

You can see what this looks like in practice, when you see a sustained hop-up in the scrubber, you can click in and watch as not just notes are pulled off but reference.  Where it first reads the number of 0's for how-far-back-to-jump, its content as a number then the length-of-that-jump's 0's, and the content, to get the information to backtrack.

The small cursors that scan from left to right show how many notes remain within the current stack location, and how far (number-of-bits-wise) the track is done playing.

![Reverse LZSS Backtrack](https://github.com/user-attachments/assets/b4f2a4d6-df65-4afb-9f7d-9c185df17192)

The way we can create backreferences can be varied.  In our case, we use Golomb-exponential coding.  Once for the run-length to go back and play.  As well as another Golomb-exponential coded variable for how far to look back.

Because it doesn't make sense to reference very small references, and it doesn't make sense to back-reference very small run-lengths, we can define minimums for both of these numbers. That way, to encode say 4 run length, if our minimum run length is 4, we only have to encode the number 0, which takes less bits to encode.

💭I am finding suspicious patterns surrounding minimum run length / minium callback distance.  Should look into that.

# Song

With all of the ground compression schemes laid, we can now move into the first challenge in the compression.  The song.

## Using BadApple-mod.mid

`BadApple-mod.mid` originally has unknown origin for source but is a midi take on "Bad Apple!!" by Zun and was on Touhou. It was uploaded On 2015-03-01 by livingston26 and uploaded [here](https://musescore.com/user/1467236/scores/678091), but credit was given to cherry_berry17.  I was unable to find them.  Then it was transcribed to MIDI.

I heavily modified it to fit the music video more specifically and also match the restricted hardware situation I am operating in.  Then later, @binary1230 added a percussion track.

If anyone knows the original author this passed through, I'd really appreciate knowing. But, it's so heavily modified, I don't think it would be recognizable.

In general this is a cover of the "Bad Apple" version by Alstroemeria Records featuring nomico.  And depending on jurisdiction, they may maintain some form of copyright.  I disclaim all copyright from the song and midi file.  Feel free to treat my transformation of it under public domain.

## Audio Compression Results

To do sanity checks, I decided to compare the compression a few steps along the way.  The compression test dataset for the .dat is the `fmraw.dat` generated file which contains a binary encoding for note, delta time and length.  (All sizes in octets (bytes))

Percentages are compared to my custom binary encoding.

| Compression | .xml | .json (jq formatted) | .json.min | .bson | .mid | .dat | 
| -- | -- | -- | -- | -- | -- | -- |
| uncompressed | 217686 (5698%) | 150802 (3947%)| 85855 (2247%) | 88677 (2321%) | 17707 (463%) | **3820 (100%)** |
| [heatshrink](https://github.com/atomicobject/heatshrink) | 32497 (850%) | 23376 (611%) | 15644 (409%) | 19192 (502%) | 3199 (83%) | 1060 (27%) |
| gzip -9      | 12001 (314%) | 11458 (300%) | 10590 (277%) | 13720 (359%) | 1329 (34%) | 682 (17.8%) |
| zstd -9      | 10052 (263%) | 9480 (248%) | 8877 (232%) | 12182 (319%)  | 1332 (35%) | 724 (19.0%) |
| bzip2 -9     | 8930 (234%) | 8800 (230%) | 8676 (227%) | 11282 (295%) | 1442 (37%) | 830 (21.7%) |

There's an issue, all of the good ones in this list these are state of the art algorithms requiring a pretty serious OS to decode.  What if we only wan to run on a microcontroller?

#### Observations

1. For small payloads, it looks like gzip outperforms zstd, in spite of zstd having 40 years to improve over it. This is not a fluke, and has been true for many of my test song datasets.
2. For larger payloads, bzip2 seems to outperform zstd. In this test, the only place that zstd really shines in the speed by which it can uncompress.
3. I expected BSON to save space, but it was larger, and compressed worse.  I guess it's only useful for increasing the speed of parsing.
4. Finding a way to express your data more concisely absolutely obliterates any compression algorithms you can throw at your problem. Don't settle for a binary representation, like bson.


### My algorithms

| Compression | Size |
| -- | -- |
| Raw .dat | 3820 (100%) |
| Huffman (1 table) | 1644 (43%) |
| Huffman (3 table) | 1516 (40%) |
| VPX (no LZSS) | 1680 (44%) |
| VPX (forward LZSS) | **665 (17.4%)** |
| VPX (reverse LZSS) | 673 (17.6%) |
| Huffman (2-table+LZSS) | 704 (18.4%) |
| Huffman (2 table+reverse LZSS)† | 856 (22.4%) |
| gzip -9 (for reference) | 682 |
| zstd -9 (for reference) | 724 |

† we used Huffman (2-table+LZSS) on the final project.  See rationale below.

Note: these tests were generated with `make sizecomp` the code for several of these tests is in the `attic/` folder.

Because we are only storing a stack, we only need to save the current location and number of notes remaining, so with 18 as the deepest we can go, our state size is only 72 bytes!

### More Observations

Note, when not using lzss, the uptick in size because to use VPX, you have to have a probability table, and huffman tables can be used in lower compression arenas to more effectivity.

So, not only is our decoder only about 50 lines of code, significantly simpler than any of the big boy compression algorithms... It can even beat every one of our big boy compressors!

This VPX solution perform VPX coding on the notes, note-lengths, and time between notes.  But it **also** perform vpx coding on the LZSS callbacks.

I decided to go back to huffman, mostly for the sake of the video and visualization! It also gave me a chance to express Exponential-Golomb coding.  Huffman is extremely simple to decode, and could have been done without any header libraries.  Even though the vpx code is available by virtue of the video, I wanted to show what it would look like with huffman.

To compare apples-to-relatively-apples, I decided to do a huffman approach, with LZSS backtracking using Exponential-Golomb coding.  It was 856 bytes, just a little less compressed, compared to 665 bytes for the VPX + Forward LZSS.

Because I was worried about RAM space, I decided to use reveres LZSS instead. Compression using reverse lzss is quite costly, because it simulates emitting the bits the whole way along.  I was happy to give up ~150 bytes of storage in exchange for a massive RAM savings.

### Actual Huffman (2 table+reverse LZSS) implementation

Every note contains 3 pieces of information:
 * The note pitch (between 47 and 80), starting at note A110 + (2 notes, so B# in that octave)
 * The length of the note length (How long the note should play)
 * The note run (or time until another note should be read)

the Note Pitch is stored in one huffman tree, and, length+run is stored in a separate huffman tree.

As described above, backreferences are stored as two different Golomb-Exponential coded values, the first, how many notes to read from the back reference, and second, the distance to look back.  For each of these, add on the minimums.

Times are in 1/16th notes (or 1/8 notes depending on meter definition).

When reading a thing (we don't know what it is yet) if the next bit is a 0, it's a note.  If it's a 1, then it's a backreference.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/0277a214-2177-40ea-9ed1-30c37e11382a>
</P>

The text of the song is as follows:

```c
#define ESPBADAPPLE_SONG_MINRL 3
#define ESPBADAPPLE_SONG_OFFSET_MINIMUM 3
#define ESPBADAPPLE_SONG_MAX_BACK_DEPTH 16
#define ESPBADAPPLE_SONG_MAX_BACK_DEPTH 16
#define ESPBADAPPLE_SONG_HIGHEST_NOTE 33
#define ESPBADAPPLE_SONG_LOWEST_NOTE 0
#define ESPBADAPPLE_SONG_LENGTH 1910

BAS_DECORATOR uint16_t espbadapple_song_huffnote[30] = {
	0x0001, 0x0102, 0x02a1, 0x0203, 0x0304, 0x0405, 0x0506, 0x0693, 0x0607, 0x9007, 0x848b, 0x9506,
	0x9789, 0x0592, 0x8305, 0x8206, 0x8791, 0x0305, 0x058e, 0x8098, 0x049a, 0x9604, 0x8a9c, 0x0485,
	0x028c, 0x9b81, 0x8f9d, 0x9e94, 0x9f00, 0x88a0, };

BAS_DECORATOR uint16_t espbadapple_song_hufflen[20] = {
	0x0088, 0x0001, 0x8afa, 0x0098, 0x0001, 0x0102, 0x0203, 0xfc80, 0xb8f8, 0x9c8c, 0x0100, 0x0203,
	0x0300, 0x04fa, 0xf99f, 0xfcba, 0x0200, 0xdad8, 0x9abf, 0xe2e0, };

BAS_DECORATOR uint32_t espbadapple_song_data[] = {
	0x3b2f43b2, 0x2f43b2f4, 0x1d13143b, 0x2027c78a, 0x858764c1, 0x05438c0a, 0x30ec9c5e, 0x2fa1d97e,
	0xa3a00908, 0x647948c1, 0xca941987, 0x1d92a1bb, 0x0a9c1926, 0x0e7630e3, 0x34106f63, 0x324c3837,
	0x330e29d8, 0xc3730628, 0xea61bbd4, 0xcc3a641b, 0x4ef30ebb, 0x628330d9, 0x64986c37, 0x1000c130,
	0xd0660ecd, 0xc930761b, 0x730e24c0, 0x461c1bb0, 0x30e1fbb1, 0xb30e0d76, 0x70c97028, 0xe06fbb18,
	0x8398706f, 0xcc3724c1, 0xc370dea1, 0xdc7ef528, 0x1b94f530, 0xe877a866, 0x43bbc930, 0x1dde1987,
	0x17624c36, 0x28802b65, 0xddb00ac1, 0x6ef30e89, 0xd3bcc3a2, 0xc7ef41d8, 0x0e3a80ce, 0xa5d83823,
	0xec52be01, 0xd7c80326, 0x9461c1a7, 0x65878345, 0x3e1a9500, 0x0550730e, 0x18382018, 0x11d5006c,
	0xc325ea1c, 0xea50730d, 0xb2d0064d, 0x24c37283, 0xa1c13d43, 0xef2448c3, 0xbc324683, 0x261d283b,
	0x0649de19, 0xc1e181b0, 0x00902918, 0x01912510, 0x8033228c, 0xd8d09d21, 0x80326f41, 0x30dc2cb1,
	0x08bef50d, 0x53eed200, 0x34c3a16f, 0x1874fbbc, 0x0e8fdde5, 0x30e94ef3, 0x801b079f, 0x383bb08f,
	0x803ec08c, 0x261c090d, 0x000a940d, 0x602be621, 0x1661d3e0, 0x02102c1e, 0x82c3cbe8, 0x0bc7a7de,
	0xc3081166, 0x070b801d, 0x435c2031, 0x60f39187, 0x5c8c38c2, 0x299fc00e, 0x0015f30e, 0xf8012a72,
	0x6182c3c5, 0x4c264009, 0x7ef4ff73, 0x7a6b51dc, 0x397a16ff, 0xfee6bcaf, 0xde9afde9, 0x561bc53f,
	0xb89cee62, 0x3bcb89ac, 0x2ff7a737, 0xaf1e9f6b, 0x6796e5b4, 0x32f49500, 0x01f0d95e, 0x95e37abf,
	0xcde9a834, 0xe197139d, 0x79713561, 0x41596f07, 0x06f2403c, 0x7e4e6b94, 0x735f93e1, 0xb0f0fdcb,
	0xc53fde9a, 0xee60565b, 0xbd6cb89c, 0x8403ad5a, 0x06f4c001, 0x87a00024, 0x5730767c, 0x5bb35987,
	0xdc898b6d, 0x15df1305, 0xeb9862f4, 0xdb46ecd6, 0x05dac064, 0x6ec8c313, 0x2257a260, 0x7a0d4868,
	0x03729851, 0xe618bb23, 0x2ef412ba, 0xacc1b476, 0x201ad661, 0xdb6addb8, 0x0b5a0010, 0x00d3521a,
	0x7c034305, 0xdc0db558, 0x80289aac, 0x4c0a41d3, 0x8ac10032, 0x426184c0, 0x2117c260, 0xdd235980,
	0x748c0115, 0xd7304131, 0x0d865007, 0x12bda513, 0x8bbda61b, 0x14401c9d, 0x76d0ad56, 0x25886100,
	0x202cb200, 0xb6f600d3, 0x7a260da3, 0x6bc02925, 0x8014da3b, 0xedf00bb1, 0x22adc88a, 0xe9000934,
	0xb986b312, 0xda3b7222, 0x806a5034, 0x9a1156e4, 0xe4620050, 0x1e27d400, 0x0147e280, 0x10072158,
	0x30880157, 0x8029fa01, 0x17eb3063, 0xc01bc300, 0x944c2a34, 0x45a0857d, 0x04c304af, 0x54a54a85,
	0x54282d4a, 0x8a52a52a, 0x94a94a94, 0x2a52a522, 0x000014a5,  };
```

As mentioned above, we pull off one bit to tell if we are doing a callback or an actual note.  If it is an actual note, then we have to do our huffman tree decoding.

You can see the definition of two of our huffman trees. Let's look at

```c
BAS_DECORATOR uint16_t espbadapple_song_huffnote[30] = {
	0x0001, // If 1 bit, [0x00+1] Move forward 1 entry.  If 0 bit, [0x01+1] move forward 2 entries.
	0x0102, // If 1 bit, [0x01+1] Move forward 2 entries.  If 0 bit, [0x02+1] move forward 3 entries.
	0x02a1, // If 1 bit, [0x02+1] Move forward 3 entries.  If 0 bit, [0xa1] 0x80 | 33 (emit note 33)
	0x0203, // If 1 bit, [0x02+1] Move forward 3 entries.  If 0 bit, [0x03+1] move forward 4 entries.
	0x0304, // If 1 bit, [0x03+1] Move forward 4 entries.  If 0 bit, [0x04+1] move forward 5 entries.
	0x0405, // If 1 bit, [0x04+1] Move forward 5 entries.  If 0 bit, [0x05+1] move forward 6 entries.
	0x0506, // If 1 bit, [0x05+1] Move forward 6 entries.  If 0 bit, [0x06+1] move forward 7 entries.
	0x0693, // If 1 bit, [0x06+1] Move forward 7 entries.  If 0 bit, [0x93] 0x80 | 10 (emit note 13)
	0x0607, // If 1 bit, [0x06+1] Move forward 7 entries.  If 0 bit, [0x06+1] move forward 8 entries.
	0x9007, // If 1 bit, [0x90] 0x80 | 16 (emit note 16).  If 0 bit, [0x07+1] move forward 8 entries.
...
};
```

The general idea is when reading a note, start at the first element in the table, then follow the most significant byte if you see a 1, or the least significant byte if you have a 0.  If the most significant bit is 0x80, then you have a terminal, and you can read the literal value out.

You can imagine just how small, fast and simple it is to read some bits!

```c
uint8_t PullHuffmanByte()
{
	int ofs = 0;
	do
	{
		uint16_t he = htree[ofs];
		int bit = ReadNextBit();

		// Select top or bottom byte
		he>>=bit*8;

		// Check if it's terminal
		if( he & 0x80 )
			return he & 0x7f;

		// Mask off top bits and update new search place.
		he &= 0xff;
		ofs = he + 1 + ofs;
	} while( 1 );
}
```

That's about it for decoding the huffman coded data with reverse LZSS.

## Synthesizing

Once we have the notes and noise decoded, we need to play.  The synthesizer is made out of [triangle waves](https://en.wikipedia.org/wiki/Triangle_wave) for the main notes and a [LFSR](https://en.wikipedia.org/wiki/Linear-feedback_shift_register) noise for the percussion. 

We really don't have very much CPU to do the audio decoding, so we can't do anything particularly complicated, and we can't use lookup tables to offset the lack of processing because we are so limited on storage.  Wavetables, [FM Synthesis](https://en.wikipedia.org/wiki/Frequency_modulation_synthesis), and other complicated synthesizer systems are infeasible.  

One reason I leaned toward the triangle wave is because it has appealing overtones.  While it doesn't have all the even harmonics of stringed instruments, it does have all the odd harmonics, each harmonic of significantly decreasing value so it doesn't have an angry sound and is more soothing.  Having the odd harmonics helps music written for normal stringed instruments "sound right" when played with the original song since it was authored with the idea of using normal chords and harmonies.

![Triangle Wave Spectrogram with Oscilloscope output](https://github.com/user-attachments/assets/49a9e6d1-fb88-42aa-8890-e024e23a83e1).

There's some artisic decisions here.  While it's not particularly difficult to always start at zero when starting and stopping to avoid a bit of a broad spectrum "pop" when the note starts playing, having that pop gives a nice sense of variety, almost like a "pluck" of a guitar string.  And at the end sometimes things line up to zero cross, and othertimes not.  This helps give the song some variety and sound less robotic.

With a little more effort and free CPU we could have made the musical instrument more interesting, adding attack/deckay/sustain, or more sophisticated harmonics, like interleving the 2nd harmonic over the first harmonic to get all stringed instrument harmonics -- or even considering what the start and end of the note should look like but, this is where I left it.  There is lots of room here for improvement, especially if additional CPU is saved elsewhere in the final product.

Next, we want to layer on another "instrument."  Much like the [SID](https://en.wikipedia.org/wiki/MOS_Technology_6581) or [POKEY](https://en.wikipedia.org/wiki/POKEY) chips used for audio synthesis in older systems, we opted to use a LFSR to create the broad-spectrum noise for percussion. LFSRs are particularly easy to construct both in hardware, and software. 

![LFSR Schematic](https://github.com/user-attachments/assets/7911ebb1-1eb5-4ee5-bd32-9312d216200d)
<P SIZE=-2 ALIGN=CENTER>LFSR Diagram from here: https://en.wikipedia.org/wiki/Linear-feedback_shift_register#/media/File:LFSR-F16.svg (CC0, KCAuXy4p - Own work)</P>

It can be handled pretty simply in the sound system using the following code in `ba_play_audio.h`. 

```c
int ntr = player->noisetremain;
if( ntr > 0 )
{
	int l = player->noiselfsr;
	// Only update LFSR every 4th sample to avoid very high frequency stuff
	if( ( outbufferhead & 3 ) == 0 )
	{
		int bit = ((l >> 0) ^ (l >> 2) ^ (l >> 3) ^ (l >> 5)) & 1u;
		l = player->noiselfsr = (l>>1) | (bit<<15);
		player->noisetremain = ntr - 32;
		if( ntr > 2048 ) ntr = 2048;
		player->noisesum = (l * ntr)>>14;
	}
	sample += player->noisesum;
}
```

When you place both the note and the percussion on top of each other, it's a lot like mixing other notes together.

![Triangle wave with percussion and oscilloscope view](https://github.com/user-attachments/assets/eaaba71e-3e93-42e8-9b9e-f5bfe0563aa0)

## CH32V006 implementation

In order to output the audio on the final chip, we use DMA-fed-PWM outputs. 

```c
volatile uint8_t out_buffer_data[AUDIO_BUFFER_SIZE];
```
...
```c
AFIO->PCFR1 = AFIO_PCFR1_TIM2_RM_0 | AFIO_PCFR1_TIM2_RM_1 | AFIO_PCFR1_TIM2_RM_2;

TIM2->PSC = 0x0001;
TIM2->ATRLR = 255; // Confirmed: 255 here = PWM period of 256.

// for channel 1 and 2, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
TIM2->CHCTLR1 = 
	TIM2_CHCTLR1_OC1M_2 | TIM2_CHCTLR1_OC1M_1 | TIM2_CHCTLR1_OC1PE |
	TIM2_CHCTLR1_OC2M_2 | TIM2_CHCTLR1_OC2M_1 | TIM2_CHCTLR1_OC2PE;

// Enable Channel outputs, set default state (based on TIM2_DEFAULT)
TIM2->CCER = TIM2_CCER_CC1E // | (TIM_CC1P & TIM2_DEFAULT);
           | TIM2_CCER_CC2E;// | (TIM_CC2P & TIM2_DEFAULT);

// initialize counter
TIM2->SWEVGR = TIM2_SWEVGR_UG | TIM2_SWEVGR_TG;
TIM2->DMAINTENR = TIM1_DMAINTENR_TDE | TIM1_DMAINTENR_UDE;

// CTLR1: default is up, events generated, edge align
// enable auto-reload of preload
TIM2->CTLR1 = TIM2_CTLR1_ARPE | TIM2_CTLR1_CEN;

// Enable TIM2

TIM2->CH1CVR = 128;
TIM2->CH2CVR = 128; 
```
...

```c
// Be sure to fill the buffer.

DMA1_Channel2->CNTR = AUDIO_BUFFER_SIZE;
DMA1_Channel2->MADDR = (uint32_t)out_buffer_data;
DMA1_Channel2->PADDR = (uint32_t)&TIM2->CH2CVR; // This is the output register for out buffer.
DMA1_Channel2->CFGR = 
	DMA_CFGR1_DIR |                      // MEM2PERIPHERAL
	DMA_CFGR1_PL_0 |                     // Med priority.
	0 |                                  // 8-bit memory
	DMA_CFGR1_PSIZE_0 |                  // 16-bit peripheral  XXX TRICKY XXX You MUST do this when writing to a timer.
	DMA_CFGR1_MINC |                     // Increase memory.
	DMA_CFGR1_CIRC |                     // Circular mode.
	DMA_CFGR1_EN;                        // Enable
```

What this sets up is a system that runs the timer at 24MHz (`Fcpu` = 48MHz, `TIM2->PSC` = 2).  And counts up to 256.  This makes our `Fsps` = 93750 samples per second.  By running at a very high frequency, it helps cover up the quality loss being only an 8-bit output, and to cover for the fact that we are not being careful about how we decide the values of the samples in time, so we can use this as a sort of antialiasing by oversampling and letting the system and human ear work together to cover it up.

Once the DMA setup code is called, the DMA system starts reading data out of `out_buffer_data`, one sample at a time once the timer reloads.

We don't use interrupts to do the playback, but instead we can see "where" in out_buffer_data the current sample was read from.  That we we can fill more samples into our buffer.  You can treat `out_buffer_data` like a circular buffer, where we keep track of our "head" and we treat the location in the buffer as a "tail."

```c
int v = AUDIO_BUFFER_SIZE - DMA1_Channel2->CNTR - 1;
if( v < 0 ) v = 0; // There is a "race condition" at DMA1_Channel2->CNTR == AUDIO_BUFFER_SIZE
ba_audio_fill_buffer( out_buffer_data, v );
```
...
```c
int outbufferhead = player->outbufferhead;
if( outbufferhead == outbuffertail ) return 1;
while( outbufferhead != outbuffertail )
{
	// Compute "sample"

	outbuffer[outbufferhead] = (volatile uint32_t)((sample >> (1+8)));

	outbufferhead = ( outbufferhead + 1 ) & ( AUDIO_BUFFER_SIZE - 1);
}
player->outbufferhead = outbufferhead;
```

![Circular buffer diagram](https://github.com/user-attachments/assets/df319c05-4ba7-4c7f-b0e2-163007328754)

## Output

![Diagram](https://github.com/user-attachments/assets/e7d4237c-7c90-40b8-9ee5-97ad38611382)

The output is accomplished simply with a PWM signal, but, with a little bit of filtering.  First, to remove the DC offset, otherwise the ground loop isolator, or whatever is downstream will be very unhappy to receive a large DC offset in the signal.

Then, that is output on the SBU pin of the USB type C connector, so it's posible to use an adapter to USB->Headphone adapter with power injection.  This means that through the same connector we can provide power to the badder apple playback unit, as well as getting the audio out to send to the rest of the system.

The filtered output (from a slightly different circuit, with the DC offset removal on the other side) looks like this.  While there's still a little bit of the PWM signal left, it's not much.

![Real Waveform](https://github.com/user-attachments/assets/b346f0c6-9f84-4f6e-be52-2d43a9421293)

## Testing

To produce the audio file for use with ffmpeg, `track-float-48000.dat`, as well as producing `badapple_song.h` in the `playback/` folder, you can use the following command:

```
make track-float-48000.dat ../playback/badapple_song.h
```

The .dat file can be used with ffmpeg as follows:

```
ffmpeg -y -f f32le -ar 48000 -ac 1 -i ../song/track-float-48000.dat <video data> <output.mp4>
```

The .h file is used with the playback system by being compiled in.

### Interprets `BadApple-mod.mid` and outputs `fmraw.dat` using `midiexport`

If you want to play with your own compression you can use this tooling, just take note of the following.

Midiexport reads in the .mid file and converts the notes into a series of note hits that has both the note to hit, the length of time the note should play (in 16th notes) and how many 16th notes between the playing of this note and the start of the next note.

This intermediate format is a series of `uint16_t`'s, where the note information is stored as:

```c
int note = (s>>8)&0xff;
int len = (s>>3)&0x1f;
int run = (s)&0x07;
```

Assuming a little endian system.

# Video

You might want to take a break, nap or go to bed.  It was nice wearing the arm floaties but we're about to jump into the deep end.

## Glyphs

The key innovation that really kicked this project into high gear was the aforementioned [Onslaught - Bad Apple 64 - C64 Demo](https://www.youtube.com/watch?v=OsDy-4L6-tQ). Instead of trying to cleverly encode every pixel, or every other pixel such as the case with [Timendus's bad apple](https://github.com/Timendus/chip-8-bad-apple), we can encode the data as "glyphs" (or just 8x8 groups of pixels)  Sort of how people in the 1970's would use [PETSCII](https://en.wikipedia.org/wiki/PETSCII) to draw something like ascii art, but with symbols that were specifically selected to aid in drawing.  Then each frame is stitched together with these tiles.  Instead of needing to keep track of all 64x48 (3072) pixels, we only need to keep track of 6x8 (48) tiles.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/593ef5a3-15a1-4c89-bf95-ad5de7d06474 WIDTH=50%>
</P>

Originally, I started with an incredibly laborous mechanism where I would create a large corpus of glyphs, then, try to find out which ones looked most similar to other glyphs.  Starting with a corpus of 100,000 glyphs or so and winnowing this back, trying to make the tiles in the stream match the glyphs and keeping track of how many times each glyph was used, culling the least used glyphs, iteratively, back and forth many times.  This process was time consuming and the output was pretty rough.  The following is one run I was able to get the size down to 75kB with, with 260 glyphs.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/49444434-3b9b-4b25-a9f3-07782b9a481d WIDTH=50%>
</P>

You might be thinking this process would be terribly slow.  And it certainly wasn't fast.  Because it had to check through 300,000 tiles for larger tilesets for every single tile of input.  There was a trick.

At this time, I thought I might try to keep everything black and white and use spatial dithering to provide a grey feel.  There was a neat trick I found.  

I could represent all the tiles as `uint64_t` numbers.  As in each bit in the 8x8 image was a bit in a number.  Then to see if tiles were identical, I could simply compare the `uint64_t`'s.  To determine how similar two tiles were, for instance if I wanted to find a "best match" I could just take two `uint64_t`'s and `xor` them together, then do a [popcount](https://github.com/hcs0/Hackers-Delight/blob/master/pop.c.txt) (or count number of 1's) to get the number of pixels that differed. 

You can see the final tiles output.  It's able to find a couple really key tiles, like black, white, half black, half white, and various shapes, but, then it kind of loses its mind going into tiles that are barely used at all.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/2e38a22f-12b5-4b2e-8c84-0a9fb218bb70 WIDTH=50%>
</P>

This process was really clunky, and left me frustrated, spending a lot of time nursing it along.  In addition, this has no facility to support greyscale, and that is something I would later find to be critical.

### Onslaught (C64 Demo) Approach

I should have asked Onslaught much earlier how they did their approach.  They replied that they used a similar approach to what they used in their [Sabrina](https://www.youtube.com/watch?v=udkTv1nC6W0), [Wilde](https://www.pouet.net/prod.php?which=76420) and [Easybananaflashrama](https://www.pouet.net/prod.php?which=74913) demos, which use a system they call "TileVQ."

While their reply was cryptic, and they did not reply to any follow-up emails, as best as I can understand, they did a two-stage tiling process.  Each section of the video uses groups of 2x2 chars across image, 4 bytes per group, where the individual chars were using VQ (presumably Vector Quantization) to create the pixels for all the individual chars.

Vector Quantization is a general approach to K-means (described below)...  Upon reviewing literateure and their email I was unable to determine any functional difference between their approach and K-means, other than it's possible they used a non-euclidean distance function for their VQ classification.

Either way, if I had sent that email beforehand, it would have saved me a very large amount of time.  I strongly encourage people to reach out to others who are working in a given space for advise before spending copious amounts of time.

## K-Means

Frustrated, I was visiting my brother in early 2024, and I started explaining my problem and both him and his wife were like "why didn't k-means work?" and I proceeded to explain the problem "why didn't k-means work?" then when I was done they asked "You didn't try k-means, did you?"

...no?

They spent about 20 minutes convincing me, I'd argue "but I have 64 dimensions, one for each pixel" they pointed out that has no issue at all.  After various other defensive arguments for something that was a clear lack of knowledge on my part. I decided to give it a shot.

K-means is performed by randomly distributing a series of points (or classes) over your data space, then:

1. For each point of your data, find the closest class.
2. For each point in a class, find its centroid.
3. Move the class center to that new centroid.
4. Repeat back to step 1 until you're happy.  

<P ALIGN=CENTER>
<IMG SRC=https://upload.wikimedia.org/wikipedia/commons/e/ea/K-means_convergence.gif WIDTH=50%>
</P>
<P ALIGN=CENTER>
(Credit and license from Wikipedia source <A HREF=https://commons.wikimedia.org/wiki/File:K-means_convergence.gif>here</A>.  (C) Chire, GNU Free Documentation License)
</P>

I did have some experience with K-means before, and it was like the above image, not a 64-dimension output. But it was shockingly straightforward. I just did exactly the same thing, but I started with completely random noise for my tiles.  I'd get rid of the least used tiles, slowly cutting back and refining my tileset, and iterating on the k-means approach until only the total number of tiles I wanted remained.  I could keep going but this provides the point.

![Progressive K-Means Tiles](https://github.com/user-attachments/assets/fd0854d9-fa0c-4256-bd90-4642020e2225)

Another happy outcome is this produces. Greyscale! Which totally came in useful later when I realized I could temporally dither my display.  Though it only gives me black, grey, and white.  

Three colors is still better than two!

## De-Blocking Filter

One thing you'll notice when looking at the preview output of random frames (see 2nd image from the top, on the left) is that the edges between tiles are very evident and ugly.  Well, there's one last trick here before we start trying to really squeeze things down.


I pulled this trick from H.264, which has a "deblocking filter" which blurs the outputs between the edges of the macroblocks.

The technique I used was to first compute the edge blurring across all the vertical lines, you can see the first blur candidates in red.  Then, after the whole image was done, I computed the horizontal blurs, in blue.

For the first blur operation, it bases all of the blocks off the original glyph data, that way, when it computes the left-edge of one cell, it doesn't corrupt the right-edge of the other cell.

It does the blurring for all vertical lines, left and right, first, and writes this data to an intermediate buffer. 

This buffer is then used for the horizontal-line blurring which does the same thing.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/64e2d526-784f-4060-9e9c-568d111055bb WIDTH=70%>
</P>

Note that P means the "previous" cell, T means "this" cell, and N means "next" cell. T and N may be interchangable and could be different on different implementations. If you are against a boarder, reflect the N back around to be equal to P, instead of wrapping to the other side of the image.  Doing this instead of not blurring lets the next technique do some really need things with the edges of blocks.

The transfer functiton, based on P, T and N were inspired by `output = (P+N)/2 + T - 0.5`, in that I wanted to most weight towards the current cell but take input from the left and right cells. But really, there was some push and shove, and we landed on the following table.

|     | P = 0 | P = 1 | P = 2 | 
| --- | --- | --- | --- |
| N = 0 | (0,0,1) | (0,1,2) | (1,1,2) |
| N = 1 | (0,1,2) | (1,1,2) | (1,2,2) |
| N = 2 | (1,1,2) | (1,2,2) | (1,2,2) |

Where inside each cell (T=0,T=1,T=2).

Using this kernel, it really helped seal the deal, and produced what I expected to be my final output. It was blurry. There were no stars, flashy motion was goopy, there were no peach blossoms, everything looked kinda lumpy, it wasn't perfect, but it would do ... for now.


<P FLOAT=LEFT>
<IMG SRC=https://github.com/user-attachments/assets/0421d0f6-18b5-4713-bebd-e1ad846a1f10 WIDTH=48%>
<IMG SRC=https://github.com/user-attachments/assets/d655cea7-16b1-482a-b62c-d1bc99004db9 WIDTH=48%>
</P>

Despite all of its flaws, it completely blew my mind that just bluring the one pixel border around each block together would make it that difficult to see the edges of the tiles!  Find the blocks on the left, now, note how you have to actually *try* to on the right.

That was until my friend Evan said "I think I can do better" -- "Better than k-means?!" I exclaimed. He pushed his glasses back indicating he meant business. "Yes."

I was not ready for what lay ahead.

# Machine Learning
Up until now, this has mostly followed Charles' general approach to projects like this - efficient compute, small memory footprint, quick iterations. 

I've got a GPU. What if we didn't care about any of that, and just threw a massive amount of compute at the problem instead? 

## Image Quality Metrics & Tiny Frames

Charles' K-means approach is based on a mean-absolute-error metric; for a given pair of images, he's computing the "distance" between them as the mean of absolute values of per-pixel differences. This is also known as a "L1 loss," and it's a good, [embarrasingly-parallel](https://en.wikipedia.org/wiki/Embarrassingly_parallel), quick-to-compute choice for a distance metric. 

The trouble is that this, and other per-pixel metrics (e.g. [PSNR](https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio)), don't really align with how humans see differences between images. We tend to care more about the structure and semantic content of images, versus specific pixel values. There's other more-complex metrics that try to take human perception into account by looking at edge contrast and colors ([SSIM](https://en.wikipedia.org/wiki/Structural_similarity_index_measure), [FLIP](https://developer.nvidia.com/blog/flip-a-difference-evaluator-for-alternating-images/)) - but these also tend to be reconstruction metrics, meant to compare between images of comparable resolutions. For a project like this one, where we're comparing a 64x48 3-color pixel-sprite rendering against a proper video, these losses are dominated by how we resample and quantize the images; even good resampling tends to lose a lot of important details. 

![Data Loss from Resampling and Quantizing](https://github.com/user-attachments/assets/65f0e7ef-8d88-4fa2-9212-2eba609e9903)

In the end, what we really want is to make something that's visually recognizeable as Bad Apple, even if that means taking some pixel-art-y artistic liberties. It would be nice if this frame had some stars in the background, for example, even if they'd never show up under any sane re-sampling method. 

For tasks like this, machine-learning-based semantic difference metrics, like [LPIPS](https://richzhang.github.io/PerceptualSimilarity/), tend to be a better fit.

## Learned Perceptual Similarity Metrics

The general idea with [LPIPS - "Learned Perceptual Image Patch Similarity"](https://richzhang.github.io/PerceptualSimilarity/) - is:
 * Take a neural network, pre-trained on an image recognition task with a big dataset;
 * Run your images through this network;
 * Pluck out intermediate values from the network at different layers, and compare _these_, instead of anything directly from the original image.

![LPIPS Diagram](https://github.com/user-attachments/assets/31e63848-1080-4bce-b0c2-495fa87f43db)

The intuition here is that the deeper you go in networks like these, the more these intermediate numbers tend to represent higher-level semantic concepts. As a useful distance metric, this checks all the boxes: it's zero if the images are the same, bigger if they're different, and symmetric (A vs B == B vs A), and you can propagate derivatives through it for optimization. The authors found that, compared to other metrics, LPIPS ranks differences between images much more like humans do - and that this seems to be an emergent property of many other vision models.

For a quick example: Here's a picture of my dog, corrupted in two ways: a single-pixel shift down and right, and a big blur over her face.
 * L1 loss says the shifted image is more different than the blurred one, even though a human would hardly see the difference.
 * LPIPS dislikes the blur more - better aligned with human perception.

![LPIPS vs L1](https://github.com/user-attachments/assets/7f0f764b-ce25-4f5d-b3d9-8275ee491381)

So: Now that we have a more human-like distance metric, all we have to do is formulate the entire problem of assembling a video from tiles, in a way that lets us optimize tiles & placement to minimize semantic loss.  

## Differentiable Video Rendering 

PyTorch is a natural choice here - not only for the neural-network losses, but also because we can use its built-in auto-diff and optimizers. If we can express video data (tiles + sequence data) as parameters, and do the whole job of assembling video using differentiable torch operations, we could directly optimize video data for our objective. 

We could assemble the video out of tiles with some indexing tricks (in Numpy below to illustrate):
```
tiles = np.fromfile("path/to/blocks.dat", dtype=np.float32).reshape(-1, 8, 8)  # Addressing: (tile_idx, row, column) -> pixel
tiles = (tiles * 255).astype(np.uint8)

stream = np.fromfile(stream_path, dtype=np.int32).reshape(-1, 48//8, 64//8)  # Addressing: (frame_idx, tile_row, tile_col) -> tile_idx

# Index into `tiles` using `stream` so that each stream is converted into an array of tiles.
assembled_tiles = tiles[stream]  # shape (s, h, w, th, tw)

# Rearrange the axes so that tile rows/columns are interleaved with grid rows/columns.
assembled_tiles = assembled_tiles.transpose(0, 1, 3, 2, 4) #  shape: (s, h, th, w, tw)

# Collapse the h/th and w/tw axes - shape (S, h*th, w*tw). This is the video. 
assembled_images = assembled_tiles.reshape(stream.shape[0], stream.shape[1] * 8, stream.shape[2] * 8)
```
However, this doesn't quite get us there - gradients could flow up into the tiles the index-by-sequence operation isn't differentiable. We'd really like a way for this optimization to swap out or rearrange tiles, if doing so can improve the video. 

For this, we're going to employ something called the Gumbel-Softmax trick. You can find a ![really good explanation of this here](https://sassafras13.github.io/GumbelSoftmax/) - the short version is that this provides a way of sampling from a discrete probability distribution, in a way that is differentiable through to both the samples and the distribution. We can express our sequence parameter as a big list of discrete probability distributions, each representing the _probability_ that a given tile is used in a given spot in a given frame; for training, we _sample_ tiles using these distributions (via the Gumbel-Softmax trick), then use these tiles to assemble frames from a video. Eventually, we expect this distribution to train towards one-hot, with a single tile being the obvious choice for a given spot. This is a bunch of parameters - 6570 frames * (6 * 8) slots/frame * 256 tiles/slot = 80,732,160 numbers to optimize - but for a modern GPU, this ~323MB of parameters is hardly anything to worry about. 

( TBD: Figure - Distributions, assembled images )

Once frames are assembled, we need to model Charles' deblocking filter, which proved to be a bit of a challenge to make efficient in a PyTorch/GPU sense - it's another very discrete operation (inputs quantized to 3 values), without obvious extension to float input or clear derivatives. I ended up with a 3D LUT much like the actual table, mapping (prev, current, next) -> (output value), and performing a trilinear lerp over this based on some strided/masked inputs. 

( TBD: Figures - Visualize trilinear LUT, masks )

With that, we now have rendered frames, differentiable all the way back to the tile/sequence data! Now we need something to optimize against. 

## Dataset & Resolutions

The "dataset" for this solve is just frames from the ground-truth Bad Apple video, marked with frame number. We can render a frame by looking up sequence data for that frame number, assemble it from the tiles, and compare this against a ground-truth frame. So, at training time, we yield random batches of these pairings. 

LPIPS loss is based on an ImageNet-trained VGG16, which had input frames at 224 x 224 px; our Bad Apple video is 64 x 48. VGG16 tends to work best closer to the trained scale, even if other resolutions will _numerically_ work with the convolutional stack. Experimentally, I found that 128 x 96 frames made for a decent trade between perf and comparison quality. Ground-truth frames are re-sampled down to this 128 x 96, and cached on the GPU; at training time, rendered 64 x 48 frames are re-sampled up to match. This also lets us compare our blocky rendered frames against something closer to the original video, with the hope being that finer details might still be "seen" by the semantic loss and represented in tiles. 

( TBD: Rendered and ground truth, at compared resolutions )

## Optical Flow 

There's another component of this which we haven't touched on much, which is _motion_. This is going to be a video, after all - humans tend to notice if motion looks shaky, flicker-y, or otherwise "looks wrong", even if individual paused frames seem fine. Motion also tends to draw our eyes' attention to particular areas of a frame, even if there aren't big bright semantically-interesting things there; for example, falling petals in Bad Apple are only really recognizable as such because they move. 

Bad Apple in general has little in the way of texture - but if masked to edges, it provides a decent signal to a ~[pyramidal Lucas-Kanade optical flow](https://en.wikipedia.org/wiki/Lucas%E2%80%93Kanade_method) estimator. L-K flow also contains only differentiable operations, so as far as Pytorch is concerned, it slots nicely into our current optimization: we compute optical flow fields on (batched sections of) the input video & our rendered video, then add up squared differences between these flow fields as another loss term.  

( TBD: L-K flow fields on ground truth sections ) 

## Optimization Results
TBD:
 * Optimization and tuning process, observations
 * Comparison & progress figures
 * Learned edge encodings under deblocking filter

## Lossless and Lossy Compression

There's two problems with this result:
1) It's _huge_ - something about this compresses very poorly; some runs end up over 80kB here, well past the 62kB budget. (Speculative: I bet we've "spread around" information in a way that, while better representing the target, is harder to compress.) 
3) There's some undesirable small flickering still present in some situations. Individual frames look fine, but it still draws attention in a bad way. (Speculative: Probably due to gumbel-softmax sampling / noisy gradients during the solve. Flow losses should penalize this, but perhaps not strongly enough to force a tile swap?) 

### "Left Align"-ing
One quirk of the deblocking filter is that, in certain cases / depending on neighboring tiles, you can swap out tiles without changing anything in the image. 

(TBD: Figure - examples) 

From the perspective of the optimizer, this is a don't-care; the image looks the same, so random choices are fine. From a compression standpoint, this isn't great: since lots of tiles can look the same as a blank one, we're fattening out the tails of tile-frequency histograms, which (on average) increases the number of bits needed to represent a given tile. We can also have cases where tiles in the video can change like crazy, for no visual benefit.

What we want is something that has no invisible transitions, and also pushes the distribution of tile frequencies to have a longer tail - moving weight to the left of a sorted histogram. 

So, as a post-processing step, we:
 * sort tiles by how frequently they're used in the video
 * try swapping out every tile with more-popular equivalents
 * check to make sure the result of the de-blocking filter didn't change.

This process is pretty quick; most tiles differ in ways that wouldn't be affected by deblocking, so in total there's only a handful of swaps we actually have to try. We run this process repeatedly until nothing changes.

( TBD: Figure - Tile distributions before and after left-align-ing )

Since there's zero visual impact, all the compression gains here are "free," in that we've not changed any tile intensities! 

### Skip Evaluation
At this stage, we're able to compress a _bit_ smaller, but we're still blowing our budget. We've also still got this odd flickering happening in some places. 

About all we can do at this stage is start to delete transitions.

(TODO: Semantic evaluation preprocess)
(TODO: Actual swap deletion and compression evaluation) 


# Actually compressing this monstrosity

While I spent months iterating on this, I'm only going to focus on the final solution.  Just know that while my first attempt was 80kB, it took all this extra work to get the final output to something that could fit on a ch32v006.

The underlying data we need to compress is only two files. 

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/e1ae4a73-202d-4cd8-bc4d-987f92d75e13>
</P>

The first was the "stream" it was just a list of what glyph to put into which tile of the video.  The second was the tiles (or glyphs) that we computed above.

## Compressing the glyphs

Let's first compress our glyphs. You can see the web demo doing this when it first starts, everything to the left of the start on the scrubber (the bottom circle) is it decompressing the glyph table that will be used.  You can also watch as each bit is pulled off the input stream, one at a time using vpx coding, to output to two bits per pixel.

![the glyphs](https://github.com/user-attachments/assets/70e1d704-b60a-4743-b634-8399be22045f)

While it's wonderful to say we can just take an input stream and VPX decompress the stream, the truth of the matter is much stickier.  Remember that pesky fact that to compress or decompress a stream, you have to know what the probability of each bit being a 1 or a 0 is.

**TODO** Explain why we can't have 4 greyscale levels **TODO** Slow-mo-video of successive frames.

We start by encoding our greyscale as **0**, **1**, or **3**.  This is somewhat arbitrary, and it could be other ways, but, it was just a convention I picked.  So the greyscale value of **2** does not exist.

We encode our data MSBit first.  Two bits per pixel.  Because there is no pixel color of **2**, if we see the first bit is a `1` then we know the next bit will be a `0`. Then, if we see the first bit is a `0`, well, there's a chance it could be 210※ chance that the next bit will be a `0`, because there are many more black pixels than grey pixels.

For the rest of this document, I will use the ※ symbol to indicate a ratio, like a per-cent but instead of per-100, it will indicate per-256 chance that a given bit will be a `0` bit as oposed to a `1` bit.

But how do we know the chance of what the next MSBit will be?  Well, by looking at all of our MSBits, we can see a pattern.  that most of the time, if you have a `0`, bit, there's a large chance the next bit will remain a `0`.  And if you have a `1` bit, there's a large chance that bit will remain `1`

And, the chance of the next bit being a `0` or `1` changes by how long the current run of `1`s or `0`s are.  This is sort of like RLE, but instead of giving a definitive count of continuing a run for a period of time, it just processes the bits, one at a time, each one having a certain chance of being `0` or `1`.  And when you get to `MAXPIXELRUNTOSTORE`, just keep it there, since the number changes very little after the 8th pixel, becuase by then it's already wrapped around to the next line.

```c
// For glyph pixel data
BADATA_DECORATOR uint8_t ba_vpx_glyph_probability_run_0_or_1[2][MAXPIXELRUNTOSTORE] = {
	{  217, 214, 211, 211, 212, 219, 228, 238, 246,}, // run of 0's, ※ chance next bit will be 0
	{    0,  26,  38,  39,  36,  31,  22,  11,   6,}  // run of 1's, ※ chance next bit will be 0
};
```

💭 Curious... I wonder if the first bit for white pixels being 0 is a bug.  That would mean if you have a black-to-white pixel transition, there is a near zero chance the next pixel will be a black pixel.

The process of reading the glyphs is done once, at start, and the glyphs are decoded into RAM.


## Glyph classifications

With our glyphs decoded into RAM, we can move onto the stream itself.  One of the things I realized early on was that there is a strong relationship between the current glyph that is in a tile, and what the next glyph in that tile will be.  For instance, you can never go from one cell into itself. 

This graph below shows each transition from one tile ID to another, and it shows the count of how frequently that transition has happend within the song.

![Tile Transition Graph](https://github.com/user-attachments/assets/71ebed6c-c7cc-4a1f-96f7-ca77a530d485)

If you zoom out, it looks really pretty (or you can click to zoom in)

![Zoomed out transition graph](https://github.com/user-attachments/assets/5ac8e515-f76d-4351-859b-f294d8db20a5)

💭 There is no reason I can think of for a diagonal symmetry.  While not actually symmetric, there is a sort of quazi-symmetry where tiles can seem "related" to each other. I am not sure why this is, and maybe it could be used to extract more entropy.

So, I wrote an algorithm to place each tile into a "class."  Then when trying to figure out which cell is next, instead of assuming all cells are the same, we can use the chance of moving to another cell based on which class we're in.

```
Classes: (Theoretical Space)
  0:  3646: 19177 bits (5.259743 bits per symbol) 0
  1:  3323: 18333 bits (5.516914 bits per symbol) 1
  2:  6493: 39627 bits (6.103096 bits per symbol) 2 13 30 55 56 60 64 74 83 95 102 103 119 135 141 154 164 165 171 183 188 191 198 201 204 219 220 239 241 246 248 254
  3:  4250: 23207 bits (5.460380 bits per symbol) 3 20 25 57 93 111 126 134 138 139 143 167 194 197 199 215 221 228 233 245 247
  4:  5014: 27885 bits (5.561340 bits per symbol) 4 33 45 61 62 87 88 89 98 108 114 116 124 130 144 174 193 196 203 211 229 231 235 236
  5:  5592: 32465 bits (5.805695 bits per symbol) 5 14 19 29 34 44 46 59 67 80 92 115 129 131 133 142 152 166 181 189 206 210 214
  6:  5130: 28666 bits (5.587907 bits per symbol) 6 18 22 27 36 39 47 65 81 107 118 137 146 149 157 170 182 185 190 200 208 240
  7:  4742: 25170 bits (5.307907 bits per symbol) 7 15 16 23 51 63 66 68 77 86 104 109 127 132 173 186 192 209 213
  8:  4798: 28245 bits (5.886834 bits per symbol) 8 28 32 58 71 90 97 101 112 117 145 159 160 162 169 175 187 195 207 216 218 223 224 225 249
  9:  3285: 19768 bits (6.017756 bits per symbol) 9 43 54 70 125 128 147 153 177 180 202 212 222 230 238 243 244 250 253
 10:  4376: 27324 bits (6.244118 bits per symbol) 10 21 26 31 38 40 41 75 84 85 100 122 163 172 178 217 232 234 237 255
 11:  5695: 34687 bits (6.090759 bits per symbol) 11 24 50 72 73 76 78 79 91 99 105 110 113 120 123 136 140 150 151 155 156 158 161 168 179 226 227 251 252
 12:  4870: 28799 bits (5.913510 bits per symbol) 12 17 35 37 42 48 49 52 53 69 82 94 96 106 121 148 176 184 205 242
```

If we are moving _from_ symbol 0, then, we would be in class 0.  From Class 0, we can compute the probability of going to each new cell.

Because we have less tahn 16 classes, we can encode each one as a hex digit, and store it in `ba_exportbinclass` each tile gets on nibble in that table for the class ID.

```c
BADATA_DECORATOR uint8_t ba_exportbinclass[128] = {
	0x10, 0x32, 0x54, 0x76, 0x98, 0xba, 0xbc, 0x6c, 0x3a, 0x9c, 0xba, 0x65, 0x32, 0xaa, 0x2b, 0x3b,
	0x66, 0x95, 0x49, 0xaa, 0x29, 0x29, 0x37, 0x43, 0x76, 0x82, 0xb7, 0xaa, 0xaa, 0x95, 0xa6, 0x55,
	0xa9, 0xaa, 0x2c, 0xa2, 0x5b, 0x3b, 0x52, 0xba, 0xb7, 0x77, 0xbb, 0x6a, 0xac, 0xa6, 0x72, 0x65,
	0x6b, 0x27, 0x55, 0xa2, 0xb8, 0x43, 0xc5, 0x85, 0xa4, 0x46, 0x95, 0x32, 0x46, 0x93, 0xc2, 0x67,
	0x55, 0x67, 0xba, 0xb8, 0x52, 0x42, 0xb6, 0xba, 0xa4, 0x9c, 0xba, 0x27, 0xb4, 0x63, 0x55, 0x5b,
	0x92, 0xa2, 0x37, 0xbb, 0xa3, 0x9a, 0x56, 0x58, 0x8a, 0x59, 0x99, 0x23, 0x75, 0xa2, 0x62, 0x2b,
	0x37, 0x67, 0x5a, 0xa2, 0x87, 0x2b, 0xc8, 0xb9, 0xa7, 0x48, 0x7a, 0x63, 0x89, 0xba, 0x8a, 0x73,
	0x72, 0x28, 0xa2, 0x6b, 0x5b, 0xb7, 0xb6, 0x8b, 0x64, 0x82, 0x22, 0x4b, 0x5a, 0x79, 0x88, 0x08,
};
```
## Continue or Update Cell

Once we have our glyphs decoded, we can move onto encoding the stream. We can start streaming. For each frame we start at the top-left, and raster-style scan to the bottom right.  Once we're done, we can wait 1/30th of a second and do it all over again.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/f7b0e27a-e925-4704-a004-d759ce9c1683 WIDTH=50%>
</P>

For each cell, we pull one bit from the VPX stream, if it's a `1` then, we don't do anything and move on, if it's a `0` then we have to decode a glyph (See below).

In order to determine the ※ chance of a given cell being a "break" in the continuation by getting a 0 bit, we store the probability table by class primarily and the length of time since the last cell transition (You can see this as the small number in the visualizer tool)

```c
uint8_t ba_vpx_probs_by_tile_run_continuous[USE_TILE_CLASSES][RUNCODES_CONTINUOUS] = {
	// --> Time since last cell transition. ※ that the continue but will be a `0` (and will have to change).
	{  33, 27, 23, 20, 17, 17, 17, 15, 14, 16, 14, 11,  8, 10, 10, 10, 10,  7,  8,  8, 11, 10,  8, 10,  6,  9, 10,  7,  6,  8,  5,  6,  6,  5,  2,  6,  4,  5,  6,  5,  4,  4,  3,  4,  3,  1,  5,  4,  3,  6,  5,  2,  4,  2,  3,  4,  9, 11,  9,  4,  5,  5,  6,  7,  5,  9,  3,  3,  2,  7,  4,  5,  0,  1,  3,  4,  2,  5,  3,  4,  2,  5,  4,  3,  1,  3,  2,  5,  3,  7,  6,  3,  2,  5,  3,  4,  6,  6,  1,  7, 10,  5,  8,  8,  5, 10,  5,  3,  6,  4,  0,  5,  0,  1,  7,  8,  6,  3,  3,  3,  0,  1,  8,  3,  1,  3,  3,  4,},
	{  22, 15, 14, 15, 16, 13, 13, 14, 12, 10, 13, 11,  8,  8, 10,  9,  6,  8,  7,  7,  5,  8,  7,  6,  8,  5, 11,  8,  5,  6,  6,  5,  6,  5,  4,  6,  6,  5,  4,  7,  8,  5,  6,  4,  4,  7,  5,  4,  5,  4,  5,  2,  1,  3,  2,  2,  3,  3,  3,  3,  2,  2,  6,  3,  2,  3,  3,  3,  3,  3,  3,  3,  1,  8,  7,  3,  0,  3,  5,  9,  4,  4,  3,  4, 11,  7,  6,  1,  4,  3,  4,  2,  2,  6,  1,  1,  5,  4,  7,  6,  2,  4,  6,  1,  3,  5,  3,  3,  2,  2,  1,  1,  2,  2,  4,  1,  1,  1,  4,  2,  0,  1,  1,  3,  4,  1,  4,  3,},
	{ 141,110, 78, 52, 47, 40, 30, 28, 16, 18, 21, 14, 15, 11, 16, 10,  6, 13, 12,  7, 13,  4,  4,  8,  3,  6,  7,  7,  5,  2,  6,  6,  0,  6,  2,  2,  0,  0,  5,  2,  0,  0,  0,  0,  2,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  0,  0,  0,  5,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,},
	{ 145,109, 83, 65, 44, 54, 30, 26, 40, 29, 26, 15, 20, 24, 13, 25,  7, 18, 14, 15, 16, 14, 11,  7,  8,  0,  8,  4,  9,  0,  4,  0,  9, 10, 27, 12,  0, 13,  7,  0,  0,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  8,  0,  0,  0,  0,  0,  0,  0, 21, 11,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0,  0, 14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,},
	...
	n classes
};
```

The more accurately we can nail down the chance of a given bit being a `1` or a `0`, the better compression we can have.  So if we find some cells where there's 0 chance that there's a break in the continuation, we can spend very little data on that check.

Nothing changes as long as we're getting the "keep going" `1`s.  But as soon as we get a `0` we have to start decoding a glyph ID.

We encode glyph IDs in binary, 8 bits, most-significant-bit-first. To determine the chance of a `1` or a `0` in the first bit, we simply compare the sum of possible tiles where the msb would be `1` to the number where it would be `0` and write that ※ into the first cell.

Then we move onto the next bit, and only include tiles which match our most signiificant bit.  And we repeat until we get to a leaf cell.

To store these ※s in a table, we can store the tree of these values in a breadth-first array representation of the tree.

<P ALIGN=CENTER>
![breadth-first array](https://github.com/user-attachments/assets/1b1b5062-6d4c-4466-94f1-ae529cb8ed03)
<BR>[credit](https://en.wikipedia.org/wiki/Binary_tree#/media/File:Binary_tree_in_array.svg)
</P>

To produce this tree, we can generate all the frequencies for all of our elements, and let `ProbabilityTreeGenerateProbabilities` generate the `probabilities` table.

```c
void ProbabilityTreeGenerateProbabilities( uint8_t * probabilities, unsigned nr_probabilities, const float * frequencies, unsigned elements, unsigned needed_bits );
```

This will generate a table that can be used both by `ProbabilityTreeWriteSym` as well as `ProbabilityTreeReadSym` to compress and decompress symbols respectively.

An example table is as follows, note that there are a lot of cells which are either 0 or 255.  That means there are few or no possible combinations where a cell moving from this class will transition into that particular cell (or branch of cells) and thus you don't have to spend any data on it.

```c
BADATA_DECORATOR uint8_t ba_chancetable_glyph_dual[BY_CLASS][255] = {
	...
{	188, 171, 158, 221, 247, 181, 246, 252, 171, 255, 248,   0, 117, 105, 255, 237,
	148,  68, 233, 139,  34, 188,   0,   0, 255, 251,   0, 229, 253,   0,   0, 128,
	102, 255, 139, 130, 251, 240,   2, 144, 128,   0, 171,   3,   0,   0,  85, 252,
	  4, 255, 103, 199, 253, 202, 128, 197,  17,  28,  17,  90,   0,  93, 116,  56,
	255, 124,  55, 190, 208, 246, 255, 171,  99,   0,  93, 233,   0,   0, 239,  85,
	255, 128, 112,  20,  16, 255,  34,   0,   0,   0, 221, 252,  99,  64,   0,   0,
	255, 101, 139, 160,   9, 255, 255,  30,   0,  45,  31,  18, 255, 255, 254, 254,
	  0, 137,  65,  14, 255, 246, 251, 245,  85,   5, 255,   0,   0, 236, 253, 238,
	117, 124, 169, 233, 221,   5, 171, 255,  32,   0, 195, 114,  64,   0,  85,  77,
	 85,   0, 193, 209, 252, 248, 255, 255, 201,   0, 102,  25, 255, 228,   0,   0,
	 17, 178,  53, 128,   0,   0, 255, 189,  30, 128,  19, 217,   0,   0,   0,   0,
	185,  29, 116, 255,  51,   0, 171, 128,   0, 235,  59, 255,   0, 183, 205, 255,
	145, 160, 116, 146, 190,   0,  15, 210, 244, 114, 144,  88,   0,   0, 214, 146,
	  0, 165, 160,   0,   0,   4,  20, 255,   7,  46, 231, 255, 255,  85,  51,  25,
	213, 110,  50, 221,  54,  85, 165,  17, 128,  94,  14,   0,  24, 188, 163,  20,
	145, 145,  90,   0, 186,  19,   0, 149, 122,  46, 255, 228, 128,   0, 214,},
...
};
```

From there, we just have to start decoding!  All of the data to decode is stored in this one array.

```c
BADATA_DECORATOR uint8_t ba_video_payload[54492] = { ... };
```

Using this cocofony of compression schemes, the video portion of the compression (from `vpxtext`) comes down to:

```
      Glyphs:    256
 Num Changes:  50412
      Stream: 294984 bits / bytes: 36873 (5.851 bits per glyph change, 5.850 theoretical)
         Run: 140952 bits / bytes: 17619

    Combined: 435936 bits / bytes: 54492
 +Tile Class:   1024 bits / bytes:   128
 + Tile Prob:  26520 bits / bytes:  3315
 +  Run Prob:  13312 bits / bytes:  1664
 +COMPGlyphs:  20072 bits / bytes:  2509
   Sound    :   5384 bits / bytes:   673

 Total:  62781 Bytes (502248 bits) (75.626 bits/frame VO) (6570 frames)
```

## Hero Frame

Just one thing remains.  A hero frame.  There's one scene that I anticipate and really love.  But, it didn't come through from the stram geneartion from Evan's ML approach.

![hero frame](https://github.com/user-attachments/assets/c5515fbb-3838-4af6-b259-d43aaed235db)

And I wanted to make sure that would come through on the compressed video.  So, I created an editor that lets me manually edit tiles, and where they go within the stream.  Just doodling on a small face made it work out quite nicely in spite of being right on a frame boundary.

![hero editor](https://github.com/user-attachments/assets/e957d886-2b60-4c94-be18-eb0ade524a3b)

With that, I had the final stream.  Everything looked like it would fit.  Just needed one more thing, to move it to the ch32v006.

# The ch32v006 implementation

## Hardware

The hardware consists of a board that has:
1. A CH32V006, 48MHz, RISC-V WCH Microcontroller $0.13/ea.
2. A 24MHz Crystal (cause why not)
3. A USB Connector wired up so that you can use a USB-Audio adapter and provide power to the system.
4. 2 Red LEDs (for the apple catch scene)
5. 2 Diodes to allow for the microcontroller to charge pump the display.
6. A connector for a Ronboe RB6448TSWHG04 64x48 OLED Display.
7. A voltage regulator.

All packed within 2.65cm² or less than 1/2 in².

![PCB](https://github.com/user-attachments/assets/ab970c76-a0e9-4e2a-8799-50a0367eb256)

![Schematic](https://github.com/user-attachments/assets/400971a1-8e42-41b8-be3e-47e74ab2087c)

**TODO** Assembly images

## General Setup

For the firmware, I used ![ch32fun](https://github.com/cnlohr/ch32fun), I pulled out all the stops and configured with:

```c
#define FUNCONF_NO_ISR 1
#define FUNCONF_OVERRIDE_VECTOR_AND_START 1
#define FUNCONF_TINYVECTOR 1
#define FUNCONF_SYSTICK_USE_HCLK 1
#define FUNCONF_USE_HSE 1
#define FUNCONF_USE_PLL 1
```

This means, I was starting with literally nothing, startup code wise.  I didn't initially do this, but I did end up here as I kept having to shave off bits anywhere I could for a while. There's also a few other niceities.

1. We aren't using ISRs.
2. We have no .data section.  (No variables come initialized, all are default-initialized to 0.)

All of this code can be found in [playback/ch32v006_firmware/badappleplay.c](playback/ch32v006_firmware/badappleplay.c).

So, our startup function could be itty bitty.

```c
void handle_reset( void )
{
	asm volatile( "\n\
.option push\n\
.option norelax\n\
	la gp, __global_pointer$\n\
.option pop\n\
	la sp, _eusrstack\n"
".option arch, +zicsr\n"
	// No ISR needs setup so just set mstatus.
"	li a0, 0x1880\n\
	csrw mstatus, a0\n"
	: : : "a0", "a3", "memory");

	// Careful: Use registers to prevent overwriting of self-data.
	// This clears out BSS.
asm volatile(
"	la a0, _sbss\n\
	la a1, _ebss\n\
	li a2, 0\n\
	bge a0, a1, 2f\n\
1:	sw a2, 0(a0)\n\
	addi a0, a0, 4\n\
	blt a0, a1, 1b\n\
2:"
	// This loads DATA from FLASH to RAM ---- BUT --- We don't use the .data section here.
/*
"	la a0, _data_lma\n\
	la a1, _data_vma\n\
	la a2, _edata\n\
1:	beq a1, a2, 2f\n\
	lw a3, 0(a0)\n\
	sw a3, 0(a1)\n\
	addi a0, a0, 4\n\
	addi a1, a1, 4\n\
	bne a1, a2, 1b\n\
2:\n"*/
: : : "a0", "a1", "a2", "a3", "memory"
);

#if defined( FUNCONF_SYSTICK_USE_HCLK ) && FUNCONF_SYSTICK_USE_HCLK
	SysTick->CTLR = 5;
#else
	SysTick->CTLR = 1;
#endif

	// Because no ISRs, we can jump straight to main!
	main();
}
```

This made the overhead for our firmware very, very small.  So we could focus on getting all of the business to play bad apple down to the 3328 byte bootloader.

Since we aren't using any of the ch32fun startup code, or systeminit.  Main has to be clever about setting up flash latency and switching over to an external crystal.  Normally you'd do a few more steps, but we can just yolo all this.

```c
	// This is normally in SystemInit() but pulling out here to keep things tight.
	FLASH->ACTLR = FLASH_ACTLR_LATENCY_2;  // Can't run flash at full speed.

	#define RCC_CSS 0
	#define HSEBYP 0
	#define BASE_CTLR	(((FUNCONF_HSITRIM) << 3) | RCC_HSION | HSEBYP | RCC_CSS)
	RCC->CTLR = BASE_CTLR | RCC_HSION | RCC_HSEON | RCC_PLLON;  // Turn on HSE + PLL
	while((RCC->CTLR & RCC_PLLRDY) == 0);                       // Wait till PLL is ready
	RCC->CFGR0 = RCC_PLLSRC_HSE_Mul2 | RCC_SW_PLL | RCC_HPRE_DIV1; // Select PLL as system clock source
```

From there, there's a other major topics that need to be performed:
1. Setup GPIOs
2. Configure `TIM2` to be audio output via PWM @ 46875 SPS
3. Configure `DMA1_Channel2` to feed the `TIM2` PWM output.
4. Configure `TIM1` to charge pump the OLED (because the OLED needs ~7V on its VCC line.
5. Setup the RB6448TSWHG04 via bit-banged I2C. (We bit bang because it's faster)
6. Call `ba_play_setup( &ctx );`
7. Call `ba_audio_setup();`

And we're off to the races. As a reminder, you can see more information in the file, itself. [playback/ch32v006_firmware/badappleplay.c](playback/ch32v006_firmware/badappleplay.c).

## De-Blocking Filter

### Vertical De-Blocking Filter

While the processor can operate at about 1.5CPI (Clocks per instruction), because we are running from flash, that's much much lower.  The ch32v006's flash is a good clip slower than the ch32v003's flash, requiring two wait cycles. So, we are speed constrained.  One of the areas which takes a good bit of performance is the de-blocking filter.

I was just thinking... What if we could do vector processing.  What if we could operate on 8 pixels at a time?  Wouldn't that be cool?  No, the ch32v006 doesn't have vector processing... or does it?

One of the things they teach you in computer engineering is how to build circuits out of logic gates, and how you can use K-maps.  I used [32x8.com](http://www.32x8.com) to generate my computation for the [msb](http://www.32x8.com/sop6_____A-B-C-D-E-F_____m_7-15-19-23-29-31-51-53-55-61-63_____d_2-6-8-9-10-11-14-18-22-24-25-26-27-30-32-33-34-35-36-37-38-39-40-41-42-43-44-45-46-47-50-54-56-57-58-59-62_____option-0_____899781960074855695700) and [lsb](http://www.32x8.com/sop6_____A-B-C-D-E-F_____m_3-5-7-12-13-15-17-19-20-21-23-28-29-31-48-49-51-52-53-55-60-61-63_____d_2-6-8-9-10-11-14-18-22-24-25-26-27-30-32-33-34-35-36-37-38-39-40-41-42-43-44-45-46-47-50-54-56-57-58-59-62_____option-0_____999781976475857595733) for the table outlined in **TODO** Section reference.

![32x8 logic image](https://github.com/user-attachments/assets/f3299121-734c-42d9-b5b4-330f38764803)

With this in-hand I decided to operate on groups of 8 pixels simultaneously.  It would have been much easier, faster and simpler if we had used a sane mathematical equation, but, this was still a big gain.  We didn't have to do all the bit logic to disassemble each pixel out fo the blending pixels, and we are able to compute both bits of all 8 pixels on each vertical blur at once.

```c
void EmitEdge( graphictype tgprev, graphictype tg, graphictype tgnext )
{
	// This should only need +2 regs (or 3 depending on how the optimizer slices it)
	// (so all should fit in working reg space)

	// pixel data is stored 8-pixels-at-a-time. The MSB for each pixel is
	// the top byte, and the LSB for each pixel is the bottom byte.

	graphictype A = tgprev >> 8;
	graphictype B = tgprev;      // implied & 0xff
	graphictype C = tgnext >> 8;
	graphictype D = tgnext;      // implied & 0xff
	graphictype E = tg >> 8;
	graphictype F = tg;          // implied & 0xff

	int tghi = (D&E)|(B&E)|(B&C&F)|(A&D&F);     // 8 bits worth of MSBs
	int tglo = E|C|A|(D&F)|(B&F)|(B&D);       // 8 bits worth of LSBs

	PMEmit( (tghi << 8) | tglo ); // Write to intermediate framebuffer.
}
```

And I do mean it would have been simpler if we had some simple sane color blending.

```c
	int tghi = (F&G)|(E&H);     // 8 bits worth of MSB of this+(next+prev+1)/2-1
	int tglo = G|E|(F&H);       // 8 bits worth of MSB|LSB of this+(next+prev+1)/2-1
```

The idea is that the horizontal filter is run by reading the given tile data from memory, applying the vertical filter and writing to an intermedia framebuffer.

So yes, in a manner of sense you could turn the ch32v006 into a SIMD processor.  This is sort of 8 SIMD, but you could process up to 32-bits worth at once!

### Horizontal D-Blocking Filter

On scanout is where we compute our horizontal deblocking filter.  Because our OLED Display has a weird scanout, we will perform the blur on the first and last pixel of every output group-of-8 pixels.

![OLED Scanout order](https://github.com/user-attachments/assets/2a333626-b95e-429d-a9ea-765c9f5f1a41)

The scan order for the OLED display goes in rows of 8 pixels at a time, from left to right for all 6 superrows.  In outputting each new byte worth of pixels, we select the first and last pixel to do the blur on while copying all of the others.

To speed up the process for determining how "bright" a given pixel should be, we built a LUT (because this [couldn't be a cnlohr project unless it had more LUTs](https://www.youtube.com/watch?v=LnqqvVp_UZg)). Where the X axis along the LUT is the "prev" cell, the Y axis is the "next" cell, and within each byte, you can shift it down by 2* the current brightness to get the intended value.

```c

// This table contains the information about how to blend one pixel at a time.
// You can index into it via X->prev, Y->next, >>(2*this)
static const uint8_t potable[16] = {
	0x50, 0xf4, 0xf5, 0xf5,
	0xf4, 0xf5, 0xfd, 0xfd,
	0xf5, 0xfd, 0xfd, 0xfd,
	0xf5, 0xfd, 0xfd, 0xfd,
};
```

And then we have the final scanout code.

```c
	for( i = 0; i < sizeof(pixelmap)/2; i++ )
	{
		if( pvx == 64 ) { pvx = 0; pvy++; }
		int pvo = pmo[i];
		uint16_t pvr = pvo & 0x7e7e;

		int pprev, pnext, pthis;

		if( i < 64 )
			pprev = ((pvo>>8)&2) | ((pvo>>1)&1);
		else
		{
			int kpre = pmo[i-64];
			pprev = ((kpre>>14)&2) | ((kpre>>7)&1);
		}
		pnext = ((pvo>>8)&2) | ((pvo>>1)&1);
		pthis = (((pvo>>7)&2) | ((pvo>>0)&1))<<1;

		int pol = (potable[pnext+pprev*4]>>pthis)&3; // <<<<<<<<<<<<<<<<<<<<<<<<<<
		pvr |= (pol & 1) | (pol&2)<<7;

		if( i >= 256 )
			pnext = ((pvo>>13)&2) | ((pvo>>6)&1);
		else
		{
			int knext = pmo[i+64];
			pnext = ((knext>>7)&2) | (knext &1);
		}
		pprev = ((pvo>>13)&2) | ((pvo>>6)&1);
		pthis = (((pvo>>14)&2) | ((pvo>>7)&1))<<1;

		pol = (potable[pnext+pprev*4]>>pthis)&3; // <<<<<<<<<<<<<<<<<<<<<<<<<<
		pvr |= (pol & 1)<<7 | (pol&2)<<14;

		//pixelbase[pmp] = pvo;
		uint16_t po = pvr;

		if( subframe != 1 ) // Set this to &1 for 50/50 grey.
			ssd1306_mini_i2c_sendbyte( po>>8 );
		else
			ssd1306_mini_i2c_sendbyte( po );
		}
```

## Why was this needed?

You might be wondering... was all of this optimization really needed?

Yes.

Even with a previous implementation hand optimized, with bits and bobs of assembly, I couldn't get it to fit, time-wise.

![perf monitor](https://github.com/user-attachments/assets/0d37af0b-47dd-410b-87b8-996927a033a8)

I used a spare GPIO pin on the 006, that I would turn on immediately before waiting for the next frame, and off while processing.  This told me how much time was spent working on a given frame.  You can see in the analog graph, when there is no CPU time left, when the % FreeTime signal is only long as long as it takes the CPU to realize it has to get right back to work!  When this happens, frames get successively further behind, causing a pileup.

![perf zoom in](https://github.com/user-attachments/assets/10c42b2a-05ea-4d00-8405-d2845c53cd4d)

Some of the random optimizations that came to mind were:

Before Final Optimization:

### Turn all applicable compiler optimiationz on

I ended up using the following.  This really made the code as small and fast as I could get it.

```
-mrelax -ffunction-sections -flto -Os -ffunction-sections -fdata-sections -fmessage-length=0 -msmall-data-limit=8 -g -Wno-unused-function -Wno-unused-variable -march=rv32ec -mabi=ilp32e 
```

### 

### MORE TODO

To get the final optimization:

### Consider register micro-optimization

Manually extract pointers from arrays and use those pointers, instead of dereferencing.

```diff
-	player->stack[stackplace].remain--;
-	CHECKPOINT( audio_stack_place = stackplace, audio_stack_remain = player->stack[stackplace].remain, decodephase = "AUDIO: Pulling Note" );
-	while( player->stack[stackplace].remain == 0 )
+	struct ba_audio_player_stack_element * stack = player->stack;
+	struct ba_audio_player_stack_element * stackpl = stack + stackplace;
+	stackpl->remain--;
+	CHECKPOINT( audio_stack_place = stackplace, 
+	while( stackpl->remain == 0 )
...
-	if( tremain > player->stack[stackplace].remain ) tremain = player->stack[stackplace].remain;  //TODO: Fold tremain into remain logic below.
-	player->stack[stackplace].remain -= tremain;
+	if( tremain > stackpl->remain ) tremain = stackpl->remain;  //TODO: Fold tremain into remain logic below.
+	stackpl->remain -= tremain;
```


### MORE TODO


 * 
 * 

The performance slipped in juuuust under the wire.

![doing better](https://github.com/user-attachments/assets/d2f1c2dc-a1bb-49b9-a114-0837575aa30c)

If we zoom into the section where there was no more CPU time, we can see that while we didn't have any spare time, it was so close, it didn't measurably impact when the frames themselves could delay, and it was only 12 frames... because we are manually syncing the display, there's no way to perceive any slowdown.

![doing better 2](https://github.com/user-attachments/assets/98682350-811d-4c7b-b2a4-35a057b03de8)

## Space constraints

One limit I kept bumping against was the storage limitation.  I wanted all of the code to do playback to live inside the bootloader.  Even though the [ch32fun startup code](https://github.com/cnlohr/ch32fun/blob/master/ch32fun/ch32fun.c#L1042) is very dense, it still did a handful of things that I could hand tweak to be even tighter for this specific tool.


## Firmware (Final notes)

While there was a lot of push and shove to get the overall size small enough eventually I was able to do so, and then continue to push, because nothing is ever in the last place I look! The final size output (at least as of the moment of writing this) is:

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       62976 B        62 KB     99.19%
      BOOTLOADER:        3324 B       3328 B     99.88%
             RAM:        8064 B         8 KB     98.44%
```

# The web viewer demo

The [web viewer](https://cnvr.io/dump/badderapple.html) was a whole nother rabbit hole. 

I wanted some tool initially, on Desktop to debug through the output stream and understand where there may be errors and where something may not have decoded correctly.  So, it started largely as a debugging tool. And from there into a demonstration tool, and from there into the web viewer we have today.

Originally, I started with a decoder, available in the [playback/](playback/) folder, originally designed as a basic tester to make sure I wasn't just making up unintelligable bitstreams.  This morphed into the [playback/interactive/](playback/interactive/) viewer.  I wrote it in [rawdraw](https://github.com/cnlohr/rawdraw), a single-file-header multi-platform graphics system because I had a sneaking suspicion I was going to want to deploy this more places than deskop, and I hate waste.

![interactive viewer](https://github.com/user-attachments/assets/fa59b807-60f9-443a-a8a7-3eb186e0dead)

This was my first time using the [Clay UI Layout Library for C](https://github.com/nicbarker/clay) but thankfully I was not the first to use it with rawdraw, since it now has support for [glScissors](https://registry.khronos.org/OpenGL-Refpages/gl4/html/glScissor.xhtml).  

The general setup for Clay is to define a Clay layout.  Once Clay has started setting things up, you can use `CLAY()` macros to build a series of laid out elements within a window.  You tell it what you need and where roughly, and Clay will determine the width/height X and Y of all of the individual elemetns and 

```c
	CNFGClearFrame();
	Clay_SetPointerState((Clay_Vector2) { mousePositionX, mousePositionY }, isMouseDown);
	Clay_BeginLayout();

	int padding = 4;
	int paddingChild = 4;

	LayoutStart();
	{
		CLAY({ .id = CLAY_ID("OuterContainer"), .layout = { .layoutDirection = CLAY_TOP_TO_BOTTOM, .sizing = {CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0)}, .padding = CLAY_PADDING_ALL(padding), .childGap = paddingChild }, .backgroundColor = COLOR_BACKGROUND })
		{

			CLAY({
				.id = CLAY_ID("Top Bar"),
				.layout = { .layoutDirection = CLAY_LEFT_TO_RIGHT, .sizing = { .height = CLAY_SIZING_FIT(), .width = CLAY_SIZING_GROW(0) }, .padding = CLAY_PADDING_ALL(padding), .childGap = paddingChild },
				.backgroundColor = COLOR_PADGREY
			})
			{
				CLAY({ .layout = { .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, .sizing = { .width = CLAY_SIZING_FIT(32), .height = CLAY_SIZING_FIT() }, .padding = CLAY_PADDING_ALL(padding), .childGap = paddingChild }, .backgroundColor = ClayButton() } )
				if( AudioEnabled )
					CLAY_TEXT(CLAY_STRING("\x04"), CLAY_TEXT_CONFIG({ .textAlignment = CLAY_TEXT_ALIGN_CENTER, .fontSize = 16, .textColor = {255, 255, 255, 255} }));
				else
					CLAY_TEXT(CLAY_STRING("\x0e"), CLAY_TEXT_CONFIG({ .textAlignment = CLAY_TEXT_ALIGN_CENTER, .fontSize = 16, .textColor = {255, 255, 255, 255} }));

				int ToggleAudio();
				int ToggleFullscreen();
				int IsFullscreen();

				if( btnClicked ) { AudioEnabled = ToggleAudio(); }

				CLAY({ .layout = { .childAlignment = { CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER}, .sizing = { .width = CLAY_SIZING_FIT(32), .height = CLAY_SIZING_FIT() }, .padding = CLAY_PADDING_ALL(padding), .childGap = paddingChild }, .backgroundColor = ClayButton() } )
				if( IsFullscreen() )
					CLAY_TEXT(CLAY_STRING( "\x1f" ), CLAY_TEXT_CONFIG({ .textAlignment = CLAY_TEXT_ALIGN_CENTER, .fontSize = 16, .textColor = {255, 255, 255, 255} }));
				else
					CLAY_TEXT(CLAY_STRING( "\x12" ), CLAY_TEXT_CONFIG({ .textAlignment = CLAY_TEXT_ALIGN_CENTER, .fontSize = 16, .textColor = {255, 255, 255, 255} }));
				if( btnClicked ) { ToggleFullscreen(); }
		...
```

One of the really fun things about Clay is that you can write code within the layout.  So this means you can conditionally emit more CLAY elements.  So the scene can be dynamic, dependent on what is visible, or where the user is scrubbing within the song.

This was really helpful, because I could write code in [interactive.c](playback/interactive/interactive.c) to handle visualizing all of the various details going on about the song, and its decompression.

To monitor bit-at-a-time, I added some macros that are no-ops on the embedded platform, like CHECKPOINT.  This would notify the interactive tool to take a sort of snapshot of everything, so when the user scrubs around using the scrubbing bars, they can see 

### Porting to WASM

Rawdraw has a mechanism to target WASM, without any extra engines like Emscripten, so when it builds out WASM targets, they contain only a WASM blob with the code, and enough javascript to load the WASM blob and give it access to a WebGL context.  For instance, the example rawdraw WASM project here is only 19kB uncompressed, and goes to around 10kB after regular gzip HTTP compression... And typically is up and rendering within about one frame of page load, so it's sometimes possible to prevent even the loading flash.

![rawdraw example app](https://github.com/user-attachments/assets/5f1e6d86-26dc-4f8f-8263-07fea3bd901d).

WASM does not have any kind of libc natively, though there are projects that seek to address it, all I had to do was drop in some headers into [weblibc_headers](playback/interactive/web/weblibc_headers) with some bog-standard functions, most of who's implementation was from ch32fun.  Within the interactive playback tool, I did use some floating point functions, so, for those I just provided a header, and did the math in Javascript.

The WASM build process is defined by rawdraw, but it just boils down to:


```sh
# Compile the WASM blob
clang -I.. -DEMU -I../.. -I../../../common -I../../../vpxtest -Iweblibc_headers -DWASM -DASYNCIFY=1 -I. -Wno-string-compare -O4 -g -I../common -DRESX=64 -DRESY=48 -DBLOCKSIZE=8 -DFRAMECT=6570 -DTARGET_GLYPH_COUNT=256 -I../common -DRESX=64 -DRESY=48 -DBLOCKSIZE=8 -DFRAMECT=6570 -DTARGET_GLYPH_COUNT=256 -DWASM -nostdlib --target=wasm32 -flto -Oz -Wl,--lto-O3 -Wl,--no-entry -Wl,--allow-undefined -matomics -Wl,--initial-memory=1073741824,--max-memory=1073741824 -mbulk-memory -Wl,--import-memory -Wl,--export=__heap_base,--export=__data_end,--export=asyncify_struct ../interactive.c -o main.wasm

# Asyncify lets us "call back" into the WASM code.
wasm-opt --asyncify  main.wasm -o main.wasm

cat main.wasm | gzip -9 | dd bs=1 skip=10 | head -c -8 | base64 | sed -e "$ ! {/./s/$/ \\\\/}" > blob_b64;

# Paste the WASM blob into the HTML file.
./subst template.js -s -f BLOB blob_b64 -o mid.js

# This squeezes the javascript or "uglifies" it
terser --module --compress -d RAWDRAW_USE_LOOP_FUNCTION=false -d RAWDRAW_NEED_BLITTER=true mid.js -o opt.js

# Paste everything together and output badderapple.html
./subst template.ht -s -f JAVASCRIPT_DATA opt.js -o badderapple.html
```
🗭WASM has a [critical flaw](https://github.com/WebAssembly/design/issues/796) that isn't even addressed in WASM2, in that it doesn't have any `goto` opcode.  This means that we have to use asyncify to make it so we can have a call that can call out to JavaScript and allow the screen to flip.  If it had a `goto` opcode, WASM code could be reentrant and allow code to sort of "wait" or support threads called back from Javascript.  As it stands, you have to use asyncify, which adds additional if() branches everywhere there is code that could be entered back into which can have a significant overhead.

Regardless, with this system, we can quickly iterate and output reasonably sized .html files that contain everything all-in-one.

Don't let BIG WASM tell you what to do. Your hello world WASM project with some OpenGL graphis doesn't need to be 13 megabytes and take hundreds of milliseconds to load.

![downloaded web version](https://github.com/user-attachments/assets/cf0fa2aa-40cc-4a33-a546-888535d3efc3)

# Setup and Running

## Prep

```
sudo apt-get install build-essential libavutil-dev libswresample-dev libavcodec-dev libavformat-dev libswscale-dev
```

For msys2 (Windows)

```
pacman -S base-devel mingw-w64-x86_64-ffmpeg 
pacman -S clang llvm clang64/mingw-w64-clang-x86_64-wasm-component-ld mingw-w64-clang-x86_64-wasmer mingw-w64-x86_64-binaryen # For web stuff.
```

For additional tooling for size comparisons on song:
```
sudo npm i -g yarn
sudo yarn global add beesn
sudo apt-get install zstd
```

### Workflow

#### Non-ML 

```bash
cd comp2
make clean tiles-64x48x8.dat
cd ../streamrecomp
make clean stream_stripped.dat
cd ../song
make clean ../playback/badapple_song_huffman_reverselzss.h track-float-48000.dat
cd ../vpxtest
make clean test.mp4
cd ../playback
make clean playback.mp4
cd interactive
make interactive
cd web
export PATH=${PATH}:/clang64/bin # if on msys2, 
make index.html
```

#### ML


```bash
cd comp2
make clean tiles-64x48x8.dat
cd ../ml
make setup runcomp
```

wait a few minutes

```bash
cd ../ml
make reconstruct
cd ../streamrecomp
make streamrecomp && ./streamrecomp
cd ../vpxtest
make clean test.gif #or test.mp4
```



## Previous Work
| Date | Platform | Storage | Res | Technique | References |
| --- | --- | --- | --- | --- | --- |
| 2011-05-17 | Game Boy Color | 8MB ROM | 160x144 | Unknown | https://emuconsoleexploitnews.blogspot.com/2011/05/bad-apple-demo-ported-to-gameboy-color.html / https://web.archive.org/web/20181013041044/http://www.geocities.jp/submarine600/html/apple.html |
| 2012-05-21 | CASIO fx-CG50 | ??? | 72x54@20FPS | RLE, FastLZ | https://github.com/oxixes/bad-apple-cg50 |
| 2012-10-01 | NES | 512kB | 64x60@15FPS | P/I Frames, Updating Rows of Image (later 30 FPS, but undocumented) | https://wiki.nesdev.com/w/index.php/Bad_Apple / https://www.nesdev.org/wiki/Bad_Apple |
| 2014-01-17 | TI-84 Plys | 2.3MB | 96x64@30FPS | 1bpp, gzipped | https://www.youtube.com/watch?v=6pAeWf3NPNU  |
| 2014-06-15 | 8088 Domination | 19.5MB | 640x200@30FPS | Row-at-a-time frame-deltas | https://www.youtube.com/watch?v=MWdG413nNkI - https://trixter.oldskool.org/2014/06/19/8088-domination-post-mortem-part-1/ |
| 2014-06-29 | Commodore 64 | 170kB | 312x184@12FPS | Full Video, glyphs (16x16) | https://www.youtube.com/watch?v=OsDy-4L6-tQ |
| 2015-05-09 | Vectrex | ??? | ??? | ??? | https://www.youtube.com/watch?v=_aFXvoTnsBU - https://web.archive.org/web/20210108203352/http://retrogamingmagazine.com/2015/07/16/bad-apple-ported-to-the-vectrex-something-that-should-technically-not-be-possible/ - http://spritesmods.com/?art=veccart&page=5 |
| 2016-02-13 | Bad Apple!! zx spectrum 512 version by Techno Lab | 640kB | 256x192@6FPS | Unknown | https://www.youtube.com/watch?v=cd5iEeIe7L0 |
| 2018-01-21 | Arduino Mega "Bad Duino" | 220,924 bytes | 128x176@60FPS | Lossy line-at-a-time deltas | https://rv6502.ca/2018/01/22/bad-duino-bad-apple-on-arduino/ https://www.youtube.com/watch?v=IWJmK5J8shY |
| 2018-09-30 | Atari 400 | 41MB | 384x232@50FPS | ANTIC hardware compression | https://www.wudsn.com/index.php/productions-atari800/demos/badapplehd |
| 2022-11-01 | CHIP-8 | 63kB | 63x32@15FPS | Unkown, appears to be heirarchical | https://koorogi.itch.io/bad-apple |
| 2022-11-03 | CHIP-8 | 61kB | 48x32@15FPS | Frame Diff, reduced diff, post processing, bounding box, huffman trees | https://github.com/Timendus/chip-8-bad-apple |
| 2022-01-01 | Thumbboy | 289kB | 52x39@30FPS +DPCM Audio | ??? | https://www.youtube.com/watch?v=vbBQ11BZWoU |
| 2024-01-01 | NES (TAS ACE exploit via Mario Bros) | ??? | 15/30FPS (160x120) | 20x15 tiles glyph'd using Mario glyphs | https://www.youtube.com/watch?v=Wa0u1CjGtEQ https://tasvideos.org/8991S |
| 2025-03-05 | SSH Keys | N/A | 17x9 | bad apple but it's ssh keys | https://www.5snb.club/posts/2025/bad-apple-but-its-ssh-keys/ |

### TODO
 * Double check hero frames.
 * Write algo to find missing glyph.
 * Find way of recovering from poorly fitted data, and generating new cells.
 * Visual hyperspectral output.
 * For hufftree, make it so that it STRICTLY goes if 0, earlier, if 1, later.

### Various techniques outlined on forum post

https://tiplanet.org/forum/viewtopic.php?t=24951

 * TI-83+ Silver - fb39ca4 - 1.5MB, 96x64, Full Video - Unknown Technique
 * Graph 75+E - ac100v - 1.27MB - 85x63, Full Video - Unknown Technique
 * Graph 90+E - Loieducode / Gooseling - 64x56, Greyscale
 * Numworks - Probably very large - M4x1m3 - 320x240
 * fx-92 College 2D+ - - 96x31


### Future TODO
 - [x] Perceptual/Semantic Loss Function
 - [x] De-Blocking Filter
 - [ ] Motion Vectors - cannot - is mutually exclusive with our current approach.
 - [x] Reference previous tiles.
 - [x] Add color inversion option for glyphs.  **when implemented, it didn't help.**
 - [x] https://engineering.fb.com/2016/08/31/core-infra/smaller-and-faster-data-compression-with-zstandard/ **compared with audio compression,  It's not that amazing.**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitreader.c **winner winner chicken dinner**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitwriter.c **winner winner chicken dinner**
 - [ ] Make a much more interesting musical instrument, adding even harmonics, and using a different attack/deckay/sustain.

### Other notes
 * I got 15% savings when I broke the "run length" and "glyph ID" fields apart.
 * When having split tables, I tried exponential-golomb coding, and it didn't help.  Savings was not worth it.
 * Tride VPX with tiles, it was awful.
 * Tried VPX with RLE.  It beat huffman. (USE_VPX_LEN)
   * Huffman tree RLE (451878 bits)
   * VPX originally per tile (441250 bits)
   * VPX when unified (440610 bits)!!
 * Show original with SKIP_FIRST_AFTER_TRANSITION
 * But could we have the holy grail?  Could we not skip transitions?

Story arc:
 * RLE/Tiles/etc in raster mode.
 * Do it per tile, in time.
 * Don't run multiple unique streams.

After Full VPX
         Run:157208 bits / bytes: 19651
         Run:140136 bits / bytes: 17517
 * But we know more, what if we consider the previous value? - Need another 256-byte table.
         Run:151848 bits / bytes: 18981
         Run:135120 bits / bytes: 16890
 * The missstep of VPX_CODING_ALLOW_BACKTRACK
 * Always do A/B tests, so the absolut doesn't matter but doing A/B to compare them more broadly.


... lots of steps
...

 * Inverting run data goes from 63365 to 63282... Totes not worth it.


# Footnotes

¹
```
ffmpeg -i badapple-sm8628149.mp4 -vf "scale=64:48,setsar=1:1" -c:v libx265 -x265-params keyint=1000:no-open-gop:intra-refresh=10000 -b:v 1k -maxrate 3k  -an outx265.mkv
ffmpeg -i outx265.mkv -pix_fmt gray8 -vf format=gray -r 30 -t 00:01:00.000 output.gif
```
