#include "std.h"
#include "emul.h"
#include "vars.h"
#include "util.h"

#include "savevid.h"


//#define DEBUG 1
//handles
HANDLE hPipe=INVALID_HANDLE_VALUE;
STARTUPINFO si;
PROCESS_INFORMATION pi;

TSVSet SVSet;           //settings
int videosaver_state;   //0-not saving, 1-saving


//AVI global hdr + video hdrs
char AVIRIFF[]=
"\x52\x49\x46\x46\x00\x00\x00\x00\x41\x56\x49\x20\x4C\x49\x53\x54" //RIFF size=0 (inf)
"\x3c\x01\x00\x00\x68\x64\x72\x6C\x61\x76\x69\x68\x38\x00\x00\x00"
"\x20\x4E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10\x01\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00"
"\x80\x02\x00\x00\xE0\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x4C\x49\x53\x54\x80\x00\x00\x00"
"\x73\x74\x72\x6C\x73\x74\x72\x68\x38\x00\x00\x00\x76\x69\x64\x73"
"\x44\x49\x42\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x01\x00\x00\x00\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\xFF\xFF\xFF\xFF\x00\x00\x00\x00\x00\x00\x00\x00"
"\x80\x02\xE0\x01\x73\x74\x72\x66\x28\x00\x00\x00\x28\x00\x00\x00"
"\x80\x02\x00\x00\xE0\x01\x00\x00\x01\x00\x18\x00\x00\x00\x00\x00"
"\x00\x10\x0E\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x4A\x55\x4E\x4B\x04\x00\x00\x00\x00\x00\x00\x00"
;

//AVI audio hdrs
char AVIRIFF2[]=
"\x4C\x49\x53\x54\x68\x00\x00\x00\x73\x74\x72\x6C\x73\x74\x72\x68"
"\x38\x00\x00\x00\x61\x75\x64\x73\x00\x00\x00\x00\x00\x00\x00\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x10\xB1\x02\x00"
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF"
"\x04\x00\x00\x00\x6D\x00\x70\x00\x67\x00\x3B\x00\x73\x74\x72\x66"
"\x10\x00\x00\x00\x01\x00\x02\x00\x44\xAC\x00\x00\x10\xB1\x02\x00"
"\x04\x00\x10\x00\x4A\x55\x4E\x4B\x04\x00\x00\x00\x00\x00\x00\x00"
;

//AVI start streams
char AVIRIFF3[]=
"\x4C\x49\x53\x54\x00\x00\x00\x00\x6D\x6F\x76\x69"; //LIST movi size=0

//AVI stream data headers
char avi_frameh_vid[] = "00db    "; //DIB
char avi_frameh_aud[] = "01wb    "; //wave


//proto
static int pipewrite(HANDLE hPipe, char *buf, int len);
static void savevideo_finish();


