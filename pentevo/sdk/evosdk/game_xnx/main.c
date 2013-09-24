#include <evo.h>
#include "resources.h"


#define MAP_WDT	38
#define MAP_HGT	22

static u8 map[MAP_WDT*MAP_HGT];
static u8 updmap[MAP_WDT*MAP_HGT];
static u8 fillmap[MAP_WDT*MAP_HGT];



struct {
	u8 x,y;
	u8 prevx,prevy;
	u8 sx,sy;
	u8 dir;
	u8 prevdir;
	u8 off;
	u16 draw;
	u8 fill;
	u8 startx,starty;
} player;



#define OBJ_MAX	10

static struct objStruct {
	i16 x,y;
	i16 dx,dy;
	u8 hit;
} objList[OBJ_MAX];

static u8 objCount;

static u16 gameScore;
static u8 gameDone;
static u8 gameLevel;
static i8 gamePower;
static u8 gameClearTarget;
static u16 gameAreaClear;
static u8 gameStartDelay;
static u8 gameRestartDelay;
static u8 gameHitCnt;
static u8 gameImageID;

static u8 explodeX;
static u8 explodeY;
static u8 explodeCnt;

static u8 keys[40];

#define FP	6

#define MARK_SIDE(off,side) if(!map[(off)]) fillmap[(off)]=(side)
#define TEST_MAP(x,y) (map[((y)>>3)*MAP_WDT+((x)>>3)])

#define LEVELS_ALL	10

//описание уровня
//картинка, палитра, музыка, сколько процентов надо набрать
//объекты по 4 байта - координаты в знакоместах, начальные дельты в пикселях
//255 в конце списка объектов

const u8 levelData1[]={
IMG_PIC1,PAL_PIC1,MUS_LOOP1,85,
 9,10,-16,16,
28,10, 16,16,
255
};

const u8 levelData2[]={
IMG_PIC2,PAL_PIC2,MUS_LOOP1,85,
 9, 8,-16,16,
19,12, 16,16,
28, 8,-16,16,
255
};

const u8 levelData3[]={
IMG_PIC3,PAL_PIC3,MUS_LOOP1,85,
 9, 6, 16, 16,
28, 6,-16, 16,
 9,15, 16,-16,
28,15,-16,-16,
255
};

const u8 levelData4[]={
IMG_PIC4,PAL_PIC4,MUS_LOOP1,80,
 9, 6, 16, 16,
28, 6,-20, 20,
 9,15, 20,-20,
28,15,-16,-16,
255
};

const u8 levelData5[]={
IMG_PIC5,PAL_PIC5,MUS_LOOP1,80,
 9, 6, 16, 16,
28, 6,-20, 20,
19,10, 18,-18,
 9,15, 20,-20,
28,15,-16,-16,
255
};

const u8 levelData6[]={
IMG_PIC6,PAL_PIC6,MUS_LOOP1,80,
 7,10,-20,-20,
14, 6,-18,-18,
14,15,-18, 18,
24, 6, 18,-18,
24,15, 18, 18,
31,10, 20, 20,
255
};

const u8 levelData7[]={
IMG_PIC7,PAL_PIC7,MUS_LOOP1,75,
 7, 6,-18,-18,
 7,15,-18, 18,
14,10, 10,-10,
19,10, 20,-20,
24,10,-10, 10,
31, 6, 18,-18,
31,15, 18, 18,
255
};

const u8 levelData8[]={
IMG_PIC8,PAL_PIC8,MUS_LOOP1,75,
10, 4,-16, 16,
28, 4,-18, 18,
16, 8, 20, 20,
22, 8,-20, 20,
16,14, 20,-20,
22,14,-20,-20,
10,18, 18, 18,
28,18, 16, 16,
255
};

const u8 levelData9[]={
IMG_PIC9,PAL_PIC9,MUS_LOOP1,70,
13, 4, 18, 18,
10, 7, 18, 18,
 7,10, 18, 18,
 4,13, 18, 18,
20,11, 26, 26,
25,18,-18,-18,
28,15,-18,-18,
31,12,-18,-18,
34, 9,-18,-18,
255
};

