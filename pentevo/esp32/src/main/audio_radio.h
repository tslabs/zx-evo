// Internet radio of the audio stream player: the HTTP reader task.
#pragma once

#include "main.h"

void radio_init();
// A new session (a number not 0) reads url; it ends when another starts or on stop
void radio_start(u32 session, const char *url);
void radio_stop();
