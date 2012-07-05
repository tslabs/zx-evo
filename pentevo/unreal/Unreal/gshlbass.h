

//typedef DWORD (CALLBACK *STREAMPROC)(HSTREAM,void*,DWORD,DWORD);


//#define MAKEMUSICPOS(order,row) (0x80000000|MAKELONG(order,row))

/*
typedef struct
{
        DWORD flags;    // device capabilities (DSCAPS_xxx flags)
        DWORD hwsize;   // size of total device hardware memory
        DWORD hwfree;   // size of free device hardware memory
        DWORD freesam;  // number of free sample slots in the hardware
        DWORD free3d;   // number of free 3D sample slots in the hardware
        DWORD minrate;  // min sample rate supported by the hardware
        DWORD maxrate;  // max sample rate supported by the hardware
        BOOL eax;               // device supports EAX? (always FALSE if BASS_DEVICE_3D was not used)
        DWORD minbuf;   // recommended minimum buffer length in ms (requires BASS_DEVICE_LATENCY)
        DWORD dsver;    // DirectSound version
        DWORD latency;  // delay (in ms) before start of playback (requires BASS_DEVICE_LATENCY)
        DWORD initflags;// "flags" parameter of BASS_Init call
        DWORD speakers; // number of speakers available
        const char *driver;     // driver
        DWORD freq;             // current output rate (OSX only)
} BASS_INFO;
*/

