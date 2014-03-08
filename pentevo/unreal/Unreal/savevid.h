#pragma once

#define VS_MAX_FFPATH 256		//ffmpeg path and name length
#define VS_MAX_FFPARM 1024 		//ffmpeg parameters length
#define VS_MAX_FFVOUT 512 		//ffmpeg out video name length

//named pipe settings
#define PIPENAME "\\\\.\\pipe\\us_video"
#define PIPESIZE 1024000

extern int videosaver_state;	//0-not saving, 1-saving

//Video Saver Settings
struct TSVSet
{
    unsigned        xsz,ysz,    //W,H in pix
                    fps,        //fps
                    sndfq,      //sample rate
                    dx,         //size of line in source buffer
                    dsll;		//size of line in RGB24 buffer (padded to 32 bit)
    unsigned char   *scrbuf, 	//aligned buffer for render
                    *scrbuf_unaligned, //allocated buffer for render
                    snden;      //sound enabled flag (max speed toggle)
    u8              *ds; 		//output RGB24 buffer
};

extern TSVSet SVSet;

void main_savevideo();
void savevideo_gfx();
void savevideo_snd();
