#pragma once

extern const char *lastpage;

void setcheck(unsigned ID, unsigned char state = 1);
unsigned char getcheck(unsigned ID);
void setup_dlg();