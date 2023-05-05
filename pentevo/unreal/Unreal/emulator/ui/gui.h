#pragma once
#include "sysdefs.h"
#include "visuals.h"

extern const char *lastpage;

void setcheck(unsigned ID, u8 state = 1);
u8 getcheck(unsigned ID);
void setup_dlg();