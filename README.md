# badder apple

# This writeup is incomplete, please do not share it on social media yet.

What started as my shot at bad apple on an ESP8266 ended in the biggest spiral into my longest running project. The final outcome from this project was all 6570 frames, at 64x48 pixels, with sound and code for playback in 64.5kB, running on a 10-cent ch32v006. This is the story of bad*der* apple.

<P ALIGN=CENTER>
<IMG SRC=https://github.com/user-attachments/assets/5c77bf51-2895-4764-a540-fee0bc53da5a WIDTH=50%>
</P>

```
Memory region         Used Size  Region Size  %age Used
           FLASH:       62976 B        62 KB     99.19%
      BOOTLOADER:        3280 B       3328 B     98.56%
             RAM:        6400 B         8 KB     78.12%
```

If you are interested in the web viewer of the bitstream explaining what every bit means (the image below) you can click [here](https://cnvr.io/dump/badderapple.html)

![Web Viewer](https://github.com/user-attachments/assets/b2d90eb1-04f1-4302-8ea7-99580beca663)

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

Originally, I used a scheme of where I would encode each tile one at a time through the length of the video.  And I would compress it as `[ run-length | symbol-to-switch-to ]` before another tile would change.  I then tried doing exactly the same, but, scanned left-to-right and top-top-bottom, and looked for changes over time.  I was using exactly the same binary representation for both. And the binary data takes exactly the same amount of data to encode each way. YET! Across the board, it was a serious win to represent the data the other way.

In an example datastream I am using 256 tile, I can represent tile IDs as uint8_t's, so the stream can be represented as 64/8 * 48/8 * 6570 (Frames) = 315360 bytes.

| Type | Data Length (32-bit tiles) | Data Length (8-bit tiles) |
| -- | -- | -- |
| Tile IDs Data                 | 1261440 (400%) | 315360 (100%) |
| Original Ordering, gzip -9    | 146191 (46.4%) | **94440 (29.9%)** | 
| Original Ordering, zstd -9    | **130802 (41.5%)** | 95240 (30.2%) |
| Reordered, gzip -9            | 119046 (37.7%) | 114779 (36.4%) |
| Reordered, zstd -9            | **110001 (34.9%)** | **110119 (34.9%)** |

Editing note: What? This is not the result I expected.  Why does this happen?  Why would the size of each tile being encoded change how effective gzip / zstd can compress the stream at a high level.

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

![VPX Coding Portion of demo](https://github.com/user-attachments/assets/6b48eb03-068c-4a27-892e-0fdbf96eef53)

The general idea is for every bit coming in, the decoder considers the `range` of possibilities left in the current value that's been decoded.  And it determines a `split` based on the probability of the outcome being a 0 or a 1 and that `range`. It also keeps track of a `value` sort of like a cursor.  If the current `value` is >= the `split` then the bit is a `1` otherwise it's a `0`.  If it is a `1` then the new `value` and `range` update from that `split`.  Then, if the `range` is now less than 1/2 the possible range, new data gets shifted into `value` to keep feeding the system.

In the image above, you can see the split, where gradually, the value is drained down because the split is so predominantly 1, and just before the second bit read, because the probability of it being a 1 is so much lower, it significantly reduces the porportion of value to range.

To speed things up, vpx coder keeps 24 (or 56 if 64-bit) bits worth of immediate to pull from, when this is drained, it reads another 3 (or 7) bytes from the buffer and refills it.

Whenever an unlikely path is taken, for instance, if a 0 is very unlikely, but is selected none the less, it can take several bits to encode that 0.  But when a likely path is taken, you can get bit after bit out without it consuming even one bit. You just have to know the chance on both the encoder and decoder so each side can make sense of the bitstream.

If you are curious to use vpx coding yourself for a project, for instance if you have a situation where you have a series of symbols, or numbers you want to express, or really, anything that can boil down to a bitstream of 0's and 1's, where you know how likely the next bit is to be a 0 or 1 is, you can use my project [vpxcoding](https://github.com/cnlohr/vpxcoding), that has an encoder, and a decoder.  And, the decoder is a header-only C library that works well for microcontrollers.  It's a little bit heavier than huffman to decode, but not by much!  To efficiently decode VPX you need a log2 table, which takes up 256 bytes (could be RAM or ROM), and only 364 bytes of x86 code to decode.  And for a decoding context, it only takes a 33-byte structure in RAM.

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
| VPX (reverse LZSS) | **673 (17.6%)** |
| Huffman (2-table+LZSS) | 704 (18.4%) |
| Huffman (2 table+reverse LZSS)† | 856 (22.4%) |

† we used this on the final project.  See rationale below.

Note: these tests were generated with `make sizecomp` the code for several of these tests is in the `attic/` folder.

Because we are only storing a stack, we only need to save the current location and number of notes remaining, so with 18 as the deepest we can go, our state size is only 72 bytes!

### More Observations

Note, when not using lzss, the uptick in size because to use VPX, you have to have a probability table, and huffman tables can be used in lower compression arenas to more effectivity.

So, not only is our decoder only about 50 lines of code, significantly simpler than any of the big boy compression algorithms... It can even beat every one of our big boy compressors!

This VPX solution perform VPX coding on the notes, note-lengths, and time between notes.  But it **also** perform vpx coding on the LZSS callbacks.

I decided to go back to huffman, mostly for the sake of the video and visualization! It also gave me a chance to express Exponential-Golomb coding.  Huffman is extremely simple to decode, and could have been done without any header libraries.  Even though the vpx code is available by virtue of the video, I wanted to show what it would look like with huffman.

To compare apples-to-relatively-apples, I decided to do a huffman approach, with LZSS backtracking using Exponential-Golomb coding.  It was 856 bytes, just a little less compressed, compared to 673 bytes for the VPX + Forward LZSS.

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

**TODO** How did Onslaught select the symbols?

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

## K-Means

Frustrated, I was visiting my brother in early 2024, and I started explaining my problem and both him and his wife were like "why didn't k-means work?" and I proceeded to explain the problem "why didn't k-means work?" then when I was done they asked "You didn't try k-means, did you?"

...no

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

**TODO** ef42

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

**TODO** Explain why we can't have 4 greyscale levels

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

![Tile Transition Graph](https://github.com/user-attachments/assets/2a30a233-0e05-4b8b-a444-b2aecca109f1)

If you zoom out, it looks really pretty.

![Zoomed out transition graph](https://github.com/user-attachments/assets/b71c4f91-3243-4ae3-b0ba-d4114912d2e1)

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



## Run Length Probability Tables

# The ch32v006 implementation

# The web viewer demo

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
 - [ ] Motion Vectors - cannot - is mutually exclusive with 
 - [x] Reference previous tiles.
 - [x] Add color inversion option for glyphs.  **when implemented, it didn't help.**
 - [x] https://engineering.fb.com/2016/08/31/core-infra/smaller-and-faster-data-compression-with-zstandard/ **compared with audio compression,  It's not that amazing.**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitreader.c **winner winner chicken dinner**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitwriter.c **winner winner chicken dinner**

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
