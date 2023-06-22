#ifndef CONVERT_H
#define CONVERT_H

#include <stdlib.h>
#include <retro_inline.h>

#include "../../Graphics/image_convert.h"

#if !defined(__MACH__) && !defined(__ppc__) && defined(__GNUC__)
#define HAVE_BSWAP 
#endif

#ifdef HAVE_BSWAP
#define bswap_32 __builtin_bswap32
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#define bswap_32(x) _byteswap_ulong(x)
#else
#define bswap_32(x) (((x) << 24) & 0xff000000) \
   | (((x) << 8) & 0xff0000) \
   | (((x) >> 8) & 0xff00) \
   | (((x) >> 24) & 0xff )
#endif

static const volatile uint8_t Five2Eight[32] =
{
      0, // 00000 = 00000000
      8, // 00001 = 00001000
     16, // 00010 = 00010000
     25, // 00011 = 00011001
     33, // 00100 = 00100001
     41, // 00101 = 00101001
     49, // 00110 = 00110001
     58, // 00111 = 00111010
     66, // 01000 = 01000010
     74, // 01001 = 01001010
     82, // 01010 = 01010010
     90, // 01011 = 01011010
     99, // 01100 = 01100011
    107, // 01101 = 01101011
    115, // 01110 = 01110011
    123, // 01111 = 01111011
    132, // 10000 = 10000100
    140, // 10001 = 10001100
    148, // 10010 = 10010100
    156, // 10011 = 10011100
    165, // 10100 = 10100101
    173, // 10101 = 10101101
    181, // 10110 = 10110101
    189, // 10111 = 10111101
    197, // 11000 = 11000101
    206, // 11001 = 11001110
    214, // 11010 = 11010110
    222, // 11011 = 11011110
    230, // 11100 = 11100110
    239, // 11101 = 11101111
    247, // 11110 = 11110111
    255  // 11111 = 11111111
};

static const volatile uint8_t Four2Eight[16] =
{
      0, // 0000 = 00000000
     17, // 0001 = 00010001
     34, // 0010 = 00100010
     51, // 0011 = 00110011
     68, // 0100 = 01000100
     85, // 0101 = 01010101
    102, // 0110 = 01100110
    119, // 0111 = 01110111
    136, // 1000 = 10001000
    153, // 1001 = 10011001
    170, // 1010 = 10101010
    187, // 1011 = 10111011
    204, // 1100 = 11001100
    221, // 1101 = 11011101
    238, // 1110 = 11101110
    255  // 1111 = 11111111
};

static const volatile uint8_t Three2Four[8] =
{
     0, // 000 = 0000
     2, // 001 = 0010
     4, // 010 = 0100
     6, // 011 = 0110
     9, // 100 = 1001
    11, // 101 = 1011
    13, // 110 = 1101
    15, // 111 = 1111
};

static const volatile uint8_t Three2Eight[8] =
{
      0, // 000 = 00000000
     36, // 001 = 00100100
     73, // 010 = 01001001
    109, // 011 = 01101101
    146, // 100 = 10010010
    182, // 101 = 10110110
    219, // 110 = 11011011
    255, // 111 = 11111111
};

static const volatile uint8_t Two2Eight[4] =
{
   0, // 00 = 00000000
   85, // 01 = 01010101
   170, // 10 = 10101010
   255  // 11 = 11111111
};

static const volatile uint8_t One2Four[2] =
{
   0, // 0 = 0000
   15, // 1 = 1111
};

static const volatile uint8_t One2Eight[2] =
{
   0, // 0 = 00000000
   255, // 1 = 11111111
};

#define RGBA8888_RGBA4444(color) (((color & 0x000000f0) <<  8) | ((color & 0x0000f000) >>  4) | ((color & 0x00f00000) >> 16) | ((color & 0xf0000000) >> 28))

static INLINE uint32_t RGBA5551_RGBA8888( uint16_t color )
{
   uint8_t r, g, b, a;
   color = swapword( color );
   r = Five2Eight[color >> 11];
   g = Five2Eight[(color >> 6) & 0x001f];
   b = Five2Eight[(color >> 1) & 0x001f];
   a = One2Eight [(color     ) & 0x0001];
   return (a << 24) | (b << 16) | (g << 8) | r;
}

// Just swaps the word
#define RGBA5551_RGBA5551(color) (swapword( color ))

static INLINE uint32_t IA88_RGBA8888( uint16_t color )
{
    uint8_t a = color >> 8;
    uint8_t i = color & 0x00FF;
    return (a << 24) | (i << 16) | (i << 8) | i;
}

static INLINE uint16_t IA88_RGBA4444( uint16_t color )
{
   uint8_t a = color >> 12;
   uint8_t i = (color >> 4) & 0x000F;
   return (i << 12) | (i << 8) | (i << 4) | a;
}

#define IA44_RGBA4444(color) (((color & 0xf0) << 8) | ((color & 0xf0) << 4) | (color))

static INLINE uint32_t IA44_RGBA8888( uint8_t color )
{
   uint8_t i = Four2Eight[color >> 4];
   uint8_t a = Four2Eight[color & 0x0F];
   return (a << 24) | (i << 16) | (i << 8) | i;
}

static INLINE uint16_t IA44_IA88( uint8_t color )
{
   uint8_t i = Four2Eight[color >> 4];
   uint8_t a = Four2Eight[color & 0x0F];
   return (a << 8) | i;
}

static INLINE uint16_t IA31_RGBA4444( uint8_t color )
{
   uint8_t i = Three2Four[color >> 1];
   uint8_t a = One2Four[color & 0x01];
   return (i << 12) | (i << 8) | (i << 4) | a;
}

static INLINE uint16_t IA31_IA88( uint8_t color )
{
   uint8_t i = Three2Eight[color >> 1];
   uint8_t a = One2Eight[color & 0x01];
   return (a << 8) | i;
}

static INLINE uint32_t IA31_RGBA8888( uint8_t color )
{
   uint8_t i = Three2Eight[color >> 1];
   uint8_t a = One2Eight[color & 0x01];
   return (i << 24) | (i << 16) | (i << 8) | a;
}

static INLINE uint16_t I8_RGBA4444( uint8_t color )
{
   uint8_t c = color >> 4;
   return (c << 12) | (c << 8) | (c << 4) | c;
}

#define I8_RGBA8888(color) ((color << 24) | (color << 16) | (color << 8) | color)

static INLINE uint16_t I4_RGBA4444( uint8_t color )
{
   uint16_t ret = color & 0x0f;
   ret |= ret << 4; ret |= ret << 8; return ret;
}

#define I4_I8(color) (Four2Eight[color & 0x0f])

static INLINE uint16_t I4_IA88( uint8_t color )
{
   uint32_t c = Four2Eight[color & 0x0f];
   return (c << 8) | c;
}

#define I8_IA88(color) ((color << 8) | color)

static INLINE uint16_t IA88_IA88( uint16_t color )
{
   uint8_t a = (color&0xFF);
   uint8_t i = (color>>8);
   return  (i << 8) | a;
}


static INLINE uint32_t I4_RGBA8888( uint8_t color )
{
   uint8_t c = Four2Eight[color];
   c |= c << 4;
   return (c << 24) | (c << 16) | (c << 8) | c;
}

#endif // CONVERT_H

