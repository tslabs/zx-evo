#pragma once

class TKeyboardBuffer
{
    unsigned char buffer[256];
    unsigned char push;
    unsigned char pop;
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

    void Push(unsigned char key)
    {
        if (!full)
        {
            buffer[push++] = key;
            if (push == sizeof(buffer)) push = 0;
            if (push == pop) full = true;
        }        
    }

    unsigned char Pop()
    {
        if (!full)
        {
            if (push != pop)
            {
                unsigned char key = buffer[pop++];
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
      unsigned char zxkeys[8];
      unsigned zxdata[2];
   };
   unsigned char mode, R7, lastscan, cmd;
   unsigned char kR1, kR2, kR3, kR4, kR5;

   void processzx(unsigned scancode, unsigned char pressed);
   unsigned char read(unsigned char scan, unsigned char zxdata);
   void setkey(unsigned scancode, unsigned char pressed);
   void reset();
   void clear();
};

struct K_INPUT
{
#pragma pack(push, 1)
   union
   {
      volatile unsigned char kbd[16];
      volatile unsigned kbd_x4[4];
   };

   union
   { // without keymatrix effect
      volatile unsigned char rkbd[16];
      volatile unsigned rkbd_x4[4];
   };
#pragma pack(pop)

   unsigned lastkey, nokb, nomouse;

   enum { KM_DEFAULT, KM_KEYSTICK, KM_PASTE_HOLD, KM_PASTE_RELEASE } keymode;

   int msx, msy, msx_prev, msy_prev, ay_x0, ay_y0;
   unsigned ay_reset_t;
   unsigned char mbuttons, ayR14;

   volatile unsigned char kjoy;
   unsigned char mousejoy;
   unsigned char kbdled, mouse_joy_led;
   unsigned char firedelay, firestate; // autofire vars

   ATM_KBD atm51;
   TKeyboardBuffer buffer;

   unsigned stick_delay;
   int prev_wheel;

   unsigned char *textbuffer;
   unsigned textoffset, textsize;
   unsigned char tdelay, tdata, wheel; //0.36.6 from 0.35b2

   unsigned char kempston_mx();
   unsigned char kempston_my();

   unsigned char aymouse_rd();
   void aymouse_wr(unsigned char val);

   void clear_zx();
   inline void press_zx(unsigned char key);
   bool process_pc_layout();
   void make_matrix();
   char readdevices();
   unsigned char read(unsigned char scan);
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
