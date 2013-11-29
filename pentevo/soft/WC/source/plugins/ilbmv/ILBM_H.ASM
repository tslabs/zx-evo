	DEVICE ZXSPECTRUM48

GSTP      EQU    4;  step in gfx view
tsFMAddr	  EQU	0x15af
MAX_BP    EQU      8;  наибольшее число битпланов.
MAX_ROWS  EQU    512;  наибольшее число колнок изображения.
MAX_LINES EQU    512;  наибольшее число линий изображения.
BUF_SIZE  EQU   1024;  размер каждого буфера.
;FBUF_SIZE EQU   8192

MAX_W_IMAGE EQU  2; наиб. ширина буфера изображения = 2 страниы.
MAX_H_IMAGE EQU 24; наиб. высота буфера изображения = 24 страниы

PAL_INV	  EQU	0x8b00; используется однократно перед использованием буфера битпланов
BP_BUF    EQU   0x9000; местный адрес буфера битпланов.
READY_BUF EQU   0x9400; местный адрес буфера готовго к
                     ; "растасовке" по страницам изображения.
PAL_BUF   EQU   0x9800
FILE_BUF  EQU   0xA000; местный адрес файлового буфера.
PAL_LEN   EQU   0x0200
FILEBUFSZ EQU   0x2000

TABL	  EQU	0xc000

CPU2WIN   EQU   #6002; номер страницы, включенной в CPU2-окне.
CPU3WIN   EQU   #6003; номер страницы, включенной в CPU3-окне.

SCREENPAGE EQU  0 ; по правилам WC
SCREENMODE EQU  %11000010; 360x288c256






