
#include <string.h>

const int8_t COBRA_vertices[] =
{
  15,0,37,
  -15,0,37,
  0,12,11,
  -59,-1,-3,
  59,-1,-3,
  -43,7,-19,
  43,7,-19,
  63,-3,-19,
  -63,-3,-19,
  0,12,-19,
  -15,-11,-19,
  15,-11,-19,
  -17,3,-19,
  -3,5,-19,
  3,5,-19,
  17,3,-19,
  17,-5,-19,
  3,-7,-19,
  -3,-7,-19,
  -17,-5,-19,
  0,0,37,
  0,0,44,
  -39,-2,-19,
  -39,2,-19,
  -43,0,-19,
  39,2,-19,
  43,0,-19,
  39,-2,-19,
  63,-3,-19,
  -63,-3,-19
};

const uint8_t COBRA_faces[] =
{
  3,0,115,53,7,0,0,0,0,2,0,1,
  3,220,117,32,28,0,0,0,0,2,1,5,
  3,36,117,32,97,0,0,0,0,6,0,2,
  3,227,119,29,136,1,0,0,0,5,1,3,
  3,29,119,29,32,6,0,0,0,4,0,6,
  3,242,126,0,16,24,0,0,0,9,2,5,
  3,14,126,0,64,40,0,0,0,6,2,9,
  3,200,113,0,0,193,0,0,0,5,3,8,
  3,56,113,0,0,4,3,0,0,7,4,6,
  7,0,0,129,0,176,30,0,0,6,9,5,8,10,11,7,
  4,237,133,24,128,64,48,0,0,8,3,1,10,
  4,0,132,24,2,0,104,0,0,1,0,11,10,
  4,19,133,24,0,2,69,0,0,11,0,4,7,
  4,0,0,130,0,0,128,7,0,16,15,14,17,
  4,0,0,130,0,0,0,120,0,18,13,12,19,
  3,0,0,129,0,0,0,128,3,24,22,23,
  3,0,0,129,0,0,0,0,28,26,25,27,
  255
};

const uint8_t COBRA_edges[] =
{
  0, 2,
  0, 1,
  1, 2,
  1, 5,
  2, 5,
  0, 6,
  2, 6,
  1, 3,
  3, 5,
  0, 4,
  4, 6,
  2, 9,
  5, 9,
  6, 9,
  3, 8,
  5, 8,
  4, 7,
  6, 7,
  7, 11,
  10, 11,
  8, 10,
  1, 10,
  0, 11,
  14, 17,
  14, 15,
  15, 16,
  16, 17,
  12, 19,
  12, 13,
  13, 18,
  18, 19,
  22, 23,
  22, 24,
  23, 24,
  25, 27,
  25, 26,
  26, 27
};

const uint8_t shiny[512] =
{
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,7,
  7,7,7,8,8,8,8,8,8,8,8,8,8,9,9,9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,13,13,13,13,13,13,13,14,14,14,
  14,14,14,15,15,15,15,15,15,16,16,16,16,16,16,17,17,17,17,17,18,18,18,18,18,19,19,19,19,19,20,20,20,20,20,21,21,21,21,22,22,22,22,22,23,23,23,23,
  24,24,24,24,25,25,25,25,26,26,26,27,27,27,27,28,28,28,29,29,29,29,30,30,30,31,31,31,32,32,32,33,33,33,34,34,34,35,35,35,36,36,36,37,37,37,38,38,
  39,39,39,40,40,41,41,41,42,42,43,43,43,44,44,45,45,46,46,47,47,47,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,56,56,57,57,58,58,59,60,60,61,
  61,62,62,63,64,64,65,65,66,67,67,68,69,69,70,70,71,72,72,73,74,74,75,76,76,77,78,79,79,80,81,81,82,83,84,84,85,86,87,88,88,89,90,91,92,92,93,94,
  95,96,97,97,98,99,100,101,102,103,104,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,130,131,
  132,133,134,135,136,138,139,140,141,142,143,145,146,147,148,150,151,152,153,155,156,157,159,160,161,163,164,165,167,168,169,171,172,174,175,176,
  178,179,181,182,184,185,187,188,190,191,193,195,196,198,199,201,203,204,206,208,209,211,213,214,216,218,219,221,223,225,227,228,230,232,234,236,
  238,239,241,243,245,247,249,251,253,255
};

float model_mat[9] =
{
  1.0, 0.0, 0.0,
  0.0, 1.0, 0.0,
  0.0, 0.0, 1.0
};

float normal_mat[9] =
{
  1.0, 0.0, 0.0,
  0.0, 1.0, 0.0,
  0.0, 0.0, 1.0
};

typedef struct
{
  int x, y;
  float z;
} xyz_t;

#define N_VERTICES  (sizeof(COBRA_vertices) / 3)
#define EDGE_BYTES  5

xyz_t projected[N_VERTICES];
u8 visible_edges[EDGE_BYTES];

u32 *dl;
u32 dlp;

