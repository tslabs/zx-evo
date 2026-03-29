    org 0

    wreg VCONFIG, VID_360X288 | VID_256C
    wreg VPAGE, 0x10

    # Copy pixels
    wreg DMASADDRL, 0
    wreg DMASADDRH, 0
    wreg DMASADDRX, 0x20
    wreg DMADADDRL, 0
    wreg DMADADDRH, 0
    wreg DMADADDRX, 0x10
    wreg DMALEN, 180 - 1
    wreg DMANUM, 0

    setb 288
l1:
    wreg DMACTR, DMA_RAM | DMA_DALGN | DMA_ASZ
    wait EVT_DMA
    djnzb l1

    # Main loop
l2:
    wait EVT_XY0
    waity 30
    setb 288
    wreg DMASADDRL, 0
    wreg DMASADDRH, 0
    wreg DMASADDRX, 0x30
    wreg DMADADDRL, 0
    wreg DMADADDRH, 0
    wreg DMALEN, 128 - 1
    wreg DMANUM, 0
    
l3:
    wait EVT_X0
    wreg DMACTR, DMA_RAM_CRAM
    wait EVT_DMA
    djnzb l3

    jump l2
