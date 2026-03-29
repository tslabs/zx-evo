    org 0

    nop
    call l3
    
    wait EVT_X0
    
    # set video mode
    wreg VCONFIG, VID_320X240 | VID_256C
    wreg VPAGE, 0x10

l1:
    #set counter for 128 cycles
    seta 128

    # set source memory addr for DMA
    wreg DMASADDRL, 0
    wreg DMASADDRH, 0
    wreg DMASADDRX, 0x20

l2:
    # wait for raster start
    # waity 0

    # set DMA for destination address (screen)
    wreg DMADADDRL, 0
    wreg DMADADDRH, 0
    wreg DMADADDRX, 0x10

    # wreg DMALEN, 160 - 1
    # wreg DMANUM, 240 - 1

    wreg DMALEN, 1
    wreg DMANUM, 0

    # run DMA and wait for transaction end
    # waitx 0xCB
    wreg DMACTR, DMA_RAM | DMA_DALGN | DMA_ASZ
    wait EVT_DMA
    signal SIG_RDY | SIG_INT
    halt

    # loop 128 times
    djnza l2

    # start over - infinite loop
    jump l1

l3:
    ret
