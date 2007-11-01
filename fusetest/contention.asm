; Contention tester

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

ORG 0xc000
	
main
PROC
	call interruptsync_init

	ld hl, 0x7fff
	ld (hl), 0x00		; nop
	inc hl
	ld (hl), 0xc9		; ret

	ld hl, _testdata

_next	ld a, (hl)
	cp 0x00
	jr z, _done
	
	ld de, delay1
	ld bc, 0x0004
	ldir

	call bigtest

	jr _next 

_done	halt

_testdata
	defw 0x375b, 0xd6d1	; 14332
	defw 0x375c, 0xd6d0	; 14333
	defw 0x375d, 0xd6cf	; 14334
	defw 0x375e, 0xd6c8	; 14335
	defw 0x375f, 0xd6c8	; 14336
	defw 0x3760, 0xd6c8	; 14337
	defw 0x3761, 0xd6c8	; 14338
	defw 0x3762, 0xd6c8	; 14339
	defw 0x3763, 0xd6c8	; 14340
	defw 0x3764, 0xd6c8	; 14341
	defw 0x3765, 0xd6c7	; 14342
	defw 0x3766, 0xd6c0	; 14343
	defw 0x3767, 0xd6c0	; 14344
	defw 0x3768, 0xd6c0	; 14345
	defw 0x3769, 0xd6c0	; 14346
	defw 0

ENDP

bigtest
PROC
	push de
	push hl
	
	call test

	ld hl, _commastring
	call printstring

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	ld hl, _commastring
	call printstring

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	ld hl, _commastring
	call printstring

	ld hl, (delay2)
	inc hl
	ld (delay2), hl
	
	call test	

	ld hl, _newline
	call printstring

	pop hl
	pop de

	ret

_commastring
	defb ',', 0

_newline
	defb 0x0d, 0

ENDP

test
PROC
	call interruptsync

	ld hl, sync_isr + 1	; 92
	ld (hl), _isr % 0x100	; 102
	inc hl			; 112
	ld (hl), _isr / 0x100	; 118

	ld hl, (delay1)		; 128
	call delay		; 144

	call 0x7fff

	ld hl, (delay2)
	call delay

	jp atiming

_isr	pop hl
	call printa
	ret
ENDP

delay1	defw 0x0000
delay2	defw 0x0000

INCLUDE atiming.asm
INCLUDE delay.asm
INCLUDE print.asm
INCLUDE sync.asm
INCLUDE guessmachine.asm

END	main
