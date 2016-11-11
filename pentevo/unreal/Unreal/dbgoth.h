#pragma once

void mon_gsdialog();
void mon_setwatch();
void mon_switchay();

extern unsigned int selbank;
void __cdecl BankNames(int i, char *Name);
int dispatch_banks();
void showwatch();
void showstack();
void show_ay();
void showbanks();
void showports();
void showdos();
void show_time();