const u8 levelData10[]={
IMG_PIC10,PAL_PIC10,MUS_LOOP1,70,
10,10,-22,-22,
12,10,-21, 21,
14,10, 20,-20,
16,10, 19, 19,
18,10,-18,-18,
20,10,-19, 19,
22,10, 20,-20,
24,10, 21, 21,
26,10,-22,-22,
255
};

const u8* const levelsData[LEVELS_ALL]={
levelData1,
levelData2,
levelData3,
levelData4,
levelData5,
levelData6,
levelData7,
levelData8,
levelData9,
levelData10
};



void put_char(u8 x,u8 y,u8 n)
{
	n=n-32;
	n=(n&15)+((n>>4)<<5);

	select_image(IMG_FONT816);
	draw_tile(x,y  ,n   );
	draw_tile(x,y+1,n+16);
}



void put_str(u8 x,u8 y,u8* str)
{
	static u8 i;

	while(1)
	{
		i=*str++;
		if(!i) return;
		put_char(x,y,i);
		++x;
	}
}



void put_num(u8 x,u8 y,u16 num,u8 figs)
{
	x+=figs;

	while(figs)
	{
		put_char(x,y,'0'+(num%10));
		num/=10;
		--x;
		--figs;
	}
}



void put_char_buf(u16* buf,u8 wdt,u8 n)
{
	n=n-32;
	n=(n&15)+((n>>4)<<5);

	buf[0  ]=n;
	buf[wdt]=n+16;
}



void put_str_buf(u16* buf,u8 wdt,u8* str)
{
	static u8 i;

	while(1)
	{
		i=*str++;
		if(!i) return;
		put_char_buf(buf,wdt,i);
		*buf++;
	}
}



void put_num_buf(u16* buf,u8 wdt,u16 num,u8 figs)
{
	while(figs)
	{
		put_char_buf(buf+figs,wdt,'0'+(num%10));
		num/=10;
		--figs;
	}
}



void put_large_str_buf(u16* buf,u8 wdt,const u8* str)
{
	static u8 i;
	static u16 off,soff,doff;

	off=0;

	while(1)
	{
		i=*str++;

		if(!i) break;

		if(i<'0'||i>'_')
		{
			off+=2;
			continue;
		}

		i-='0';
		soff=((i&15)*3)+((i>>4)*192);
		doff=off;

		for(i=0;i<4;++i)
		{
			buf[doff+0]=soff+0;
			buf[doff+1]=soff+1;
			buf[doff+2]=soff+2;
			soff+=48;
			doff+=wdt;
		}

		off+=3;
	}
}



void put_large_str(const u8* str)
{
	static u8 i,j;
	static u16 off;
	static u16 buf[26*4];

	memset(buf,0xff,sizeof(buf));
	put_large_str_buf(buf,26,str);

	off=0;

	for(i=0;i<4;++i)
	{
		for(j=0;j<26;++j)
		{
			if(buf[off]!=0xffff)
			{
				select_image(IMG_TEXTBACK1);
				draw_tile(7+j,9+i,(i<<2)+(j&3));
				select_image(IMG_FONT2432);
				draw_tile_key(7+j,9+i,buf[off]);
			}

			++off;
		}
	}
}



void fade_to_black(void)
{
	static u8 i,j;

	for(i=1;i<3;++i)
	{
		pal_bright(BRIGHT_MID-i);
		delay(5);
	}
}



void player_init(u8 x,u8 y)
{
	player.x=x;
	player.y=y;
	player.prevx=x;
	player.prevy=y;
	player.startx=x;
	player.starty=y;
	player.dir=0;
	player.prevdir=0;
	player.off=0;
	player.draw=0;
	player.fill=0;

	gameRestartDelay=100;

	memset(fillmap,0,sizeof(fillmap));
}



