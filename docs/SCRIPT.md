In engineering, the hardest problem is knowing if something can be done. If
you know it’s possible, it’s just a matter of getting there. A few years ago,
my friend Luke Beno was working on a board and had a fixed footprint. I
thought there was no way he would make it fit.  But he looked at me and said
"it always fits." (And it did).  I’ve begun to live my life that way.  It’s
ALWAYS possible. And I’ve started applying this mentality to software. It lead
me on a journey where the foundations of my understanding of information
theory would be shaken to their core.

This is the Bad Apple music video.  And I’ve encoded all six thousand five
hundred and seventy frames and my own chiptune-y take on the song into the
64.5kB in this CH32V006, a new RISC-V microcontroller by WCH, the makers of
the CH32V003, which has become beloved by some of my favorite makers.

The story of the Bad Apple music video is an amazing and unlikely one,
starting as a track in an episode of the video game Touhou that was
professionally covered by Masayoshi Minoshima (簑島マサヨシ) and voiced by Nomico.
When a fan of the game game, Μμ, drew and uploaded a storyboard to Nico Nico
Douga where a collaborative lead by Anira (あにら) took up the baton it and
turned it into what so many people know today as the "Bad Apple" music video.

While the music video remains a cultural touchstone to many outside of the
engineering professions, it’s become something else entirely to those within.
Programmers and engineers have strived to play Bad Apple on all manner of
display technologies and computing devices.  Where you can find this video
compressed and played back on devices those who created the devices would
expect motion video playback to be completely impossible.  For many it’s become
an obsession and I am no exception. This is the story of 8 years of trials,
tests, failures and eventually triumphs from the wider community, other
bad-applers, and some of my friends. This is the story my bad-der apple. 

Video compression is a field that studies how to represent video in as few bits
as possible given practical real-world constraints. In a world where bandwidth
costs can be the major driver for some industries, even small savings of data
transfer can result in millions of dollars of cost savings. The thought
processes and techniques we are going to cover are not specific to video but
are more general within the field of compression.

There’s many different algorithms that uses many different techniques to do
video compression.  The best video compression algorithms have taken
man-decades of effort and work to develop, tweak and improve.  Algorithms
like h.265 by the  Video Coding Experts Group, or VP9 by Google, where even
small increases in quality or compression ratio can result in millions of
dollars saved. So, while there’s no way me and my friends could rival the
compression ratios of the best algorithms, both because of my lack of
experience, skill, and the fact I am interested in processors with
1/1,000,000th the computational power of a desktop PC, it does make sense to
ask, what does it look like to try to compress this music video with
state-of-the-art compression?

Hmm... this is going to be a doozie (reword).

When I started badderapple, I had the hopes of compressing the bad apple music
video down to be small enough to fit onto an ESP8285, a stand-alone ESP-8266,
to make the smallest bad apple, as I already had one trick where I could
broadcast video signals over the air by twiddling a GPIO connected to the I2S
bus. I was still learning and eventually gave up trying to get the video at the
original resolution down to under 1MB. I kept putting the project on the shelf
and taking it off.  Eventually I did find a way to get it under 1MB, but it
wasn’t as interesting for other reasons by then.

I would find videos and write-ups of approaches other people took to compress
and play back bad apple. While there were some compression schemes I imagined
that could go down to 400 or 500kB at lower framerates, one of the biggest
touchstones for me was finding out about Onslaught’s approach to compress bad
apple to 170kB on a Commodore 64. Their approach to use glyphs, and while
their video was only 12FPS compared to the original 30, it was mind bending
to me that a video could look so good with such little storage.

Another surprise was in 2022 when there were two bad apple submissions to
Octojam 9, for the XO-CHIP pseudo-microcontroller, both bad apple attempts
and both in under 64kB.  Both using extremely novel approaches to compress.
One, that seemed to use some sort of variable resolution by Koorogi.  And
another by Timendus that came with some incredible deep discussion about
their approach and techniques involved. **DISCUSS THIS MORE**

Both of these approaches would only work at 15FPS and have visual fidelity
losses that were a little higher than my threshold.  I really wanted to keep
the 30FPS from the original video.  It was just a LOT of data, and these folks
had already pulled out all the stops to get down to 64kB.

