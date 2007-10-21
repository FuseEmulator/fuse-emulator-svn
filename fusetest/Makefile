# Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>

# This program is licensed under the GNU General Public License. See the
# file `COPYING' for details

Z80AS := pasmo
Z80ASFLAGS := --alocal

targets := contention.tap fusetest.tap

all: $(targets)

%.tap: %.asm
	$(Z80AS) $(Z80ASFLAGS) --tapbas $< $@

contention.tap: \
	atiming.asm \
	contention.asm \
	delay.asm \
	print.asm \
	sync.asm

fusetest.tap: \
	atiming.asm \
	delay.asm \
	fusetest.asm \
	guessmachine.asm \
	print.asm \
	sync.asm \
	tests.asm \

clean:
	rm -rf $(targets)
