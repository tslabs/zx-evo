#include "std.h"
#include "emul.h"
#include "vars.h"
#include "memory.h"
#include "draw.h"

void atm_memswap()
{
   if (!conf.atm.mem_swap) return;
   // swap memory address bits A5-A7 and A8-A10
   for (unsigned start_page = 0; start_page < conf.ramsize*1024; start_page += 2048) {
      unsigned char buffer[2048], *bank = memory + start_page;
      for (unsigned addr = 0; addr < 2048; addr++)
         buffer[addr] = bank[(addr & 0x1F) + ((addr >> 3) & 0xE0) + ((addr << 3) & 0x700)];
      memcpy(bank, buffer, 2048);
   }
}

void AtmApplySideEffectsWhenChangeVideomode(unsigned char val)
{
    int NewVideoMode = (val & 7);
    int OldVideoMode = (comp.pFF77 & 7);

    // Константы можно задать жёстко, потому что между моделями АТМ2 они не меняются.
    const int tScanlineWidth = 224;
    const int tScreenWidth = 320/2;
    const int tEachBorderWidth = (tScanlineWidth - tScreenWidth)/2;
    const int iLinesAboveBorder = 56;

    int iRayLine = cpu.t / tScanlineWidth;
    int iRayOffset = cpu.t % tScanlineWidth;
/*    
    static int iLastLine = 0;
    if ( iLastLine > iRayLine )
    {
        printf("\nNew Frame Begin\n");
        __debugbreak();
    }
    iLastLine = iRayLine;
    printf("%d->%d %d %d\n", OldVideoMode, NewVideoMode, iRayLine, iRayOffset);
*/

    if (OldVideoMode == 3 || NewVideoMode == 3)
    {
        // Переключение из/в sinclair режим не имеет полезных побочных эффектов
        // (по словам AlCo даже синхра сбивается)
        for (unsigned y = 0; y < 200; y++)
        {
            AtmVideoCtrl.Scanlines[y+56].VideoMode = NewVideoMode;
            AtmVideoCtrl.Scanlines[y+56].Offset = ((y & ~7) << 3) + 0x01C0;
        }
        return;
    }

    if (OldVideoMode != 6 && NewVideoMode != 6)
    {
        // Нас интересуют только переключения между текстовым режимом и расширенными графическими.
        // Текстового режима нет ни до, ни после переключения. 
        // Следовательно, нет и побочных эффектов.
        
        // Распространяем новый видеорежим на неотрисованные сканлинии
        if (iRayOffset >= tEachBorderWidth)
            ++iRayLine;

        while (iRayLine < 256)
        {
            AtmVideoCtrl.Scanlines[iRayLine++].VideoMode = NewVideoMode;
        }
//        printf("%d->%d SKIPPED!\n",  OldVideoMode, NewVideoMode);
        return;
    }

    //
    // Терминология и константы:
    // Экран содержит 312 сканлиний, по 224такта (noturbo) в каждой.
    //
    //  "строка" - младшие 3 бита номера отрисовываемой видеоконтроллером (лучом) сканлинии
    //  "адрес"  - текущий адрес в видеопамяти, с которого выбирает байты видеоконтроллер
    //             адрес меняется с гранулярностью +8 или +64
    //  "экран"  - растровая картинка. Со всех сторон окружена "бордюром".
    //             Размеры бордюра: 
    //          сверху и снизу от растра: 56 сканлиний
    //              слева и справа от растра: 32 такта (64пикселя).
    // 
    // +64 происходит, когда CROW 1->0, то есть:
    //  либо в текстмоде при переходе со строки 7 на строку 0,
    //  либо при переключении текстмод->графика при адресе A5=0,
    //  либо при переключении графика->текстмод при адресе A5=1 и строке 0..3
    //
    // +8 происходит на растре в конце 64-блока пикселей (каждые 32такта) независимо от режима
    // (ВАЖНО: +8 не завраплены внутри A3..A5 а производят честное сложение с адресом.)
    //
    // сброс A3..A5 (накопленных +8) происходит, когда RCOL 1->0, то есть:
    //  либо в текстмоде при переходе с растра на бордюр,
    //  либо на бордюре при переключении графика->текстмод
    //


    if (iRayLine >= 256)
    {
        return;
    }

    // Получим оффсет текущей сканлинии (фактически видеоадрес её начала)
    int Offset = AtmVideoCtrl.Scanlines[iRayLine].Offset;

    // Вычислим реальный видеоадрес, с учётом инкрементов при отрисовке растра.
    // Также определим, куда применяем +64 инкременты при переключении видеорежима:
    //  - Если луч на бордюре сверху от растра или на бордюре слева от растра - изменяем оффсет для текущей сканлинии
    //  - Если луч на растре или на бордюре справа от растра - изменяем оффсет для следующей сканлинии
    bool bRayOnBorder = true;
    if ( iRayLine < iLinesAboveBorder || iRayOffset < tEachBorderWidth )
    {
        // Луч на бордюре. Либо сверху от растра, либо слева от растра.
        // Все изменения применяем к текущей сканлинии.

        // Обработаем переключение видеорежима.
        if ( NewVideoMode == 6 )
        {
            // Переключение В текстовый режим.
            if ( (Offset & 32) //< проверка условия "при адресе A5=1"
                 && (iRayLine & 7) < 4 //< проверка условия "в строке 0..3"
               )
            {
//                printf("CASE-1: 0x%.4x Incremented (+64) on line %d\n", Offset, iRayLine);
                Offset += 64;
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
            }

            AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;

            // После прохода всех точек растра текущей сканлинии в текстовом режиме будут сброшены A3..A5
            Offset &= (~0x38); // Сброс A3..A5
//            printf("CASE-1a, reset A3..A5: 0x%.4x\n", Offset);
            
            // Рассчёт видеоадресов для нижлежащих сканлиний
            while (++iRayLine < 256)
            {
                if ( 0 == (iRayLine & 7))
                {
                    Offset += 64;
                }
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
                AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;
            }
        } else {
            // Переключение ИЗ текстового режима.
            if ( 0 == (Offset & 32) ) //< проверка условия "при адресе A5=0"
            {
//                printf("CASE-2: 0x%.4x Incremented (+64) on line %d\n", Offset, iRayLine);
                Offset += 64;
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
            }
            AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;

            // Рассчёт видеоадресов для нижлежащих сканлиний
            while (++iRayLine < 256)
            {
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
                AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;
            }
        }
    } else {
        // Луч рисует растр, либо бордюр справа от растра.

        // Вычисляем текущее значение видеоадреса

        // Прибавляем к видеоадресу все +64 инкременты, 
        // сделанные в ходе отрисовки растра данной сканлинии
        if (iRayLine == AtmVideoCtrl.CurrentRayLine)
        {
            Offset += AtmVideoCtrl.IncCounter_InRaster;
        } else {
            // Счётчик инкрементов устарел (т.к. накручен на сканлинию отличную от текущей)
            // Инициализируем его для текущей сканлинии.
            AtmVideoCtrl.CurrentRayLine = iRayLine;
            AtmVideoCtrl.IncCounter_InRaster = 0;
            AtmVideoCtrl.IncCounter_InBorder = 0;
        }
        
        // Прибавляем к видеоадресу все +8 инкременты, произошедшие при отрисовке растра
        bool bRayInRaster = iRayOffset < (tScreenWidth + tEachBorderWidth);
        
        int iScanlineRemainder = 0; //< Сколько +8 инкрементов ещё будет сделано до конца сканлинии 
                                    //  (т.е. уже после переключения видеорежима)
        if ( bRayInRaster )
        {
            // Луч рисует растр. 
            // Прибавляем к текущему видеоадресу столько +8, 
            // сколько было полностью отрисованных 64пиксельных блока.
            int iIncValue = 8 * ((iRayOffset-tEachBorderWidth)/32);
            iScanlineRemainder = 40 - iIncValue;
//            printf("CASE-4: 0x%.4x Incremented (+%d) on line %d\n", Offset, iIncValue, iRayLine);
            Offset += iIncValue;
        } else {
            // Отрисовка растра лучом завершена.
            // Т.е. все 5-ять 64-пиксельных блока были пройдены. Прибавляем к адресу +40.
//            printf("CASE-5: 0x%.4x Incremented (+40) on line %d\n", Offset, iRayLine);
            Offset += 40;

            // Если предыдущим режимом был текстовый режим,
            // То при переходе с растра на бордюр должны быть сброшены A3..A5
            if (OldVideoMode == 6)
            {
                Offset &= (~0x38); // Сброс A3..A5
//                printf("CASE-5a, reset A3..A5: 0x%.4x\n", Offset);
            }
        }

        // Прибавляем к видеоадресу все +64 инкременты, 
        // сделанные в ходе отрисовки бордюра за растром данной сканлинии
        Offset += AtmVideoCtrl.IncCounter_InBorder;

        // Текущее значение видеоадреса вычислено. 
        // Обрабатываем переключение видеорежима.
        int OffsetInc = 0;
        if ( NewVideoMode == 6 )
        {
            // Переключение В текстовый режим.
            if ( (Offset & 32) //< проверка условия "при адресе A5=1"
                && (iRayLine & 7) < 4 //< проверка условия "в строке 0..3"
                )
            {
                OffsetInc = 64;
//                printf("CASE-6: 0x%.4x Incremented (+64) on line %d\n", Offset, iRayLine);
                Offset += OffsetInc;
            }
            // Рассчёт видеоадресов для нижлежащих сканлиний
            Offset += iScanlineRemainder;
            while (++iRayLine < 256)
            {
                if ( 0 == (iRayLine & 7))
                    Offset += 64;
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
                AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;
            }
        } else {
            // Переключение ИЗ текстового режима.
            if ( 0 == (Offset & 32) ) //< проверка условия "при адресе A5=0"
            {
                OffsetInc = 64;
//                printf("CASE-7: 0x%.4x Incremented (+64) on line %d\n", Offset, iRayLine);
                Offset += OffsetInc;
            }

            // Рассчёт видеоадресов для нижлежащих сканлиний
            Offset += iScanlineRemainder;
            while (++iRayLine < 256)
            {
                AtmVideoCtrl.Scanlines[iRayLine].Offset = Offset;
                AtmVideoCtrl.Scanlines[iRayLine].VideoMode = NewVideoMode;
                Offset += 40;
            }
        }

        // запоминаем сделанный инкремент на случай, 
        // если их будет несколько в ходе отрисовки текущей сканлинии.
        if ( bRayInRaster )
        {
            AtmVideoCtrl.IncCounter_InRaster += OffsetInc;
        } else {
            AtmVideoCtrl.IncCounter_InBorder += OffsetInc;
        }
    }
}

