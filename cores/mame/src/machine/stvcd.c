/***************************************************************************

  machine/stvcd.c

	-- we should adapt this to use the .cue / .bin or whatever format we
	decide to keep the cd in once we have one

	since sports fishing 2 is the only st-v game to require the cd, is an
	incomplete dump, and will also require the mpeg decoder it seems fairly
	unlikely this will be done soon

***************************************************************************/

#include "driver.h"
#include "machine/stvcd.h"
#include <stdio.h>

UINT8 	CD_cr_first;
UINT8 	CD_cr_writing;
UINT16	CR1;
UINT16	CR2;
UINT16	CR3;
UINT16	CR4;
UINT16	CD_hirq;
UINT16 	CD_hirq_i;
UINT16  CD_mask;
UINT8	CD_status;
UINT32	CD_fad;
UINT8	CD_track;
UINT8	CD_control;
UINT8	CD_index;

UINT32 fn;

INT32	CD_com;				/* last command being processed*/
INT32	CD_com_play;			/* last play command*/
UINT8	CD_stat;				/* drive status*/
UINT8	CD_flag;				/* 0x00 = CD-DA or <SEEK> or <SCAN> 0x80 = CD-ROM*/


UINT32	CD_last_part;			/* last buffer partition accessed*/
filt_t	CD_filt[CDB_SEL_NUM];		/* filters*/

UINT32	CD_play_fad;			/* play start address*/
UINT32	CD_play_range;			/* play range*/
UINT32	CD_seek_target;			/* seek target address*/
UINT8	CD_scan_dir;			/* scan direction*/
UINT32	CD_search_pn;			/* search result, partition number*/
UINT32	CD_search_sp;			/* search result, sector position*/
UINT32	CD_search_fad;			/* search result, fad*/
UINT32	CD_file_scope_first;
UINT32	CD_file_scope_last;


UINT32	CD_data_pn;			/* data transfer partition number*/
UINT32	CD_data_sp;			/* data transfer sector position*/
UINT32	CD_data_sn;			/* data transfer sector number*/
UINT32	CD_data_count;			/* data transfer current byte count*/
UINT32	CD_data_delete;			/* data must be deleted upon read*/
UINT32	CD_data_size;			/* data transfer size in bytes*/

char * 	CD_info_ptr;			/* info transfer buffer pointer*/
UINT32	CD_info_count;			/* info transfer byte count*/
UINT32	CD_info_size;			/* info transfer total byte count*/

UINT32	CD_trans_type;			/* 0 = DATA, 1 = INFO   */ /*maybe signed int*/

UINT32	CD_actual_size;			/* used by "calcactualsize" and "getactualsize"*/

/******************************************************************/

sect_t	CD_sect[CDB_SECT_NUM];	/* sector buffer*/
part_t	CD_part[CDB_SEL_NUM];	/* buffer partitions*/
filt_t	CD_filt[CDB_SEL_NUM];	/* filters*/
UINT32	CD_free_space;		/* free space in sector units*/

UINT8	CD_filt_num;		/* cdrom drive connector*/
UINT8	CD_mpeg_filt_num;	/* mpeg connector*/

/******************************************************************/


UINT32	CD_cur_fad;			/* current pickup position info*/
UINT32	CD_cur_track;			/**/
UINT32	CD_cur_ctrl;			/**/
UINT32	CD_cur_idx;			/**/
UINT32	CD_cur_fid;			/**/



char	cdb_sat_file_info[254 * 12];	/* current file info*/
char cdb_sat_subq[5 * 2];		/* current subcode q*/
char cdb_sat_subrw[12 * 2];		/* current subcode r~w*/
char CD_sat_subq[5 * 2];			/* current subcode q*/
char CD_sat_subrw[12 * 2];		/* current subcode r~w*/
toc_t	CD_toc;			/* disc toc*/
file_t	CD_file[CDB_FID_NUM];	/* file table (directory table)*/
UINT32	CD_file_num;		/* total file infos stored*/
char	CD_sat_toc[408];	/* current cdrom toc*/


UINT8	CD_init_flag;
UINT8	CD_flag;			/* 0x00 = CD-DA or <SEEK> or <SCAN> 0x80 = CD-ROM*/
UINT32	CD_repeat;			/* repeat frequency*/
UINT32	CD_standby;			/* standby wait*/
UINT32	CD_repeat_max;			/* max repeat frequency*/
UINT8	CD_ecc;
UINT32	CD_drive_speed;			/* 0 = noop, 1 = 1x, 2 = 2x*/