I would pick this project back up, tinker, reproduce other’s work, while these
new approaches helped weave together a sort of framework in my head of what
could be done, nothing really struck the INTJ focus in me, that was until the
announcement of the ch32v006 from WCH. This was a target. A new $0.10 RISC-V
chip. With 62kB of flash, and at the same time Ronbo, a display company I have
done business with was releasing their 64x48 pixel OLED display for under
$1/ea.  Paired together, this was a target, a dream.  One that that seemed
almost impossible, and yet tantalizingly close

That on-again off-again mentality that was living in my brain rent-free for the
last 10 years ramped up into an all-out obsession.

I set to work trying to hone my own approach, and sharpening the tools in my
toolbelt. I knew I wanted to use glyphs (or tiles). But, how does one generate
an optimal set of glyphs? I was doing some terrrrrribly inefficient methods
trying to generate them based on existing tiles, and averaging, but it was
becoming unwieldy and slow.  

(It wasn’t until I was talking to my brother and his wife (both PHDs in math)
that they just causally said "oh K-means will just do that for you". I rolled
my eyes, yeah, right, like the textbook approach could "just work" on
64-dimensional problem (since each pixel intensity within a block is
effectively another dimension) with 315,000 tiles. Let alone be even remotely
tolerable. But they assured me that it was simple and was fine.)

I went home that evening and to humor them I gave it a shot, and much to my
shock and awe... It just ... worked.  It produced a beautiful tileset.  With
that I could construct frame after frame of just information about what tiles
to use instead of all the pixels in the image.  It had its flaws, like there
were very obvious lines in the "grid" that the tiles made up, but that was a
problem for tomorrow.

We’re going to be covering a few algorithms in this video, and because I
learned these things and thought each one was the bees knees you’re going to
have to learn about them too!

Let’s start with the audio.

Audio:

Bad Apple is a music video after all! But I’m not a music guy, but I have a
little experience editing MIDIs. I couldn’t find anyone to help out, so you
guys get stuck with programmer music.  I found a MIDI that was uploaded by
livingston26, credited to cherry_berry17.  The midi was vaguely close to what I
was hoping for, since I was worried about the complexity of the playback
engine, I decided everything should just be plain notes.

I also knew I would be limited in how many voices, (or concurrent tones) I
could use. So, my general rules were:  I wanted the song to consist of notes,
quantized to some sane pattern, and never more than 4 playing concurrently.

I used midi-edit to fix up a few bugs in the song, more accurately quantize
the song, and make it all fit into just 4 voices.

Once I was done I had a little MIDI file that I was confident I’d be able to
write a player for in very little code.

Next, I had to figure out how I was going to store this MIDI file.  To make
sense of the midi file, I tried converting it to JSON.  Where each note was a
reference that had a start and stop time.  The JSON alone, even if minified
blew the entire space budget for the whole device.  Using compression
algorithms like gzip brought that down to 8kB, or ZSTD, the current state of
the art, 6kB. But, this was compressing a JSON.  this was not going to do.

What if instead of storing the data as a JSON file, which was rather verbose,
I compressed the MIDI file, and interpreted that?  Well, the MIDI file was
12kB, but compressed down to 1042 bytes both with gzip and zstd!

That was huge! A 6:1 ratio, still providing exactly the same information,
without any loss.

Entropy:

This gets into what a principle that underlines this entire project.  Entropy.
We need to get rid of entropy.  Compression algorithms do this, but they can
only do so well.  We can find entropy elsewhere and when we get rid of it,
the compression systems can work even better.

To clarify, this is why sending JSON blobs around between servers and clients
isn’t the best idea, and why justifying it by saying compression will handle
it... Is just not a good move.  By compressing a binary format for my data,
using exactly the same industry standard compression, I was able to make the
data burden 6 times lighter!  The same is true for just about any other field.
If you store your numbers as just numbers, and compress them, they are going
to be much much larger than if you store your numbers in a binary format and
compress that. 

TODO: Try piping the data through protobufs.

