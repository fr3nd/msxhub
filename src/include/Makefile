ASM_FILES := $(wildcard *.s)
REL_FILES := $(subst .s,.rel,$(ASM_FILES))
C_FILES := $(wildcard *.c)
REL2_FILES := $(subst .c,.rel,$(C_FILES))
DOCKER_IMAGE = fr3nd/sdcc:3.7.0-2
AS = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdasz80
CC = docker run -it --rm -v $$(pwd):/usr/src -u $$(id -u):$$(id -g) ${DOCKER_IMAGE} sdcc

all: $(REL_FILES) $(REL2_FILES)

%.rel: %.s
	$(AS) -o $^

%.rel: %.c
	$(CC) -mz80 -c -o $@ $^

clean:
	rm -f *.asm *.lst *.sym *.rel

# vim:ft=make
#
