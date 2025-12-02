//============================================
//=== Audio XA decoding
//=== Kazzuya
//============================================

typedef struct
{
	int	y0, y1;
} ADPCM_Decode_t;

typedef struct
{                                                                   
	int				freq;
	int				nbits;
	int				stereo;
	int				nsamples;
	ADPCM_Decode_t	left, right;
	short			pcm[16384];
} xa_decode_t;



