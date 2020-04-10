.PHONY: clean test release

PROGRAM = hub.com

#DOCKER_IMAGE = fr3nd/sdcc:3.8.0-1
#AS = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdasz80
#CC = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdcc
#HEX2BIN = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} hex2bin
AS = sdasz80
CC = sdcc
HEX2BIN = hex2bin
INCLUDEDIR = include

S_FILES := $(wildcard *.s) $(wildcard $(INCLUDEDIR)/*.s)
REL_FILES := $(subst .s,.rel,$(S_FILES))
C_FILES := $(wildcard $(INCLUDEDIR)/*.c)
IHX_FILES := $(subst .c,.ihx,$(C_FILES))
REL_IHX_FILES := $(subst .c,.rel,$(C_FILES))
LIB_FILES := $(wildcard $(INCLUDEDIR)/*.lib)

CCFLAGS := --code-loc 0x0178 --data-loc 0 --disable-warning 196 -mz80 --no-std-crt0 --opt-code-speed
#CCFLAGS := --code-loc 0x0178 --data-loc 0 --disable-warning 196 -mz80 --no-std-crt0 --opt-code-size
#CCFLAGS := --code-loc 0x0178 --data-loc 0 --disable-warning 196 -mz80 --no-std-crt0

all: $(PROGRAM)

%.rel: %.s
	$(AS) -o $(@:.s=.rel) $^

%.ihx: %.c
	$(CC) $(CCFLAGS) -o $(@:.c=.ihx) $^

$(subst .com,.ihx,$(PROGRAM)): $(REL_FILES) $(IHX_FILES) $(C_FILES) $(subst .com,.c,$(PROGRAM))
	$(CC) $(CCFLAGS) -I$(INCLUDEDIR) $(REL_FILES) $(REL_IHX_FILES) $(LIB_FILES) $(subst .com,.c,$(PROGRAM))

$(PROGRAM): $(REL_FILES) $(IHX_FILES) $(subst .com,.ihx,$(PROGRAM))
	@mkdir -p bin
	$(HEX2BIN) -e com $^
	@cp $@ bin/

clean:
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
	rm -f $(INCLUDEDIR)/*.com $(INCLUDEDIR)/*.asm $(INCLUDEDIR)/*.lst $(INCLUDEDIR)/*.sym $(INCLUDEDIR)/*.bin $(INCLUDEDIR)/*.ihx $(INCLUDEDIR)/*.lk $(INCLUDEDIR)/*.map $(INCLUDEDIR)/*.noi $(INCLUDEDIR)/*.rel
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