UINT8			cdda_buff[8192];		/* CD-DA buffer for SCSP communication*/
UINT32		cdda_pos;




/******************************************************************/

#define ISO_BUFF_SIZE	(2352 * 16)	/* cache size*/

/******************************************************************/

 char	* iso_buff = NULL;
static FILE	* iso_file = NULL;
static UINT32	iso_media_present;
static UINT32	iso_mp3_init = 0;

UINT32	cdb_get_sect_size	= 2048;
UINT32	cdb_put_sect_size	= 2048;


INT32 fsize(FILE * f){

 INT32 size=0;

 if(f != NULL)
 {

   fseek(f, 0, SEEK_END);
   size = ftell(f);
   fseek(f, 0, SEEK_SET);
 }
    return(size);
}

static struct {

	INT32	size;			/* negative value means not present*/
	UINT32	ctrl;			/* control*/
	UINT32	idx;			/* index*/
	UINT32	type;			/* iso : sector size*/
	UINT32	off;			/* bin : offset info iso_file*/
	UINT32	fad;			/* position on the disc*/
	UINT32	len;			/* length (in sectors)*/
	char	path[256];		/* file path*/

}		iso_track[100],
		iso_leadout;

static UINT32	iso_track_first;
static UINT32	iso_track_last;
static UINT32	iso_track_num;

static INT32	iso_size;		/* < 0 means not present*/
static UINT32	iso_type;		/* 0 = ISO 1 = BIN*/

/******************************************************************/
/* CD Block Interface*/



 void cdb_inject_file_info(UINT32 fid, UINT8 * dst){

	/* converts standard cdb file info in saturn format*/
	/* and 'injects' it in the supplied destination. ;)*/

	dst[0x0] = CD_file[fid].fad >> 24;
	dst[0x1] = CD_file[fid].fad >> 16;
	dst[0x2] = CD_file[fid].fad >> 8;
	dst[0x3] = CD_file[fid].fad;

	dst[0x4] = CD_file[fid].size >> 24;
	dst[0x5] = CD_file[fid].size >> 16;
	dst[0x6] = CD_file[fid].size >> 8;
	dst[0x7] = CD_file[fid].size;

	dst[0x8] = CD_file[fid].gap;
	dst[0x9] = CD_file[fid].unit;
	dst[0xa] = fid;
	dst[0xb] = CD_file[fid].attr;
}

UINT32 cdb_find_track(UINT32 fad){

	/* finds the track that contains the specified fad*/

	int i;

	for(i = CD_toc.first.num-1; i < CD_toc.last.num-1; i++)
		if(CD_toc.track[i].fad <= fad && CD_toc.track[i+1].fad > fad)
			return(i+1);

	if(fad && CD_toc.leadout.fad > fad)
		return(CD_toc.last.num);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: no track for the poor fad %x\n", fad);
	exit(1);

	return((INT32)-1);
}

 UINT32 cdb_find_file(UINT32 fad){

	/* finds the file that contains the specified fad*/

	int i;

	for(i = 0; i < CDB_FID_NUM; i++)
		if((CD_file[i].fad <= fad) &&
		   ((CD_file[i].fad + (CD_file[i].size + 2047) / 2048) > fad))
			return(i+2);

	return(0); /* no file found (audio track) - not an error!*/
}

 UINT32 cdb_find_dest(UINT32 fnstv, UINT32 * pn){

	/* finds the sector data contained in *pn destination*/
	/* according to the filters it must pass through.*/

	filt_t * f;
	UINT32 cond;

	f = &CD_filt[fnstv];

	do{
		cond = 0;

		if(f->mode & CDB_FILTMODE_RANGE){

			if((CD_cur_fad < f->fad) ||
			   (CD_cur_fad >= (f->fad + f->range)))
				cond = 1;
		}

		if(f->mode & CDB_FILTMODE_COD){

			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: cod check required\n");
			exit(1);
		}

		if(f->mode & CDB_FILTMODE_SUB){

			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: sub check required\n");
			exit(1);
		}

		if(f->mode & CDB_FILTMODE_CHAN){

			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: chan check required\n");
			exit(1);
		}

		if(f->mode & CDB_FILTMODE_FID){

			if(f->fid) /* only if fid is valid*/
				if(CD_cur_fid != f->fid) cond = 1;
		}

		if(cond == 0){

			/* all checks passed, data is okay*/

			if(f->true_cond == 0xff) /* disconnected*/
				return(1); /* discard data*/

			*pn = (UINT32)f->true_cond;
			return(0);
		}

		if(f->false_cond != 0xff)
			f = &CD_filt[f->false_cond];

	}while(f->false_cond != 0xff);

	return(1);
}

