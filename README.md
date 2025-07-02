# espbadapple
My shot at bad apple on an '8266.  The source video for bad apple is 512x384


## Prep

```
sudo apt-get install build-essential libavutil-dev libswresample-dev libavcodec-dev libavformat-dev libswscale-dev
```

For msys2 (Windows)

```
pacman -S base-devel mingw-w64-x86_64-ffmpeg 
pacman -S clang llvm clang64/mingw-w64-clang-x86_64-wasm-component-ld mingw-w64-clang-x86_64-wasmer mingw-w64-x86_64-binaryen # For web stuff.
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


### Future TODO
 - [x] Perceptual/Semantic Loss Function
 - [x] De-Blocking Filter
 - [ ] Motion Vectors
 - [x] Reference previous tiles.
 - [x] Add color inversion option for glyphs.  **when implemented, it didn't help.**
 - [x] https://engineering.fb.com/2016/08/31/core-infra/smaller-and-faster-data-compression-with-zstandard/ **compared with audio compression,  It's not that amazing.**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitreader.c **winner winner chicken dinner**
 - [x] https://github.com/webmproject/libvpx/blob/main/vpx_dsp/bitwriter.c **winner winner chicken dinner**

## History

My thought to port bad apple started back in early 2017.  But I quickly ran out of steam.

### RLE

My first attempt was with RLE on the raster image, and I tried a number of other tricks, like variable width integers, etc.  But nothing could get it compress all that much.  With the 512x384 res, it took about 1kB per frame to store.  Total size was about 11,602,173 bytes.

### What is this C64 demo doing?

It wasn't until Brendan Becker showed me https://www.youtube.com/watch?v=OsDy-4L6-tQ -- One of the clear huge boons is that the charset.  They claim 70 bytes/frame, at 12fps.

It's clear that they're doing a 40x25 grid of 8x8's.  That would be 1,000 cells? At any rate

### The lull

I somehow lost interest since there wasn't any really compelling and cool combination that could fit on the ESP8285.  So I stopped playing around until 2023, when the CH32V003 came out.  I continued to refine my algorithms, and playing.

By really pushing things to the limits, one could theoretically fit bad apple in XXX kb **TODO IMAGE**

Then, I spent a while talking to my brother and his wife, both PhD's in math, and they introduced me to the K-means algorithm.  But by this time, I had been fatigued by bad apple.

But, then, in early 2024, things really got into high gear again, because WCH, the creators of the CH32V003, announced other chips in the series with FLASH ranging from 32kB to 62kB, so it was time for the rubber to hit the road again.

I implemented a k-means approach, and wowee! The tiles that came out of k-means was AMAZING!!!

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

Any time you do an experiment with this you make headway.  It feels like recent AI research.


# Primary Compression Techniques

## Huffman Coding

TODO

## VPX Coding

TODO

## LZSS

TODO

## Reverse-LZSS

TODO

# Song

## Using BadApple-mod.mid

`BadApple-mod.mid` originally has unknown origin for source but is a midi take on "Bad Apple!!" by Zun and was on Touhou. It was uploaded On 2015-03-01 by livingston26 and uploaded [here](https://musescore.com/user/1467236/scores/678091), but credit was given to cherry_berry17.  I was unable to find them.  Then it was transcribed to MIDI.

I heavily modified it to fit the music video more specifically and also match the restricted hardware situation I am operating in.  Then later, @binary1230 added a percussion track.

If anyone knows the original author this passed through, I'd really appreciate knowing. But, it's so heavily modified, I don't think it would be recognizable.

In general this is a cover of the "Bad Apple" version by Alstroemeria Records featuring nomico.  And depending on jurisdiction, they may maintain some form of copyright.  I disclaim all copyright from the song and midi file.  Feel free to treat my transformation of it under public domain.

## Results

To do sanity checks, I decided to compare the compression a few steps along the way.  The compression test dataset for the .dat is the `fmraw.dat` generated file which contains a binary encoding for note, delta time and length.  (All sizes in octets (bytes))

Percentages are compared to my custom binary encoding.

| Compression | .xml | .json (jq formatted) | .json.min | .mid | .dat | 
| -- | -- | -- | -- | -- | -- |
| uncompressed | 217686 (5698%) | 150802 (3947%)| (2247%) 85855 | (463%) 17707 | (100%) 3820 |
| [heatshrink](https://github.com/atomicobject/heatshrink) | 32497 (850%) | 23376 (611%) | 15644 (409%) | 3199 (83%) | 1060 (27%) |
| gzip -9      | 12001 (314%) | 11458 (300%) | 10590 (277%) | 1329 (34%) | 682 (17.8%) |
| zstd -9      | 10052 (263%) | 9480 (248%) | 8877 (232%) | 1332 (35%) | 724 (19.0%) |
| bzip2 -9     | 8930 (234%) | 8800 (230%) | 8676 (227%) | 1442 (37%) | 830 (21.7%) |

Curiously for small payloads, it looks like gzip outperforms zstd, in spite of zstd having 40 years to improve over it. This is not a fluke, and has been true for many of my test song datasets.

There's an issue, all of the good ones in this list these are state of the art algorithms requiring a pretty serious OS to decode.  What if we only wan to run on a microcontroller?

### My algorithms

| Compression | Size |
| -- | -- |
| Raw .dat | 3820 (100%) |
| Huffman (1 table) | 1644 (43%) |
| Huffman (3 table) | 1516 (40%) |
| VPX (no LZSS) | 1680 (44%) |
| VPX (entropy LZSS) | **673 (17.6%)** |
| Huffman (2-table+LZSS) | 704 (18.4%) |
| Huffman (2 table+reverse LZSS)† | 856 (22.4%) |

† we used this on the final project.  See rationale below.

Note: these tests were generated with `make sizecomp` the code for several of these tests is in the `attic/` folder.

Note, when not using lzss, the uptick in size because to use VPX, you have to have a probability table, and huffman tables can be used in lower compression arenas to more effectivity.

I decided to do a 1/2 hour experiment, and hook up VPX (with probability trees) with LZSS, (heatshrink-style).  It was down to around 720 bytes... But I then also used entropy coding to encode the run lengths and indexes where I assumed the numbers were smaller, so for small jumps, it would use less bits, and the size went down to 673 bytes!

So, not only is our decoder only about 50 lines of code, significantly simpler than any of the big boy compression algorithms... It can even beat every one of our big boy compressors!

This VPX solution perform VPX coding on the notes, note-lengths, and time between notes.  But it **also** perform vpx coding on the LZSS callbacks.

I decided to go back to huffman, mostly for the sake of the video and visualization! It also gave me a chance to express Exponential-Golomb coding.

To compare apples-to-relatively-apples, I decided to do a huffman approach, with LZSS backtracking using Exponential-Golomb coding.  It was 704 bytes, just a little less compressed, at 704 bytes.

There's one **big issue** with LZSS, it assumes you can refer back to an earlier part of your stream. This is great when you have lots of RAM, but not great if you are strapped for RAM resources.  So, I took a different approach. I'm going to call it reverse LZSS, which assumes you have no decomrpession buffer.  So this would be suitable for systems where RAM is extremely limited.

Instead of referring back to earlier emitted bytes, we refer back to another part of the bitstream.  From there, the bistream also may refer back to an earlier part of the bistream, with varying lengths, etc.  Compression using reverse lzss is quite costly, because it simulates emitting the bits the whole way along.  I was happy to give up ~150 bytes of storage in exchange for a massive RAM savings.

While we do need to remember where we were in our callback-stack, like which bit we were decoding before the last callback and how many notes to go, that works out to a very small amout of RAM.


## Testing

To produce the audio file for use with ffmpeg, `track-float-48000.dat`, as well as producing `badapple_song.h` in the `playback/` folder, you can use the following command:

```
make track-float-48000.dat ../playback/badapple_song.h
```

### Interprets `BadApple-mod.mid` and outputs `fmraw.dat` using `midiexport`

Midiexport reads in the .mid file and converts the notes into a series of note hits that has both the note to hit, the length of time the note should play (in 16th notes) and how many 16th notes between the playing of this note and the start of the next note.

This intermediate format is a series of `uint16_t`'s, where the note information is stored as:

```c
int note = (s>>8)&0xff;
int len = (s>>3)&0x1f;
int run = (s)&0x07;
```

Assuming a little endian system.

### Ingests `fmraw.dat` and produces `../playback/badapple_song.h`

This employs a 4-part compression algorithm.

1. Uses a [heathrink](https://github.com/atomicobject/heatshrink)-like compression algorithm, based on [LZSS](https://en.wikipedia.org/wiki/Lempel%E2%80%93Ziv%E2%80%93Storer%E2%80%93Szymanski) with the twist that it uses the VPX entropy coding to reduce the cost of smaller jumps and lengths.  So on avearge it can outperform heatshrink or LZSS.
2. Then creates two probability trees.  One for notes, the other for (note length | duration until next note).
3. NOTE: When creating the probabilities or encoding the data, it only considers the probability of the remaining notes after LZSS.  This is crucial for good compression.
4. It creates two VPX probability trees for these two values.
5. It creates another table so that it can tightly pack the (length | duration) tree.
6. It produces a VPX bitstream output.

The .dat file can be used with ffmpeg as follows:

```
ffmpeg -y -f f32le -ar 48000 -ac 1 -i ../song/track-float-48000.dat <video data> <output.mp4>
```

The .h file is used with the playback system by being compiled in.





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
 * fx-92 College 2D+ - - 96x31 - 
 
