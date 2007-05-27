; Routine to sync with interrupts

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

; Setup routine; must be called before interruptsync

interruptsync_init
PROC
	di

	ld hl, 0xfe00
	ld (hl), 0xfd
	ld de, 0xfe01
	ld bc, 0x0100
	ldir

	ld hl, 0xfdfd
	ld (hl), 0xc3		; JP

	ld a,0xfe
	ld i,a

	im 2

	ret
ENDP

; Synchronisation routine: return from this will be exactly 88 tstates
; after interrupt

interruptsync
PROC
	ld hl, 0xfdfe
	ld (hl), _isr % 0x100
	inc hl
	ld (hl), _isr / 0x100
	ld b, 0x00

	ei
	halt

	; jp _isr		  19 - 22
_isr
	ld hl, 0xfdfe		; 29 - 32
	ld (hl), _isr3 % 0x100	; 39 - 42
	inc hl			; 49 - 52
	ld (hl), _isr3 / 0x100	; 55 - 58

_isr1	ld hl, 0x0a7c 		; 65 - 68

_isr2	dec hl			; 75 - 78 + n * 26
	ld a, l			; 81 - 84 + n * 26
	or h			; 85 - 88 + n * 26
	jr nz, _isr2		; 89 - 92 + n * 26

	ei			; 69854 - 69857
	ld a,i			; 69858 - 69861
	xor a			; 69867 - 69870
	nop			; 69871 - 69874
	nop			; 69875 - 69878
	nop			; 69879 - 69882
	nop			; 69883 - 69886
	inc a			; 69887 or interrupt occurred
	dec a			; Should not be executed
	halt

	; jp _isr3		  19 - 22
_isr3	pop hl			; 29 - 32
	cp b			; 39 - 42
	nop			; 43 - 46
	nop			; 47 - 50
	nop			; 51 - 54
	jp z, _isr1		; 55 - 58
	pop hl			; 68
	ret			; 78

ENDP
