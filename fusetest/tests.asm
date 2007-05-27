; Tests run by the test harness

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details
	
; Check undocumented flags after BIT n,(IX+d) (bug #1726543)
	
bitnixtest
PROC
	ld ix, _data - 0xc4
	bit 5, (ix+0xc4)
	push af
	pop bc
	ld a, 0x7c
	cp c
	ret z
	ld a,c
	ret

_data	defb 0xe7
	
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

	ld hl, 0x0220		; 124

_delay	dec hl			; 134 + n * 26
	ld a, l			; 140 + n * 26
	or h			; 144 + n * 26
	jr nz, _delay		; 148 + n * 26

	nop			; 14273
	nop			; 14277
	nop			; 14281
	nop			; 14285

	ld hl, 0x0000		; 14289
	ld de, 0x7fff		; 14299
	ld bc, 0x0002		; 14309
	ldir			; 14319, 14358

	ld hl, 0x0842		; 14374

_delay1	dec hl			; 14384 + n * 26
	ld a, l			; 14390 + n * 26
	or h			; 14394 + n * 26
	jr nz, _delay1		; 14398 + n * 26

	nop			; 69343
	nop			; 69347
	nop			; 69351
	jp atiming		; 69355

_isr	pop hl
	ret
ENDP