UINT32 cdb_make_room(UINT32 pn){

	int i;

	/* finds the first available free sector and*/
	/* allocates it for use by the curent CDROM play*/
	/* operation.*/

	for(i = 0; i < 200; i++){

		if(CD_sect[i].size == 0){

			CD_sect[i].size = 2048;
			CD_part[pn].sect[CD_part[pn].size] = &CD_sect[i];
			CD_part[pn].size++;
			CD_free_space--;

			return(i);
		}
	}

	/* should never happen, BFUL prevents it*/

	log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: cdb_make_room found no free sector\n");
	exit(1);
}














UINT32 iso_find_track(UINT32 fad){

	int i;

	for(i = iso_track_first-1; i < iso_track_last; i++)
		if(iso_track[i].fad <= fad && iso_track[i+1].fad > fad)
			return(i+1);

	if(iso_leadout.fad > fad)
		return(iso_track_last);

	return((INT32)-1);
}

UINT32 iso_read_sector(UINT32 mode, UINT32 fad, UINT8 * dst){

	static char buff[2352];

	log_cb(RETRO_LOG_DEBUG, LOGPRE "mode = %i fad = %i ", mode, fad);

	if(iso_type == 0){

		/* ISO*/

		UINT32 tn;
		FILE * f;

		tn = iso_find_track(fad);

		log_cb(RETRO_LOG_DEBUG, LOGPRE "track = %i ", tn);

		f = fopen(iso_track[tn-1].path, "rb");
		if(f == NULL){
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: couldn't open %s\n", iso_track[tn-1].path);
			exit(1);
		}

		logerror( "reading fad:%x off:%x tn:%i from %s\n",
		fad, fad - iso_track[tn-1].fad, tn, iso_track[tn-1].path);

		fseek(f, (fad - iso_track[tn-1].fad) * 2048, SEEK_SET); /* 2352*/
		if(fread(buff, 1, 2352, f) != 2352){
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: couldn't read from iso (fad = %06x)\n", fad);
			exit(1);
		}

		fclose(f);

		if(iso_track[tn-1].type != 0){

			/* if CDDA or FORM 2 CDROM*/

			log_cb(RETRO_LOG_DEBUG, LOGPRE " [2352] : %i\n", (fad - 150) * 2352);

			if(mode == 0)
				memcpy(dst, &buff[16], 2048);
			else
				memcpy(dst, buff, 2352);

			return(mode);

		}else{

			/* CDROM*/

			log_cb(RETRO_LOG_DEBUG, LOGPRE " [2048] : %i\n", (fad - 150) * 2048);

			if(mode == 0)
				memcpy(dst, buff, 2048);
			else
				memcpy(&dst[16], buff, 2048);

			return(0);
		}
	}

	/* BIN*/

	return(0);
}

void iso_seek_sector(UINT32 fad){

}

/********************************************************/

UINT32 iso_get_track_info(UINT32 tn, UINT32 * ctrl, UINT32 * idx, UINT32 * fad){

	if(tn < iso_track_first || tn > iso_track_last)
		return(1);

	*fad	= iso_track[tn-1].fad;
	*ctrl	= iso_track[tn-1].ctrl;
	*idx	= iso_track[tn-1].idx;

	return(0);
}

void iso_get_leadout_info(UINT32 * ctrl, UINT32 * idx, UINT32 * fad){

	*fad	= iso_leadout.fad;
	*ctrl	= iso_track[iso_track_last-1].ctrl;
	*idx	= iso_track[iso_track_last-1].idx;
}

UINT32 iso_get_first_track(void){	return(iso_track_first); }
UINT32 iso_get_last_track(void){		return(iso_track_last); }

/********************************************************/

UINT32 iso_get_status(void){

	UINT32 stat;

	stat = 0;

/*	if(iso_media_present) stat |= CDSTAT_MEDIA_PRESENT;*/

/*	stat |= CDSTAT_CAN_RAW_AUDIO;*/

	return(stat);
}

/********************************************************/
/* Local Procedures*/

