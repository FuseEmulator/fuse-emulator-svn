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
	ld bc, _delay1
	ld hl, _table1
	call guessmachine_table

	ld bc, _delay2
	ld hl, _table2
	call guessmachine_table
	
	call interruptsync

	cp 0x00			; 92
	ret nz			; 99

	ld hl, 0xfdfe		; 104
	ld (hl), _isr % 0x100	; 114
	inc hl			; 124
	ld (hl), _isr / 0x100	; 130

	ld hl, (_delay1)	; 140
	call delay		; 156

				; 48K / 128K / +3 timings
	ld hl, 0x0000		; 14289 / 14315 / 14322
	ld de, 0x7fff		; 14299 / 14325 / 14332
	ld bc, 0x0002		; 14309 / 14335 / 14342
	ldir			; 14319, 14358 / 14345, 14384 / 14352, 14380

	ld hl, (_delay2)	; 14374 / 14400 / 14400
	call delay		; 14390 / 14416 / 14416

	jp atiming		; 69355 / 70375 / 70375

_isr	pop hl
	ret

_table1	defw 0x3735, 0x3735 + 0x001a, 0x3756
_table2	defw 0xd6b5, 0xd6b5 - 0x001a + 0x03fc, 0xda9b

_delay1	defw 0x0000
_delay2	defw 0x0000

ENDP

; Check for IN timings and floating bus behaviour (bug #1708957)

contendedintest
PROC
	ld bc, _delay
	ld hl, _table
	call guessmachine_table
	
	ld hl, 0x5a0f
	ld b,(hl)
	push bc
	ld (hl), 0x40
	
	call interruptsync

	cp 0x00			; 92
	jr nz, _fail		; 99

	ld hl, (_delay)		; 106
	call delay		; 122

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

_table	defw 0xa791, 0xa791 + 0x001a + 4 * 0x0080
_delay	defw 0x0000

ENDP

; Test memory contention

contendedmemorytest
PROC
	ld bc, _delay1
	ld hl, _table1
	call guessmachine_table

	ld bc, _delay2
	ld hl, _table2
	call guessmachine_table
	
	ld hl, _in
	ld de, 0x7ffe
	ld bc, _inend - _in
	ldir
	
	call interruptsync
	
	cp 0x00			; 92
	ret nz			; 99

	ld hl, 0xfdfe		; 104
	ld (hl), _isr % 0x100	; 114
	inc hl			; 124
	ld (hl), _isr / 0x100	; 130

	ld hl, (_delay1)	; 140
	call delay		; 156

				; 48K / 128K / +3 timings
	ld a, 0xff		; 14307 / 14333 / 14335
	call 0x7ffe		; 14314 / 14340 / 14342

	ld hl, (_delay2)	; 14358 / 14384 / 14387
	call delay		; 14374 / 14400 / 14403
	
	jp atiming		; 69355 / 70375 / 70375

_isr	pop hl
	ret
	
_in	in a, (0xff)		; 14331 / 14357 / 14359
	ret			; 14348 / 14374 / 14377
_inend

_table1	defw 0x3747, 0x3747 + 0x001a, 0x3763
_table2	defw 0xd6c5, 0xd6c5 - 0x001a + 0x03fc, 0xdaa4

_delay1	defw 0x0000
_delay2	defw 0x0000

ENDP
