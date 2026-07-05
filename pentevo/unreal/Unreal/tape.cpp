#include "std.h"
#include "emul.h"
#include "vars.h"
#include "tape.h"
#include "memory.h"
#include "util.h"

unsigned tape_pulse[0x100];
unsigned max_pulses = 0;
unsigned tape_err = 0;

u8 *tape_image = 0;
unsigned tape_imagesize = 0;

#ifdef LOG_TAPE_IN
	extern FILE *f_log_tape_in;
#endif

TAPEINFO *tapeinfo;
unsigned tape_infosize;

unsigned appendable;

// tape image contains indexes in tape_pulse[]

unsigned find_pulse(unsigned t)
{
   if (max_pulses < 0x100)
   {
      for (unsigned i = 0; i < max_pulses; i++)
         if (tape_pulse[i] == t)
             return i;
      tape_pulse[max_pulses] = t;
      return max_pulses++;
   }
   if (!tape_err)
   {
       errmsg("pulse table full");
       tape_err = 1;
   }
   unsigned nearest = 0; int delta = 0x7FFFFFFF;
   for (unsigned i = 0; i < 0x100; i++)
   {
      if (delta > abs((int)t - (int)tape_pulse[i]))
      {
         nearest = i;
         delta = abs((int)t - (int)tape_pulse[i]);
      }
   }
   return nearest;
}

void find_tape_index()
{
   for (unsigned i = 0; i < tape_infosize; i++)
      if (comp.tape.play_pointer >= tape_image + tapeinfo[i].pos)
         comp.tape.index = i;
   temp.led.tape_started = -600*3500000;
}

void find_tape_sizes()
{
   for (unsigned i = 0; i < tape_infosize; i++) {
      unsigned end = (i == tape_infosize-1) ? tape_imagesize : tapeinfo[i+1].pos;
      unsigned sz = 0;
      for (unsigned j = tapeinfo[i].pos; j < end; j++)
         sz += tape_pulse[tape_image[j]];
      tapeinfo[i].t_size = sz;
   }
}

void stop_tape()
{
   find_tape_index();
   const char *msg = "tape stopped";
   if (comp.tape.play_pointer == comp.tape.end_of_tape)
      comp.tape.index = 0, msg = "end of tape";
   comp.tape.play_pointer = 0;
   strcpy(statusline, msg); statcnt = 40;
   comp.tape.edge_change = 0x7FFFFFFFFFFFFFFF;
   comp.tape.tape_bit = -1;
}

void reset_tape()
{
   comp.tape.index = 0;
   comp.tape.play_pointer = 0;
   comp.tape.edge_change = 0x7FFFFFFFFFFFFFFF;
   comp.tape.tape_bit = -1;
}

void start_tape()
{
   if (!tape_image) return;
   strcpy(statusline, "tape started"); statcnt = 40;
   comp.tape.play_pointer = tape_image + tapeinfo[comp.tape.index].pos;
   comp.tape.end_of_tape = tape_image + tape_imagesize;
   comp.tape.edge_change = comp.t_states + cpu.t;
   temp.led.tape_started = -600*3500000;
   comp.tape.tape_bit = -1;
}

void closetape()
{
   if (tape_image) free(tape_image), tape_image = 0;
   if (tapeinfo) free(tapeinfo), tapeinfo = 0;
   comp.tape.play_pointer = 0; // stop tape
   comp.tape.index = 0; // rewind tape
   tape_err = max_pulses = tape_imagesize = tape_infosize = 0;
   comp.tape.edge_change = 0x7FFFFFFFFFFFFFFF;
   comp.tape.tape_bit = -1;
}

void reserve(unsigned datasize)
{
   const int blocksize = 16384;
   unsigned newsize = align_by(datasize+tape_imagesize+1, blocksize);
   if (!tape_image) tape_image = (u8*)malloc(newsize);
   if (align_by(tape_imagesize, blocksize) < newsize) tape_image = (u8*)realloc(tape_image, newsize);
}

void makeblock(u8 *data, unsigned size, unsigned pilot_t,
      unsigned s1_t, unsigned s2_t, unsigned zero_t, unsigned one_t,
      unsigned pilot_len, unsigned pause, u8 last = 8)
{
   reserve(size*16 + pilot_len + 3);
   if (pilot_len != -1) {
      unsigned t = find_pulse(pilot_t);
      for (unsigned i = 0; i < pilot_len; i++)
         tape_image[tape_imagesize++] = t;
      tape_image[tape_imagesize++] = find_pulse(s1_t);
      tape_image[tape_imagesize++] = find_pulse(s2_t);
   }
   unsigned t0 = find_pulse(zero_t), t1 = find_pulse(one_t);
   for (; size>1; size--, data++)
      for (u8 j = 0x80; j; j >>= 1)
         tape_image[tape_imagesize++] = (*data & j) ? t1 : t0,
         tape_image[tape_imagesize++] = (*data & j) ? t1 : t0;
   for (u8 j = 0x80; j != (u8)(0x80 >> last); j >>= 1) // last byte
      tape_image[tape_imagesize++] = (*data & j) ? t1 : t0,
      tape_image[tape_imagesize++] = (*data & j) ? t1 : t0;
   if (pause) tape_image[tape_imagesize++] = find_pulse(pause*3500);
}