I decided to see how much more I could iron out before compressing, so I made
up my own format, that used two bytes per note, packing which note was being
played, how long it was being played for, and how far in the future the next
note would be played. Because in this rendition, there’s 1412 notes, that means
we need 2824 bytes.  Same data, but, over 20x less data... And compressed, that
data was 577 bytes for gzip or 611 bytes for zstd.  Interesting that zstd,
despite having years of innovation and complexity on gzip is actually is larger
than gzip in this case. This gave me a fair lower bound, and one that would
free up a lot of space for everything else.

There’s some interesting techniques that have been very well known for a long
time, like LZSS compression (short for Lempel–Ziv–Storer–Szymanski
compression).  The idea is we can start emitting notes that we want to play,
but, we can at any point in time say "hey, the next section is the same as what
you played n notes ago.  Just go read those notes from then."

One big note is that if you try to compress totally random data with LZSS, the
output is going to be bigger, that’s because for each chunk of data, we need to
let the decompressor know if the next data is new notes or if it’s going to
reference something earlier in the data stream.

This compression scheme is used in common compressors like RAR and all sorts of
bespoke compression systems because it’s so simple to implement.  One common
one used for lots of embedded projects is called heatshrink.  It is where you
explicitly specify a maximum look-back window, and a maximum size for the
amount to play back from that lookback.  You process the bitstream one bit at a
time.  The first bit tells you if the following 8 bits make up a byte, or if
the following bytes are a callback to earlier in the file.  This did well, 889
bytes, but if I was going to spend 8 years of my life on a project, I wasn’t
going to settle for that.

In addition, heatshrink (And for that matter, most major compression algorithms)
assume you can reference the decoded data.  But, the processor I landed on
doesn’t have very much RAM. I wanted to save as much as possible for the video
decoding.  I decided to change my compression scheme to instead of referencing
the uncompressed data, reference the compressed data, that way I wouldn’t need
to maintain a buffer.

This has an added cost because very rarely would you want to reference bits
that don’t align with a note, so most reverse jumps would not be useful, so, it
is a little worse from an entropy point of view and not by a little bit.  The
smallest I was able to get the compression using regular LZSS with VPX coding
(that we’ll cover later) was 547 bytes.  But, what I settled on was 672 bytes.
Giving up these 125 bytes had the benefit of needing virtually no RAM, and that
was a trade-off I was willing to take.

While there are a variety of techniques to figure out where patterns showed up
previously, because there are only 1412 notes, it wasn’t hard to just search
all previous bits in the steam to see what the most efficient choice we could
make would be for what to reference in the past, or to just keep emitting
notes.  We live in the future where O(N^3) algorithms grow slowly enough we can
just laugh off some moderate N sizes.

When heatshrink emits a raw symbol or literal, it emits a 1 bit then the
literal byte that needs to be decoded.  When it creates a back reference, it
first emits a 0 bit, then a number, encoded using two’s compliment, most
significant bit first, for how far back to look for the reference, and then
another number, encoded with two’s compliment for how many bytes are located
there.  Two’s compliment is just another way of storing a number in binary.
Just like how you write numbers in base 10, two’s complement is just writing
them where each digit can be 0 or 1. And instead of 10’s 100’s 1000’s, you
have 1’s, 2’s, 4’s, etc.

You might be wondering, if heatshrink was 889 bytes, how did my compression
get down to 672 bytes, or, 547 bytes?  While LZSS is an optimal coding
strategy that gets rid of entropy, and is provably perfect, we can still do
better! We have two more tricks up our sleeve. 

