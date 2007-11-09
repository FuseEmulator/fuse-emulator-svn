; Simple printing utilities

; Copyright 2007 Philip Kendall <philip-fuse@shadowmagic.org.uk>
	
; This program is licensed under the GNU General Public License. See the
; file `COPYING' for details

; Print a null-terminated string at (HL)
	
printstring
PROC
	ld a, (hl)
	cp 0x00
	ret z
	rst 0x10
	inc hl
	jr printstring
ENDP

; Print the value of A in hex

printa
PROC
	ld b, a
	and 0xf0
	rrca
	rrca
	rrca
	rrca
	call _digit
	ld a, b
	and 0x0f
_digit	cp 0x0a
	jr nc, _letter
	add a, 0x30
	jr _x
_letter	add a, 'a'-10
_x	rst 0x10
	ld a, b
	ret
ENDP
