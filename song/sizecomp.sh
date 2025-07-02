#!/bin/bash

COMPLIST="fmraw.dat.xml fmraw.dat.json.jq fmraw.dat.json badder-apple-only-kick-track.mid fmraw.dat fmraw.dat.bson"

cat fmraw.dat.json | jq > fmraw.dat.json.jq
rm -rf fmraw.dat.bson
beesn -x -i fmraw.dat.json -o fmraw.dat.bson

i=0
for s in ${COMPLIST}; do \
	declare "results_0$i=$(cat $s | wc | tr -s ' ' | cut -d' ' -f4-)"
	declare "results_2$i=$(cat $s | gzip -9 | wc | tr -s ' ' | cut -d' ' -f4-)"
	declare "results_3$i=$(cat $s | zstd -9 | wc | tr -s ' ' | cut -d' ' -f4-)"
	declare "results_4$i=$(cat $s | bzip2 -9 | wc | tr -s ' ' | cut -d' ' -f4-)"
	declare "results_1$i=$(cat $s | heatshrink/heatshrink | wc | tr -s ' ' | cut -d' ' -f4-)"
	i=$((i+1))
done

r00="results_00"
r01="results_01"
r02="results_02"
r03="results_03"
r04="results_04"
r05="results_05"
r10="results_10"
r11="results_11"
r12="results_12"
r13="results_13"
r14="results_14"
r15="results_15"
r20="results_20"
r21="results_21"
r22="results_22"
r23="results_23"
r24="results_24"
r25="results_25"
r30="results_30"
r31="results_31"
r32="results_32"
r33="results_33"
r34="results_34"
r35="results_35"
r40="results_40"
r41="results_41"
r42="results_42"
r43="results_43"
r44="results_44"
r45="results_45"


echo "| Compression | .xml | .json (jq formatted) | .json.min | .bson | .mid | .dat | "
echo "| -- | -- | -- | -- | -- | -- |"
echo "| uncompressed | ${!r00} | ${!r01} | ${!r02} | ${!r05} | ${!r03} | ${!r04} |"
echo "| [heatshrink](https://github.com/atomicobject/heatshrink) | ${!r10} | ${!r11} | ${!r12} | ${!r15} | ${!r13} | ${!r14} |"
echo "| gzip -9      | ${!r20} | ${!r21} | ${!r22} | ${!r25} | ${!r23} | ${!r24} |"
echo "| zstd -9      | ${!r30} | ${!r31} | ${!r32} | ${!r35} | ${!r33} | ${!r34} |"
echo "| bzip2 -9     | ${!r40} | ${!r41} | ${!r42} | ${!r45} | ${!r43} | ${!r44} |"


gcc attic/fmcomp-singletable.c -o attic/fmcomp-singletable -I../common -lm
gcc attic/fmcomp-huffman.c -o attic/fmcomp-huffman -I../common -lm
gcc attic/fmcomp-vpx.c -o attic/fmcomp-vpx -I../common -lm
gcc attic/fmcomp-tripletable.c -o attic/fmcomp-tripletable -I../common -lm
#gcc attic/fmcomp-tripletable.c -o attic/fmcomp-tripletable -I../common -lm
gcc attic/fmcomp-huffman-lzss2.c -o attic/fmcomp-huffman-lzss2 -I../common -lm

echo "..."
echo "| Compression | Size |"
echo "| -- | -- |"
echo "| Raw .dat | ${!r04} |"
echo "| Huffman (1 table) | $(attic/fmcomp-singletable | tail -n 1) |"
echo "| Huffman (3 table) | $(attic/fmcomp-tripletable | tail -n 1) |"
echo "| VPX (no LZSS) | $(attic/fmcomp-vpx | tail -n 1) |"
echo "| VPX (entropy LZSS) | $(./fmcomp-vpx-lzss | tail -n 1) |"
echo "| Huffman (2-table+LZSS) | $(attic/fmcomp-huffman-lzss2 | tail -n 1) |"
echo "| Huffman (2 table+reverse LZSS) | $(./fmcomp-huffman-reverselzss2 | tail -n 1) |"