void draw_map_part(u8 x,u8 y,u8 w,u8 h,u8 light)
{
	static u8 i,j,mask;
	static u16 off,soff1,soff2;

	soff1=y*MAP_WDT+x;
	soff2=soff1;

	select_image(gameImageID);

	++x;
	y+=2;
	w+=x;
	h+=y;

	for(i=y;i<h;++i)
	{
		off=soff1;
		soff1+=MAP_WDT;

		for(j=x;j<w;++j)
		{
			if(updmap[off])
			{
				if(map[off]) draw_tile(j,i,off);
			}

			++off;
		}
	}

	select_image(IMG_BGMASK);

	for(i=y;i<h;++i)
	{
		off=soff2;
		soff2+=MAP_WDT;

		for(j=x;j<w;++j)
		{
			if(updmap[off])
			{
				updmap[off]=0;

				if(!map[off])
				{
					draw_tile(j,i,257);
				}
				else
				{
					mask=0;

					if(i>2)
					{
						if(!map[off-MAP_WDT  ]) mask|=1;
						if(!map[off-MAP_WDT-1]) mask|=16;
						if(!map[off-MAP_WDT+1]) mask|=32;
					}
					if(i<1+MAP_HGT-1)
					{
						if(!map[off+MAP_WDT  ]) mask|=2;
						if(!map[off+MAP_WDT-1]) mask|=64;
						if(!map[off+MAP_WDT+1]) mask|=128;
					}
					if(j>1)
					{
						if(!map[off-1]) mask|=4;
					}
					if(j<MAP_WDT-1)
					{
						if(!map[off+1]) mask|=8;
					}

					if(light) draw_tile_key(j,i,256);
					draw_tile_key(j,i,mask);
				}
			}

			++off;
		}
	}
}



void draw_map(void)
{
	draw_map_part(0,0,MAP_WDT,MAP_HGT,FALSE);
}



void draw_map_tile(u8 x,u8 y,u8 light)
{
	static u16 off;

	off=y*MAP_WDT+x;
	updmap[off]=1;

	draw_map_part(x,y,1,1,light);
}



void update_stats_score(void)
{
	put_num(6,0,gameScore,5);
}



void update_stats_done(void)
{
	static u32 done;

	done=(u32)gameAreaClear*100/((MAP_HGT-2)*(MAP_WDT-2));
	gameDone=(u8)done;

	put_num(26,0,gameDone,3);
}



void update_stats_power(void)
{
	put_num(35,0,gamePower,3);
}



void update_stats_all(void)
{
	put_str(1,0,"SCORE:      LEVEL:   DONE:   % POW:");
	update_stats_score();
	put_num(18,0,gameLevel+1,2);
	update_stats_done();
	update_stats_power();
}



void player_screen_coords(void)
{
	u8 x,y;

	x=4-2+(player.x<<2);
	y=(player.y<<3)+8+4;

	switch(player.dir)
	{
	case JOY_LEFT:  x+=(player.off>>1); break;
	case JOY_RIGHT: x-=(player.off>>1); break;
	case JOY_UP:    y+= player.off;     break;
	case JOY_DOWN:  y-= player.off;     break;
	}

	player.sx=x;
	player.sy=y;
}



void player_hit(i8 objid)
{
	static u16 off;

	if(objid>=0) objList[objid].hit=25;

	player_screen_coords();

	explodeX=player.sx;
	explodeY=player.sy;
	explodeCnt=8<<2;

	gameHitCnt=10;

	for(off=MAP_WDT+1;off<MAP_WDT*MAP_HGT-MAP_WDT;++off)
	{
		if(map[off]==2)
		{
			map[off]=0;
			updmap[off]=1;
		}
	}

	draw_map();

	gamePower-=10;

	if(gamePower<=0)
	{
		gameDone=255;
		gameStartDelay=100;
		gameRestartDelay=100;
		gamePower=0;
	}
	else
	{
		player_init(player.startx,player.starty);
	}

	update_stats_power();
	sfx_play(SFX_EXPLODE,8);
}



