; Guess which machine we're running on
; Currently, detects 48K (or +), 128K (or +2), +3 (or +2A) and Pentagon.

guessmachine
PROC
	ld bc, 0x9100
	sbc hl, bc
	jr z, _m48
	
	ld bc, 0x94fc - 0x9100
	sbc hl, bc
	jr z, _m128

	ld bc, 0x9800 - 0x94fc
	sbc hl, bc
	jr z, _mpent
	
	xor a
	ld hl, _unknown
	jr _end
	
_m48	xor a
	ld hl, _m48string
	jr _end

	;; The code section below also tests for bug #1821604 (128K / +3 ULA
	;; handling slightly broken).

_m128	ld a, 0xff		; Distinguish between 128K and +3 by
	out (0xfe), a		; behaviour of bit 6 of reading from the
	in a, (0xfe)		; ULA
	ld b, a
	ld a, 0xe7
	out (0xfe), a
	in a, (0xfe)
	xor b
	and 0x40
	jr z, _mplus3
	ld a, 0x01
	ld hl, _m128string
	jr _end

_mplus3	ld a, 0x02
	ld hl, _mplus3string
	jr _end

_mpent	ld a, 0x03
	ld hl, _mpentstring

_end	ld (guessmachine_guess), a
	call printstring
	ret

_m48string defb '48K', 0x0d, 0
_m128string defb '128K', 0x0d, 0
_mplus3string defb '+3', 0x0d, 0
_mpentstring defb 'Pentagon', 0x0d, 0
_unknown defb 'unknown', 0x0d, 0

ENDP

guessmachine_table
PROC
	xor a
	ld a, (guessmachine_guess)
	ld d, 0x00
	ld e, a
	adc hl, de
	adc hl, de
	ld a, (hl)
	ld (bc), a
	inc hl
	inc bc
	ld a, (hl)
	ld (bc), a
	ret
ENDP

guessmachine_guess defb 0x00

