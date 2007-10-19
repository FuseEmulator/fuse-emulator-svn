; Tests run by the test harness

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details
	
; Check undocumented flags after BIT n,(IX+d) (bug #1726543)
	
bitnixtest
PROC
	ld ix, _data - 0x44
	bit 5, (ix+0x44)
	push af
	pop bc
	ld a, 0x10
	cp c
	ret z
	ld a,c
	ret

_data	defb 0xff
	
ENDP
	
; Check for the undocumented behaviour of DAA (patch #1724193)
	
daatest
PROC
	ld bc, 0x9a02
	ld hl, 0xcbdd
	push bc
	pop af
	daa
	push af
	pop bc
	add hl, bc
	ld a, h
	or l
	ret z
	ld a, b
	ret
ENDP

; Check the behaviour of LDIR at contended memory boundary (revision 2841).
	
ldirtest
PROC
	call interruptsync

	cp 0x00			; 92
	ret nz			; 99

	ld hl, 0xfdfe		; 104
	ld (hl), _isr % 0x100	; 114
	inc hl			; 124
	ld (hl), _isr / 0x100	; 130

	ld hl, 0x373b		; 140
	call delay		; 150

	ld hl, 0x0000		; 14289
	ld de, 0x7fff		; 14299
	ld bc, 0x0002		; 14309
	ldir			; 14319, 14358

	ld hl, 0xd6bb		; 14374
	call delay		; 14384

	jp atiming		; 69355

_isr	pop hl
	ret
ENDP

; Check for IN timings and floating bus behaviour (bug #1708957)

contendedintest
PROC
	ld hl, 0x5a0f
	ld b,(hl)
	push bc
	ld (hl), 0x40
	
	call interruptsync

	cp 0x00			; 92
	jr nz, _fail		; 99

	ld hl, 0xa797		; 106
	call delay		; 116

	ld bc, 0x40ff		; 43019
	ld d, 0x40		; 43029

	ld hl, 0x5a0f		; 43036

	in a,(c)		; 43046; floating bus read at 43069

	pop bc
	ld (hl),b

	cp d
	ret

_fail	pop bc
	ld hl, 0x5a0f
	ld (hl), b
	ret
ENDP

; Test memory contention

contendedmemorytest
PROC
	ld hl, _in
	ld de, 0x7ffe
	ld bc, _inend - _in
	ldir
	
	call interruptsync
	
	cp 0x00			; 92
	ret nz			; 99

	ld hl, 0xfdfe		; 104
	ld (hl), _isr % 0x100	; 114
	inc hl			; 126
	ld (hl), _isr / 0x100	; 130

	ld hl, 0x374d		; 140
	call delay		; 150

	ld a, 0xff		; 14307
	call 0x7ffe		; 14314

	ld hl, 0xd6cb		; 14358
	call delay		; 14368
	
	jp atiming		; 69355

_isr	pop hl
	ret
	
_in	in a, (0xff)		; 14331
	ret			; 14348
_inend

ENDP
