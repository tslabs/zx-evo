	device pentagon1024

	org #4000
begin
	jp ayfx.INIT		;#4000
	jp ayfx.PLAY		;#4003
	jp ayfx.FRAME		;#4006
	jp pt3player.INIT	;#4009
	jp pt3player.PLAY	;#400c

	include "pt3play.asm"
	include "ayfxplay.asm"

end

	display "Top: ",/h,$
	savebin "sound.bin",begin,end-begin