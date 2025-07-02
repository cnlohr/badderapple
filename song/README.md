# Very compressed song-maker

## Using BadApple-mod.mid

`BadApple-mod.mid` originally has unknown origin for source but is a midi take on "Bad Apple!!" by Zun and was on Touhou. It was uploaded On 2015-03-01 by livingston26 and uploaded [here](https://musescore.com/user/1467236/scores/678091), but credit was given to cherry_berry17.  I was unable to find them.  Then it was transcribed to MIDI.

I heavily modified it to fit the music video more specifically and also match the restricted hardware situation I am operating in.

If anyone knows the original author this passed through, I'd really appreciate knowing. But, it's so heavily modified, I don't think it would be recognizable.

In general this is a cover of the "Bad Apple" version by Alstroemeria Records featuring nomico.  And depending on jurisdiction, they may maintain some form of copyright.  I disclaim all copyright from the song and midi file.  Feel free to treat my transformation of it under public domain.

## Results

To do sanity checks, I decided to compare the compression a few steps along the way.  The compression test dataset for the .dat is the `fmraw.dat` generated file which contains a binary encoding for note, delta time and length.  (All sizes in octets (bytes))

TODO: Add %ages. TODO: REWRITE

| Compression | .xml | .json (jq formatted) | .json.min | .mid | .dat | 
| -- | -- | -- | -- | -- | -- |
| uncompressed | 217686 | 150802 | 85855 | 17707 | 3820 |
| [heatshrink](https://github.com/atomicobject/heatshrink) | 32497 | 23376 | 15644 | 3199 | 1060 |
| gzip -9      | 12001 | 11458 | 10590 | 1329 | 682 |
| zstd -9      | 10052 | 9480 | 8877 | 1332 | 724 |
| bzip2 -9     | 8930 | 8800 | 8676 | 1442 | 830 |

Curiously for small payloads, it looks like gzip outperforms zstd, in spite of zstd having 40 years to improve over it. This is not a fluke, and has been true for many of my test song datasets.

There's an issue, all of the good ones in this list these are state of the art algorithms requiring a pretty serious OS to decode.  What if we only wan to run on a microcontroller?

### My algorithms

| Compression | Size |
| -- | -- |
| Raw .dat | 3820 |
| Huffman (1 table) | 1644 |
| Huffman (3 table) | 1516 |
| VPX (no LZSS) | 1680 |
| VPX (entropy LZSS) | 673 |
| Huffman (2-table+LZSS) | 704 |
| Huffman (2 table+reverse LZSS)† | 856 |

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




