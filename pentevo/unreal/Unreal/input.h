#pragma once

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
                u8 key = buffer[pop++];
                if (pop == sizeof(buffer)) pop = 0;
                return key;
            }
            return 0;
        }
        return 0xFF;
    }
};

struct ATM_KBD
{
   union {
      u8 zxkeys[8];
      unsigned zxdata[2];
   };
   u8 mode, R7, lastscan, cmd;
   u8 kR1, kR2, kR3, kR4, kR5;

   void processzx(unsigned scancode, u8 pressed);
   u8 read(u8 scan, u8 zxdata);
   void setkey(unsigned scancode, u8 pressed);
   void reset();
   void clear();
};

struct K_INPUT
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

   enum { KM_DEFAULT, KM_KEYSTICK, KM_PASTE_HOLD, KM_PASTE_RELEASE } keymode;

   int msx, msy, msx_prev, msy_prev, ay_x0, ay_y0;
   unsigned ay_reset_t;
   u8 mbuttons, ayR14;

   volatile u8 kjoy;
   u8 mousejoy;
   u8 kbdled, mouse_joy_led;
   u8 firedelay, firestate; // autofire vars

   ATM_KBD atm51;
   TKeyboardBuffer buffer;

   unsigned stick_delay;
   int prev_wheel;

   u8 *textbuffer;
   unsigned textoffset, textsize;
   u8 tdelay, tdata, wheel; //0.36.6 from 0.35b2

   u8 kempston_mx();
   u8 kempston_my();

   u8 aymouse_rd();
   void aymouse_wr(u8 val);

   void clear_zx();
   inline void press_zx(u8 key);
   bool process_pc_layout();
   void make_matrix();
   char readdevices();
   u8 read(u8 scan);
   u8 read_quorum(u8 scan);
   void paste();

   K_INPUT()
   {
      textbuffer = 0;
      // random data on coords -> some programs detects mouse by this
      ay_x0 = msx = 31,
      ay_y0 = msy = 85;

      nokb = nomouse = 0;
   }
};