static void iso_build_disc_bin(void){

/*	log_cb(RETRO_LOG_DEBUG, LOGPRE "loading BIN: %s\n", config.cdrom_image);*/
	exit(1);
}

static void iso_build_disc_iso(void){

	/*char b[2048];*/
	char s[256], t[256];
	FILE * f;
	int i, j;
	UINT32 fad;

/*	char fmt[][12] = {*/
/*		"%02d.iso", "-%02d.iso", "_%02d.iso", " %02d.iso",*/
/*		"%02d.wav", "-%02d.wav", "_%02d.wav", " %02d.wav",*/
/*		"%02d.mp3", "-%02d.mp3", "_%02d.mp3", " %02d.mp3",*/
/*	};*/

	char fmt[3][12] = {
		"%02d.iso",
		"%02d.wav",
		"%02d.mp3",
	};

	strcpy(s, "roms/sfish2");
	strcat(s, "/track_");

	fad = 150;

	iso_track_first = 100;
	iso_track_last = 1;

	for(i = 1; i < 100; i++){

		for(j = 0; j < 3; j++){

			strcpy(t, s);
			strcat(t, fmt[j]);
			sprintf(t, t, i);

			f = fopen(t, "rb");
			if(f != NULL){

				log_cb(RETRO_LOG_DEBUG, LOGPRE "found track : %s\n", t);

				if(iso_track_first > i) iso_track_first = i;
				if(iso_track_last  < i) iso_track_last  = i;

				if(i == 1)
					iso_media_present = 1;

				if(j == 0){

					/* ISO file*/

					if(i == 1){
/*
						// first track
						log_cb(RETRO_LOG_DEBUG, LOGPRE "1, lunghezza %d, iso buff contiene %s\n",fsize(f),iso_buff);
						fseek(f, 0, SEEK_SET);
						int pp=fread(iso_buff, 1, 0x110, f);
						fseek(f, 0, SEEK_SET);
						log_cb(RETRO_LOG_DEBUG, LOGPRE "2\n");
						log_cb(RETRO_LOG_DEBUG, LOGPRE "%d\t%s\n",pp,iso_buff);

						if(strncmp(&iso_buff[0x00], "SEGA SEGASATURN ", 16)){
							log_cb(RETRO_LOG_DEBUG, LOGPRE "2b\n");
							// ISO 2048-bytes data track

							//setup_game_info(&iso_buff[0x00]);
							//setup_area_symbol(iso_buff[0x40]);

						}else
						if(strncmp(&iso_buff[0x10], "SEGA SEGASATURN ", 16)){

							// ISO 2352-bytes data track

							//setup_game_info(&iso_buff[0x10]);
							//setup_area_symbol(iso_buff[0x50]);

							iso_track[i-1].type = 1;

						}else{

							log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: unknown track %i format (file: %s)\n", i, t);
							exit(1);
						}
						log_cb(RETRO_LOG_DEBUG, LOGPRE "3\n");
						memset(iso_buff, 0x00, 0x110);

						iso_track[i-1].type = 0; // 2048-bytes
*/
					}else{

						/* non-first track*/
						/* assume ISO 2352-bytes data track*/

						iso_track[i-1].type = 1; /* 2352-bytes*/
					}

					iso_track[i-1].size	= fsize(f);
					iso_track[i-1].ctrl	= 4;
					iso_track[i-1].idx	= 1;
					iso_track[i-1].off	= 0;
					iso_track[i-1].fad	= fad;
					iso_track[i-1].len	= (iso_track[i-1].size + 2047) / 2048;

				}else
				if(j == 1){

					/* WAV file*/

					iso_track[i-1].size	= fsize(f);
					iso_track[i-1].ctrl	= 1;
					iso_track[i-1].idx	= 1;
					iso_track[i-1].type	= 1; /* WAV, 2352-bytes*/
					iso_track[i-1].off	= 0;
					iso_track[i-1].fad	= fad;
					iso_track[i-1].len	= (fsize(f) + 2047) / 2048;

				}else{

					/* MP3 file*/

					if(!iso_mp3_init){
						/* mp3_init();*/
						iso_mp3_init = 1;
					}

					iso_track[i-1].size	= fsize(f); /* fixme*/
					iso_track[i-1].ctrl	= 1;
					iso_track[i-1].idx	= 1;
					iso_track[i-1].type	= 2; /* MP3*/
					iso_track[i-1].off	= 0;
					iso_track[i-1].fad	= fad;
					iso_track[i-1].len	= (fsize(f) + 2047) / 2048; /* fixme*/
				}

				strcpy(iso_track[i-1].path, t);

				fad += iso_track[i-1].len;

				fclose(f);
			}
		}
	}

	iso_leadout.fad = iso_track[iso_track_last-1].fad+iso_track[iso_track_last-1].len + 150; /* 2 sec after the last track*/

	iso_track_num = (iso_track_last - (iso_track_first - 1));

	iso_type = 0;
}