void desc(u8 *data, unsigned size, char *dst)
{
   u8 crc = 0; char prg[10];
   unsigned i; //Alone Coder 0.36.7
   for (/*unsigned*/ i = 0; i < size; i++) crc ^= data[i];
   if (!*data && size == 19 && (data[1] == 0 || data[1] == 3)) {
      for (i = 0; i < 10; i++) prg[i] = (data[i+2] < ' ' || data[i+2] >= 0x80) ? '?' : data[i+2];
      for (i = 9; i && prg[i] == ' '; prg[i--] = 0);
      sprintf(dst, "%s: \"%s\" %d,%d", data[1] ? "Bytes":"Program", prg,
         data[14] | (data[15] << 8), data[12] | (data[13] << 8));
   } else if (*data == 0xFF) sprintf(dst, "data block, %d bytes", size-2);
   else sprintf(dst, "#%02X block, %d bytes", *data, size-2);
   sprintf(dst + strlen(dst), ", crc %s", crc ? "bad":"ok");
}

void alloc_infocell()
{
   tapeinfo = (TAPEINFO*)realloc(tapeinfo, (tape_infosize+1)*sizeof(TAPEINFO));
   tapeinfo[tape_infosize].pos = tape_imagesize;
   appendable = 0;
}

void named_cell(const void *nm, unsigned sz = 0)
{
   alloc_infocell();
   if (sz)
   {
      if (sz >= sizeof tapeinfo[tape_infosize].desc)
         sz = sizeof tapeinfo[tape_infosize].desc - 1;
      memcpy(tapeinfo[tape_infosize].desc, nm, sz);
      tapeinfo[tape_infosize].desc[sz] = 0;
   }
   else
   {
      strncpy(tapeinfo[tape_infosize].desc, (const char*)nm, sizeof tapeinfo[tape_infosize].desc - 1);
      tapeinfo[tape_infosize].desc[sizeof tapeinfo[tape_infosize].desc - 1] = 0;
   }
   tape_infosize++;
}

int readTAP()
{
   u8 *ptr = snbuf;
   u8 *file_end = snbuf + snapsize;
   closetape();
   while (ptr < file_end) {
      if ((unsigned)(file_end - ptr) < 2)
         return 0;
      unsigned size = ptr[0] | (ptr[1] << 8);
      ptr += 2;
      if (!size) break;
      if (size > (unsigned)(file_end - ptr))
         return 0;
      alloc_infocell();
      desc(ptr, size, tapeinfo[tape_infosize].desc);
      tape_infosize++;
      makeblock(ptr, size, 2168, 667, 735, 855, 1710, (*ptr < 4) ? 8064 : 3220, 1000);
      ptr += size;
   }
   find_tape_sizes();
   return (ptr == file_end);
}

int readCSW()
{
   if (snapsize < 0x20)
       return 0;

   u8 *file_end = snbuf + snapsize;
   closetape();
   if (snbuf[0x1B] != 1)
       return 0; // unknown compression type
   unsigned sample_rate = snbuf[0x19] | (snbuf[0x1A] << 8);
   if (!sample_rate)
       return 0;
   unsigned rate = Z80FQ / sample_rate; // usually 3.5mhz / 44khz
   if (!rate)
       return 0;
   named_cell("CSW tape image");
   reserve(snapsize - 0x18);
   if (!(snbuf[0x1C] & 1))
       tape_image[tape_imagesize++] = find_pulse(1);
   for (u8 *ptr = snbuf + 0x20; ptr < file_end; )
   {
      unsigned len = *ptr++ * rate;
      if (!len)
      {
          if ((unsigned)(file_end - ptr) < 4)
              return 0;
          len = (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24)) * rate;
          ptr += 4;
      }
      tape_image[tape_imagesize++] = find_pulse(len);
   }
   tape_image[tape_imagesize++] = find_pulse(Z80FQ/10);
   find_tape_sizes();
   return 1;
}

void create_appendable_block()
{
   if (!tape_infosize || appendable) return;
   named_cell("set of pulses"); appendable = 1;
}

