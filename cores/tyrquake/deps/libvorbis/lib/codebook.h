/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2015             *
 * by the Xiph.Org Foundation https://xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: basic shared codebook operations

 ********************************************************************/

#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

#include <ogg/ogg.h>

/* This structure encapsulates huffman and VQ style encoding books; it
   doesn't do anything specific to either.

   valuelist/quantlist are nonNULL (and q_* significant) only if
   there's entry->value mapping to be done.

   If encode-side mapping must be done (and thus the entry needs to be
   hunted), the auxiliary encode pointer will point to a decision
   tree.  This is true of both VQ and huffman, but is mostly useful
   with VQ.

*/

typedef struct static_codebook{
  long   dim;           /* codebook dimensions (elements per vector) */
  long   entries;       /* codebook entries */
  char  *lengthlist;    /* codeword lengths in bits */

  /* mapping ***************************************************************/
  int    maptype;       /* 0=none
                           1=implicitly populated values from map column
                           2=listed arbitrary values */

  /* The below does a linear, single monotonic sequence mapping. */
  long     q_min;       /* packed 32 bit float; quant value 0 maps to minval */
  long     q_delta;     /* packed 32 bit float; val 1 - val 0 == delta */
  int      q_quant;     /* bits: 0 < quant <= 16 */
  int      q_sequencep; /* bitflag */

  long     *quantlist;  /* map == 1: (int)(entries^(1/dim)) element column map
                           map == 2: list of dim*entries quantized entry vals
                        */
  int allocedp;
} static_codebook;

typedef struct codebook{
  long dim;           /* codebook dimensions (elements per vector) */
  long entries;       /* codebook entries */
  long used_entries;  /* populated codebook entries */
  const static_codebook *c;

  /* for encode, the below are entry-ordered, fully populated */
  float        *valuelist;  /* list of dim*entries actual entry values */
  ogg_uint32_t *codelist;   /* list of bitstream codewords for each entry */

  /* The current encoder uses only centered, integer-only lattice books. */
  int           quantvals;
  int           minval;
  int           delta;
} codebook;

typedef struct dec_codebook{
  signed char   dim;           /* codebook dimensions (elements per vector) */
  signed char   minlength;
  signed char   maxlength;
  signed char   firsttablen;   /* number of bits in decode fastpath table */
  ogg_int32_t   entries;       /* codebook entries */
  ogg_int32_t   hi_max;        /* the maximum value of the fastpath hi hint */
  signed char   hint_shift;    /* the amount to shift the hints */

  /* The below does a linear, single monotonic sequence mapping. */
  signed char   maptype;
  signed char   q_quant;     /* bits: 0 < quant <= 16 */
  signed char   q_sequencep; /* bitflag */
  ogg_uint32_t  q_min;       /* packed 32 bit float;
                                quant value 0 maps to minval */
  ogg_uint32_t  q_delta;     /* packed 32 bit float; val 1 - val 0 == delta */
  ogg_uint16_t *quantlist;   /* map == 1:
                                 (int)(entries^(1/dim)) element column map
                                map == 2:
                                 list of dim*entries quantized entry vals */

  ogg_uint32_t *firsttable; /* decode fastpath lookup table */
  /* the below are ordered by bitreversed codeword and only used entries are
     populated */
  ogg_uint32_t *codelist;   /* unordered: list of codewords for each entry.
                               ordered: sentinel value for each length as large
                                as any codeword (of that length or shorter)
                                with any trailing bits appended. */
  signed char  *codelengths;/* unordered: length of each codeword.
                               ordered: unused (NULL). */
  ogg_int32_t  *index;      /* unordered: a mapping from codeword index (in the
                               collapsed, sorted list in codelist) to the entry
                               in the original (possibly-sparse) codebook.
                               ordered: cumulative count of entries for each
                               length (starting from the minimum length). */
  float        *valuelist;  /* list of dim*entries actual entry values */

} dec_codebook;

extern void vorbis_staticbook_destroy(static_codebook *b);
extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(dec_codebook *c);
extern void vorbis_book_clear(codebook *b);
extern void vorbis_decbook_clear(dec_codebook *c);

extern void _book_unquantize(float *r,const dec_codebook *b,int n,
                             ogg_int64_t *map);
extern float *_book_logdist(const static_codebook *b,float *vals);
extern float _float32_unpack(long val);
extern long   _float32_pack(float val);
extern int  _best(codebook *book, float *a, int step);
extern long _book_maptype1_quantvals(long dim, long entries);

extern int vorbis_book_besterror(codebook *book,float *a,int step,int addmul);
extern long vorbis_book_codeword(codebook *book,int entry);
extern long vorbis_book_codelen(codebook *book,int entry);



extern int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *b);
extern int vorbis_decbook_unpack(dec_codebook *c,oggpack_buffer *opb);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);

extern long vorbis_book_decode(dec_codebook *book, oggpack_buffer *b);
extern long vorbis_book_decodevs_add(dec_codebook *book, float *a,
                                     oggpack_buffer *b,int n);
extern long vorbis_book_decodev_set(dec_codebook *book, float *a,
                                    oggpack_buffer *b,int n);
extern long vorbis_book_decodev_add(dec_codebook *book, float *a,
                                    oggpack_buffer *b,int n);
extern long vorbis_book_decodevv_add(dec_codebook *book, float **a,
                                     long off,int ch,
                                     oggpack_buffer *b,int n);



#endif
