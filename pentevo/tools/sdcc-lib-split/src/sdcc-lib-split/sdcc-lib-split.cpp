
#include "stdafx.h"

char cc[65536];
char incl[65536];
char fn[256];

// int _tmain(int argc, _TCHAR* argv[])
int main(int argc, char* argv[])
{
  FILE *f_in = fopen(argv[1], "rb");

  bool is_sq = false;
  bool is_dq = false;
  bool is_ls = false;
  bool is_empty = true;
  bool req_fclose = false;
  bool req_fclose2 = false;
  bool req_incl = false;
  int cb = 0;
  bool is_fopen = false;
  int f_cnt = 0;
  incl[0] = 0;

  FILE *f_out;

  while (!feof(f_in))
  {
    char c;

    if (fread(&c, 1, 1, f_in))
    {
      switch (c)
      {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        break;

        default:
          is_empty = false;
      }

      switch (c)
      {
        case ';':
        break;
        
        default:
          req_fclose2 = req_fclose;
      }
      switch (c)
      {
        case '\\':
          is_ls = !is_ls;
        break;
      }

      if (!is_ls)
        switch (c)
        {
          case '#':
          {
            int ft = ftell(f_in);
            fscanf(f_in, "%256[^\r\n]s", cc);
            fseek(f_in, ft, SEEK_SET);
            if (!strncmp(cc, "include", 7))
              req_incl = true;
          }
          break;

          case '\'':
              is_sq = !is_sq;
          break;

          case '\"':
            is_dq = !is_dq;
          break;

          case '{':
            if (!is_sq && !is_dq)
              cb++;
          break;

          case '}':
            if (!is_sq && !is_dq)
            {
              cb--;
              if (cb == 0)
              {
                if (is_fopen)
                  req_fclose = true;
              }
            }
          break;
        }

      if (c != '\\')
        is_ls = false;

      if (!is_fopen && !is_empty)
      {
        // printf("\n--- open %d ---\n", f_cnt);
        // printf("%s", incl);
        char ff[256];
        char *fs = argv[1];
        for (int a = 0; a <= sizeof(ff); a++)
        {
          ff[a] = (fs[a] == '.') ? '_' : fs[a];
          if (!fs[a]) break;
        }
        sprintf(fn, "%s_%d.c", ff, f_cnt);
        f_out = fopen(fn, "wb");
        is_fopen = true;
        fprintf(f_out, "%s", incl);
      }

      if (req_incl)
      {
        req_incl = false;
        sprintf(&incl[strlen(incl)], "#%s\r\n", cc);
      }

      if (req_fclose2)
      {
        // printf("\n--- close %d ---\n", f_cnt);
        fclose(f_out);
        f_out = 0;
        req_fclose = false;
        req_fclose2 = false;
        is_fopen = false;
        is_empty = true;
        printf("%s\n", fn);
        f_cnt++;
      }

      if (is_fopen)
      {
        // printf("%c", c == '\r' ? '\n' : c);
        fprintf(f_out, "%c", c);
      }
    }
  }

  if (f_out)
    fclose(f_out);
  fclose(f_in);
  return 0;
}
