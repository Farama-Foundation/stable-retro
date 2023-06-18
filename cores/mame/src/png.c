/*********************************************************************

  png.c

  PNG reading functions.

  07/15/1998 Created by Mathis Rosenhauer
  10/02/1998 Code clean up and abstraction by Mike Balfour
             and Mathis Rosenhauer
  10/15/1998 Image filtering. MLR
  11/09/1998 Bit depths 1-8 MLR
  11/10/1998 Some additional PNG chunks recognized MLR
  05/14/1999 Color type 2 and PNG save functions added
  05/15/1999 Handle RGB555 while saving, use mame_fxxx
             functions for writing MSH
  04/27/2001 Simple MNG support MLR

  TODO : Fully comply with the "Recommendations for Decoders"
         of the W3C

*********************************************************************/

#include <math.h>
#include <compat/zlib.h>
#include "driver.h"
#include "png.h"


/* convert_uint is here so we don't have to deal with byte-ordering issues */
static UINT32 convert_from_network_order (UINT8 *v)
{
	UINT32 i;

	i = (v[0]<<24) | (v[1]<<16) | (v[2]<<8) | (v[3]);
	return i;
}

int png_unfilter(struct png_info *p)
{
	int i, j, bpp, filter;
	INT32 prediction, pA, pB, pC, dA, dB, dC;
	UINT8 *src, *dst;

	if((p->image = (UINT8 *)malloc (p->height*p->rowbytes))==NULL)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "Out of memory\n");
		free (p->fimage);
		return 0;
	}

	src = p->fimage;
	dst = p->image;
	bpp = p->bpp;

	for (i=0; i<p->height; i++)
	{
		filter = *src++;
		if (!filter)
		{
			memcpy (dst, src, p->rowbytes);
			src += p->rowbytes;
			dst += p->rowbytes;
		}
		else
			for (j=0; j<p->rowbytes; j++)
			{
				pA = (j<bpp) ? 0: *(dst - bpp);
				pB = (i<1) ? 0: *(dst - p->rowbytes);
				pC = ((j<bpp)||(i<1)) ? 0: *(dst - p->rowbytes - bpp);

				switch (filter)
				{
				case PNG_PF_Sub:
					prediction = pA;
					break;
				case PNG_PF_Up:
					prediction = pB;
					break;
				case PNG_PF_Average:
					prediction = ((pA + pB) / 2);
					break;
				case PNG_PF_Paeth:
					prediction = pA + pB - pC;
					dA = abs(prediction - pA);
					dB = abs(prediction - pB);
					dC = abs(prediction - pC);
					if (dA <= dB && dA <= dC) prediction = pA;
					else if (dB <= dC) prediction = pB;
					else prediction = pC;
					break;
				default:
					log_cb(RETRO_LOG_INFO, LOGPRE "Unknown filter type %i\n",filter);
					prediction = 0;
				}
				*dst = 0xff & (*src + prediction);
				dst++; src++;
			}
	}

	free (p->fimage);
	return 1;
}

int png_verify_signature (mame_file *fp)
{
	INT8 signature[8];

	if (mame_fread (fp, signature, 8) != 8)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "Unable to read PNG signature (EOF)\n");
		return 0;
	}

	if (memcmp(signature, PNG_Signature, 8))
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "PNG signature mismatch found: %s expected: %s\n",signature,PNG_Signature);
		return 0;
	}
	return 1;
}

int png_inflate_image (struct png_info *p)
{
	unsigned long fbuff_size;

	fbuff_size = p->height * (p->rowbytes + 1);

	if((p->fimage = (UINT8 *)malloc (fbuff_size))==NULL)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "Out of memory\n");
		free (p->zimage);
		return 0;
	}

	if (uncompress(p->fimage, &fbuff_size, p->zimage, p->zlength) != Z_OK)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "Error while inflating image\n");
		return 0;
	}

	free (p->zimage);
	return 1;
}

