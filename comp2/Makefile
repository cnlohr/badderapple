all : decodevideo

include ../common/make.inc

ifeq ($(OS),Windows_NT)
#-Wl,--verbose 
LDFLAGS_FFMPEG+=-LC:\\msys64\\mingw64\\bin -lavcodec-60 -lavformat-60 -lavutil-58 -lswscale-7
#-Wl,--no-gc-sections ../common/ffmpegwindows/avcodec-58.dll ../common/ffmpegwindows/avformat-58.dll ../common/ffmpegwindows/avutil-56.dll ../common/ffmpegwindows/swresample-3.dll ../common/ffmpegwindows/swscale-7.dll
else
LDFLAGS_FFMPEG:=-lavcodec -lavformat -lswscale -lavutil 
endif

#  -fsanitize=address -static-libasan
CFLAGS:=-O2 -g  -Wall -Wno-unused-but-set-variable -Wno-unused-result -Wno-unused-variable -Wno-unused-function -mavx2 -DBLOCKSIZE=${BLOCKSIZE} -I../common -DTARGET_GLYPH_COUNT=${TARGET_GLYPH_COUNT} ${FFMPEG_CFLAGS}

hufftreegentest : hufftreegentest.c
	gcc $(CFLAGS) -o $@ $^

decodevideo : ../common/ffmdecode.c decodevideo.c
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS_FFMPEG) $(CFLAGS) $(LDFLAGS)

videoout-${RES}.dat : badapple-sm8628149.mp4 decodevideo
	./decodevideo $< ${RESX} ${RESY} $@

videocomp : videocomp.c ../common/bacommon.h
	gcc $(CFLAGS) -o $@ $< $(LDFLAGS)

stream-${RES}.dat tiles-${RES}.dat : videoout-${RES}.dat videocomp
	./videocomp $< ${RESX} ${RESY} tiles-${RES}.dat stream-${RES}.dat

streamcomp : streamcomp.c ../common/bacommon.h
	gcc $(CFLAGS) -o $@ $< $(LDFLAGS)

video-${RES}.gif : streamcomp stream-${RES}.dat tiles-${RES}.dat
	./streamcomp stream-${RES}.dat tiles-${RES}.dat video-${RES}.gif ${RESX} ${RESY}

midclean :
	rm -rf test-${RES}.gif streamcomp

test.mp4 : video-64x48x8.gif
	ffmpeg -y -f f32le -ar 48000 -ac 1 -i ../song/track-float-48000.dat -r 30 -i video-64x48x8.gif -af adelay=1350 -vf "scale=trunc(iw/2)*4:trunc(ih/2)*4" -r 30 -crf 15 test.mp4

clean : midclean
	rm -rf decodevideo videocomp videoout-${RES}.dat
	rm -rf *~ test.mp4



