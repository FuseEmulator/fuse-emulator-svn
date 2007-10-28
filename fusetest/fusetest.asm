; Fuse regression testing harness

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

ORG 0xc000
	
main
PROC
	call interruptsync_init

	ld hl, _framestring
	call printstring

	call framelength

	ld a, c
	and b
	cp 0xff
	jr nz, _ok

	;; Frame length couldn't be determined
	ld hl, _unknownstring
	call printstring

	ret			; Done

_ok
	push bc
	
	ld hl, _conststring
	call printstring
	ld a, b
	call printa
	ld a, c
	call printa
	ld a, 0x0d
	rst 0x10

	ld hl, _machinetype
	call printstring
	pop hl
	call guessmachine
	ld a, 0x0d
	rst 0x10	

	ld hl, _testdata

_test	ld a,(hl)
	cp 0x00
	jr z, _end

	call printstring
	inc hl
	push hl
	call _jumphl

	jr z, _pass
	ld b,a
	ld hl, _failstring1
	call printstring
	ld a,b
	call printa
	ld hl, _failstring2
	call printstring
	jr _next

_pass	ld hl, _passstring
	call printstring

_next	pop hl
	inc hl
	inc hl

	jr _test

_end	halt

_jumphl ld e,(hl)
	inc hl
	ld d,(hl)
	ex de,hl
	jp (hl)
	
_passstring defb '... passed', 0x0d, 0
_failstring1 defb '... failed (0x', 0
_failstring2 defb ')', 0x0d, 0

_testdata
	defb 'LDIR', 0
	defw ldirtest

	defb 'BIT n,(IX+d)', 0
	defw bitnixtest

	defb 'DAA', 0
	defw daatest

	defb 'Contended IN', 0
	defw contendedintest

	defb 'Contended memory', 0
	defw contendedmemorytest
	
	defb 0

_framestring defb 'Frame length ', 0
_unknownstring defb 'unknown', 0x0d, 0
_conststring defb '0x8000 + 0x', 0
_machinetype defb 'Machine type: ', 0
	
ENDP

INCLUDE tests.asm
	
INCLUDE sync.asm
INCLUDE delay.asm
INCLUDE atiming.asm
INCLUDE print.asm
INCLUDE framelength.asm
INCLUDE guessmachine.asm

END	main
