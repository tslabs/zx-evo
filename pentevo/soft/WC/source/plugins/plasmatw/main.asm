;---------------------------------------
; Plasma screensaver
; by VBI'2014
;---------------------------------------

        DEVICE ZXSPECTRUM128

       	include "wcKernel.h.asm"
startCode
        org #0000
        include "pluginHead.asm"
        align 512
        DISP #8000
mainStart
		include "twister.asm"
mainEnd
        ENT
        align 512
block1  incbin "rorat3d wmf.tga.pix4.000"
        align 512
eblock1
block2  incbin "rorat3d wmf.tga.pix4.001"
        align 512
eblock2
block3  incbin "rorat3d wmf.tga.pix4.002"
        align 512
eblock3
endCode

	SAVEBIN "PLASMATW.WMF", startCode, endCode-startCode
