; Guess which machine we're running on
; Currently, only does 48K or 128K

guessmachine
PROC
	ld bc, 0x9100
	sbc hl, bc
	jr z, _m48
	
	ld bc, 0x94fc - 0x9100
	sbc hl, bc
	jr z, _m128
	
	xor a
	ld hl, _unknown
	jr _end
	
_m48	xor a
	ld hl, _m48string
	jr _end

_m128	ld a, 0x01
	ld hl, _m128string

_end	ld (guessmachine_guess), a
	call printstring
	ret

_m48string defb '48K', 0x0d, 0
_m128string defb '128K', 0x0d, 0
_unknown defb 'unknown', 0x0d, 0

ENDP

guessmachine_table
PROC
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

