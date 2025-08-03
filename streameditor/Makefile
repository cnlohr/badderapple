all : streameditor

include ../common/make.inc

streameditor : streameditor.c
	gcc $(CFLAGS) -o $@ $^ $(LDFLAGS) -lGL

clean:
	rm -rf streameditor

