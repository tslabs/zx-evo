#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#if defined(_MSC_VER)
#include "SDL.h"
#else
#include "SDL/SDL.h"
#endif

SDL_Surface *plane;


void net_init(void);
void net_recv(void);
void parse_recv_data(char * buf,int bytes_read);
void bufrender(void);

void render(void);

void exitt(int);

#define SIZEX 896
#define SIZEY 640
#define BITSIZE 32

#define ZXSIZEX 1792
#define ZXSIZEY 320

char zxbuf[ZXSIZEX*ZXSIZEY*4];


// Entry point
int main(int argc, char *argv[])
{
	// Initialize SDL's subsystems - in this case, only video.
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) 
	{
		fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
		exit(1);
	}

	// Register SDL_Quit to be called at exit; makes sure things are
	// cleaned up when we quit.
	atexit(SDL_Quit);
    
	// Attempt to create a window with 32bit pixels.
	plane = SDL_SetVideoMode(SIZEX, SIZEY, BITSIZE, SDL_SWSURFACE);
  
	// If we fail, return error.
	if ( plane == NULL ) 
	{
		fprintf(stderr, "Unable to set 896x640 video: %s\n", SDL_GetError());
		exit(1);
	}

	
	net_init();  
  
  
	// Main loop: loop forever.
	while (1)
	{
		net_recv();

		render();

		// Poll for events, and handle the ones we care about.
		SDL_Event event;
		while (SDL_PollEvent(&event)) 
		{
			switch (event.type) 
			{
			case SDL_KEYDOWN:
				break;
			case SDL_KEYUP:
				// If escape is pressed, return (and thus, quit)
				if (event.key.keysym.sym == SDLK_ESCAPE)
					return 0;
				break;
			case SDL_QUIT:
				return(0);
			}
		}
	}
	return 0;
}






int sock=(-1), listener=(-1);
struct sockaddr_in addr;




void net_init(void)
{
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if(listener < 0)
	{
		perror("socket init error");
		exit(1);
	}
	
	int tr=1;
	
	setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&tr,sizeof(int));
    
	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(listener, (struct sockaddr *)&addr, sizeof(addr)) < 0)
	{
		perror("bind error");
		exit(2);
	}

	listen(listener, 1);
    
	sock = accept(listener, NULL, NULL);
	if(sock < 0)
	{
		perror("accept error");
		exitt(3);
	}
}





void net_recv(void)
{
#define BUFSIZE 10240
	char buf[BUFSIZE];

	
	int bytes_read;



	bytes_read = recv(sock,buf,BUFSIZE, MSG_DONTWAIT);
//	bytes_read = recv(sock,buf,BUFSIZE, 0);

	if( bytes_read<0 )
	{
		if( ! ((errno==EAGAIN) || (errno==EWOULDBLOCK)) )
		{
			printf("recv failed - error %d!\n",errno);
			exitt(1);
		}
	}
	else if( bytes_read==0 )
	{
		printf("recv failed - closed! errno=%d\n",errno);
		exitt(1);
	}
	else // >0
	{
		parse_recv_data(buf,bytes_read);
	}

//	if( 4!=sscanf(buf,"%X<=%X:%X with %X",&addr,&dat_hi,&dat_lo,&sel) )
}





void parse_recv_data(char * buf,int bytes_read)
{
#define PBUF_SIZE 63
	char pbuf[PBUF_SIZE+1];

	static int parsed = 0;


	int i;
	char c;
	char r,g,b;
	char * pix;

	int xcoord,ycoord,color;


	i=0;

	while( bytes_read>0 )
	{
		c = *(buf++);

		if(c=='\n')
		{
			pbuf[parsed]=0;
			parsed=0;

			if( (*pbuf)=='p' )
			{
				//printf("%s\n",pbuf+1);

				if( 3==sscanf(pbuf+1,"%X,%X,%X",&xcoord,&ycoord,&color) )
				{
					if( (0<=xcoord)&&(xcoord<ZXSIZEX)&&
					    (0<=ycoord)&&(ycoord<ZXSIZEY) )
					{
						pix = zxbuf + ( xcoord*4 + (ycoord*ZXSIZEX*4) );
						
						r = (color<<2);
						g = (color<<4);
						b = (color<<6);

						r &= 0xC0;
						g &= 0xC0;
						b &= 0xC0;

						r |= r>>2;
						g |= g>>2;
						b |= b>>2;

						r |= r>>4;
						g |= g>>4;
						b |= b>>4;

						*(pix+0) = b;
						*(pix+1) = g;
						*(pix+2) = r;
					}
				}
			}
		}
		else
		{
			if(parsed>=PBUF_SIZE)
			{
				printf("too long string in network data!\n");
				exitt(1);
			}
			pbuf[parsed++]=c;
		}

		bytes_read--;
	}
}





void bufrender(void)
{

	int bx,by;

	int * la1, * la2;

	unsigned char * za;

	int r,g,b;
	int value;


	for(by=0;by<ZXSIZEY;by++)
	{
		la1 = ((int *)plane->pixels) + by*((plane->pitch)>>2)*2;
		la2 = la1 + ((plane->pitch)>>2);

		za = (unsigned char *)zxbuf + (by*ZXSIZEX*4);

		for(bx=0;bx<(ZXSIZEX/2);bx++)
		{
			r = *(za+2);
			g = *(za+1);
			b = *(za+0);

			r += *(za+6);
			g += *(za+5);
			b += *(za+4);

			r <<= 15;
			g <<= 7;
			b >>= 1;

			r &= 0x00FF0000;
			g &= 0x0000FF00;
			b &= 0x000000FF;

			za += 8;

			value = r|g|b;

			*(la1++) = value;
			*(la2++) = value;
		}
	}
}










void render()
{   
	// Lock surface if needed
	if( SDL_MUSTLOCK(plane) ) 
		if (SDL_LockSurface(plane) < 0) 
			return;

	// Ask SDL for the time in milliseconds
	int tick = SDL_GetTicks();

	// Unlock if needed
	if( SDL_MUSTLOCK(plane) )
		SDL_UnlockSurface(plane);

	//render picture
	bufrender();

	// Tell SDL to update the whole plane
	SDL_UpdateRect(plane, 0, 0, SIZEX, SIZEY);
}








void exitt(int a)
{

	if( sock>=0 ) close(sock);

	if( listener>=0 ) close(listener);

	exit(a);





}

