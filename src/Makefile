.PHONY: clean libs libsclean
C_FILES := $(wildcard *.c)
IHX_FILES := $(subst .c,.ihx,$(C_FILES))
COM_FILES := $(subst .ihx,.com,$(IHX_FILES))
DOCKER_IMAGE = fr3nd/sdcc:3.7.0-2
AS = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdasz80
CC = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdcc
HEX2BIN = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} hex2bin
INCLUDEDIR = include

CCFLAGS := --code-loc 0x0178 --data-loc 0 --disable-warning 196 -mz80 --no-std-crt0 -I$(INCLUDEDIR) $(INCLUDEDIR)/crt0msx_msxdos_advanced.rel

all: libs $(COM_FILES)

libs:
	$(MAKE) -C $(INCLUDEDIR)

libsclean:
	$(MAKE) -C $(INCLUDEDIR) clean

%.ihx: %.c
	$(CC) $(CCFLAGS) $^

%.com: %.ihx
	mkdir -p bin
	$(HEX2BIN) -e com $^
	cp $@ bin/

clean: libsclean
	find dsk \
		-not -name 'COMMAND2.COM' \
		-not -name 'COMMAND.COM' \
		-not -name 'MSXDOS2.SYS' \
		-not -name 'MSXDOS.SYS' \
		-not -name 'NEXTOR.SYS' \
		-not -name 'NEXTORJP.SYS' \
		-not -name 'AUTOEXEC.BAT' \
		-delete || true
	rm -rf dsk/hub dsk/config
	rm -f *.com *.asm *.lst *.sym *.bin *.ihx *.lk *.map *.noi *.rel
	rm -rf bin/
	rm -f hub.zip

test: release
	openmsx -machine msx2 -ext msxdos2 -diska dsk/ -script ../emulation/boot.tcl

release: all
	mkdir -p dsk/hub
	cp bin/hub.com dsk/hub
	cp ../README.md dsk/hub
	7z a hub.zip ./dsk/*

# vim:ft=make
#
