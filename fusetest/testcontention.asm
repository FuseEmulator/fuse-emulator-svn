testcontention
PROC
	ld de, 0x00af
	xor a
	sbc hl, de
	ld (_predelay), hl

	push hl
	pop de
	ld hl, (framelen)
	xor a
	sbc hl, de
	ld de, 0x7d21
	add hl, de
	ld (_postdelay), hl

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
	ret

_fail
	ld a, 0x80
	ret

_predelay defw 0x0000
_postdelay defw 0x0000

ENDP
