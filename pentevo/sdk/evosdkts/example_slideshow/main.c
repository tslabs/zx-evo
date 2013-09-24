#include <evo.h>



void show_picture(u8 image,u8 dir,u8 fade)
{
	static u8 i,j;

	if(fade)
	{
		for(i=0;i<4;++i)
		{
			pal_bright(dir?BRIGHT_MID+i:BRIGHT_MID-i);
			for(j=0;j<4;++j) vsync();
		}
	}

	draw_image(0,0,image);
	pal_select(image);
	swap_screen();

	if(fade)
	{
		for(i=0;i<4;++i)
		{
			pal_bright(dir?BRIGHT_MAX-i:BRIGHT_MIN+i);
			for(j=0;j<4;++j) vsync();
		}
	}
}



void main(void)
{
	static u8 i,joy,joyprev,image;

	image=1;
	joyprev=0;

	show_picture(image,0,1);
    border(3);
	while(1)
	{
		i=joystick();
		joy=i^joyprev&i;
		joyprev=i;

		if(joy&JOY_LEFT)
		{
			--image;
			if(image>4) image=4;

			show_picture(image,0,1);
		}

		if(joy&JOY_RIGHT)
		{
			++image;
			if(image>4) image=0;

			show_picture(image,1,1);
		}

		if(joy&JOY_UP)
		{
			--image;
			if(image>4) image=4;

			show_picture(image,0,0);
		}

		if(joy&JOY_DOWN)
		{
			++image;
			if(image>4) image=0;

			show_picture(image,0,0);
		}
	}
}