u8 player_set_move(u8 dir)
{
	static u16 off;
	static u8 x,y;

	if(!player.draw)
	{
		player.startx=player.x;
		player.starty=player.y;
	}
	else
	{
		if(player.prevdir==JOY_LEFT &&dir==JOY_RIGHT) return 0;
		if(player.prevdir==JOY_RIGHT&&dir==JOY_LEFT ) return 0;
		if(player.prevdir==JOY_UP   &&dir==JOY_DOWN ) return 0;
		if(player.prevdir==JOY_DOWN &&dir==JOY_UP   ) return 0;
	}

	switch(dir)
	{
	case JOY_LEFT:  --player.x; break;
	case JOY_RIGHT: ++player.x; break;
	case JOY_UP:    --player.y; break;
	case JOY_DOWN:  ++player.y; break;
	}

	off=player.y*MAP_WDT+player.x;

	if(!map[off])
	{
		++player.draw;
		sfx_play(SFX_CUT,0);
	}

	if(player.draw)
	{
		if(map[off]==1)//заливка по окончании движения
		{
			player.draw=0;
			player.fill=TRUE;
		}
		else
		{
			if(map[off]==2)//самоликвидация
			{
				player_hit(-1);
				return 0;
			}
			else
			{
				switch(dir)//помечаем стороны линии для заливки
				{
				case JOY_LEFT:
					MARK_SIDE(off-MAP_WDT  ,2);
					MARK_SIDE(off-MAP_WDT+1,2);
					MARK_SIDE(off+MAP_WDT  ,1);
					MARK_SIDE(off+MAP_WDT+1,1);
					break;

				case JOY_RIGHT:
					MARK_SIDE(off-MAP_WDT  ,1);
					MARK_SIDE(off-MAP_WDT-1,1);
					MARK_SIDE(off+MAP_WDT  ,2);
					MARK_SIDE(off+MAP_WDT-1,2);
					break;

				case JOY_UP:
					MARK_SIDE(off-1        ,1);
					MARK_SIDE(off-1+MAP_WDT,1);
					MARK_SIDE(off+1        ,2);
					MARK_SIDE(off+1+MAP_WDT,2);
					break;

				case JOY_DOWN:
					MARK_SIDE(off-1        ,2);
					MARK_SIDE(off-1-MAP_WDT,2);
					MARK_SIDE(off+1        ,1);
					MARK_SIDE(off+1-MAP_WDT,1);
					break;
				}

				map    [off]=2;//рисуемая линия
				fillmap[off]=0;//не проверять эти координаты при заливке
				updmap [off]=1;//обновить тайл
			}
		}
	}

	player.dir=dir;
	player.off=8;

	return 0;
}



u16 area_fill(u16 off,u8 area)
{
	static u16 icnt,tcnt,len,max;
	static u16 front1[128],front2[128];
	static u16 *src,*dst;

	map[off]=area;
	front1[0]=off;
	src=front1;
	dst=front2;
	tcnt=0;
	len=1;

	while(1)
	{
		icnt=0;

		while(len)
		{
			off=*src++-1;
			--len;

			if(!map[off]) { map[off]=area; dst[icnt++]=off; }
			off+=2;
			if(!map[off]) { map[off]=area; dst[icnt++]=off; }
			off-=MAP_WDT+1;
			if(!map[off]) { map[off]=area; dst[icnt++]=off; }
			off+=MAP_WDT*2;
			if(!map[off]) { map[off]=area; dst[icnt++]=off; }
		}

		if(!icnt) break;

		len=icnt;
		tcnt+=icnt;

		if(dst==front2)
		{
			src=front2;
			dst=front1;
		}
		else
		{
			src=front1;
			dst=front2;
		}
	}

	return tcnt;
}



