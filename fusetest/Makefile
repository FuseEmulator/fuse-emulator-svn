# Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>

# This program is licensed under the GNU General Public License. See the
# file `COPYING' for details

Z80AS := pasmo
Z80ASFLAGS := --alocal

target := fusetest.tap

all: $(target)

%.tap: %.asm
	$(Z80AS) $(Z80ASFLAGS) --tapbas $< $@

fusetest.tap: \
	atiming.asm \
	fusetest.asm \
	print.asm \
	sync.asm \
	tests.asm \

clean:
	rm -rf $(target)