void parse_hardware(u8 *ptr)
{
   unsigned n = *ptr++;
   if (!n) return;
   named_cell("- HARDWARE TYPE ");
   static char ids[] =
     "computer\0"
        "ZX Spectrum 16k\0"
        "ZX Spectrum 48k, Plus\0"
        "ZX Spectrum 48k ISSUE 1\0"
        "ZX Spectrum 128k (Sinclair)\0"
        "ZX Spectrum 128k +2 (Grey case)\0"
        "ZX Spectrum 128k +2A, +3\0"
        "Timex Sinclair TC-2048\0"
        "Timex Sinclair TS-2068\0"
        "Pentagon 128\0"
        "Sam Coupe\0"
        "Didaktik M\0"
        "Didaktik Gama\0"
        "ZX-81 or TS-1000 with  1k RAM\0"
        "ZX-81 or TS-1000 with 16k RAM or more\0"
        "ZX Spectrum 128k, Spanish version\0"
        "ZX Spectrum, Arabic version\0"
        "TK 90-X\0"
        "TK 95\0"
        "Byte\0"
        "Elwro\0"
        "ZS Scorpion\0"
        "Amstrad CPC 464\0"
        "Amstrad CPC 664\0"
        "Amstrad CPC 6128\0"
        "Amstrad CPC 464+\0"
        "Amstrad CPC 6128+\0"
        "Jupiter ACE\0"
        "Enterprise\0"
        "Commodore 64\0"
        "Commodore 128\0"
        "\0"
     "ext. storage\0"
        "Microdrive\0"
        "Opus Discovery\0"
        "Disciple\0"
        "Plus-D\0"
        "Rotronics Wafadrive\0"
        "TR-DOS (BetaDisk)\0"
        "Byte Drive\0"
        "Watsford\0"
        "FIZ\0"
        "Radofin\0"
        "Didaktik disk drives\0"
        "BS-DOS (MB-02)\0"
        "ZX Spectrum +3 disk drive\0"
        "JLO (Oliger) disk interface\0"
        "FDD3000\0"
        "Zebra disk drive\0"
        "Ramex Millenia\0"
        "Larken\0"
        "\0"
     "ROM/RAM type add-on\0"
        "Sam Ram\0"
        "Multiface\0"
        "Multiface 128k\0"
        "Multiface +3\0"
        "MultiPrint\0"
        "MB-02 ROM/RAM expansion\0"
        "\0"
     "sound device\0"
        "Classic AY hardware\0"
        "Fuller Box AY sound hardware\0"
        "Currah microSpeech\0"
        "SpecDrum\0"
        "AY ACB stereo; Melodik\0"
        "AY ABC stereo\0"
        "\0"
     "joystick\0"
        "Kempston\0"
        "Cursor, Protek, AGF\0"
        "Sinclair 2\0"
        "Sinclair 1\0"
        "Fuller\0"
        "\0"
     "mice\0"
        "AMX mouse\0"
        "Kempston mouse\0"
        "\0"
     "other controller\0"
        "Trickstick\0"
        "ZX Light Gun\0"
        "Zebra Graphics Tablet\0"
        "\0"
     "serial port\0"
        "ZX Interface 1\0"
        "ZX Spectrum 128k\0"
        "\0"
     "parallel port\0"
        "Kempston S\0"
        "Kempston E\0"
        "ZX Spectrum 128k +2A, +3\0"
        "Tasman\0"
        "DK'Tronics\0"
        "Hilderbay\0"
        "INES Printerface\0"
        "ZX LPrint Interface 3\0"
        "MultiPrint\0"
        "Opus Discovery\0"
        "Standard 8255 chip with ports 31,63,95\0"
        "\0"
     "printer\0"
        "ZX Printer, Alphacom 32 & compatibles\0"
        "Generic Printer\0"
        "EPSON Compatible\0"
        "\0"
     "modem\0"
        "VTX 5000\0"
        "T/S 2050 or Westridge 2050\0"
        "\0"
     "digitaiser\0"
        "RD Digital Tracer\0"
        "DK'Tronics Light Pen\0"
        "British MicroGraph Pad\0"
        "\0"
     "network adapter\0"
        "ZX Interface 1\0"
        "\0"
     "keyboard / keypad\0"
        "Keypad for ZX Spectrum 128k\0"
        "\0"
     "AD/DA converter\0"
        "Harley Systems ADC 8.2\0"
        "Blackboard Electronics\0"
        "\0"
     "EPROM Programmer\0"
        "Orme Electronics\0"
        "\0"
     "\0";
   for (unsigned i = 0; i < n; i++) {
      u8 type_n = *ptr++;
      u8 id_n = *ptr++;
      u8 value_n = *ptr++;
      const char *type = ids, *id, *value;
      unsigned j; //Alone Coder 0.36.7
      for (/*unsigned*/ j = 0; j < type_n; j++) {
         if (!*type) break;
         while (*(short*)type) type++;
         type += 2;
      }
      if (!*type) type = id = "??"; else {
         id = type + strlen(type) + 1;
         for (j = 0; j < id_n; j++) {
            if (!*id) { id = "??"; break; }
            id += strlen(id)+1;
         }
      }
      switch (value_n) {
         case 0: value = "compatible with"; break;
         case 1: value = "uses"; break;
         case 2: value = "compatible, but doesn't use"; break;
         case 3: value = "incompatible with"; break;
         default: value = "??";
      }
      char bf[512]; sprintf(bf, "%s %s: %s", value, type, id);
      named_cell(bf);
   }
   named_cell("-");
}