u16 map_fill(void)
{
	static u16 off,size,x,y,area_size[2];
	static u8 i,side,t1,t2,t3,t4;

	//заливаем области для каждой из сторон линии и считаем их площадь

	area_size[0]=0;
	area_size[1]=0;

	for(off=MAP_WDT+1;off<MAP_WDT*MAP_HGT-MAP_WDT;++off)
	{
		if(fillmap[off])
		{
			side=fillmap[off];
			fillmap[off]=0;
			area_size[side-1]+=area_fill(off,side+2);
		}
	}

	//проверяем попадание объектов в области и сбрасываем размер если попали

	for(i=0;i<objCount;++i)
	{
		x=objList[i].x>>FP;
		y=objList[i].y>>FP;

		t1=TEST_MAP(x+4,y-4);
		t2=TEST_MAP(x-4,y-4);
		t3=TEST_MAP(x+4,y+4);
		t4=TEST_MAP(x-4,y+4);

		if(t1==3||t2==3||t3==3||t4==3) area_size[1]=0;
		if(t1==4||t2==4||t3==4||t4==4) area_size[0]=0;
	}

	//выбираем наименьшую область
	//но если обе области заняты, то сбрасываем заливку

	side=area_size[0]<area_size[1]?3:4;
	if(!area_size[0]&&!area_size[1]) side=5;

	//заменяем залитую область пустотой если не залита
	//заодно считаем размер линии

	size=0;

	for(off=MAP_WDT+1;off<MAP_WDT*MAP_HGT-MAP_WDT;++off)
	{
		i=map[off];

		if(i<2) continue;

		if(i==2||i==side)
		{
			++size;
			updmap[off]=1;
			map[off]=1;
		}
		else
		{
			map[off]=0;
		}
	}

	if(size) sfx_play(SFX_SCORE,0);

	return size;
}



void level_screen(void)
{
	static u8 str[]="LEVEL   ";
	static u16 buf[40*7];
	static u8 i,j,x,y,spr;
	static i16 xoff,off;
	const i16 delay=240;

	pal_bright(0);
	pal_select(PAL_TEXTBACK1);

	clear_screen(0);
	swap_screen();

	memset(buf,0xff,sizeof(buf));

	i=gameLevel+1;

	if(i>=10)
	{
		str[6]='0'+i/10;
		str[7]='0'+i%10;
		off=9;
	}
	else
	{
		str[6]='0'+i%10;
		str[7]=' ';
		off=11;
	}

	put_large_str_buf(buf+off,40,str);

	put_str_buf(buf+5*40+15,40,"TARGET    %");
	put_num_buf(buf+5*40+21,40,gameClearTarget,3);

	pal_bright(BRIGHT_MID);

	xoff=0;

	sprites_start();

	music_play(MUS_LEVEL);

	while(xoff<380)
	{
		off=xoff>>2;

		if(!(xoff&3)&&off<40)
		{
			x=off;
			y=10;

			for(i=0;i<7;++i)
			{
				if(i!=4&&buf[off]!=0xffff)
				{
					select_image(IMG_TEXTBACK1);
					draw_tile(x,y,(i<<2)+(off&3));
					select_image(i<5?IMG_FONT2432:IMG_FONT816);
					draw_tile_key(x,y,buf[off]);
				}

				off+=40;
				++y;
			}
		}

		off=(xoff-delay-28)>>2;

		if(!(xoff&3)&&off>=0&&off<40)
		{
			y=10;

			for(i=0;i<7;++i)
			{
				if(i!=4) draw_tile(off,y,47);
				++y;
			}
		}

		y=10*8;
		spr=0;

		for(i=0;i<3;++i)
		{
			off=xoff-28;

			for(j=0;j<4;++j)
			{
				if(off>=0&&off<160-8)
				{
					set_sprite(spr,off,y,SPR_TITLEMASK+8+j);
					++spr;
				}

				if((off-delay)>=0&&(off-delay)<160-8)
				{
					set_sprite(spr,off-delay,y,SPR_TITLEMASK+12+j);
					++spr;
				}

				off+=8;
			}

			y+=i<1?16:24;
		}

		xoff+=2;

		swap_screen();
	}

	music_stop();
	sprites_stop();
}



