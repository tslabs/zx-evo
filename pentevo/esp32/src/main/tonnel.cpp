
u32 tunnel(void *data)
{
  static int phase = 0;
  int warp_x = 5;
  int warp_y = 10;
  int red   = 25;
  int green = 140;
  int blue  = 255;
  int n_phases = 50;
  int depth = 50;
  int n_points = 24;
  int scale = 100;

  u32 *dl = (u32*)data;
  u32 p = 0;

  dl[p++] = ClearColorRGB(0, 0, 0);
  dl[p++] = Clear();
  dl[p++] = ColorA(255);
  dl[p++] = Begin(FT_POINTS);

  float phase_shift = (float)phase / n_phases;
  float lm = log((float)depth);

  for (int z1 = 0; z1 < depth; z1++)
  {
    float z = z1 + phase_shift;
    float alg = lm - log((float)(depth - z));

    int d = (int)(alg * scale);
    int c = (int)((alg / 6) * 1000);
    int rf = (int)(alg * 2.5 * 16);

    int cr = min(255, (c * red) / 1000);
    int cg = min(255, (c * green) / 1000);
    int cb = min(255, (c * blue) / 1000);

    dl[p++] = ColorRGB(cr, cg, cb);
    dl[p++] = PointSize(rf);

    float s = (float)depth - z;
    float sx = s * warp_x;
    float sy = s * warp_y;
    int step = 360 / n_points;

    for (int i = 0; i < 360; i += step)
    {
      int xf = (int)(16 * (sin_tab[i] * d + 512 + sx));
      int yf = (int)(16 * (sin_tab[(i + 90) % 360] * d + 384 + sy));
      dl[p++] = Vertex2f(xf, yf);
    }
  }

  dl[p++] = End();
  dl[p++] = Display();

  phase++; phase %= n_phases;
  return p * 4;
}
