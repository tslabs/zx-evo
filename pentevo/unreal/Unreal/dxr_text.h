#pragma once

extern const unsigned char *fontdata;

void create_font_tables();
void __fastcall render_text(u8 *dst, u32 pitch);
