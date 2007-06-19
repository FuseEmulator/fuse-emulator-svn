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

	ld hl, 0xfdfe		; 88
	ld (hl), _isr % 0x100	; 98
	inc hl			; 108
	ld (hl), _isr / 0x100	; 114

	ld hl, 0x374b		; 124
	call delay		; 134

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

	ld hl, 0xa7a9		; 88
	call delay		; 98

	ld bc, 0x40ff		; 43019
	ld d, 0x40		; 43029

	ld hl, 0x5a0f		; 43036

	in a,(c)		; 43046; floating bus read at 43069

	pop bc
	ld (hl),b

	cp d
	ret
ENDP
