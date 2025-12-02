#ifndef __NOPIC_H__
#define __NOPIC_H__

/* these are just deps, to be removed */

static const struct {
  unsigned int   width;
  unsigned int   height;
  unsigned int   bytes_per_pixel; /* 3:RGB, 4:RGBA */ 
  unsigned char  pixel_data[128 * 96 * 3 + 1];
} NoPic_Image = {
  128, 96, 3, ""
};

extern void PaintPicDot(unsigned char * p,unsigned char c);
extern unsigned char cFont[10][120];

void DrawNumBorPic(unsigned char *pMem, int lSelectedSlot)
{
 unsigned char *pf;
 int x,y;
 int c,v;

 pf=pMem+(103*3);                                      // offset to number rect

 for(y=0;y<20;y++)                                     // loop the number rect pixel
  {
   for(x=0;x<6;x++)
    {
     c=cFont[lSelectedSlot][x+y*6];                    // get 4 char dot infos at once (number depends on selected slot)
     v=(c&0xc0)>>6;
     PaintPicDot(pf,(unsigned char)v);pf+=3;                // paint the dots into the rect
     v=(c&0x30)>>4;
     PaintPicDot(pf,(unsigned char)v);pf+=3;
     v=(c&0x0c)>>2;
     PaintPicDot(pf,(unsigned char)v);pf+=3;
     v=c&0x03;
     PaintPicDot(pf,(unsigned char)v);pf+=3;
    }
   pf+=104*3;                                          // next rect y line
  }

 pf=pMem;                                              // ptr to first pos in 128x96 pic
 for(x=0;x<128;x++)                                    // loop top/bottom line
  {
   *(pf+(95*128*3))=0x00;*pf++=0x00;
   *(pf+(95*128*3))=0x00;*pf++=0x00;                   // paint it red
   *(pf+(95*128*3))=0xff;*pf++=0xff;
  }
 pf=pMem;                                              // ptr to first pos
 for(y=0;y<96;y++)                                     // loop left/right line
  {
   *(pf+(127*3))=0x00;*pf++=0x00;
   *(pf+(127*3))=0x00;*pf++=0x00;                      // paint it red
   *(pf+(127*3))=0xff;*pf++=0xff;
   pf+=127*3;                                          // offset to next line
  }
}

#endif /* __NOPIC_H__ */
