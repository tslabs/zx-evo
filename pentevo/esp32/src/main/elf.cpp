
#include <stdint.h>
#include <string.h>
#include <elf.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "main.h"
#include "esp_spi_defs.h"
#include "mem_obj.h"
#include "elf.cpp.h"

int load_elf(u8 *elf, MEM_OBJ *obj, int opt)
{
  enum
  {
    TYPE_NONE,
    TYPE_TEXT,
    TYPE_RODATA,
    TYPE_DATA,
    TYPE_BSS,
  };

  Elf32_Ehdr* elf_header = (Elf32_Ehdr*)elf;
  Elf32_Shdr* section_headers = (Elf32_Shdr*)(elf + elf_header->e_shoff);
  Elf32_Shdr* shstrtab_header = &section_headers[elf_header->e_shstrndx];
  const char* shstrtab = (const char*)(elf + shstrtab_header->sh_offset);
  u32 entry = 0;
  int size = 0;

  int num_sec = elf_header->e_shnum;

  Elf32_Rela* rld = NULL;
  int rld_num = 0;

  // Parse sections to determine type and find dynamic relocation section
  for (int i = 0; i < num_sec; i++)
  {
    Elf32_Shdr* section = &section_headers[i];

    if (!section->sh_addr) continue;

    section->sh_info = TYPE_NONE;
    section->sh_link = 0;

    switch (section->sh_type)
    {
      case SHT_RELA:
        section->sh_type = TYPE_NONE;
        rld = (Elf32_Rela*)&elf[section->sh_offset];
        rld_num = section->sh_size / sizeof(Elf32_Rela);
      break;

      case SHT_NOBITS:
        section->sh_type = TYPE_BSS;
      break;

      case SHT_PROGBITS:
        if (section->sh_flags & SHF_EXECINSTR)
        {
          section->sh_type = TYPE_TEXT;
          section->sh_link = 1;   // .text is always used if no symbol refere it
        }

        else if (section->sh_flags & SHF_WRITE)
          section->sh_type = TYPE_DATA;

        else
          section->sh_type = TYPE_RODATA;
      break;

      default:
        section->sh_type = TYPE_NONE;
    }
  }

  // Parse relocation symbols to find used sections
  for (int i = 0; i < rld_num; i++)
  {
    u32 rld_offs = rld[i].r_offset;
    u32 *sym_offs = (u32*)&elf[rld_offs];
    u32 sym_val = *sym_offs;

    for (int j = 0; j < num_sec; j++)
    {
      Elf32_Shdr* section = &section_headers[j];

      if (section->sh_addr)
      {
        if ((sym_val >= section->sh_addr) && (sym_val <= (section->sh_addr + section->sh_size - 1)))
        {
          section->sh_link++;
          break;
        }
      }
    }
  }

  // Allocate memory for sections
#ifdef VERBOSE
  printf("\nSections used:\n");
#endif

  for (int i = 0; i < num_sec; i++)
  {
    Elf32_Shdr* section = &section_headers[i];

    if (!section->sh_addr) continue;
    if (!section->sh_link) continue;

    section->sh_info = 0;

    switch (section->sh_type)
    {
      case TYPE_TEXT:
        section->sh_info = (u32)heap_caps_aligned_alloc(section->sh_addralign, section->sh_size, MALLOC_CAP_INTERNAL);
        if (!section->sh_info) goto cleanup;
        entry = elf_header->e_entry - section->sh_offset + section->sh_info + 0x6F0000;
        obj->text = (void*)section->sh_info;
        obj->entry = (void*)entry;
        size += section->sh_size;
      break;

      case TYPE_BSS:
        section->sh_info = (u32)heap_caps_aligned_alloc(section->sh_addralign, section->sh_size, (opt & ESP_OPT_BSS_SRAM) ? MALLOC_CAP_INTERNAL : MALLOC_CAP_SPIRAM);
        if (!section->sh_info) goto cleanup;
        obj->bss = (void*)section->sh_info;
        size += section->sh_size;
      break;

      case TYPE_DATA:
        section->sh_info = (u32)heap_caps_aligned_alloc(section->sh_addralign, section->sh_size, (opt & ESP_OPT_DATA_SRAM) ? MALLOC_CAP_INTERNAL : MALLOC_CAP_SPIRAM);
        if (!section->sh_info) goto cleanup;
        obj->data = (void*)section->sh_info;
        size += section->sh_size;
      break;

      case TYPE_RODATA:
        section->sh_info = (u32)heap_caps_aligned_alloc(section->sh_addralign, section->sh_size, (opt & ESP_OPT_RODATA_SRAM) ? MALLOC_CAP_INTERNAL : MALLOC_CAP_SPIRAM);
        if (!section->sh_info) goto cleanup;
        obj->rodata = (void*)section->sh_info;
        size += section->sh_size;
      break;
    }

#ifdef VERBOSE
    const char* section_name = shstrtab + section->sh_name;
    printf("%-10s  Addr: %06lX  Size: %-5ld  Symbols: %-3ld  Alloc: %lX  Align: %ld\n", section_name, section->sh_addr, section->sh_size, section->sh_link, section->sh_info, section->sh_addralign);
#endif
  }

  // Relocate dynamic symbols
#ifdef VERBOSE
  printf("\nRelocated symbols:\n");
#endif

  for (int i = 0; i < rld_num; i++)
  {
    u32 rld_offs = rld[i].r_offset;
    u32 *sym_offs = (u32*)&elf[rld_offs];
    u32 sym_val = *sym_offs;
    u32 sym_addr = 0;
    const char* name = "";

    for (int j = 0; j < num_sec; j++)
    {
      Elf32_Shdr* section = &section_headers[j];

      if (section->sh_addr)
      {
        if ((sym_val >= section->sh_addr) && (sym_val <= (section->sh_addr + section->sh_size - 1)))
        {
          sym_addr = sym_val - section->sh_offset + section->sh_info + ((section->sh_type == TYPE_TEXT) ? 0x6F0000 : 0);
          *sym_offs = sym_addr;
          name = shstrtab + section->sh_name;
          break;
        }
      }
    }

#ifdef VERBOSE
    printf("%-4d Offs: %-5lX Value: %-6lX Addr: %lX  Section: %s\n", i, rld_offs, sym_val, sym_addr, name);
#endif
  }

  // Initialize memory for sections
  for (int i = 0; i < num_sec; i++)
  {
    Elf32_Shdr* section = &section_headers[i];

    if (!section->sh_addr) continue;
    if (!section->sh_link) continue;

    switch (section->sh_type)
    {
      case TYPE_TEXT:
      case TYPE_DATA:
      case TYPE_RODATA:
        if (section->sh_info) memcpy((void*)section->sh_info, &elf[section->sh_offset], section->sh_size);
      break;

      case TYPE_BSS:
        if (section->sh_info) memset((void*)section->sh_info, 0, section->sh_size);
      break;
    }
  }

#ifdef VERBOSE
  printf("\nEntry: %lX\n", entry);
#endif

  return size;

cleanup:
  if (obj->text) free(obj->text);
  if (obj->rodata) free(obj->rodata);
  if (obj->data) free(obj->data);
  if (obj->bss) free(obj->bss);

  return -1;
}
