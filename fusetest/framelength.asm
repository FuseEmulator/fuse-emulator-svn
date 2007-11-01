; Routine to get frame length in tstates

; Return framelength - 0x8000 in BC.
; Assumes 0x8000 <= framelength < 0x18000 and that framelength % 4 = 0

framelength
PROC
	call _setisr1
	dec hl

	ei
	halt

	ld (hl), _isr2 % 0x100	; 39
	inc hl			; 49
	ld (hl), _isr2 / 0x100	; 55

	ld hl, 32697		; 65
	call delay		; 75
	xor a			; 32772
	ei			; 32776
	
	;; Phase 1: work out ( frame length - 0x8000 ) / 0x100
_phase1
	push af			; 32768 + n * 256 + 12
	ld hl, 209		; 32768 + n * 256 + 23
	call delay		; 32768 + n * 256 + 33
	pop af			; 32768 + n * 256 + 242
	inc a			; 32768 + n * 256 + 252
	jr _phase1		; 32768 + n * 256 + 256

	;; Phase 2: work out ( frame length % 0x100 ) / 4
_phase2
	ld b, a
	call _setisr1
	dec hl
	
	ei
	halt

	ld (hl), _isr3 % 0x100	; 39
	inc hl			; 49
	ld (hl), _isr3 / 0x100	; 55

	ld hl, 32674		; 65
	call delay		; 75

	ld l, 0x00		; 32749
	ld h, b			; 32756
	call delay		; 32760

	ei			; 32760 + n * 256
	xor a			; 32764 + n * 256

	;; 63 x 'inc a' follows here

	inc a			; 32768 + n * 256
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	inc a
	inc a
	inc a
	inc a
	inc a
	inc a
	inc a

	di			; Should never be executed
	ld bc, 0xffff
	ret

_phase3
	add a, a
	add a, a
	ld c, a
	ret

_setisr1			
	ld hl, sync_isr + 1
	ld (hl), _isr1 % 0x100
	inc hl
	ld (hl), _isr1 / 0x100
	ret

	; jp _isr1		  19
_isr1
	ret			; 29

	; jp _isr2		  19
_isr2
	pop bc			; 29
	jp _phase2		; 39

	; jp _isr3		  19
_isr3
	pop de			; 29
	jp _phase3		; 39
	
ENDP