int png_read_file(mame_file *fp, struct png_info *p)
{
	/* translates color_type to bytes per pixel */
	const int samples[] = {1, 0, 3, 1, 2, 0, 4};

	UINT32 chunk_length, chunk_type=0, chunk_crc, crc;
	UINT8 *chunk_data, *temp;
	UINT8 str_chunk_type[5], v[4];

	struct idat
	{
		struct idat *next;
		int length;
		UINT8 *data;
	} *ihead, *pidat;

	if ((ihead = malloc (sizeof(struct idat))) == 0)
		return 0;

	pidat = ihead;

	p->zlength = 0;
	p->num_palette = 0;
	p->num_trans = 0;
	p->trans = NULL;
	p->palette = NULL;

	if (png_verify_signature(fp)==0)
		return 0;

	while (chunk_type != PNG_CN_IEND)
	{
		if (mame_fread(fp, v, 4) != 4)
			log_cb(RETRO_LOG_INFO, LOGPRE "Unexpected EOF in PNG\n");
		chunk_length=convert_from_network_order(v);

		if (mame_fread(fp, str_chunk_type, 4) != 4)
			log_cb(RETRO_LOG_INFO, LOGPRE "Unexpected EOF in PNG file\n");

		str_chunk_type[4]=0; /* terminate string */

		crc=crc32(0,str_chunk_type, 4);
		chunk_type = convert_from_network_order(str_chunk_type);

		if (chunk_length)
		{
			if ((chunk_data = (UINT8 *)malloc(chunk_length+1))==NULL)
			{
				log_cb(RETRO_LOG_INFO, LOGPRE "Out of memory\n");
				return 0;
			}
			if (mame_fread (fp, chunk_data, chunk_length) != chunk_length)
			{
				log_cb(RETRO_LOG_INFO, LOGPRE "Unexpected EOF in PNG file\n");
				free(chunk_data);
				return 0;
			}

			crc=crc32(crc,chunk_data, chunk_length);
		}
		else
			chunk_data = NULL;

		if (mame_fread(fp, v, 4) != 4)
			log_cb(RETRO_LOG_INFO, LOGPRE "Unexpected EOF in PNG\n");
		chunk_crc=convert_from_network_order(v);

		if (crc != chunk_crc)
		{
			log_cb(RETRO_LOG_INFO, LOGPRE "CRC check failed while reading PNG chunk %s\n",str_chunk_type);
			log_cb(RETRO_LOG_INFO, LOGPRE "Found: %08X  Expected: %08X\n",crc,chunk_crc);
			return 0;
		}

		log_cb(RETRO_LOG_INFO, LOGPRE "Reading PNG chunk %s\n", str_chunk_type);

		switch (chunk_type)
		{
		case PNG_CN_IHDR:
			p->width = convert_from_network_order(chunk_data);
			p->height = convert_from_network_order(chunk_data+4);
			p->bit_depth = *(chunk_data+8);
			p->color_type = *(chunk_data+9);
			p->compression_method = *(chunk_data+10);
			p->filter_method = *(chunk_data+11);
			p->interlace_method = *(chunk_data+12);
			free (chunk_data);

			log_cb(RETRO_LOG_INFO, LOGPRE "PNG IHDR information:\n");
			log_cb(RETRO_LOG_INFO, LOGPRE "Width: %i, Height: %i\n", p->width, p->height);
			log_cb(RETRO_LOG_INFO, LOGPRE "Bit depth %i, color type: %i\n", p->bit_depth, p->color_type);
			logerror("Compression method: %i, filter: %i, interlace: %i\n",
					p->compression_method, p->filter_method, p->interlace_method);
			break;

		case PNG_CN_PLTE:
			p->num_palette=chunk_length/3;
			p->palette=chunk_data;
			log_cb(RETRO_LOG_INFO, LOGPRE "%i palette entries\n", p->num_palette);
			break;

		case PNG_CN_tRNS:
			p->num_trans=chunk_length;
			p->trans=chunk_data;
			log_cb(RETRO_LOG_INFO, LOGPRE "%i transparent palette entries\n", p->num_trans);
			break;

		case PNG_CN_IDAT:
			pidat->data = chunk_data;
			pidat->length = chunk_length;
			if ((pidat->next = malloc (sizeof(struct idat))) == 0)
				return 0;
			pidat = pidat->next;
			pidat->next = 0;
			p->zlength += chunk_length;
			break;

		case PNG_CN_tEXt:
			{
				char *text = (char *)chunk_data;

				while(*text++) ;
				chunk_data[chunk_length]=0;
 				log_cb(RETRO_LOG_INFO, LOGPRE "Keyword: %s\n", chunk_data);
				log_cb(RETRO_LOG_INFO, LOGPRE "Text: %s\n", text);
			}
			free(chunk_data);
			break;

		case PNG_CN_tIME:
			{
				UINT8 *t=chunk_data;
				logerror("Image last-modification time: %i/%i/%i (%i:%i:%i) GMT\n",
					((short)(*t) << 8)+ (short)(*(t+1)), *(t+2), *(t+3), *(t+4), *(t+5), *(t+6));
			}

			free(chunk_data);
			break;

		case PNG_CN_gAMA:
			p->source_gamma	 = convert_from_network_order(chunk_data)/100000.0;
			log_cb(RETRO_LOG_INFO, LOGPRE  "Source gamma: %f\n",p->source_gamma);

			free(chunk_data);
			break;

		case PNG_CN_pHYs:
			p->xres = convert_from_network_order(chunk_data);
			p->yres = convert_from_network_order(chunk_data+4);
			p->resolution_unit = *(chunk_data+8);
			log_cb(RETRO_LOG_INFO, LOGPRE "Pixel per unit, X axis: %i\n",p->xres);
			log_cb(RETRO_LOG_INFO, LOGPRE "Pixel per unit, Y axis: %i\n",p->yres);
			if (p->resolution_unit)
				log_cb(RETRO_LOG_INFO, LOGPRE "Unit is meter\n");
			else
				log_cb(RETRO_LOG_INFO, LOGPRE "Unit is unknown\n");
			free(chunk_data);
			break;

		case PNG_CN_IEND:
			break;

		default:
			if (chunk_type & 0x20000000)
				log_cb(RETRO_LOG_INFO, LOGPRE "Ignoring ancillary chunk %s\n",str_chunk_type);
			else
				log_cb(RETRO_LOG_INFO, LOGPRE "Ignoring critical chunk %s!\n",str_chunk_type);
			if (chunk_data)
				free(chunk_data);
			break;
		}
	}
	if ((p->zimage = (UINT8 *)malloc(p->zlength))==NULL)
	{
		log_cb(RETRO_LOG_INFO, LOGPRE "Out of memory\n");
		return 0;
	}

	/* combine idat chunks to compressed image data */
	temp = p->zimage;
	while (ihead->next)
	{
		pidat = ihead;
		memcpy (temp, pidat->data, pidat->length);
		free (pidat->data);
		temp += pidat->length;
		ihead = pidat->next;
		free (pidat);
	}
	p->bpp = (samples[p->color_type] * p->bit_depth) / 8;
	p->rowbytes = ceil((p->width * p->bit_depth * samples[p->color_type]) / 8.0);

	if (png_inflate_image(p)==0)
		return 0;

	if(png_unfilter (p)==0)
		return 0;

	return 1;
}