class Poly
{
  int x0, y0, x1, y1;
  int x[8], y[8];
  u8 n;

  void restart()
  {
    n = 0;
    x0 = 16 * 480;
    x1 = 0;
    y0 = 16 * 272;
    y1 = 0;
  }

  void perim()
  {
    for (u8 i = 0; i < n; i++)
      dl[dlp++] = Vertex2f(x[i], y[i]);

    dl[dlp++] = Vertex2f(x[0], y[0]);
  }

 public:
  void begin()
  {
    restart();

    dl[dlp++] = ColorMask(0,0,0,0);
    dl[dlp++] = StencilOp(FT_KEEP, FT_INVERT);
    dl[dlp++] = StencilFunc(FT_ALWAYS, 255, 255);
  }

  void v(int _x, int _y)
  {
    x0 = min(x0, _x >> 4);
    x1 = max(x1, _x >> 4);
    y0 = min(y0, _y >> 4);
    y1 = max(y1, _y >> 4);
    x[n] = _x;
    y[n] = _y;
    n++;
  }

  void paint()
  {
    x0 = max(0, x0);
    y0 = max(0, y0);
    x1 = min(16 * 480, x1);
    y1 = min(16 * 272, y1);
    dl[dlp++] = ScissorXY(x0, y0);
    dl[dlp++] = ScissorSize(x1 - x0 + 1, y1 - y0 + 1);
    dl[dlp++] = Begin(FT_EDGE_STRIP_B);
    perim();
  }

  void finish()
  {
    dl[dlp++] = ColorMask(1,1,1,1);
    dl[dlp++] = StencilFunc(FT_EQUAL, 255, 255);

    dl[dlp++] = Begin(FT_EDGE_STRIP_B);
    dl[dlp++] = Vertex2ii(0, 0, 0, 0);
    dl[dlp++] = Vertex2ii(511, 0, 0, 0);
  }

  void draw()
  {
    paint();
    finish();
  }

  void outline()
  {
    dl[dlp++] = Begin(FT_LINE_STRIP);
    perim();
  }
};

void transform_normal(int8_t &nx, int8_t &ny, int8_t &nz)
{
  int8_t xx = nx * normal_mat[0] + ny * normal_mat[1] + nz * normal_mat[2];
  int8_t yy = nx * normal_mat[3] + ny * normal_mat[4] + nz * normal_mat[5];
  int8_t zz = nx * normal_mat[6] + ny * normal_mat[7] + nz * normal_mat[8];
  nx = xx;
  ny = yy;
  nz = zz;
}

void rotate(float *m, float *mi, float angle, float *axis)
{
  float x = axis[0];
  float y = axis[1];
  float z = axis[2];

  float s = sin(angle);
  float c = cos(angle);

  float xx = x*x*(1-c);
  float xy = x*y*(1-c);
  float xz = x*z*(1-c);
  float yy = y*y*(1-c);
  float yz = y*z*(1-c);
  float zz = z*z*(1-c);

  float xs = x * s;
  float ys = y * s;
  float zs = z * s;

  m[0] = xx + c;
  m[1] = xy - zs;
  m[2] = xz + ys;

  m[3] = xy + zs;
  m[4] = yy + c;
  m[5] = yz - xs;

  m[6] = xz - ys;
  m[7] = yz + xs;
  m[8] = zz + c;

  mi[0] = m[0];
  mi[1] = xy + zs;
  mi[2] = xz - ys;

  mi[3] = xy - zs;
  mi[4] = m[4];
  mi[5] = yz + xs;

  mi[6] = xz + ys;
  mi[7] = yz - xs;
  mi[8] = m[8];
}

#define M(nm,i,j)       ((nm)[3 * (i) + (j)])

void mult_matrices(float *a, float *b, float *c)
{
  int i, j, k;
  float result[9];
  
  for (i = 0; i < 3; i++) 
  {
    for (j = 0; j < 3; j++) 
    {
      M(result,i,j) = 0.0f;
      
      for(k = 0; k < 3; k++) 
        M(result,i,j) +=  M(a,i,k) *  M(b,k,j);
    }
  }
  
  memcpy(c, result, sizeof(result));
}

void rotation(float angle, float *axis)
{
  float mat[9];
  float mati[9];

  rotate(mat, mati, angle, axis);
  mult_matrices(model_mat, mat, model_mat);
  mult_matrices(mati, normal_mat, normal_mat);
}

void cobra_project(float distance)
{
  int pm = 0;
  int pq = N_VERTICES;
  xyz_t *dst = projected;
  int8_t x, y, z;

  while (pq--)
  {
    x = COBRA_vertices[pm++];
    y = COBRA_vertices[pm++];
    z = COBRA_vertices[pm++];
    float xx = x * model_mat[0] + y * model_mat[3] + z * model_mat[6];
    float yy = x * model_mat[1] + y * model_mat[4] + z * model_mat[7];
    float zz = x * model_mat[2] + y * model_mat[5] + z * model_mat[8] + distance;
    float q = 240 / (100 + zz);
    dst->x = 16 * (240 + xx * q);
    dst->y = 16 * (136 + yy * q);
    dst->z = zz;
    dst++;
  }
}

