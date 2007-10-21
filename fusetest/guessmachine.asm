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
	
	ld hl, _unknown
	jr _end
	
_m48	ld hl, 335
	ld (guessmachine_1), hl
	ld hl, 665
	ld (guessmachine_2), hl
	ld hl, _m48string
	jr _end

_m128	ld hl, 361
	ld (guessmachine_1), hl
	ld hl, 639
	ld (guessmachine_2), hl
	ld hl, _m128string

_end	call printstring
	ret

_m48string defb '48K', 0x0d, 0
_m128string defb '128K', 0x0d, 0
_unknown defb 'unknown', 0x0d, 0
	
ENDP

guessmachine_precontend
PROC
	ld hl, (guessmachine_1)
	call delay
	ret
ENDP

guessmachine_postcontend
PROC
	ld hl, (guessmachine_2)
	call delay
	ret
ENDP

guessmachine_1 defw 335		; Contention starts at 14000 + this
guessmachine_2 defw 665		; If you increase guessmachine_1, decrease
				; this by the same amount
