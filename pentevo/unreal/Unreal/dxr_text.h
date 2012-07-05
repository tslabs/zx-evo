#pragma once

extern const unsigned char *fontdata;

void create_font_tables();
void __fastcall render_text(unsigned char *dst, unsigned pitch);
