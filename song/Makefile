all : testsynth midiexport fmcomp-vpx-lzss track-float-48000.dat 

CFLAGS:=-Og -g -I../common

midiexport : midiexport.c
	gcc $(CFLAGS) -o $@ $^ -lm

fmraw.dat : midiexport badderapple.mid
	./midiexport < badderapple.mid

track-float-48000.dat : testsynth fmraw.dat
	./testsynth  > track-float-48000.dat

test : testsynth fmraw.dat fmcomp-vpx-lzss
	./fmcomp-vpx-lzss
	./testsynth  | aplay -D pulse -c 1 -t raw -r 48000 -f FLOAT_LE

../playback/badapple_song.h : fmcomp-vpx-lzss fmraw.dat
	./fmcomp-vpx-lzss

fmcomp-vpx-lzss : fmcomp-vpx-lzss.c
	gcc $(CFLAGS) -o $@ $^ -lm

test-vpx-lzss : test-vpx-lzss.c
	gcc $(CFLAGS) -o $@ $^ -lm -I../playback

test-huff-revlzss : test-huff-revlzss.c
	gcc $(CFLAGS) -o $@ $^ -lm -I../playback

testsynth : testsynth.c
	gcc $(CFLAGS) -o $@ $^ -lm

fmraw.dat.jq.json : fmraw.dat.json
	cat fmraw.dat.json | jq > fmraw.dat.jq.json

# Another try with huffman

fmcomp-huffman-reverselzss2 : fmcomp-huffman-reverselzss2.c
	gcc $(CFLAGS) -o $@ $^ -lm

# VPX with forward LZSS
attic/fmcomp-vpx-with-forward-lzss : attic/fmcomp-vpx-with-forward-lzss.c
	gcc $(CFLAGS) -o $@ $^ -lm 

../playback/badapple_song_huffman_reverselzss.h : fmcomp-huffman-reverselzss2 fmraw.dat
	./fmcomp-huffman-reverselzss2

sizecomp : fmraw.dat heatshrink fmcomp-vpx-lzss fmcomp-huffman-reverselzss2
	./sizecomp.sh

heatshrink :
	git clone https://github.com/atomicobject/heatshrink
	cd heatshrink; make

clean :
	rm -rf *.o ~* test-vpx-lzss midiexport fmcomp fmcomp-vpx fmcomp-vpx-lzss testsynth bitlzsstest huffTN_fmraw.dat huffTL_fmraw.dat huffD_fmraw.dat fmraw.dat track-float-48000.dat ../playback/badapple_song.h
