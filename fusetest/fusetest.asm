; Fuse regression testing harness

; Copyright 2007-2008 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

ORG 0xa000
	
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
	
	ld hl, _offsetstring
	call printstring
	call first_contended
	ld hl, first_offset
	ld a, (hl)
	cp 0x80
	jp nc, _first_fail

_first_digits
	push af
	ld hl, _hexstring
	call printstring
	pop af
	call printa

_first2
	ld a, 0x0d
	rst 0x10
	ld a, 0x0d
	rst 0x10

	ld hl, first_offset
	ld a, (hl)
	add a, 0xe8
	ld hl, first_delay_1
	ld (hl), a

	ld hl, first_offset
	ld a, (hl)
	neg
	add a, 0xf8
	ld hl, first_delay_2
	ld (hl), a

	ld hl, _testdata

_test	ld a,(hl)
	cp 0x00
	jr z, _end

	call printstring
	inc hl

        ; See if we should run this test on this machine
        ld a, (guessmachine_guess)
        ld b, a
        inc b
        ld a, (hl)
_loop   rra
        djnz _loop

        inc hl
	push hl
        jr nc, _skip

	call _jumphl

	jr z, _pass
	push af
	ld a, b
	cp 0x00
	jr z, _fail

	; B != 0 => incomplete etc
_incomplete
	ld hl, _incompletestring1
	call printstring
	ld a, b
	call printa
	ld hl, _failstring2
	call printstring
	pop af
	jr _next

_skip   ld hl, _skipstring1
        call printstring
        jr _next

_fail	ld hl, _failstring1
	call printstring
        pop af
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

_end	im 1
	ei
	ret

_jumphl ld e,(hl)
	inc hl
	ld d,(hl)
	ex de,hl
	jp (hl)

_first_fail
	jr z, _first_fail_sync
	cp 0x81
	jr z, _first_fail_short
	cp 0x82
	jp nz, _first_digits
	ld hl, _notfound_string

_first_fail2
	call printstring
	ld hl, first_offset
	ld a, 0x08
	ld (hl), a
	jp _first2

_first_fail_sync
	ld hl, _nosync_string
	jr _first_fail2

_first_fail_short
	ld hl, _short_string
	jr _first_fail2

_passstring defb '... passed', 0x0d, 0
_failstring1 defb '... failed (0x', 0
_failstring2 defb ')', 0x0d, 0
_skipstring1 defb '... skipped', 0x0d, 0
_incompletestring1 defb '... incomplete (B=0x', 0

_testdata
	defb 'BIT n,(IX+d)', 0
        defb 0x1f
	defw bitnixtest

	defb 'DAA', 0
        defb 0x1f
	defw daatest

        defb 'OUTI', 0
        defb 0x1f
        defw outitest

	defb 'LDIR', 0
        defb 0x1f
	defw ldirtest

	defb 'Contended IN', 0
        defb 0x1f
	defw contendedintest

	defb 'Floating bus', 0
        defb 0x13
	defw floatingbustest

	defb 'Contended memory', 0
        defb 0x1f
	defw contendedmemorytest

	defb 'High port contention 1', 0
        defb 0x1f
	defw highporttest1

	defb 'High port contention 2', 0
        defb 0x1f
	defw highporttest2

	defb '0xbffd read', 0
        defb 0x1f
	defw hexbffdreadtest

	defb '0x3ffd read', 0
        defb 0x12
	defw hex3ffdreadtest

	defb '0x7ffd read', 0
        defb 0x12
	defw hex7ffdreadtest

	defb 0

_framestring defb 'Frame length ', 0
_unknownstring defb 'unknown', 0x0d, 0
_conststring defb '0x8000 + 0x', 0
_machinetype defb 'Machine type: ', 0
_offsetstring defb 'Contention offset: ', 0
_hexstring defb '0x', 0

_notfound_string defb 'no contention found', 0
_nosync_string defb 'no sync', 0
_short_string defb 'negative contention?', 0
	
ENDP

INCLUDE tests.asm
	
INCLUDE sync.asm
INCLUDE delay.asm
INCLUDE atiming.asm
INCLUDE print.asm
INCLUDE framelength.asm
INCLUDE guessmachine.asm
INCLUDE first.asm

END	main