The first trick is Exponential-Golomb (pronounced like Shalom but with a
frog in your throat. (https://youtu.be/oBpUr-jWWLQ?t=1921 32:01)) coding
(or as the H.265 video spec calls it UE, for unsigned exponential encoding
which is how I will be referring to it because it’s hard for me to say
Solomon Golomb’s last name).  With UE coding we can emit numbers, like the
back reference distance and length with a variable number of bits instead
of needing to use fixed sizes like heatshrink.

Most of the time, when we reference things in the past, or runs we only
need to reference a small number, but sometimes we need to be able to
reference big jumps... It makes sense that we would want to use the same
number of bits to encode all references... since most of the time the most
significant bits are just zeroes.  The way UE coding works is to figure out
how many bits we are going to need to emit our number. We’re going to add
1 to this number, because for UE to work you need at least one 1 bit.
Next we’ll need to compute how big the number we are emitting is. For
instance, the number we start with is 9, we add 1 to get 10, and 10 can
be represented with a binary number with 4 digits.  We "prefix" the output
with 0’s.  If we emit 3 0’s, and the first bit is a 1, the decoder will
know to expect 4 binary digits (including that 1).  So we can emit 0001010
if we wanted to emit the number 9.

While it takes almost twice the amount of data to emit any one number, than
the number of bits needed to represent that number, there's actually a big 
cost savings here, because most of our numbers are very small.  This is a
similar principle to many systems where you may be able to have some major
prediction system, but then have small residuals where most of the numbers
are small, but some are big.  The net savings can be huge.

And the second trick is huffman trees.

Huffman trees use the idea of symbols.  For us, a note length and next note
time, or note frequency are symbols.  For some systems, these could be letters
in an alphabet, or something like that.  But what we’re trying to get to is
that there’s a finite number of symbols, and they each occur at a frequency. 
Because for instance there are many more instances of the letter "E" in english
than the letter "Q" we could encode "E" using fewer bits than "Q" and get a
savings.

If you make a list of all the symbols, and you pick the two least frequent
symbols, and pair them up.  Then treat that new pair of symbols as a new node,
find the next two least frequent things, whether they be nodes or symbols,
and pair them up.  Each time, building it into a tree.  Then once you join the
last two, now most common nodes, you have generated a tree.

Now, to get to any leaf (or symbol), we just record if we went right or left
at each tree branch.  Doing this lets us theoretically optimize the symbols.
It’s even mathematically provable that huffman trees provide the optimal
encoding for number of whole bits per symbol for any data stream.  But this
does assume you know the frequency of everything.

Anyway, we can take these huffman trees and encode our data, selecting a note,
and outputting a 1 or a 0 each time we take a step along our tree to find the
note we want to emit.  At each step, we need to take one more bit and say
whether we are emitting a note, or, we are going to reach back in time and
invoke LZSS.

To decompress, bit at a time we can read the input stream, determine if this
next note will come from something that can be expressed in bits earlier in the
bit stream, or if a new note needs asdfasdfasdf **TODO** **TODO**

SAVE FOR VPX: TODO: REORGANIZE ME

There’s a neat principle in entropy that can be expressed through the Bernoulli
process.  Imagine a coin that, when flipped comes up with heads 99 times more
than tails.  Thi **TODO** Continue

K-Means:

For those who don’t know, K-means is an approach where you turn your data
set into a series of points.  Can be 1D, 2D, 3D, or as many dimensions as you
can imagine.  As I mentioned, for me this was one dimension for each pixel,
where it went from 0 (or black) to 1 (or white).

For the sake of simplicity, let’s just assume two dimensions since that’s
easy to draw.  So draw out these points.

Now, 

Step 1: Create some points and randomly place some points in that space.
Just pick some number of points, doesn’t have to be how many you’ll end
up with.  Just to get the ball rolling.  These are going to be the "categories"

Step 2: For each category, find all the data points closest to that category
than any other categories.

Step 3: Find the geometric center of the points that fit into the category.

Step 4: Move the category’s location to this geometric center.

Step 5: Repeat steps 2-4 til things calm down.

For me, this looked like generating a bunch of random tiles, our categories,
then finding all the glyphs in the input stream that mapped reasonably to those
tiles, and making the new tile the average color of all the tiles in that group.

Of course when dealing with random noise, lots of these categories aren’t close
to any of the glyphs in the input video stream, so they just get yeeted.

I had pretty good success starting with way too many categories, and slowly
culling them back til I had about 256 tiles.  This seemed like a pretty
reasonable middleground.  On one side, if we have too few tiles, the video will
look like we are compressing garbage… And we could also have a huge number of
categories.  Our video would look perfect, but, we have to store all these as
our own tile set, so we would blow our compression budget.
Selecting 256 tiles, with 3 or 4-levels of greyscale means we need to store/use
4 kilobytes of data.

**TODO** 

* CLay
* Rawdraw
* hardware




[1] https://www.nicovideo.jp/watch/nm3601701