int readTZX()
{
   u8 *ptr = snbuf;
   u8 *file_end = snbuf + snapsize;
   closetape();

   auto has_bytes = [file_end](const u8 *pos, unsigned size)
   {
      return pos <= file_end && size <= (unsigned)(file_end - pos);
   };
   auto read_u16 = [](const u8 *pos)
   {
      return (unsigned)pos[0] | ((unsigned)pos[1] << 8);
   };
   auto read_u24 = [](const u8 *pos)
   {
      return (unsigned)pos[0] | ((unsigned)pos[1] << 8) | ((unsigned)pos[2] << 16);
   };
   auto read_u32 = [](const u8 *pos)
   {
      return (unsigned)pos[0] | ((unsigned)pos[1] << 8) | ((unsigned)pos[2] << 16) | ((unsigned)pos[3] << 24);
   };

   unsigned size, pause, i, j, n, t, t0;
   u8 pl, last, *end;
   char *p;
   unsigned loop_n = 0, loop_p;
   char nm[512];
   while (ptr < file_end)
   {
      switch (*ptr++) {
         case 0x10: // normal block
         {
            if (!has_bytes(ptr, 4)) return 0;
            size = read_u16(ptr+2);
            pause = read_u16(ptr);
            if (!size || !has_bytes(ptr + 4, size)) return 0;
            alloc_infocell();
            desc(ptr + 4, size, tapeinfo[tape_infosize].desc);
            tape_infosize++;
            makeblock(ptr + 4, size, 2168, 667, 735, 855, 1710,
                (ptr[4] < 4) ? 8064 : 3220, pause);
            ptr += size + 4;
            break;
         }
         case 0x11: // turbo block
         {
            if (!has_bytes(ptr, 0x12)) return 0;
            size = read_u24(ptr+0x0F);
            last = ptr[12];
            if (!size || last < 1 || last > 8 || !has_bytes(ptr + 0x12, size)) return 0;
            alloc_infocell();
            desc(ptr + 0x12, size, tapeinfo[tape_infosize].desc);
            tape_infosize++;
            makeblock(ptr + 0x12, size,
               read_u16(ptr), read_u16(ptr+2),
               read_u16(ptr+4), read_u16(ptr+6),
               read_u16(ptr+8), read_u16(ptr+10),
               read_u16(ptr+13), last);
            ptr += size + 0x12;
            break;
         }
         case 0x12: // pure tone
         {
            if (!has_bytes(ptr, 4)) return 0;
            create_appendable_block();
            pl = find_pulse(read_u16(ptr));
            n = read_u16(ptr+2);
            reserve(n);
            for (i = 0; i < n; i++) tape_image[tape_imagesize++] = pl;
            ptr += 4;
            break;
         }
         case 0x13: // sequence of pulses of different lengths
         {
            if (!has_bytes(ptr, 1)) return 0;
            n = *ptr++;
            if (!has_bytes(ptr, 2 * n)) return 0;
            create_appendable_block();
            reserve(n);
            for (i = 0; i < n; i++, ptr += 2)
               tape_image[tape_imagesize++] = find_pulse(read_u16(ptr));
            break;
         }
         case 0x14: // pure data block
         {
            if (!has_bytes(ptr, 0x0A)) return 0;
            size = read_u24(ptr+7);
            last = ptr[4];
            if (!size || last < 1 || last > 8 || !has_bytes(ptr + 0x0A, size)) return 0;
            create_appendable_block();
            makeblock(ptr + 0x0A, size, 0, 0, 0, read_u16(ptr),
                      read_u16(ptr+2), -1, read_u16(ptr+5), last);
            ptr += size + 0x0A;
            break;
         }
         case 0x15: // direct recording
         {
            if (!has_bytes(ptr, 8)) return 0;
            size = read_u24(ptr+5);
            t0 = read_u16(ptr);
            pause = read_u16(ptr+2);
            last = ptr[4];
            if (!size || last < 1 || last > 8 || !has_bytes(ptr + 8, size)) return 0;
            named_cell("direct recording");
            ptr += 8;
            pl = 0; n = 0;
            for (i = 0; i < size; i++) // count number of pulses
               for (j = 0x80; j; j >>= 1)
                  if ((ptr[i] ^ pl) & j) n++, pl ^= -1;
            t = 0; pl = 0;
            reserve(n+2);
            for (i = 1; i < size; i++, ptr++) // find pulses
               for (j = 0x80; j; j >>= 1) {
                  t += t0;
                  if ((*ptr ^ pl) & j) {
                     tape_image[tape_imagesize++] = find_pulse(t);
                     pl ^= -1; t = 0;
                  }
               }
            // find pulses - last byte
            for (j = 0x80; j != (u8)(0x80 >> last); j >>= 1) {
               t += t0;
               if ((*ptr ^ pl) & j) {
                  tape_image[tape_imagesize++] = find_pulse(t);
                  pl ^= -1; t = 0;
               }
            }
            ptr++;
            tape_image[tape_imagesize++] = find_pulse(t); // last pulse ???
            if (pause) tape_image[tape_imagesize++] = find_pulse(pause*3500);
            break;
         }
         case 0x20: // pause (silence) or 'stop the tape' command
         {
            if (!has_bytes(ptr, 2)) return 0;
            pause = read_u16(ptr);
            sprintf(nm, pause? "pause %d ms" : "stop the tape", pause);
            named_cell(nm);
            reserve(2); ptr += 2;
            if (!pause) { // at least 1ms pulse as specified in TZX 1.13
               tape_image[tape_imagesize++] = find_pulse(3500);
               pause = -1;
            } else pause *= 3500;
            tape_image[tape_imagesize++] = find_pulse(pause);
            break;
         }
         case 0x21: // group start
         {
            if (!has_bytes(ptr, 1)) return 0;
            n = *ptr++;
            if (!has_bytes(ptr, n)) return 0;
            named_cell(ptr, n); ptr += n;
            appendable = 1;
            break;
         }
         case 0x22: // group end
            break;
         case 0x23: // jump to block
         {
            if (!has_bytes(ptr, 2)) return 0;
            named_cell("* jump"); ptr += 2;
            break;
         }
         case 0x24: // loop start
         {
            if (!has_bytes(ptr, 2)) return 0;
            loop_n = read_u16(ptr); loop_p = tape_imagesize; ptr += 2;
            break;
         }
         case 0x25: // loop end
         {
            if (!loop_n) break;
            size = tape_imagesize - loop_p;
            if (size && loop_n - 1 > (UINT_MAX - tape_imagesize) / size) return 0;
            reserve((loop_n-1) * size);
            for (i = 1; i < loop_n; i++)
               memcpy(tape_image + loop_p + i*size, tape_image + loop_p, size);
            tape_imagesize += (loop_n-1) * size;
            loop_n = 0;
            break;
         }
         case 0x26: // call
         {
            if (!has_bytes(ptr, 2)) return 0;
            n = read_u16(ptr);
            if (!has_bytes(ptr + 2, 2 * n)) return 0;
            named_cell("* call"); ptr += 2 + 2 * n;
            break;
         }
         case 0x27: // ret
            named_cell("* return");
            break;
         case 0x28: // select block
         {
            if (!has_bytes(ptr, 2)) return 0;
            size = read_u16(ptr);
            if (size < 1 || !has_bytes(ptr + 2, size)) return 0;
            end = ptr + 2 + size;
            sprintf(nm, "* choice: ");
            n = ptr[2]; p = (char*)ptr+3;
            for (i = 0; i < n; i++) {
               if ((u8*)p > end || (unsigned)(end - (u8*)p) < 3) return 0;
               t = *(u8*)(p+2);
               if ((unsigned)(end - (u8*)p) < 3 + t) return 0;

               unsigned nm_len = strlen(nm);
               if (i) {
                  if (nm_len > sizeof nm - 4) return 0;
                  memcpy(nm + nm_len, " / ", 3);
                  nm_len += 3;
                  nm[nm_len] = 0;
               }
               if (t >= sizeof nm - nm_len) return 0;
               memcpy(nm + nm_len, p+3, t);
               nm[nm_len+t] = 0;
               p += t+3;
            }
            named_cell(nm); ptr = end;
            break;
         }
         case 0x2A: // stop if 48k
         {
            if (!has_bytes(ptr, 4)) return 0;
            size = read_u32(ptr);
            if (!has_bytes(ptr + 4, size)) return 0;
            named_cell("* stop if 48K"); ptr += 4 + size;
            break;
         }
         case 0x30: // text description
         {
            if (!has_bytes(ptr, 1)) return 0;
            n = *ptr++;
            if (!has_bytes(ptr, n)) return 0;
            named_cell(ptr, n); ptr += n;
            appendable = 1;
            break;
         }
         case 0x31: // message block
         {
            if (!has_bytes(ptr, 2)) return 0;
            n = ptr[1];
            if (!has_bytes(ptr + 2, n)) return 0;
            named_cell("- MESSAGE BLOCK ");
            end = ptr + 2 + n;
            p = (char*)ptr+2;
            while ((u8*)p < end) {
               char *line_end = p;
               while ((u8*)line_end < end && *line_end != 0x0D) line_end++;
               if (line_end == p) named_cell("");
               else named_cell(p, line_end - p);
               p = line_end;
               if ((u8*)p < end) p++;
            }
            ptr = end;
            named_cell("-");
            break;
         }
         case 0x32: // archive info
         {
            if (!has_bytes(ptr, 3)) return 0;
            size = read_u16(ptr);
            if (size < 1 || !has_bytes(ptr + 2, size)) return 0;
            end = ptr + 2 + size;
            named_cell("- ARCHIVE INFO ");
            p = (char*)ptr + 3;
            for (i = 0; i < ptr[2]; i++) {
               const char *info;
               if ((u8*)p > end || (unsigned)(end - (u8*)p) < 2) return 0;
               switch (*(u8*)p) {
                  case 0: info = "Title"; break;
                  case 1: info = "Publisher"; break;
                  case 2: info = "Author"; break;
                  case 3: info = "Year"; break;
                  case 4: info = "Language"; break;
                  case 5: info = "Type"; break;
                  case 6: info = "Price"; break;
                  case 7: info = "Protection"; break;
                  case 8: info = "Origin"; break;
                  case 0xFF: info = "Comment"; break;
                  default:info = "info"; break;
               }
               n = *(u8*)(p+1);
               if ((unsigned)(end - (u8*)p) < 2 + n) return 0;
               sprintf(nm, "%s: %.*s", info, (int)n, p+2);
               p += n + 2;
               named_cell(nm);
            }
            named_cell("-");
            ptr = end;
            break;
         }
         case 0x33: // hardware type
         {
            if (!has_bytes(ptr, 1)) return 0;
            n = *ptr;
            if (!has_bytes(ptr + 1, 3 * n)) return 0;
            parse_hardware(ptr);
            ptr += 1 + 3 * n;
            break;
         }
         case 0x34: // emulation info
         {
            if (!has_bytes(ptr, 8)) return 0;
            named_cell("* emulation info"); ptr += 8;
            break;
         }
         case 0x35: // custom info
         {
            if (!has_bytes(ptr, 0x14)) return 0;
            size = read_u32(ptr+0x10);
            if (!has_bytes(ptr + 0x14, size)) return 0;
            end = ptr + 0x14 + size;
            if (!memcmp(ptr, "POKEs           ", 16)) {
               if ((unsigned)(end - (ptr + 0x14)) < 1) return 0;
               named_cell("- POKEs block ");
               n = ptr[0x14];
               if ((unsigned)(end - (ptr + 0x15)) < n) return 0;
               named_cell(ptr+0x15, n);
               p = (char*)ptr + 0x15 + n;
               if ((u8*)p >= end) return 0;
               n = *(u8*)p++;
               for (i = 0; i < n; i++) {
                  if ((u8*)p >= end) return 0;
                  t = *(u8*)p;
                  if ((unsigned)(end - (u8*)p) < t + 1) return 0;
                  named_cell(p+1, t);
                  p += t+1;
                  if ((u8*)p >= end) return 0;
                  t = *(u8*)p++;
                  strcpy(nm, "POKE ");
                  for (j = 0; j < t; j++) {
                     if ((unsigned)(end - (u8*)p) < 5) return 0;
                     char item[32];
                     if (*(u8*)p & 0x10)
                        sprintf(item, "%d,nn", read_u16((u8*)p+1));
                     else
                        sprintf(item, "%d,%d", read_u16((u8*)p+1), *(u8*)(p+3));
                     if (!(*(u8*)p & 0x08)) sprintf(item + strlen(item), "(page %d)", *(u8*)p & 7);
                     strcat(item, "; ");
                     if (strlen(nm) + strlen(item) < sizeof nm) strcat(nm, item);
                     p += 5;
                  }
                  named_cell(nm);
               }
               named_cell("-");
            } else {
               sprintf(nm, "* custom info: %.16s", (char*)ptr);
               named_cell(nm);
            }
            ptr = end;
            break;
         }
         case 0x40: // snapshot
         {
            if (!has_bytes(ptr, 4)) return 0;
            size = read_u24(ptr+1);
            if (!has_bytes(ptr + 4, size)) return 0;
            named_cell("* snapshot"); ptr += 4 + size;
            break;
         }
         case 0x5A: // 'Z'
            if (!has_bytes(ptr, 9)) return 0;
            ptr += 9;
            break;
         default:
            return 0;
      }
   }
   for (i = 0; i < tape_infosize; i++) {
      if (*(short*)tapeinfo[i].desc == WORD2('*', ' ') &&
          strlen(tapeinfo[i].desc) <= sizeof(tapeinfo[i].desc) - sizeof(" [UNSUPPORTED]"))
         strcat(tapeinfo[i].desc, " [UNSUPPORTED]");
      if (*tapeinfo[i].desc == '-')
         while (strlen(tapeinfo[i].desc) < sizeof(tapeinfo[i].desc)-1)
            strcat(tapeinfo[i].desc, "-");
   }
   if (tape_imagesize && tape_pulse[tape_image[tape_imagesize-1]] < 350000)
      reserve(1), tape_image[tape_imagesize++] = find_pulse(350000); // small pause [rqd for 3ddeathchase]
   find_tape_sizes();
   return (ptr == file_end);
}