void set_turbo(void)
{
	if (comp.pFF77 & 8)
		turbo(3);	// 1x = 14MHz (actually effective 11MHz)
	else
	{
		if (comp.pEFF7 & 16)
			turbo(1);	// 01 = 3.5MHz
		else
			turbo(2);	// 00 = 7MHz
	}
}

void set_atm_FF77(unsigned port, unsigned char val)
{
   if ((comp.pFF77 ^ val) & 1)
       atm_memswap();
   

   if ((comp.pFF77 & 7) ^ (val & 7))
   {
        // Происходит переключение видеорежима
        AtmApplySideEffectsWhenChangeVideomode(val);
   }

   comp.pFF77 = val;
   comp.aFF77 = port;
   cpu.int_gate = ((comp.pFF77 & 0x20) != false) || (conf.mem_model==MM_ATM3); // lvd added no INT gate to pentevo (atm3)
   set_banks();
}

void set_atm_aFE(unsigned char addr)
{
   unsigned char old_aFE = comp.aFE;
   comp.aFE = addr;
   if ((addr ^ old_aFE) & 0x40) atm_memswap();
   if ((addr ^ old_aFE) & 0x80) set_banks();
}

static u8 atm_pal[0x10] = { 0 };

void atm_writepal(unsigned char val)
{
   // assert(comp.border_attr < 0x10); // commented (tsl)
   atm_pal[comp.ts.border & 0xF] = val;
   val ^= 0xFF;		// inverted RU2 value
   comp.cram[comp.ts.border] =
	   ((val & 0x02) << 13) |	// R (CLUT bit 14)
	   ((val & 0x40) <<  7) |	// r (CLUT bit 13)
	   ((val & 0x10) <<  5) |	// G (CLUT bit 9)
	   ((val & 0x80) <<  1) |	// g (CLUT bit 8)
	   ((val & 0x01) <<  4) |	// B (CLUT bit 4)
	   ((val & 0x20) >>  2);	// b (CLUT bit 3)
   update_clut(comp.ts.border);
   temp.comp_pal_changed = 1; // check it to remove!
}

u8 atm_readpal()
{
   return atm_pal[comp.ts.border & 0xF];
}

unsigned char atm450_z(unsigned t)
{
   // PAL hardware gives 3 zeros in secret short time intervals
   if (conf.frame < 80000) { // NORMAL SPEED mode
      if ((unsigned)(t-7200) < 40 || (unsigned)(t-7284) < 40 || (unsigned)(t-7326) < 40) return 0;
   } else { // TURBO mode
      if ((unsigned)(t-21514) < 40 || (unsigned)(t-21703) < 80 || (unsigned)(t-21808) < 40) return 0;
   }
   return 0x80;
}
