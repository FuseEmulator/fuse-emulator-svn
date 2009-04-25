; Work out where the first contended cycle is

; Copyright 2008 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

first_contended
PROC
	ld hl, first_offset
	xor a
	ld (hl), a

	ld hl, (guessmachine_contended)
	ld de, 0x00b7
	xor a
	sbc hl, de
	ld (_predelay), hl

	ex de, hl
	ld hl, (framelen)
	xor a
	sbc hl, de
	ld de, 0x7d21
	add hl, de
	ld (_postdelay), hl

	ld hl, _nop
	ld de, 0x8001 - ( _nopend - _nop )
	ld bc, _nopend - _nop
	ldir
_test	
	call interruptsync

	cp 0x00
	jr nz, _fail

	ld hl, sync_isr + 1
	ld (hl), _isr % 0x100
	inc hl
	ld (hl), _isr / 0x100

	ld hl, (_predelay)
	call delay
	
	call 0x7fff
	
	ld hl, (_postdelay)
	call delay

	jp atiming

_isr	pop hl
	cp 0x00
	jr z, _next
	cp 0xfd
	ret nc

	;; Time taken to run code was shorter than no contention case
	;; Don't understand this, so give up
	ld a, 0x81
	jr _fail2

_next
	ld hl, first_offset
	inc (hl)
	ld a, (hl)
	cp 0x10
	jr nc, _notfound
	
	ld hl, _predelay
	inc (hl)
	ld hl, _postdelay
	dec (hl)
	jr _test

_fail	ld a, 0x80
_fail2	ld hl, first_offset
	ld (hl), a
	ret

_notfound
	ld a, 0x82
	jr _fail2

_nop	nop
	ret
_nopend

_predelay  defw 0x0000
_postdelay defw 0x0000

ENDP

first_offset defw 0x0000

first_delay_1 defw 0x0000
first_delay_2 defw 0x0000
