static void f3dttexa_loadtex(uint32_t w0, uint32_t w1)
{
   rdp_settextureimage(0x3d100000, w1);

   rdp_settile(0x35100000, 0x07000000);

   rdp_loadblock(0x33000000,
         0x27000000 | (w0 & 0x00FFFFFF)
         );
}

static void f3dttexa_settilesize(uint32_t w0, uint32_t w1)
{
    uint32_t firstHalf = (w1 & 0xFF000000) >> 15;

    rdp_settile(0x35400000 | firstHalf,
          w0 & 0x00FFFFFF);

    rdp_settilesize(0x32000000,
          w1 & 0x00FFFFFF);
}
