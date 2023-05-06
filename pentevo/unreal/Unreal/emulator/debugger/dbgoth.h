#pragma once

void mon_gsdialog();
void mon_setwatch();
void mon_switchay();

extern unsigned int selbank, showbank;
void __cdecl BankNames(int i, char *name);
int dispatch_banks();
void bup();
void bdown();
void benter();

void showwatch();
void showstack();
void show_ay();
void showbanks();
void showports();
void showdos();
void show_time();
void show_pc_history();