/*	Expands a p->image from p->bit_depth to 8 bit */
int png_expand_buffer_8bit (struct png_info *p)
{
	int i,j, k;
	UINT8 *inp, *outp, *outbuf;

	if (p->bit_depth < 8)
	{
		if ((outbuf = (UINT8 *)malloc(p->width*p->height))==NULL)
		{
			log_cb(RETRO_LOG_INFO, LOGPRE "Out of memory\n");
			return 0;
		}

		inp = p->image;
		outp = outbuf;

		for (i = 0; i < p->height; i++)
		{
			for(j = 0; j < p->width / ( 8 / p->bit_depth); j++)
			{
				for (k = 8 / p->bit_depth-1; k >= 0; k--)
					*outp++ = (*inp >> k * p->bit_depth) & (0xff >> (8 - p->bit_depth));
				inp++;
			}
			if (p->width % (8 / p->bit_depth))
			{
				for (k = p->width % (8 / p->bit_depth)-1; k >= 0; k--)
					*outp++ = (*inp >> k * p->bit_depth) & (0xff >> (8 - p->bit_depth));
				inp++;
			}
		}
		free (p->image);
		p->image = outbuf;
	}
	return 1;
}

void png_delete_unused_colors (struct png_info *p)
{
	int i, tab[256], pen=0, trns=0;
	UINT8 ptemp[3*256], ttemp[256];

	memset (tab, 0, 256*sizeof(int));
	memcpy (ptemp, p->palette, 3*p->num_palette);
	memcpy (ttemp, p->trans, p->num_trans);

	/* check which colors are actually used */
	for (i = 0; i < p->height*p->width; i++)
		tab[p->image[i]]++;

	/* shrink palette and transparency */
	for (i = 0; i < p->num_palette; i++)
		if (tab[i])
		{
			p->palette[3*pen+0]=ptemp[3*i+0];
			p->palette[3*pen+1]=ptemp[3*i+1];
			p->palette[3*pen+2]=ptemp[3*i+2];
			if (i < p->num_trans)
			{
				p->trans[pen] = ttemp[i];
				trns++;
			}
			tab[i] = pen++;
		}

	/* remap colors */
	for (i = 0; i < p->height*p->width; i++)
		p->image[i]=tab[p->image[i]];

	if (p->num_palette!=pen)
		log_cb(RETRO_LOG_INFO, LOGPRE "%i unused pen(s) deleted\n", p->num_palette-pen);

	p->num_palette = pen;
	p->num_trans = trns;
}