u8 tape_bit() // used in io.cpp & sound.cpp
{
	__int64 cur = comp.t_states + cpu.t;
   if (cur < comp.tape.edge_change)
		return (u8)comp.tape.tape_bit;
   while (comp.tape.edge_change < cur)
   {
      if (!temp.sndblock)
      {
         unsigned t = (unsigned)(comp.tape.edge_change - comp.t_states - temp.cpu_t_at_frame_start);
         if ((int)t >= 0)
         {
            unsigned tape_in = conf.sound.micin_vol & comp.tape.tape_bit;
//            comp.tape.sound.update(t, tape_in, tape_in); //Alone Coder
            comp.tape_sound.update(t, tape_in, tape_in); //Alone Coder
         }
      }
      unsigned pulse; comp.tape.tape_bit ^= -1;

#ifdef LOG_TAPE_IN
	fprintf(f_log_tape_in, "%d\t%d\r\n", (u32)comp.tape.edge_change, comp.tape.tape_bit & 1);
#endif

      if (comp.tape.play_pointer == comp.tape.end_of_tape ||
          (pulse = tape_pulse[*comp.tape.play_pointer++]) == -1)
              stop_tape();
      else
          comp.tape.edge_change += pulse;
   }
   return (u8)comp.tape.tape_bit;
}