u8 game_loop(void)
{
	static u8 i,j,pp,spr,fill,frame,done,bright,t1,t2,t3,t4,pause;
	static u16 x,y,off,score;
	static i16 dx,dy;
	static u8* levelData;

	levelData=levelsData[gameLevel];
	gameClearTarget=levelData[3];
	gameImageID=levelData[0];

	level_screen();

	pal_bright(0);
	pal_select(levelData[1]);

	memset(map,0,sizeof(map));

	for(i=0;i<MAP_WDT;++i)
	{
		map[i]=1;
		map[i+(MAP_HGT-1)*MAP_WDT]=1;
	}

	off=MAP_WDT;

	for(i=0;i<MAP_HGT-2;++i)
	{
		map[off]=1;
		map[off+MAP_WDT-1]=1;
		off+=MAP_WDT;
	}

	memcpy(updmap,map,sizeof(updmap));

	player_init(MAP_WDT>>1,0);

	gameDone=0;
	gameAreaClear=0;
	gameHitCnt=0;
	gameStartDelay=100;

	objCount=0;
	pp=4;

	while(objCount<OBJ_MAX)
	{
		if(levelData[pp]==255) break;

		objList[objCount].x=levelData[pp+0]<<(FP+3);
		objList[objCount].y=levelData[pp+1]<<(FP+3);
		objList[objCount].dx=((i8)levelData[pp+2])<<(FP-4);
		objList[objCount].dy=((i8)levelData[pp+3])<<(FP-4);
		objList[objCount].hit=0;

		++objCount;
		pp+=4;
	}

	clear_screen(0);
	draw_map();
	update_stats_all();
	swap_screen();

	fill=FALSE;
	bright=0;
	frame=0;
	pause=FALSE;
	explodeCnt=0;

	sprites_start();

	music_play(levelData[2]);

	while(1)
	{
		if(gameHitCnt)
		{
			pal_bright(gameHitCnt&2?BRIGHT_MID:BRIGHT_MAX);
		}
		else
		{
			pal_bright(bright);

			if(gameStartDelay)
			{
				if(!(frame&3)) if(bright<BRIGHT_MID) ++bright;
			}
		}

		++frame;

		keyboard(keys);

		if(!gameStartDelay&&!gameRestartDelay&&(keys[KEY_H]&KEY_PRESS))
		{
			pause^=TRUE;
			bright=pause?2:3;
		}

		if(pause) continue;

		if(!gameStartDelay&&!gameRestartDelay&&gameDone<gameClearTarget)
		{
			i=joystick();

			if(keys[KEY_Q]) i|=JOY_UP;
			if(keys[KEY_A]) i|=JOY_DOWN;
			if(keys[KEY_O]) i|=JOY_LEFT;
			if(keys[KEY_P]) i|=JOY_RIGHT;

			if(!player.off)
			{
				if(i&JOY_LEFT &&player.x>0        ) i=player_set_move(JOY_LEFT);
				if(i&JOY_RIGHT&&player.x<MAP_WDT-1) i=player_set_move(JOY_RIGHT);
				if(i&JOY_UP   &&player.y>0        ) i=player_set_move(JOY_UP);
				if(i&JOY_DOWN &&player.y<MAP_HGT-1) i=player_set_move(JOY_DOWN);

				if(!i&&!player.off&&player.draw) player_set_move(player.prevdir);
			}
			else
			{
				player.off-=2;

				if(!player.off)
				{
					if(player.fill)
					{
						player.fill=FALSE;
						player.draw=0;
						fill=TRUE;
					}

					if(player.draw>1) draw_map_tile(player.prevx,player.prevy,TRUE);
					if(player.draw>0) draw_map_tile(player.x,player.y,TRUE);

					player.prevx=player.x;
					player.prevy=player.y;
					player.prevdir=player.dir;
					player.dir=0;
				}
			}
		}

		spr=0;

		for(i=0;i<objCount;++i)
		{
			j=SPR_SPIKEBALL+(((frame)>>2)&7);

			if(objList[i].hit)
			{
				if(objList[i].hit&1) j+=8;
				--objList[i].hit;
			}

			set_sprite(spr,4+(objList[i].x>>(FP+1))-4,16+(objList[i].y>>FP)-8,j);

			if(gameDone==255)
			{
				++spr;
				continue;
			}

			x=(objList[i].x+objList[i].dx)>>FP;
			y= objList[i].y>>FP;

			t1=TEST_MAP(x-3,y-3);
			t2=TEST_MAP(x+3,y-3);
			t3=TEST_MAP(x-3,y+3);
			t4=TEST_MAP(x+3,y+3);

			if(t1||t2||t3||t4)
			{
				objList[i].dx=-objList[i].dx;
				dx=0;
			}
			else
			{
				dx=objList[i].dx;
			}

			if(t1==2||t2==2||t3==2||t4==2) player_hit(i);

			x= objList[i].x>>FP;
			y=(objList[i].y+objList[i].dy)>>FP;

			t1=TEST_MAP(x-3,y-3);
			t2=TEST_MAP(x+3,y-3);
			t3=TEST_MAP(x-3,y+3);
			t4=TEST_MAP(x+3,y+3);

			if(t1||t2||t3||t4)
			{
				objList[i].dy=-objList[i].dy;
				dy=0;
			}
			else
			{
				dy=objList[i].dy;
			}

			if(t1==2||t2==2||t3==2||t4==2) player_hit(i);

			if(!gameStartDelay&&!objList[i].hit)
			{
				objList[i].x+=dx;
				objList[i].y+=dy;
			}

			++spr;
		}

		player_screen_coords();

		if(gameRestartDelay)
		{
			i=(gameRestartDelay&16?10:0);
		}
		else
		{
			i=!player.draw?0:((frame>>1)&1);
		}

		if(explodeCnt)
		{
			set_sprite(spr,explodeX,explodeY,9-(explodeCnt>>2));
		}
		else
		{
			set_sprite(spr,player.sx,player.sy,gameDone!=255?SPR_PLAYER+i:SPRITE_END);
		}

		if(!explodeCnt&&!gameRestartDelay&&gameDone>=gameClearTarget) break;

		swap_screen();

		if(fill)
		{
			fill=FALSE;
			score=map_fill();
			gameAreaClear+=score;
			gameScore+=score;
			gamePower+=score/10;
			if(gamePower>100) gamePower=100;
			update_stats_done();
			update_stats_score();
			update_stats_power();
			draw_map();
		}

		if(gameStartDelay) --gameStartDelay;
		if(gameRestartDelay) --gameRestartDelay;
		if(gameHitCnt) --gameHitCnt;
		if(explodeCnt) --explodeCnt;
	}

	swap_screen();
	sprites_stop();
	music_stop();

	if(gameDone<=100)
	{
		memset(map,1,sizeof(map));
		memset(updmap,1,sizeof(updmap));

		draw_image(1,2,gameImageID);

		for(i=0;i<15;++i)
		{
			swap_screen();
			vsync();
		}

		sample_play(SMP_MEOW);
	}

	delay(50);

	fade_to_black();

	return gameDone<=100?TRUE:FALSE;
}



