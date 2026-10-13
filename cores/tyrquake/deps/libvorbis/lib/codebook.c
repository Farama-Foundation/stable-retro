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

 function: basic codebook pack/unpack/code/decode operations

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codebook.h"
#include "scales.h"
#include "misc.h"
#include "os.h"

/* packs the given codebook into the bitstream **************************/

int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *opb){
  long i,j;
  int ordered=0;

  /* first the basic parameters */
  oggpack_write(opb,0x564342,24);
  oggpack_write(opb,c->dim,16);
  oggpack_write(opb,c->entries,24);

  /* pack the codewords.  There are two packings; length ordered and
     length random.  Decide between the two now. */

  for(i=1;i<c->entries;i++)
    if(c->lengthlist[i-1]==0 || c->lengthlist[i]<c->lengthlist[i-1])break;
  if(i==c->entries)ordered=1;

  if(ordered){
    /* length ordered.  We only need to say how many codewords of
       each length.  The actual codewords are generated
       deterministically */

    long count=0;
    oggpack_write(opb,1,1);  /* ordered */
    oggpack_write(opb,c->lengthlist[0]-1,5); /* 1 to 32 */

    for(i=1;i<c->entries;i++){
      char this=c->lengthlist[i];
      char last=c->lengthlist[i-1];
      if(this>last){
        for(j=last;j<this;j++){
          oggpack_write(opb,i-count,ov_ilog(c->entries-count));
          count=i;
        }
      }
    }
    oggpack_write(opb,i-count,ov_ilog(c->entries-count));

  }else{
    /* length random.  Again, we don't code the codeword itself, just
       the length.  This time, though, we have to encode each length */
    oggpack_write(opb,0,1);   /* unordered */

    /* algortihmic mapping has use for 'unused entries', which we tag
       here.  The algorithmic mapping happens as usual, but the unused
       entry has no codeword. */
    for(i=0;i<c->entries;i++)
      if(c->lengthlist[i]==0)break;

    if(i==c->entries){
      oggpack_write(opb,0,1); /* no unused entries */
      for(i=0;i<c->entries;i++)
        oggpack_write(opb,c->lengthlist[i]-1,5);
    }else{
      oggpack_write(opb,1,1); /* we have unused entries; thus we tag */
      for(i=0;i<c->entries;i++){
        if(c->lengthlist[i]==0){
          oggpack_write(opb,0,1);
        }else{
          oggpack_write(opb,1,1);
          oggpack_write(opb,c->lengthlist[i]-1,5);
        }
      }
    }
  }

  /* is the entry number the desired return value, or do we have a
     mapping? If we have a mapping, what type? */
  oggpack_write(opb,c->maptype,4);
  switch(c->maptype){
  case 0:
    /* no mapping */
    break;
  case 1:case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */

    if(!c->quantlist){
      /* no quantlist?  error */
      return(-1);
    }

    /* values that define the dequantization */
    oggpack_write(opb,c->q_min,32);
    oggpack_write(opb,c->q_delta,32);
    oggpack_write(opb,c->q_quant-1,4);
    oggpack_write(opb,c->q_sequencep,1);

    {
      int quantvals;
      switch(c->maptype){
      case 1:
        /* a single column of (c->entries/c->dim) quantized values for
           building a full value list algorithmically (square lattice) */
        quantvals=_book_maptype1_quantvals(c->dim, c->entries);
        break;
      case 2:
        /* every value (c->entries*c->dim total) specified explicitly */
        quantvals=c->entries*c->dim;
        break;
      default: /* NOT_REACHABLE */
        quantvals=-1;
      }

      /* quantized values */
      for(i=0;i<quantvals;i++)
        oggpack_write(opb,labs(c->quantlist[i]),c->q_quant);

    }
    break;
  default:
    /* error case; we don't have any other map types now */
    return(-1);
  }

  return(0);
}

