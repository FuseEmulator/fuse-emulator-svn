; Program to find the first contended tstate for "any" machine
; Currently works only for machines with 224 tstate lines

; Copyright 2007-2009 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

ORG 0xa000
	
main
PROC
	ld hl, _nop
	ld de, 0x8001 - ( _nopend - _nop )
	ld bc, _nopend - _nop
	ldir

	call interruptsync_init

	ld hl, _framestring
	call printstring

	call framelength

	ld a, c
	and b
	cp 0xff
	jr nz, _ok

	; Frame length couldn't be determined
	ld hl, _unknownstring
	call printstring

	ret			; Done

_ok
	ld (framelen), bc
	
	ld hl, _conststring
	call printstring
	ld a, b
	call printa
	ld a, c
	call printa
	ld a, 0x0d
	rst 0x10

	ld a, (_state)
	cp 0x80
	jp nc, _end

	ld hl, _testingstring
	call printstring
	ld bc, (_testvalue)
	ld a, b
	call printa
	ld a, c
	call printa
	ld hl, _ellipsisstring
	call printstring

	ld hl, (_testvalue)
	call testcontention

	jr z, _uncontended
	ld hl, _contendedstring
_printresult
	call printstring
	jr z, _end

_uncontended
	ld hl, _uncontendedstring
	jr _printresult

_end
	im 1
	ei
	ret

; Define our state machine. Each state has four entries
; State to go to if no contention found
; Modification to test value if no contention found
; State to go to if contention found
; Modification to test value if contention found

; See findcontention.dot for a graphical representation of this

_statemachine

	defb 1		; State 0
	defw 4
	defb 4
	defw -1

	defb 2		; State 1
	defw 112
	defb 4
	defw -1

	defb 3		; State 2
	defw 4
	defb 4
	defw -1

	defb 255	; State 3
	defw 0
	defb 4
	defw -1

	defb 5		; State 4
	defw -1
	defb 4
	defw -1

	defb 6		; State 5
	defw -1
	defb 4
	defw -1

	defb 7		; State 6
	defw -221
	defb 4
	defw -1

	defb 254	; State 7
	defw 224
	defb 7
	defw -224

_state	defb 0		; Our current state
_testvalue defw 20000   ; Current tstate being tested
	
_framestring defb 'Frame length ', 0
_unknownstring defb 'unknown', 0x0d, 0
_conststring defb '0x8000 + 0x', 0
_testingstring defb 'Testing 0x', 0
_ellipsisstring defb '... ', 0
_contendedstring defb 'contended', 0x0d, 0
_uncontendedstring defb 'uncontended', 0x0d, 0

_nop	nop
	ret
_nopend

ENDP

INCLUDE sync.asm
INCLUDE delay.asm
INCLUDE atiming.asm
INCLUDE print.asm
INCLUDE framelength.asm
INCLUDE testcontention.asm

END	main
