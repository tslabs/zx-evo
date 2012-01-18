CLS
DIM d AS STRING * 32
DIM c AS STRING * 1

OPEN "font1.tga" FOR BINARY AS 1
SEEK 1, 19


OPEN "font32.pal" FOR BINARY AS 2

FOR i = 0 TO 15
GET 1, , c: b = INT(ASC(c) / 10.2)
GET 1, , c: g = INT(ASC(c) / 10.2)
GET 1, , c: r = INT(ASC(c) / 10.2)
x = r * 1024 + g * 32 + b
h$ = CHR$(INT(x / 256))
l$ = CHR$(x - INT(x / 256) * 256)
PUT 2, , l$
PUT 2, , h$
NEXT i

CLOSE 2

OPEN "font32.fnt" FOR BINARY AS 2

FOR b2 = 0 TO 5
FOR b1 = 0 TO 9
FOR b3 = 0 TO 31
SEEK 1, 19 + 768 + b1 * 32 + b2 * 320 * 32 + b3 * 320

FOR b0 = 0 TO 15
GET 1, , c: h = ASC(c)
GET 1, , c: l = ASC(c)
c = CHR$(h * 16 + l): PUT 2, , c
NEXT b0

NEXT b3
NEXT b1
NEXT b2

CLOSE 2
CLOSE 1