//init:
//ffmpeg_exec  - path/name of ffmpeg executable (ex: "ffmpeg.exe")
//ffmpeg_param - params for ffmpeg output video (ex: "-c:a libmp3lame -b:a 320k")
//out_fname    - output video name (ex: "video0.avi")
//newcons      - create new console for ffmpeg (0/1)
//w,h          - width, height (ex: 460, 480)
//fps          - frames per second (ex: 50)
//sndfq        - sound sample rate (ex: 44100)
static int savevideo_init(const char *ffmpeg_exec, const char *ffmpeg_param, const char *out_fname, char newcons, int w, int h, int fps, int sndfq)
{
    //create named pipe for stream video to ffmpeg
    hPipe=CreateNamedPipe(
        PIPENAME,                 // pipe name
        PIPE_ACCESS_OUTBOUND,     // write access
        PIPE_TYPE_BYTE |          // byte type pipe
        PIPE_READMODE_BYTE |      // byte-read mode
        PIPE_NOWAIT,              // non blocking mode (for async connect)
        1,                        // max. instances
        PIPESIZE,                 // output buffer size
        1024,                     // input buffer size
        0,                        // client time-out
        NULL);                    // default security attribute

    if(hPipe==INVALID_HANDLE_VALUE)
    {
        color(CONSCLR_ERROR); printf("error: CreateNamedPipe failed.\n");
        return -1;
    }
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: named pipe '%s' created.\n",PIPENAME);
#endif

    //start ffmpeg process
    char args[VS_MAX_FFPATH+VS_MAX_FFPARM+VS_MAX_FFVOUT+100];
    _snprintf(args, sizeof(args), "\"%s\" -i %s %s -y %s", ffmpeg_exec, PIPENAME, ffmpeg_param, out_fname);
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: %s\n", args);
#endif

    memset(&si, 0, sizeof(si));
    si.cb=sizeof(si);
    //si.wShowWindow=SW_SHOW;
    si.wShowWindow=SW_MINIMIZE;
    //si.wShowWindow=SW_HIDE;
    si.dwFlags=STARTF_USESHOWWINDOW;
    memset(&pi, 0, sizeof(pi));

    if(!CreateProcess(NULL, // no app name
        args,               // cmd line
        NULL,               // proc attr
        NULL,               // thread attr
        FALSE,              // Inherit Handles
        (newcons)?CREATE_NEW_CONSOLE:0, // Creation Flags
        NULL,               // Environment
        NULL,               // Current Directory
        &si,                // Startup Info
        &pi))               // Process Information
    {
        color(CONSCLR_ERROR); printf("error: can not start ffmpeg.\n");
        CloseHandle(hPipe);
        return -1;
    }
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: ffmpeg started.\n");
#endif

    //wait for connection from ffmpeg, 5 sec timeout
    unsigned long t=GetTickCount();
    int conn=0;
    while(t+5000ul>GetTickCount())
    {
        conn=ConnectNamedPipe(hPipe, NULL) ? 1 : (GetLastError()==ERROR_PIPE_CONNECTED);
        if(conn) break;
        Sleep(10);
    }

    //connected?
    if (!conn)
    {
        color(CONSCLR_ERROR); printf("error: no connection from ffmpeg.\n");
        CloseHandle(hPipe);
        return -1;
    }
    DWORD dwMode=PIPE_READMODE_BYTE | PIPE_WAIT;
    SetNamedPipeHandleState(hPipe, &dwMode, NULL,NULL); //set blocking mode
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: got connection from pipe.\n");
#endif

    //patch and send video header
    //with negative height we can use linear bitmap data! :)
    //this also fix ffmpeg's bug-o-feature with BottomUp property when '-c:v copy' is used
    *(unsigned int*)(AVIRIFF+0x20)=1000000/fps;
    *(unsigned int*)(AVIRIFF+0x40)=w;
    *(unsigned int*)(AVIRIFF+0x44)=-h;

    *(unsigned int*)(AVIRIFF+0x84)=fps;
    *(unsigned short*)(AVIRIFF+0xa0)=w;
    *(unsigned short*)(AVIRIFF+0xa2)=-h;
    *(unsigned int*)(AVIRIFF+0xb0)=w;
    *(unsigned int*)(AVIRIFF+0xb4)=-h;

    *(unsigned int*)(AVIRIFF2+0x2c)=sndfq*4;
    *(unsigned int*)(AVIRIFF2+0x58)=sndfq;

    int res=pipewrite(hPipe,AVIRIFF,sizeof(AVIRIFF)-1);
    res+=pipewrite(hPipe,AVIRIFF2,sizeof(AVIRIFF2)-1);
    res+=pipewrite(hPipe,AVIRIFF3,sizeof(AVIRIFF3)-1);
    if(res<0)
    {
        //pipe error, finish
        color(CONSCLR_ERROR); printf("error: ffmpeg aborted connection at the beginning.\n");
        savevideo_finish();
        return -1;
    }
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: video header sent.\n");
#endif

    return 0;
}


//send video frame to stream:
//buf  - picture buffer (raw bmp RGB24 format)
//size - size of picture (W*H*3), bytes
static int savevideo_put_vframe(char *buf, unsigned size)
{
    //send frame header
    *(unsigned int*)(avi_frameh_vid+4)=size;
    int res=pipewrite(hPipe,avi_frameh_vid,8);
    if(res<0) return -1;

    //send frame data
    res=pipewrite(hPipe,buf,size);
    if(res<0) return -1;
    return 0;
}

//send audio frame to stream:
//buf  - sound buffer (raw 16 bit le stereo)
//size - length of sound data, bytes
static int savevideo_put_aframe(char *buf, unsigned int size)
{
    //send frame header
    *(unsigned int*)(avi_frameh_aud+4)=size;
    int res=pipewrite(hPipe,avi_frameh_aud,8);
    if(res<0) return -1;

    //send frame data
    res=pipewrite(hPipe,buf,size);
    if(res<0) return -1;
    return 0;
}

