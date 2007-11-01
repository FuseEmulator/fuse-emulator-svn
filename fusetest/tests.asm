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
	ld a, 0x30
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

	ld hl, sync_isr + 1	; 104
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

; Check for contended IN timings (part of bug #1708957)

contendedintest
PROC
	ld bc, _delay1
	ld hl, _table1
	call guessmachine_table
	
	ld bc, _delay2
	ld hl, _table2
	call guessmachine_table
	
	call interruptsync

	cp 0x00			; 92
	jr nz, _fail		; 99

	ld hl, sync_isr + 1	; 106
	ld (hl), _isr % 0x100	; 116
	inc hl			; 126
	ld (hl), _isr / 0x100	; 132

	ld hl, (_delay1)	; 142
	call delay		; 158

				; 48K / 128K / +3 timings
	ld bc, 0x40ff		; 43036 / 43574 / 43574
	in a,(c)		; 43046 / 43584 / 43584

	ld hl, (_delay2)	; 43070 / 43608 / 43596
	call delay		; 43086 / 43624 / 43608

	jp atiming		; 69355 / 70375 / 70735

_isr	pop hl
	ret

_fail	ret

_table1	defw 0xa77e
	defw 0xa77e + 0x001a + 4 * 0x0080
	defw 0xa77e + 0x001a + 4 * 0x0080
_table2 defw 0x669d
	defw 0x669d - 0x001a - 4 * 0x0080 + 0x03fc
	defw 0x669d - 0x001a - 4 * 0x0080 + 0x03fc + 0x000c

_delay1	defw 0x0000
_delay2	defw 0x0000

ENDP

; Check for floating bus behaviour (rest of bug #1708957)

floatingbustest
PROC
	ld bc, _delay
	ld hl, _table
	call guessmachine_table
	
	ld hl, 0x5a0f
	ld b,(hl)
	push bc
	ld (hl), 0x53
	
	call interruptsync

	cp 0x00			; 92
	jr nz, _fail		; 99

	ld hl, (_delay)		; 106
	call delay		; 122

				; 48K / 128K timings
	ld bc, 0x40ff		; 43019 / 43557
	ld d, 0x53		; 43029 / 43567

	ld hl, 0x5a0f		; 43036 / 43574

	in a,(c)		; 43046 / 43584
				; floating bus read at 43069 / 43607

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
	
	ld hl, _nop
	ld de, 0x8001 - ( _nopend - _nop )
	ld bc, _nopend - _nop
	ldir
	
	call interruptsync
	
	cp 0x00			; 92
	ret nz			; 99

	ld hl, sync_isr + 1	; 104
	ld (hl), _isr % 0x100	; 114
	inc hl			; 124
	ld (hl), _isr / 0x100	; 130

	ld hl, (_delay1)	; 140
	call delay		; 156

				; 48K / 128K / +3 timings
	call 0x7fff		; 14318 / 14344 / 14346

	ld hl, (_delay2)	; 14355 / 14381 / 14384
	call delay		; 14371 / 14397 / 14400
	
	jp atiming		; 69355 / 70375 / 70375

_isr	pop hl
	ret
	
_nop	nop			; 14335 / 14361 / 14363
	ret			; 14345 / 14371 / 14374
_nopend

_table1	defw 0x3752, 0x3752 + 0x001a, 0x3752 + 0x001a + 0x0002
_table2	defw 0xd6c8, 0xd6c8 - 0x001a + 0x03fc, 0xd6c8 - 0x001a - 0x0003 + 0x03fc

_delay1	defw 0x0000
_delay2	defw 0x0000

ENDP
