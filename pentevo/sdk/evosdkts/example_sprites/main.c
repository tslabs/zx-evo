//этот пример отображает движущиес€ спрайты на фоне изображени€


#include <evo.h>
#include "resources.h"



//структура объекта

struct spriteStruct {
	i16 x,y;	//координаты
	i16 dx,dy;	//дельты скорости
};

//список объектов

#define SPRITES_ALL	22	//в этом примере столько спрайтов успевает отрисоватьс€ за кадр

struct spriteStruct spriteList[SPRITES_ALL];



void main(void)
{
	static u8 i;
	static u8 palette[16];

	//чЄрный экран на врем€ подготовки

	pal_bright(BRIGHT_MIN);

	//инициализаци€ параметров объектов

	for(i=0;i<SPRITES_ALL;++i)
	{
		spriteList[i].x=1+rand16()%(160-8-2);
		spriteList[i].y=1+rand16()%(200-16-2);
		spriteList[i].dx=rand16()&1?-1:1;
		spriteList[i].dy=rand16()&1?-1:1;
	}

	//вывод фона на теневой экран

	draw_image(0,0,IMG_BACK);

	//переключение экранов, теперь фон на видимом экране

	swap_screen();

	//запуск спрайтов

	sprites_start();

	//установка палитры, она собираетс€ из двух разных палитр
	//цвета 0..5 дл€ фона, цвета 6..15 дл€ спрайтов

	pal_copy(PAL_BACK,palette);

	for(i=0;i<6;++i) pal_col(i,palette[i]);

	pal_copy(PAL_BALLS,palette);

	for(i=6;i<16;++i) pal_col(i,palette[i]);

	//установка нормальной €ркости

	pal_bright(BRIGHT_MID);

	//главный цикл

	while(1)
	{
		//перемещение объектов и заполнение списка спрайтов

		for(i=0;i<SPRITES_ALL;++i)
		{
			//i&3 выбирает один из четырех разноцветных шариков

			set_sprite(i,spriteList[i].x,spriteList[i].y,i&3);

			if(spriteList[i].x==160-8 ||spriteList[i].x==0) spriteList[i].dx=-spriteList[i].dx;
			if(spriteList[i].y==200-16||spriteList[i].y==0) spriteList[i].dy=-spriteList[i].dy;

			spriteList[i].x+=spriteList[i].dx;
			spriteList[i].y+=spriteList[i].dy;
		}

		//обновление экрана, спрайты вывод€тс€ автоматически

		swap_screen();
	}
}