/* unpacks a codebook from the packet buffer into the codebook struct */
int vorbis_decbook_unpack(dec_codebook *c,oggpack_buffer *opb){
  long i;

  /* make sure alignment is correct */
  if(oggpack_read(opb,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  c->dim=(signed char)oggpack_read(opb,16);
  c->entries=(ogg_int32_t)oggpack_read(opb,24);
  if(c->entries==-1)goto _eofout;

  if(ov_ilog(c->dim)+ov_ilog(c->entries)>24)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch((int)oggpack_read(opb,1)){
  case 0:{
    long unused;
    /* allocated but unused entries? */
    unused=oggpack_read(opb,1);
    if((c->entries*(unused?1:5)+7)>>3>opb->storage-oggpack_bytes(opb))
      goto _eofout;
    /* unordered */
    c->codelengths=_ogg_malloc(sizeof(*c->codelengths)*c->entries);
    if(c->codelengths==NULL)goto _errout;

    /* allocated but unused entries? */
    if(unused){
      /* yes, unused entries */

      for(i=0;i<c->entries;i++){
        if(oggpack_read(opb,1)){
          long num=oggpack_read(opb,5);
          if(num==-1)goto _eofout;
          c->codelengths[i]=(signed char)(num+1);
        }else
          c->codelengths[i]=0;
      }
    }else{
      /* all entries used; no tagging */
      for(i=0;i<c->entries;i++){
        long num=oggpack_read(opb,5);
        if(num==-1)goto _eofout;
        c->codelengths[i]=(signed char)(num+1);
      }
    }

    break;
  }
  case 1:
    /* ordered */
    {
      ogg_int32_t cum_entries[32];
      long minlength=oggpack_read(opb,5)+1;
      long maxlength;
      long length;
      if(minlength==0)goto _eofout;
      maxlength=length=minlength;
      for(i=0;i<c->entries;length++){
        long num=oggpack_read(opb,ov_ilog(c->entries-i));
        if(num==-1)goto _eofout;
        if(length>32 || num>c->entries-i ||
           (num>0 && (num-1)>>(length-1)>1)){
          goto _errout;
        }
        i+=num;
        cum_entries[length-minlength]=(ogg_int32_t)i;
        /* skip lengths with 0 entries at the start, to avoid creating a
           malformed codeword sentinel in vorbis_book_init_decode(). */
        if(i==0)minlength++;
        maxlength=length;
      }
      c->minlength=(signed char)minlength;
      c->maxlength=(signed char)maxlength;
      c->index=_ogg_malloc((maxlength-minlength+1)*sizeof(*c->index));
      if(c->index==NULL)goto _errout;
      memcpy(c->index,cum_entries,(maxlength-minlength+1)*sizeof(*c->index));
    }
    break;
  default:
    /* EOF */
    goto _eofout;
  }

  /* Do we have a mapping to unpack? */
  switch((c->maptype=(signed char)oggpack_read(opb,4))){
  case 0:
    /* no mapping */
    break;
  case 1: case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */

    c->q_min=(ogg_uint32_t)oggpack_read(opb,32);
    c->q_delta=(ogg_uint32_t)oggpack_read(opb,32);
    c->q_quant=(signed char)oggpack_read(opb,4)+1;
    c->q_sequencep=(signed char)oggpack_read(opb,1);
    if(c->q_sequencep==-1)goto _eofout;

    {
      long q=0;
      int quantvals=0;
      switch(c->maptype){
      case 1:
        quantvals=(c->dim==0?0:_book_maptype1_quantvals(c->dim, c->entries));
        break;
      case 2:
        quantvals=c->entries*c->dim;
        break;
      }

      /* quantized values */
      if(((quantvals*c->q_quant+7)>>3)>opb->storage-oggpack_bytes(opb))
        goto _eofout;
      c->quantlist=_ogg_malloc(sizeof(*c->quantlist)*quantvals);
      if(c->quantlist==NULL)goto _errout;
      for(i=0;i<quantvals;i++){
        q=oggpack_read(opb,c->q_quant);
        c->quantlist[i]=(ogg_uint16_t)q;
      }

      if(q==-1)goto _eofout;
    }
    break;
  default:
    goto _errout;
  }

  /* all set */
  /* we could finish initializing the decode tables here, but this was a
     separate step back when we had a shared encoder/decoder codebook struct,
     and deferring it ensures we continue to report the same kinds of errors
     in the same places. */
  return(0);

 _errout:
 _eofout:
  /* our caller will clean up the partially constructed book */
  return(-1);
}

/* returns the number of bits ************************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  if(a<0 || a>=book->c->entries)return(0);
  oggpack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* the 'eliminate the decode tree' optimization actually requires the
   codewords to be MSb first, not LSb.  This is an annoying inelegancy
   (and one of the first places where carefully thought out design
   turned out to be wrong; Vorbis II and future Ogg codecs should go
   to an MSb bitpacker), but not actually the huge hit it appears to
   be.  The first-stage decode table catches most words so that
   bitreverse is not in the main execution path. */

static ogg_uint32_t bitreverse(ogg_uint32_t x){
  x=    ((x>>16)&0x0000ffff) | ((x<<16)&0xffff0000);
  x=    ((x>> 8)&0x00ff00ff) | ((x<< 8)&0xff00ff00);
  x=    ((x>> 4)&0x0f0f0f0f) | ((x<< 4)&0xf0f0f0f0);
  x=    ((x>> 2)&0x33333333) | ((x<< 2)&0xcccccccc);
  return((x>> 1)&0x55555555) | ((x<< 1)&0xaaaaaaaa);
}

STIN long decode_packed_entry_number(dec_codebook *book, oggpack_buffer *b){
  ogg_uint32_t testword;
  int  read=book->maxlength;
  long lo,hi;
  long lok = oggpack_look(b,book->firsttablen);

  if (lok >= 0) {
    long entry = book->firsttable[lok];
    if(entry&0x80000000UL){
      lo=((entry>>15)&0x7fff)<<book->hint_shift;
      hi=book->hi_max-((entry&0x7fff)<<book->hint_shift);
    }else{
      oggpack_adv(b, (int)(entry&0x3f));
      return(entry>>6);
    }
  }else{
    lo=0;
    hi=book->hi_max;
  }

  /* Single entry codebooks use a firsttablen of 1 and a
     dec_maxlength of 1.  If a single-entry codebook gets here (due to
     failure to read one bit above), the next look attempt will also
     fail and we'll correctly kick out instead of trying to walk the
     underformed tree */

  lok = oggpack_look(b, read);

  while(lok<0 && read>1)
    lok = oggpack_look(b, --read);
  if(lok<0)return -1;

  testword=bitreverse((ogg_uint32_t)lok);
  if(book->codelengths==NULL){
    int length;
    /* ordered codebook: search for the codeword length */
    while(testword>book->codelist[lo])lo++;
    length=(int)lo+book->minlength;
    if(length<=read){
      long entry=
       book->index[lo]-(((book->codelist[lo]-testword)>>(32-length))+1);
      oggpack_adv(b, length);
      return(entry);
    }
  }else{
    /* bisect search for the codeword in the ordered list */
    while(hi-lo>1){
      long p=(hi-lo)>>1;
      long test=book->codelist[lo+p]>testword;
      lo+=p&(test-1);
      hi-=p&(-test);
      }

    if(book->codelengths[lo]<=read){
      oggpack_adv(b, book->codelengths[lo]);
      return(lo);
    }
  }

  oggpack_adv(b, read);

  return(-1);
}

/* Decode side is specced and easier, because we don't need to find
   matches using different criteria; we simply read and map.  There are
   two things we need to do 'depending':

   We may need to support interleave.  We don't really, but it's
   convenient to do it here rather than rebuild the vector later.

   Cascades may be additive or multiplicitive; this is not inherent in
   the codebook, but set in the code using the codebook.  Like
   interleaving, it's easiest to do it here.
   addmul==0 -> declarative (set the value)
   addmul==1 -> additive
   addmul==2 -> multiplicitive */

/* returns the [original, not compacted] entry number or -1 on eof *********/
long vorbis_book_decode(dec_codebook *book, oggpack_buffer *b){
  if(book->codelist){
    long packed_entry=decode_packed_entry_number(book,b);
    if(packed_entry>=0){
      /* if we have an unordered book, look up the original entry number */
      if(book->codelengths)
        return(book->index[packed_entry]);
      /* if we have an ordered book, the packed_entry number is the original */
      return(packed_entry);
    }
  }

  /* if there's no codelist, the codebook hasn't been init'd */
  return(-1);
}

/* returns 0 on OK or -1 on eof *************************************/
/* decode vector / dim granularity gaurding is done in the upper layer */
long vorbis_book_decodevs_add(dec_codebook *book,float *a,oggpack_buffer *b,
                              int n){
  if(book->codelist){
    int step=n/book->dim;
    long *entry = alloca(sizeof(*entry)*step);
    float **t = alloca(sizeof(*t)*step);
    int i,j,o;

    for (i = 0; i < step; i++) {
      entry[i]=decode_packed_entry_number(book,b);
      if(entry[i]==-1)return(-1);
      t[i] = book->valuelist+entry[i]*book->dim;
    }
    for(i=0,o=0;i<book->dim;i++,o+=step)
      for (j=0;o+j<n && j<step;j++)
        a[o+j]+=t[j][i];
  }
  return(0);
}

/* decode vector / dim granularity gaurding is done in the upper layer */
long vorbis_book_decodev_add(dec_codebook *book,float *a,oggpack_buffer *b,
                             int n){
  if(book->codelist){
    int i,j,entry;
    float *t;

    for(i=0;i<n;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      for(j=0;i<n && j<book->dim;)
        a[i++]+=t[j++];
    }
  }
  return(0);
}

/* unlike the others, we guard against n not being an integer number
   of <dim> internally rather than in the upper layer (called only by
   floor0) */
long vorbis_book_decodev_set(dec_codebook *book,float *a,oggpack_buffer *b,
                             int n){
  if(book->codelist){
    int i,j,entry;
    float *t;

    for(i=0;i<n;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      t     = book->valuelist+entry*book->dim;
      for (j=0;i<n && j<book->dim;){
        a[i++]=t[j++];
      }
    }
  }else{
    int i;

    for(i=0;i<n;){
      a[i++]=0.f;
    }
  }
  return(0);
}

long vorbis_book_decodevv_add(dec_codebook *book,float **a,long offset,int ch,
                              oggpack_buffer *b,int n){

  long i,j,entry;
  int chptr=0;
  if(book->codelist){
    int m=(offset+n)/ch;
    for(i=offset/ch;i<m;){
      entry = decode_packed_entry_number(book,b);
      if(entry==-1)return(-1);
      {
        const float *t = book->valuelist+entry*book->dim;
        for (j=0;i<m && j<book->dim;j++){
          a[chptr++][i]+=t[j];
          if(chptr==ch){
            chptr=0;
            i++;
          }
        }
      }
    }
  }
  return(0);
}