/********************************************************/

void iso_shutdown(void){

	if(iso_buff != NULL){ free(iso_buff); iso_buff = NULL; }
	if(iso_file != NULL){ fclose(iso_file); iso_file = NULL; }
}

void iso_reset(void){

	int i;

	iso_media_present = 0;
	iso_size = -1;
	iso_type = 0;

	iso_shutdown();

	for(i = 0; i < 100; i++){

		iso_track[i].size	= -1;
		iso_track[i].ctrl	= 0;
		iso_track[i].idx	= 0;
		iso_track[i].off	= 0;
		iso_track[i].fad	= 0;
		iso_track[i].len	= 0;

		strcpy(iso_track[i].path, "NULL");
	}

	iso_build_disc_iso();	/* supposed ISO image*/
}

void iso_init(void){

}



UINT32 FAD_TO_MIN(UINT32 fad){
/*
	UINT32 tmp=fad/75;
	UINT32 tmp2=0;
	while (tmp>60){
        tmp -=60;
		tmp2++;
	}
	return(tmp2);
*/
	return (fad/4500);
}

UINT32 FAD_TO_SEC(UINT32 fad){
/*
	UINT32 tmp=fad/75;
	while (tmp>60){
        tmp -=60;
	}
	return(tmp);
*/
	return( (fad/75) %60);
}

UINT32 FAD_TO_FRA(UINT32 fad){

	return(fad%75);
}

