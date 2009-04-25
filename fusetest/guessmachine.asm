; Guess which machine we're running on
; Currently, detects 48K (or +), 128K (or +2), +3 (or +2A), Pentagon,
; TS2068 and NTSC 48K.

guessmachine
PROC
        ld bc, 0x6540
        sbc hl, bc
        jr z, _mts2068

	ld bc, 0x6700 - 0x6540
	sbc hl, bc
	jr z, _m48ntsc

	ld bc, 0x9100 - 0x6700
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

_m48ntsc
	ld a, 0x05
	ld hl, _m48ntscstring
	jr _end

	;; Distinguish between 128K and +3 by looking at the partial
	;; decoding of "port 0x7ffd"

_m128	ld a, (0x5b5c)
	ld bc, 0x7ffd
	ld d, 0x07
	ld e, a
	
_loop	and 0xf8
	or d
	out (c), a
	ld a, d
	ld (0xe000), a
	dec d
	cp 0x00
	ld a, e
	jr nz, _loop

	ld bc, 0x3ffd
	and 0xf8
	or 1
	out (c), a
	ld a, (0xe000)
	cp 0x00
	jr z, _mplus3
	
	ld a, 0x01
	ld hl, _m128string
	jr _end

_mplus3	ld a, 0x02
	ld hl, _mplus3string
	jr _end

_mpent	ld a, 0x03
	ld hl, _mpentstring

_mts2068
        ld a, 0x04
        ld hl, _mts2068string

_end	ld (guessmachine_guess), a
	call printstring
      
	ld hl, _contention_table
	ld bc, guessmachine_contended
	call guessmachine_table

	ret

_m48string defb '48K', 0x0d, 0
_m48ntscstring defb '48K (NTSC)', 0x0d, 0
_m128string defb '128K', 0x0d, 0
_mplus3string defb '+3', 0x0d, 0
_mpentstring defb 'Pentagon', 0x0d, 0
_mts2068string defb 'TS2068', 0x0d, 0

_unknown defb 'unknown', 0x0d, 0

; First contended cycle for each machine

_contention_table
    defw 0x37ff		; 48K
    defw 0x3819		; 128K/+2
    defw 0x3819		; +2A/+3
    defw 0x4643		; Pentagon (see below)
    defw 0x23d0		; TS2068
    defw 0x22ff		; 48K NTSC

; The Pentagon has no contended memory. The number here
; is one pixel before the screen.

ENDP

guessmachine_table
PROC
	ld a, (guessmachine_guess)
	ld d, 0x00
	ld e, a
	add hl, de
	add hl, de
	ld a, (hl)
	ld (bc), a
	inc hl
	inc bc
	ld a, (hl)
	ld (bc), a
	ret
ENDP

guessmachine_guess defb 0x00
guessmachine_contended defw 0x0000
