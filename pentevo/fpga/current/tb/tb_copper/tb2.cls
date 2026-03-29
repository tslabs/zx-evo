    org 0

    waitx 0
    
    wreg VCONFIG, VID_320X240 | VID_256C
    wreg VPAGE, 0x10

l1:
    setb 288
    waity 32

l2:
    wreg BORDER, 0xFF
    waitx 1
    wreg BORDER, 0xFE
    waitx 2
    wreg BORDER, 0xFD
    waitx 3
    wreg BORDER, 0xFC
    
    wait EVT_X0
    djnzb l2

    jump l1