//finish saving: close handles
static void savevideo_finish()
{
    //send video trailer (none)
    //close pipe
    CloseHandle(hPipe);

    //wait for ffmpeg done
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: waiting for ffmpeg finish.\n");
#endif
    WaitForSingleObject(pi.hProcess, INFINITE);
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: saving video done.\n");
#endif

    //close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}


//send data block to pipe
static int pipewrite(HANDLE hPipe, char *buf, int len)
{
    DWORD cbWritten = 0;
    int res = 0;

    int p=0;
    while(p<len) //is all data sent?
    {
        res = WriteFile( 
            hPipe,      // handle to pipe 
            &buf[p],    // buffer to write from 
            len-p,      // number of bytes to write 
            &cbWritten, // number of bytes written 
            NULL);      // not overlapped I/O 
        if(res)
        {
            p+=cbWritten;
        }
        else
        {
#ifdef DEBUG
color(CONSCLR_INFO); printf("debug: pipe disconnected.\n");
#endif
            return -1;
        }
    }
    return 0;
}



/*****************************************************************************
 * Public functions                                                          *
 *****************************************************************************/

//main function of video saver
void main_savevideo()
{
    static int vidn=0;
    char video_name[VS_MAX_FFVOUT],tmp[VS_MAX_FFVOUT]={0};
    int res;

    if(videosaver_state==0) //start saver
    {
        //construct name of output file
        strncpy(tmp,conf.ffmpeg.vout,sizeof(tmp)-1);
        char *p=strstr(tmp,"#");
        if(p)
        {   //"#" found, split name into two parts, insert a number
            *p=0;
            _snprintf(video_name,sizeof(video_name),"%s%u%s",tmp,vidn,p+1);
        }
        else
            _snprintf(video_name,sizeof(video_name),"%s",tmp);

        //init ffmpeg
        res=savevideo_init(conf.ffmpeg.exec, conf.ffmpeg.parm, video_name, 
            conf.ffmpeg.newcons, temp.ox, temp.oy, conf.intfq, conf.sound.fq);
        if(res) //init ok?
        {
            color(CONSCLR_ERROR); printf("error: init video saver failed.\n");
            return;
        }
        sprintf(statusline, "start saving video");


        //store screen and audio settings
        SVSet.xsz=temp.ox;
        SVSet.ysz=temp.oy;
        SVSet.fps=conf.intfq;
        SVSet.sndfq=conf.sound.fq;
        SVSet.snden=conf.sound.enabled;

        //allocate buffers for pictures
        SVSet.dx = temp.ox * temp.obpp / 8;
        SVSet.scrbuf_unaligned = (unsigned char*)malloc(SVSet.dx * temp.oy + CACHE_LINE);
        SVSet.scrbuf = (unsigned char*)align_by(SVSet.scrbuf_unaligned, CACHE_LINE);
        SVSet.dsll = ((temp.ox * 3 + 3) & ~3);
        SVSet.ds = (u8*)malloc(SVSet.dsll * temp.oy);

        vidn++;
        videosaver_state=1;
    }
    else //saving done
    {
        //stop ffmpeg
        savevideo_finish();
        sprintf(statusline, "stop saving video");

        //free buffers
        free(SVSet.ds);
        free(SVSet.scrbuf_unaligned);

        videosaver_state=0;
    }

    statcnt = 25;  //show status during 25 frames
}


//save graphics handler
void savevideo_gfx()
{
    //is format changed?
    if(temp.ox!=SVSet.xsz || temp.oy!=SVSet.ysz || 
       conf.intfq!=SVSet.fps || conf.sound.fq!=SVSet.sndfq || 
       conf.sound.enabled!=SVSet.snden)
    {
        main_savevideo(); //stop saving!
        return;
    }

    //render screen to scrbuf buffer
    renders[conf.render].func(SVSet.scrbuf, SVSet.dx); // render to memory buffer (PAL8, YUY2, RGB15, RGB16, RGB32)
    //convert colors to RGB24
    ConvBgr24(SVSet.ds, SVSet.scrbuf, SVSet.dx);
    //send frame to encoder
    if(savevideo_put_vframe((char*)SVSet.ds, SVSet.dsll*SVSet.ysz))
    {
        //stop saving if error occured
        color(CONSCLR_ERROR); printf("error: ffmpeg aborted connection.\n");
        main_savevideo();
        return;
    }
}

//save sound handler
void savevideo_snd()
{
    //is format changed?
    if(temp.ox!=SVSet.xsz || temp.oy!=SVSet.ysz || 
       conf.intfq!=SVSet.fps || conf.sound.fq!=SVSet.sndfq || 
       conf.sound.enabled!=SVSet.snden)
    {
        main_savevideo(); //stop saving!
        return;
    }

    //send frame to encoder
    if(savevideo_put_aframe((char*)sndplaybuf,spbsize))
    {
        //stop saving if error occured
        color(CONSCLR_ERROR); printf("error: ffmpeg aborted connection.\n");
        main_savevideo();
        return;
    }
}
