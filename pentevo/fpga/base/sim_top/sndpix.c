#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "sndpix.h"

DPI_LINK_DECL DPI_DLLESPEC
int
sndpix(
    int hcoord,
    int vcoord,
    int rrggbb,
    int hperiod,
    int vperiod)
{
	static int hsize=0,vsize=0;


	static int need_init = 1;
	static int sock_error=0;
	static int mysocket;
	struct sockaddr_in addr;
	char buf[256];


	char * curbuf;
	int tosend;
	int socksent;



	if( need_init )
	{
		need_init = 0;


		mysocket = socket(AF_INET, SOCK_STREAM, 0);

		if(mysocket<0) sock_error++;


		addr.sin_family = AF_INET;
		addr.sin_port = htons(12345);
//		addr.sin_addr.s_addr = htonl(0xAC100594); //172.16.5.148
		addr.sin_addr.s_addr = htonl(0x7F000001); //127.0.0.1



		if( !sock_error)
		{
			if( connect(mysocket, (struct sockaddr *)&addr, sizeof(addr) ) )
			{
				sock_error++;
			}
		}
	}



	if( !sock_error )
	{
//		sprintf(buf,"%08X<=%08X:%08X with %02X\n",adr,dat_hi,dat_lo,sel);

		buf[0] = 0;

		if( (hperiod>0) && (vperiod>0) )
		{
			if( (hperiod!=hsize) || (vperiod!=vsize) )
			{
				hsize = hperiod;
				vsize = vperiod;

				sprintf(buf,"s%04X,%04X\n",hsize,vsize);
			}
		}

		if( (hsize>0) && (vsize>0) )
			sprintf(buf+strlen(buf),"p%04X,%04X,%02X\n",hcoord,vcoord,rrggbb);

		if( strlen(buf)>0 )
		{
			tosend = strlen(buf);
			curbuf = buf;



			while( tosend>0 )
			{
				socksent = send(mysocket, curbuf, tosend, 0);

				if( socksent<=0 )
				{
					sock_error++;
					break;
				}

				tosend -= socksent;
				curbuf += socksent;
			}
			
//			if( strlen(buf)!=socksent )
//			{
//				printf("send() sent %d bytes, errno=%d\n",socksent,errno);
//				sock_error++;
//			}
		}
	}
	else
	{
	//	sock_error = 0;
	//	need_init = 1;

		if( mysocket>=0 )
			close(mysocket);
	}


	return 0;
}