void fast_tape()
{
   u8 *ptr = am_r(cpu.pc);
   unsigned p = *(unsigned*)ptr;
   if (p == WORD4(0x3D,0x20,0xFD,0xA7))
   { // dec a:jr nz,$-1
      cpu.t += ((u8)(cpu.a-1))*16; cpu.a = 1;
      return;
   }
   if ((u16)p == WORD2(0x10,0xFE))
   { // djnz $
      cpu.t += ((u8)(cpu.b-1))*13; cpu.b = 1;
      return;
   }
   if ((u16)p == WORD2(0x3D,0xC2) && (cpu.pc & 0xFFFF)==(p>>16))
   { // dec a:jp nz,$-1
      cpu.t += ((u8)(cpu.a-1))*14; cpu.a = 1;
      return;
   }
   if ((p | WORD4(0,0,0,0xFF)) == WORD4(0x04,0xC8,0x3E,0xFF))
   {
      if (*(unsigned*)(ptr+4) == WORD4(0xDB,0xFE,0x1F,0xD0) &&
          *(unsigned*)(ptr+8) == WORD4(0xA9,0xE6,0x20,0x28) && ptr[12] == 0xF3)
      { // find edge (rom routine)
         for (;;)
         {
            if (cpu.b == 0xFF)
                return;
            if ((tape_bit() ? 0x20 : 0) ^ (cpu.c & 0x20))
                return;
            cpu.b++; cpu.t += 59;
         }
      }
      if (*(unsigned*)(ptr+4) == WORD4(0xDB,0xFE,0xCB,0x1F) &&
          *(unsigned*)(ptr+8) == WORD4(0xA9,0xE6,0x20,0x28) && ptr[12] == 0xF3)
      { // rra,ret nc => rr a (popeye2)
         for (;;)
         {
            if (cpu.b == 0xFF) return;
            if ((tape_bit() ^ cpu.c) & 0x20) return;
            cpu.b++; cpu.t += 58;
         }
      }
      if (*(unsigned*)(ptr+4) == WORD4(0xDB,0xFE,0x1F,0x00) &&
          *(unsigned*)(ptr+8) == WORD4(0xA9,0xE6,0x20,0x28) && ptr[12] == 0xF3)
      { // ret nc nopped (some bleep loaders)
         for (;;)
         {
            if (cpu.b == 0xFF) return;
            if ((tape_bit() ^ cpu.c) & 0x20) return;
            cpu.b++; cpu.t += 58;
         }
      }
      if (*(unsigned*)(ptr+4) == WORD4(0xDB,0xFE,0xA9,0xE6) &&
          *(unsigned*)(ptr+8) == WORD4(0x40,0xD8,0x00,0x28) && ptr[12] == 0xF3)
      { // no rra, no break check (rana rama)
         for (;;)
         {
            if (cpu.b == 0xFF) return;
            if ((tape_bit() ^ cpu.c) & 0x40) return;
            cpu.b++; cpu.t += 59;
         }
      }
      if (*(unsigned*)(ptr+4) == WORD4(0xDB,0xFE,0x1F,0xA9) &&
          *(unsigned*)(ptr+8) == WORD4(0xE6,0x20,0x28,0xF4))
      { // ret nc skipped: routine without BREAK checking (ZeroMusic & JSW)
         for (;;)
         {
            if (cpu.b == 0xFF) return;
            if ((tape_bit() ^ cpu.c) & 0x20) return;
            cpu.b++; cpu.t += 54;
         }
      }
   }
   if ((p | WORD4(0,0,0,0xFF)) == WORD4(0x04,0x20,0x03,0xFF) &&
        ptr[6] == 0xDB && *(unsigned*)(ptr+8) == WORD4(0x1F,0xC8,0xA9,0xE6) &&
        (*(unsigned*)(ptr+0x0C) | WORD4(0,0,0,0xFF)) == WORD4(0x20,0x28,0xF1,0xFF))
   { // find edge from Donkey Kong
      for (;;) {
         if (cpu.b == 0xFF) return;
         if ((tape_bit() ^ cpu.c) & 0x20) return;
         cpu.b++; cpu.t += 59;
      }
   }
   if ((p | WORD4(0,0xFF,0,0)) == WORD4(0x3E,0xFF,0xDB,0xFE) &&
       *(unsigned*)(ptr+4) == WORD4(0xA9,0xE6,0x40,0x20) &&
       (*(unsigned*)(ptr+8) | WORD4(0xFF,0,0,0)) == WORD4(0xFF,0x05,0x20,0xF4))
   { // lode runner
      for (;;)
      {
         if (cpu.b == 1) return;
         if ((tape_bit() ^ cpu.c) & 0x40) return;
         cpu.t += 52; cpu.b--;
      }
   }
}