void cobra_draw_navlight(u8 nf)
{
  float l0z = projected[N_VERTICES - 2].z;
  float l1z = projected[N_VERTICES - 1].z;
  u8 i;

  if (nf == 0)  // draw the one with smallest z
    i = (l0z < l1z) ? (N_VERTICES - 2) : (N_VERTICES - 1);
  else
    i = (l0z < l1z) ? (N_VERTICES - 1) : (N_VERTICES - 2);

  // dl[dlp++] = SaveContext();
  dl[dlp++] = BlendFunc(FT_SRC_ALPHA, FT_ONE);
  dl[dlp++] = Begin(FT_BITMAPS);
  // dl[dlp++] = BitmapHandle(LIGHT_HANDLE);

  dl[dlp++] = ColorRGB((i == N_VERTICES - 2) ? 0xfe2b18 : 0x4fff82);
  // dl[dlp++] = Vertex2f(projected[i].x - (16 * LIGHT_WIDTH / 2), projected[i].y - (16 * LIGHT_WIDTH / 2));
  // dl[dlp++] = RestoreContext();
}

void cobra_draw_faces()
{
  memset(visible_edges, 0, sizeof(visible_edges));

  int p = 0;
  u8 n;
  int c = 1;
  Poly po;

  while ((n = COBRA_faces[p++]) != 0xff)
  {
    int8_t nx = COBRA_faces[p++];
    int8_t ny = COBRA_faces[p++];
    int8_t nz = COBRA_faces[p++];

    u8 face_edges[EDGE_BYTES];

    for (u8 i = 0; i < EDGE_BYTES; i++)
      face_edges[i] = COBRA_faces[p++];

    u8 v1 = COBRA_faces[p];
    u8 v2 = COBRA_faces[p + 1];
    u8 v3 = COBRA_faces[p + 2];

    long x1 = projected[v1].x;
    long y1 = projected[v1].y;
    long x2 = projected[v2].x;
    long y2 = projected[v2].y;
    long x3 = projected[v3].x;
    long y3 = projected[v3].y;

    long area = (x1 - x3) * (y2 - y1) - (x1 - x2) * (y3 - y1);

    if (area > 0)
    {
      for (u8 i = 0; i < EDGE_BYTES; i++)
        visible_edges[i] |= face_edges[i];

      po.begin();

      for (int i = 0; i < n; i++)
      {
        u8 vi = COBRA_faces[p++];
        xyz_t *v = &projected[vi];
        po.v(v->x, v->y);
      }

      {
        transform_normal(nx, ny, nz);

        uint16_t r = 10, g = 10, b = 20;  // Ambient
        int d = -ny;                      // diffuse light from +ve Y

        if (d > 0)
        {
          r += d >> 2;
          g += d >> 1;
          b += d;
        }
                                          // use specular half angle
        d = ny * -90 + nz * -90;          // Range -16384 to +16384

        if (d > 8192)
        {
          u8 l = shiny[(d - 8192) >> 4];
          r += l;
          g += l;
          b += l;
        }

        dl[dlp++] = ColorRGB(min(255, r), min(255, g), min(255, b));
      }

      po.draw();

    }
    else
      p += n;

    c++;
  }
}

void cobra_draw_edges()
{
  dl[dlp++] = ColorRGB(0x2e666e);
  dl[dlp++] = Begin(FT_LINES);
  dl[dlp++] = LineWidth(20);

  int p = 0;
  u8 *pvis = visible_edges;
  u8 vis = 0;

  for (u8 i = 0; i < sizeof(COBRA_edges) / 2; i++)
  {
    if ((i & 7) == 0)
      vis = *pvis++;
    u8 v0 = COBRA_edges[p++];
    u8 v1 = COBRA_edges[p++];

    if (vis & 1)
    {
      int x0 = projected[v0].x;
      int y0 = projected[v0].y;
      int x1 = projected[v1].x;
      int y1 = projected[v1].y;

      dl[dlp++] = Vertex2f(x0, y0);
      dl[dlp++] = Vertex2f(x1, y1);
    }

    vis >>= 1;
  }
}

u32 cobra(void *data)
{
  float angle = 0.01;
  float axis[3] = {1,1,1};

  dl = (u32*)data;
  dlp = 0;

  dl[dlp++] = ClearColorRGB(0, 0, 0);
  dl[dlp++] = Clear();
  
  dl[dlp++] = Begin(FT_BITMAPS);
  
  if (angle != 0.0f)
    rotation(angle, axis);

  cobra_project(0);
  // cobra_draw_navlight(1);
  cobra_draw_faces();
  dl[dlp++] = RestoreContext();
  cobra_draw_edges();
  // cobra_draw_navlight(0);
  // dl[dlp++] = RestoreContext();

  dl[dlp++] = End();
  dl[dlp++] = Display();

  return dlp * 4;
}
