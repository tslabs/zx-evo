//этот пример показывает опрос мыши и вывод указател€ с использованием спрайта
//а также опрос джойстика и установку указател€ мыши в новые координаты

#include <evo.h>
#include "resources.h"




void main(void)
{
	static u8 i,col;
	static u8 mouse_x,mouse_y;

	//чЄрный экран на врем€ подготовки

	pal_bright(BRIGHT_MIN);

	//вывод фона на теневой экран и установка палитры

	draw_image(0,0,IMG_BACK);
	pal_select(PAL_BACK);

	//переключение экранов, теперь фон на видимом экране

	swap_screen();

	//запуск спрайтов

	sprites_start();

	//установка нормальной €ркости

	pal_bright(BRIGHT_MID);

	//установка позиции и области перемещени€ мыши

	mouse_clip(0,0,160-8,200-16);
	mouse_set(80,100);

	//главный цикл

	while(1)
	{
		//опрос мыши

		i=mouse_pos(&mouse_x,&mouse_y);

		//установка цвета бордюра при нажатии кнопок

		switch(i)
		{
		case MOUSE_LBTN: col=12; break;
		case MOUSE_MBTN: col=14; break;
		case MOUSE_RBTN: col=15; break;
		default: col=0;
		}

		border(col);

		//опрос джойстика и изменение координат мыши по нажати€м направлений

		i=joystick();

		if(i&JOY_LEFT)
		{
			mouse_x-=2;
			if(mouse_x>160-8) mouse_x=0;
		}
		
		if(i&JOY_RIGHT)
		{
			mouse_x+=2;
			if(mouse_x>160-8) mouse_x=160-8;
		}
		
		if(i&JOY_UP)
		{
			mouse_y-=4;
			if(mouse_y>200-16) mouse_y=0;
		}
		
		if(i&JOY_DOWN)
		{
			mouse_y+=4;
			if(mouse_y>200-16) mouse_y=200-16;
		}

		mouse_set(mouse_x,mouse_y);

		//вывод стрелки

		set_sprite(0,mouse_x,mouse_y,0);

		//обновление экрана, спрайты вывод€тс€ автоматически

		swap_screen();
	}
}