void cdb_build_toc(void){

	UINT32 ctrl, idx, fad;
	int i;
	int oo=0;

	memset(CD_sat_toc, 0xff, 0xcc*2);
	memset(&CD_toc.track, 0xff, sizeof(track_t)*100);

	CD_toc.first.num	= iso_get_first_track();
	CD_toc.last.num		= iso_get_last_track();

	logerror(
		"%i tracks found (first:%i last:%i)\n",
		(CD_toc.last.num - (CD_toc.first.num - 1)),
		CD_toc.first.num,
		CD_toc.last.num
	);

	for(i = CD_toc.first.num-1; i <= CD_toc.last.num-1; i++){

		if(iso_get_track_info(i+1, &ctrl, &idx, &fad)){
			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: error on cdb_build_toc, iso_get_track_info tn=%i\n", i+1);
			exit(1);
		}

		CD_toc.track[i].ctrl	= (ctrl << 4);
		CD_toc.track[i].idx	= idx;
		CD_toc.track[i].fad	= fad;
		CD_toc.track[i].min	= FAD_TO_MIN(fad);
		CD_toc.track[i].sec	= FAD_TO_SEC(fad);
		CD_toc.track[i].fra	= FAD_TO_FRA(fad);

		CD_sat_toc[(i*4)+0]	= (ctrl << 4) | idx;
		CD_sat_toc[(i*4)+1]	= (UINT8)((UINT32)fad >> 16);
		CD_sat_toc[(i*4)+2]	= (UINT8)((UINT32)fad >> 8);
		CD_sat_toc[(i*4)+3]	= (UINT8)((UINT32)fad);

		logerror("track#%02i: %02i:%02i:%02i (addr: %i ctrl:%i idx:%i)\n",
		i+1, FAD_TO_MIN(fad), FAD_TO_SEC(fad), FAD_TO_FRA(fad), fad, ctrl, idx);
	}

	CD_toc.first.ctrl		= CD_toc.track[CD_toc.first.num-1].ctrl;
	CD_toc.first.idx		= CD_toc.track[CD_toc.first.num-1].idx;
	CD_toc.first.fad		= CD_toc.track[CD_toc.first.num-1].fad;
	CD_toc.first.min		= CD_toc.track[CD_toc.first.num-1].min;
	CD_toc.first.sec		= CD_toc.track[CD_toc.first.num-1].sec;
	CD_toc.first.fra		= CD_toc.track[CD_toc.first.num-1].fra;

	CD_sat_toc[0x18c]		= (CD_toc.first.ctrl) | CD_toc.first.idx;
	CD_sat_toc[0x18d]		= CD_toc.first.num;
	CD_sat_toc[0x18e]		= 0x00;	/* psec ?*/
	CD_sat_toc[0x18f]		= 0x00;	/* pfra ?*/

	logerror("track#%02i: %02i:%02i:%02i (addr: %i)\n",
	CD_toc.first.num, CD_toc.first.min, CD_toc.first.sec, CD_toc.first.fra, CD_toc.first.fad);

	CD_toc.last.ctrl	= CD_toc.track[CD_toc.last.num-1].ctrl;
	CD_toc.last.idx		= CD_toc.track[CD_toc.last.num-1].idx;
	CD_toc.last.fad		= CD_toc.track[CD_toc.last.num-1].fad;
	CD_toc.last.min		= CD_toc.track[CD_toc.last.num-1].min;
	CD_toc.last.sec		= CD_toc.track[CD_toc.last.num-1].sec;
	CD_toc.last.fra		= CD_toc.track[CD_toc.last.num-1].fra;

	CD_sat_toc[0x190]		= (CD_toc.last.ctrl) | CD_toc.last.idx;
	CD_sat_toc[0x191]		= CD_toc.last.num;
	CD_sat_toc[0x192]		= 0x00;	/* psec ?*/
	CD_sat_toc[0x193]		= 0x00;	/* pfra ?*/

	logerror("track#%02i: %02i:%02i:%02i (addr: %i)\n",
	CD_toc.last.num, CD_toc.last.min, CD_toc.last.sec, CD_toc.last.fra, CD_toc.last.fad);

	iso_get_leadout_info(&ctrl, &idx, &fad);

	CD_toc.leadout.ctrl		= ctrl;
	CD_toc.leadout.idx		= idx;
	CD_toc.leadout.fad		= fad;
	CD_toc.leadout.min		= FAD_TO_MIN(fad);
	CD_toc.leadout.sec		= FAD_TO_SEC(fad);
	CD_toc.leadout.fra		= FAD_TO_FRA(fad);

	CD_sat_toc[0x194]		= (ctrl << 4) | idx;
	CD_sat_toc[0x195]		= (UINT8)((UINT32)fad >> 16);
	CD_sat_toc[0x196]		= (UINT8)((UINT32)fad >> 8);
	CD_sat_toc[0x197]		= (UINT8)((UINT32)fad);

	logerror("leadout:  %02i:%02i:%02i (addr: %i)\n",
	CD_toc.leadout.min, CD_toc.leadout.sec, CD_toc.leadout.fra, CD_toc.leadout.fad);

	log_cb(RETRO_LOG_DEBUG, LOGPRE "\n\nTOC DUMP\n\n");
	while (oo<408){
	log_cb(RETRO_LOG_DEBUG, LOGPRE "%2x %2x %2x %2x\n", CD_sat_toc[oo],CD_sat_toc[oo+1],CD_sat_toc[oo+2],CD_sat_toc[oo+3]);
	oo+=4;
	}

}


void cdb_build_ftree(void){

	UINT32 addr, fad, off;
	UINT32 i, j, size;

static UINT8 buff[4096];

	addr = 40960;
	fad = (addr / 2048) + 150;
	off = (addr & 2047);

	i = 0;

	while(fad < CD_toc.leadout.fad && i < CDB_FID_NUM){

		iso_read_sector(0, fad+0, &buff[2048 * 0]);
		iso_read_sector(0, fad+1, &buff[2048 * 1]);

		size = buff[off+0x00];

		if(size == 0){
			/* check for subdirs*/
			break;
		}

		CD_file[i].fad	= (buff[off+0x06] << 24) | (buff[off+0x07] << 16) | (buff[off+0x08] << 8) | buff[off+0x09];
		CD_file[i].size	= (buff[off+0x0e] << 24) | (buff[off+0x0f] << 16) | (buff[off+0x10] << 8) | buff[off+0x11];
		CD_file[i].attr	= buff[off+0x19];
		CD_file[i].unit	= buff[off+0x1a];
		CD_file[i].gap	= buff[off+0x1b];
		CD_file[i].name_len = (buff[off+0x20] > 32) ? 32 : buff[off+0x20];

		for(j = 0; j < CD_file[i].name_len; j++)
			CD_file[i].name[j] = buff[off+0x21+j];
		CD_file[i].name[j] = '\0';

		CD_file[i].fad += 150; /* LSN to FAD*/

		logerror(	"ANY 2 #%08i : (fad=%i off=%i, size=%02X) fad=%06X size=%06X attr=%02X %s\n",
			i, fad, off, size,
			CD_file[i].fad,
			CD_file[i].size,
			CD_file[i].attr,
			CD_file[i].name
		);

		addr += size;
		fad = (addr / 2048) + 150;
		off = (addr & 2047);
		i++;
	}

	CD_file_num = (i < 2) ? 2 : i; /* to prevent stupid ISO bugs*/
	log_cb(RETRO_LOG_DEBUG, LOGPRE "trovati %d file\n",CD_file_num);
}

