#pragma once

extern unsigned asmii;
extern const unsigned cs[3][2];

int disasm_line(unsigned addr, char *line);

void mon_step();
void mon_stepover();

void cfindpc();
void chere();
void cfindtext();
void cfindcode();
void cgoto();
void cbpx();
void center();
void csetpc();
void cup();
void cdown();
void cleft();
void cright();
void chere();
void cpgup();
void cpgdn();
void pop_pos();
void cjump();
void cdjump();

void csave1();
void csave2();
void csave3();
void csave4();
void csave5();
void csave6();
void csave7();
void csave8();
void crest1();
void crest2();
void crest3();
void crest4();
void crest5();
void crest6();
void crest7();
void crest8();

void cfliplabels();
void c_lbl_import();

namespace z80dbg
{
void __cdecl SetLastT();
}
void showtrace();
char dispatch_trace();
