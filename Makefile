all:
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean

test:
	$(MAKE) -C src test

release:
	$(MAKE) -C src release

# vim:ft=make
#
