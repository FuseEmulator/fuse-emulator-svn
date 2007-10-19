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

; Synchronisation routine: return from this will be exactly 92 tstates
; after interrupt with A reset if successful. If it couldn't sync with
; interrupts, it will return with A set to 0xde.

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

_isr1	ld hl, 0xffff 		; 65 - 68
	call delay		; 75 - 78
	ld hl, 0x10a3		; 65610 - 65613
	call delay		; 65620 - 65623

	ei			; 69879 - 69882
	xor a			; 69883 - 69886
	inc a			; 69887 or interrupt occurred
	di			; Should not be executed
	ld hl, _nosync
	call printstring
	pop hl
	ld a, 0xde		; Error code
	ret

	; jp _isr3		  19 - 22
_isr3	pop hl			; 29 - 32
	cp b			; 39 - 42
	nop			; 43 - 46
	nop			; 47 - 50
	nop			; 51 - 54
	jp z, _isr1		; 55 - 58
	pop hl			; 68
	xor a			; 78
	ret			; 82

_nosync defb '... no sync', 0

ENDP