void title_screen(void)
{
	const u8 pressStartStr[]="PRESS SPACE TO START";
	static u8 i,j,spr,xoff,dx,done,frame;
	static u16 off,pp;
	static i16 x,y;

	pal_bright(BRIGHT_MIN);
	pal_select(PAL_TITLE);
	clear_screen(0);
	swap_screen();
	pal_bright(BRIGHT_MID);

	select_image(IMG_TITLE);

	off=0;

	sprites_start();

	music_play(MUS_INTRO);

	while(off<18*4)
	{
		pp=17;
		y=4;
		xoff=off>>2;

		if((off&3)==0)
		{
			for(i=0;i<10;++i)
			{
				dx=i<5?i:9-i;
				x=19-xoff+dx;
				if(x>=3&&x<=19) draw_tile(x,y,pp-xoff+dx);
				pp+=36;
				++y;
			}
		}

		if((off&3)==2)
		{
			for(i=0;i<10;++i)
			{
				dx=i<5?i:9-i;
				x=20+xoff-dx;
				if(x<37&&x>=20) draw_tile(x,y,pp+xoff-dx+1);
				pp+=36;
				++y;
			}
		}

		spr=0;
		y=3*8;

		for(i=0;i<3;++i)
		{
			dx=i<<3;

			x=19*4-off+dx;
			if(x<8) x=8;

			set_sprite(spr+0,x  ,y,SPR_TITLEMASK+2+0);
			set_sprite(spr+1,x-8,y,SPR_TITLEMASK+2+4);

			x=19*4-2+off-dx;
			if(x>36*4) x=36*4;

			set_sprite(spr+2,x  ,y,SPR_TITLEMASK+2+1);
			set_sprite(spr+3,x+8,y,SPR_TITLEMASK+2+5);

			spr+=4;
			y+=16;
		}

		for(i=3;i<6;++i)
		{
			dx=(5-i)<<3;

			x=19*4-off+dx;
			if(x<8) x=8;

			set_sprite(spr+0,x  ,y,SPR_TITLEMASK+0);
			set_sprite(spr+1,x-8,y,SPR_TITLEMASK+4);

			x=19*4-2+off-dx;
			if(x>36*4) x=36*4;

			set_sprite(spr+2,x  ,y,SPR_TITLEMASK+1);
			set_sprite(spr+3,x+8,y,SPR_TITLEMASK+5);

			spr+=4;
			y+=16;
		}

		++off;

		swap_screen();
	}

	sprites_stop();

	for(i=BRIGHT_MID;i<=BRIGHT_MAX;++i)
	{
		pal_bright(i);
		delay(3);
	}

	put_str(10,18,pressStartStr);
	put_str(14,22,"pd$o#$ shiru");

	swap_screen();

	for(i=BRIGHT_MAX;i>=BRIGHT_MID;--i)
	{
		pal_bright(i);
		delay(5);
	}

	sprites_start();

	frame=0;
	done=FALSE;

	while(!done)
	{
		if(frame&16)
		{
			put_str(10,18,pressStartStr);
		}
		else
		{
			for(i=0;i<20;++i) put_char(10+i,18,' ');
		}

		swap_screen();

		if(joystick()&JOY_FIRE) done=TRUE;

		++frame;
	}

	sprites_stop();
	put_str(10,18,pressStartStr);
	swap_screen();

	music_stop();
	sample_play(SMP_START);

	fade_to_black();
}