void tape_traps()
{
   unsigned pulse;
   do
   {
       if (comp.tape.play_pointer>=comp.tape.end_of_tape ||
          (pulse=tape_pulse[*comp.tape.play_pointer++])==-1)
       {
           stop_tape();
           return;
       }
   }while (pulse > 770);
   comp.tape.play_pointer++;

   // loading header
   cpu.l=0;
   for (unsigned bit=0x80;bit;bit>>=1)
   {
       if (comp.tape.play_pointer>=comp.tape.end_of_tape||
         (pulse=tape_pulse[*comp.tape.play_pointer++])==-1)
       {
         stop_tape();
         cpu.pc = 0x05E2;
         return;
       }
       cpu.l |= (pulse>1240) ? bit : 0;
       comp.tape.play_pointer++;
   }

   // loading data
   do
   {
     cpu.l = 0;
     for (unsigned bit=0x80; bit; bit >>= 1)
     {
       if (comp.tape.play_pointer >= comp.tape.end_of_tape ||
          (pulse = tape_pulse[*comp.tape.play_pointer++])==-1)
       {
         stop_tape();
         cpu.pc = 0x05E2;
         return;
       }
       cpu.l |= (pulse > 1240) ? bit : 0;
       comp.tape.play_pointer++;
     }
     cpu.DbgMemIf->wm(cpu.ix++, cpu.l);
     cpu.de--;
   }while (cpu.de & 0xFFFF);

   // loading CRC
   cpu.l = 0;
   for (unsigned bit = 0x80; bit; bit >>= 1)
   {
     if (comp.tape.play_pointer>=comp.tape.end_of_tape||
       (pulse=tape_pulse[*comp.tape.play_pointer++])==-1)
     {
       stop_tape();
       cpu.pc = 0x05E2;
       return;
     }
     cpu.l |= (pulse > 1240) ? bit : 0;
     comp.tape.play_pointer++;
   }

   cpu.pc = 0x05DF;
   cpu.f |= CF;
   cpu.bc = 0xB001;
   cpu.h = 0;

     /*cpu.pc=0x0604; // the old one
     unsigned pulse;
     pulse = tape_pulse[*comp.tape.play_pointer++];
     if (pulse == -1) stop_tape();
     else{
       comp.t_states+=pulse;
       comp.tape.edge_change = comp.t_states + cpu.t + 520;

       cpu.b+=(pulse-520)/56;
       cpu.f|=CF;
     }*/
}