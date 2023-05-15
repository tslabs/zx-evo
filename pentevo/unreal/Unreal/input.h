#pragma once
#include "sysdefs.h"

class TKeyboardBuffer
{
    u8 buffer[256];
    u8 push;
    u8 pop;
    bool full;
    bool enabled;

public:
    TKeyboardBuffer() : enabled(false) { Empty(); }
    
    bool Enabled() { return enabled; }
    
    void Enable(bool enable)
    {
        Empty();
        enabled = enable;
    }

    void Empty()
    {
        push = pop = 0;
        full = false;
    }

    void Push(u8 key)
    {
        if (!full)
        {
            buffer[push++] = key;
            if (push == sizeof(buffer)) push = 0;
            if (push == pop) full = true;
        }        
    }

    u8 Pop()
    {
        if (!full)
        {
            if (push != pop)
            {
	            const u8 key = buffer[pop++];
                if (pop == sizeof(buffer)) pop = 0;
                return key;
            }
            return 0;
        }
        return 0xFF;
    }
};

struct k_input
{
#pragma pack(push, 1)
   union
   {
      volatile u8 kbd[16];
      volatile unsigned kbd_x4[4];
   };

   union
   { // without keymatrix effect
      volatile u8 rkbd[16];
      volatile unsigned rkbd_x4[4];
   };
#pragma pack(pop)

   unsigned lastkey, nokb, nomouse;

   enum { km_default, km_keystick, km_paste_hold, km_paste_release } keymode;

   int msx, msy, msx_prev, msy_prev, ay_x0, ay_y0;
   unsigned ay_reset_t;
   u8 mbuttons, ay_r14;

   volatile u8 kjoy;
   u8 mousejoy;
   u8 kbdled, mouse_joy_led;
   u8 firedelay, firestate; // autofire vars

   TKeyboardBuffer buffer;

   unsigned stick_delay;
   int prev_wheel;

   u8 *textbuffer;
   unsigned textoffset, textsize;
   u8 tdelay, tdata, wheel; //0.36.6 from 0.35b2

   u8 kempston_mx() const;
   u8 kempston_my() const;

   u8 aymouse_rd() const;
   void aymouse_wr(u8 val);

   void clear_zx();
   inline void press_zx(u8 key);
   bool process_pc_layout();
   void make_matrix();
   char readdevices();
   u8 read(u8 scan);
   u8 read_quorum(u8 scan);
   void paste();

   k_input()
   {
      textbuffer = nullptr;
      // random data on coords -> some programs detects mouse by this
      ay_x0 = msx = 31;
      ay_y0 = msy = 85;

      nokb = nomouse = 0;
   }
};