void gameover_screen(void)
{
	static u8 i;

	pal_bright(BRIGHT_MIN);
	pal_select(PAL_TEXTBACK2);

	clear_screen(0);

	put_large_str("GAME OVER");
	put_str(11,15,"TOTAL SCORE:");
	put_num(23,15,gameScore,5);

	swap_screen();

	music_play(MUS_GAMEOVER);

	for(i=BRIGHT_MIN;i<=BRIGHT_MID;++i)
	{
		pal_bright(i);
		delay(8);
	}

	while(1)
	{
		vsync();

		if(joystick()&JOY_FIRE) break;
	}

	music_stop();
	fade_to_black();
}



void welldone_screen(void)
{
	static u8 i;

	pal_bright(BRIGHT_MIN);
	pal_select(PAL_TEXTBACK3);

	clear_screen(0);

	put_large_str("WELL DONE");
	put_str(11,15,"ALL LEVELS CLEAR !");
	put_str(11,18,"TOTAL SCORE:");
	put_num(23,18,gameScore,5);

	swap_screen();

	music_play(MUS_WELLDONE);

	for(i=BRIGHT_MIN;i<=BRIGHT_MID;++i)
	{
		pal_bright(i);
		delay(8);
	}

	while(1)
	{
		vsync();

		if(joystick()&JOY_FIRE) break;
	}

	music_stop();
	fade_to_black();
}



void main(void)
{
	color_key(1);

	while(1)
	{
		title_screen();

		gameScore=0;
		gameLevel=0;
		gamePower=0;

		while(1)
		{
			if(game_loop())
			{
				++gameLevel;

				if(gameLevel==LEVELS_ALL)
				{
					welldone_screen();
					break;
				}
			}
			else
			{
				gameover_screen();
				break;
			}
		}
	}
}