void CD_com_update(UINT32 count){


	if(!CD_cr_writing && !CD_cr_first) {



		log_cb(RETRO_LOG_DEBUG, LOGPRE "---- periodic update ----\n");

		/* prevents update to change anything before*/
		/* the init string is read by the host*/
		/*if(CD_cr_first) return ();*/

		/* prevents update from overwriting cr registers*/
		/* while a command is being written*/
		/*if(CD_cr_writing) return ();*/

		if(CD_status == CDB_STAT_PAUSE){

			if(CD_free_space && (CD_hirq & HIRQ_BFUL)){

				/* PEND ?*/

				log_cb(RETRO_LOG_DEBUG, LOGPRE "BFUL -> PLAY\n");

				CD_hirq &= ~HIRQ_BFUL;
				CD_status = CDB_STAT_PLAY;
			}
		}

		if(CD_status == CDB_STAT_PLAY){

			if(CD_toc.track[CD_cur_track-1].ctrl & 0x40){

				/* CDROM*/

				UINT32 pn;

				if(cdb_find_dest(CD_filt_num, &pn) == 0){

					/* dest partition found, else data is discarded*/

					UINT32 sn;

					sn = cdb_make_room(pn);

					CD_part[pn].sect[sn]->size	= cdb_get_sect_size; /* 2048, 2352*/
					CD_part[pn].sect[sn]->fad	= CD_cur_fad;
					CD_part[pn].sect[sn]->fid	= CD_cur_fid;
					CD_part[pn].sect[sn]->chan	= 0;
					CD_part[pn].sect[sn]->sub	= 0;
					CD_part[pn].sect[sn]->cod	= 0;

					logerror("PLAY CDROM : fad=%06x [%06x~%06x] track=%i ctrl=%x idx=%i -> pn=%i sn=%i\n",
					CD_cur_fad, CD_play_fad, CD_play_fad+CD_play_range,
					CD_cur_track, CD_cur_ctrl, CD_cur_idx, pn, sn);

					if(iso_read_sector(1, CD_cur_fad, CD_part[pn].sect[sn]->data) == 0 &&
					  2048 != 2048){

						#if 0

						/* couldn't read synch, header, subheader*/
						/* and EDC/ECC data. generate it! (FORM2 MODE1)*/

						/* synch data (12 bytes)*/
						memset(&CD_part[pn].sect[sn]->data[1], 0xff, 10);
						CD_part[pn].sect[sn]->data[0] = 0x00;
						CD_part[pn].sect[sn]->data[11] = 0x00;

						/* header (4 bytes)*/
						CD_part[pn].sect[sn]->data[12] = FAD_TO_MIN(CD_cur_fad);	/* min*/
						CD_part[pn].sect[sn]->data[13] = FAD_TO_SEC(CD_cur_fad);	/* sec*/
						CD_part[pn].sect[sn]->data[14] = FAD_TO_FRA(CD_cur_fad);	/* fra*/
						CD_part[pn].sect[sn]->data[15] = 0;				/* mode*/

						/* subheader (8 bytes)*/

						/* EDC/ECC*/
						memset(&CD_part[pn].sect[sn]->data[2048+16+8], 0xff, 280);

						#endif
					}

					if(CD_free_space <= 0){

						/* buffer full*/

						log_cb(RETRO_LOG_DEBUG, LOGPRE "BFUL!\n");

						CD_hirq |= HIRQ_BFUL;
					/*	CD_hirq |= HIRQ_PEND; */ /* not sure*/
						CD_hirq |= HIRQ_DRDY; /* not sure*/

						CD_status = CDB_STAT_PAUSE;
					}
				}

				CD_flag = CD_FLAG_CDROM;

			}else{

				/* CDDA*/

				logerror("PLAY CDDA  : fad=%06x [%06x~%06x] track=%i ctrl=%i idx=%i\n",
				CD_cur_fad, CD_play_fad, CD_play_fad+CD_play_range,
				CD_cur_track, CD_cur_ctrl, CD_cur_idx);

				CD_flag = 0; /* playing CDDA*/

				/* set 1x drive speed*/

				/*CD_update_timings(1);*/

				/* fill the CD-DA buffer*/

				if(iso_read_sector(1, CD_cur_fad, &cdda_buff[cdda_pos & (8192 - 1)]) == 0){

					/* not all the 2352 data could be read*/
					/* CD-DA cannot be played*/

					memset(cdda_buff, 0x00, 8192);

				}else{

					/* the CD-DA data resides in cdda_buff[]*/
				}

				cdda_pos += 2352;

				/* update subcode info for *next* sector*/

				{

				/* generate subcode q*/

				UINT32 pfad, qfad;

				pfad = (CD_cur_fad + 1) - CD_toc.first.fad;
				qfad = (CD_cur_fad + 1);

				CD_sat_subq[0] = CD_toc.track[CD_cur_track-1].ctrl | 1;	/* ctrl, adr*/
				CD_sat_subq[1] = CD_cur_track;					/* tno*/
				CD_sat_subq[2] = 1;							/* idx*/
				CD_sat_subq[3] = pfad >> 16;						/* pfad*/
				CD_sat_subq[4] = pfad >> 8;						/* pfad*/
				CD_sat_subq[5] = pfad;							/* pfad*/
				CD_sat_subq[6] = 0x00;							/* -*/
				CD_sat_subq[7] = qfad >> 16;						/* qfad*/
				CD_sat_subq[8] = qfad >> 8;						/* qfad*/
				CD_sat_subq[9] = qfad;							/* qfad*/

				/* generate subcode rw*/

				memset(CD_sat_subrw, 0x00, 24);					/* eeer ... ;)*/

				}
			}

			CD_cur_fad++;	/* points to *next* sector*/

			CD_cur_track	= iso_find_track(CD_cur_fad);
			CD_cur_ctrl	= CD_toc.track[CD_cur_track-1].ctrl;
			CD_cur_idx		= CD_toc.track[CD_cur_track-1].idx;
			/*CD_cur_fid		= iso_find_file(CD_cur_fad);*/

			if((CD_cur_fad < CD_play_fad) ||
			   (CD_cur_fad >= (CD_play_fad + CD_play_range))){

				if(((CD_flag & 0x80) == 0) &&		/* CD-DA*/
				   (CD_repeat_max != 0xff) &&		/* no ignore errors*/
				   ((CD_repeat_max == 0xfe) ||		/* infinite repeat*/
				    (CD_repeat < CD_repeat_max))){	/* didn't finish number of repeats*/

					/* setup repeat*/

					log_cb(RETRO_LOG_DEBUG, LOGPRE "REPEAT (%i / %i)\n", CD_repeat, CD_repeat_max);

				}else{

					/* play ended*/

					log_cb(RETRO_LOG_DEBUG, LOGPRE "PLAY ended\n");

					CD_status = CDB_STAT_PAUSE;

					CD_hirq |= HIRQ_PEND;
					if(CD_flag != 0)
						CD_hirq |= HIRQ_DRDY;

					CD_flag = 0;

					/* return to default speed (not correct, but can do it)*/
					/* should be done on next pickup-movement command instead*/

					/*CD_update_timings(CD_drive_speed);*/
				}
			}

			CD_hirq |= HIRQ_SCDQ;
			CD_hirq |= HIRQ_CSCT; /* shouldn't be here!*/

		}else
		if(CD_status == CDB_STAT_SCAN){

			CD_flag = 0;

			if((CD_scan_dir == 0) && /* scanning forward*/
			   (CD_toc.track[CD_cur_track-1].ctrl & 0x40) == 0){ /* CDDA*/

				/* play CDDA*/

				log_cb(RETRO_LOG_DEBUG, LOGPRE "SCAN - PLAY CDDA\n");

				/* set 1x drive speed*/

				/*CD_update_timings(1);*/
			}

			/* update fad*/



			log_cb(RETRO_LOG_DEBUG, LOGPRE "ERROR: scanning\n");
			/*exit(1);*/
		}

		CD_hirq |= HIRQ_SCDQ;

		CDB_SEND_REPORT();
		CR1 |= CDB_STAT_PERI << 8; /* periodic response*/
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "CD block update\n");
}
