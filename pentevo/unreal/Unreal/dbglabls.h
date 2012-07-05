#pragma once
struct MON_LABEL
{
    unsigned char *address;
    unsigned name_offs;
};

struct MON_LABELS
{
   MON_LABEL *pairs;
   unsigned n_pairs;
   char *names;
   unsigned names_size;

   MON_LABELS() { pairs = 0, names = 0, n_pairs = names_size = 0; hNewUserLabels = 0; }
   ~MON_LABELS() { free(pairs), free(names); stop_watching_labels(); }

   unsigned add_name(char *name);
   void clear(unsigned char *start, unsigned size);
   void clear_ram() { clear(RAM_BASE_M, MAX_RAM_PAGES*PAGE); }
   void sort();

   char *find(unsigned char *address);
   void add(unsigned char *address, char *name);
   unsigned load(char *filename, unsigned char *base, unsigned size);


   char xas_errstr[80];
   unsigned char xaspage;
   void find_xas();

   enum { MAX_ALASM_LTABLES = 16 };
   char alasm_valid_char[0x100];
   unsigned alasm_found_tables;
   unsigned alasm_offset[MAX_ALASM_LTABLES];
   unsigned alasm_count[MAX_ALASM_LTABLES];
   unsigned alasm_chain_len(unsigned char *page, unsigned offset, unsigned &end);
   void find_alasm();

   void import_menu();
   void import_xas();
   void import_alasm(unsigned offset, char *caption);


   HANDLE hNewUserLabels;
   char userfile[0x200];
   void stop_watching_labels();
   void start_watching_labels();
   void notify_user_labels();
   void import_file();

};

extern MON_LABELS mon_labels;

void load_labels(char *filename, unsigned char *base, unsigned size);
void mon_show_labels();
void init_labels(char* filename);

