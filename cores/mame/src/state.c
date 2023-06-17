/* State save/load functions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "driver.h"
#include <compat/zlib.h>

/* Save state file format:
 *
 *	0.. 7  'MAMESAVE"
 *	8	   Format version (this is format 1)
 *	9	   Flags
 *	a..13  Game name padded with \0
 * 14..17  Signature
 * 18..end Save game data
 */

/* Available flags */
enum {
	SS_NO_SOUND = 0x01,
	SS_MSB_FIRST = 0x02
};

/*#define VERBOSE*/

#ifdef VERBOSE
#define TRACE(x) do {x;} while(0)
#else
#define TRACE(x)
#endif

enum {MAX_INSTANCES = 25};

enum {
	SS_INT8,
	SS_UINT8,
	SS_INT16,
	SS_UINT16,
	SS_INT32,
	SS_UINT32,
	SS_INT,
	SS_DOUBLE,
	SS_FLOAT
};

#ifdef VERBOSE
static const char *ss_type[] =	{ "i8", "u8", "i16", "u16", "i32", "u32", "int", "dbl", "flt" };
#endif
static int		   ss_size[] =	{	 1,    1,	  2,	 2, 	4,	   4,	  4,	 8,     4 };

static void ss_c2(unsigned char *, unsigned);
static void ss_c4(unsigned char *, unsigned);
static void ss_c8(unsigned char *, unsigned);

static void (*ss_conv[])(unsigned char *, unsigned) = {
	0, 0, ss_c2, ss_c2, ss_c4, ss_c4, 0, ss_c8, ss_c4
};


typedef struct ss_entry {
	struct ss_entry *next;
	char *name;
	int type;
	void *data;
	unsigned size;
	int tag;
	unsigned offset;
} ss_entry;

typedef struct ss_module {
	struct ss_module *next;
	char *name;
	ss_entry *instances[MAX_INSTANCES];
} ss_module;

static ss_module *ss_registry;

typedef struct ss_func {
	struct ss_func *next;
	void (*func)(void);
	int tag;
} ss_func;

static ss_func *ss_prefunc_reg;
static ss_func *ss_postfunc_reg;
static int ss_current_tag;

static unsigned char *ss_dump_array;
static unsigned int ss_dump_size;


static UINT32 ss_get_signature(void)
{
	ss_module *m;
	unsigned int size = 0, pos = 0;
	char *info;
	UINT32 signature;

	/* Pass 1 : compute size*/

	for(m = ss_registry; m; m=m->next) {
		int i;
		size += strlen(m->name) + 1;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			size++;
			for(e = m->instances[i]; e; e=e->next)
				size += strlen(e->name) + 1 + 1 + 4;
		}
	}

	info = malloc(size);

	/* Pass 2 : write signature info*/

	for(m = ss_registry; m; m=m->next) {
		int i;
		strcpy(info+pos, m->name);
		pos += strlen(m->name) + 1;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			info[pos++] = i;
			for(e = m->instances[i]; e; e=e->next) {
				strcpy(info+pos, e->name);
				pos += strlen(e->name) + 1;
				info[pos++] = e->type;
				info[pos++] = e->size;
				info[pos++] = e->size >> 8;
				info[pos++] = e->size >> 16;
				info[pos++] = e->size >> 24;
			}
		}
	}

	/* Pass 3 : Compute the crc32*/
	signature = crc32(0, (unsigned char *)info, size);

	free(info);
	return signature;
}

void state_save_reset(void)
{
	ss_func *f;
	ss_module *m = ss_registry;
	while(m) {
		ss_module *mn = m->next;
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e = m->instances[i];
			while(e) {
				ss_entry *en = e->next;
				free(e->name);
				free(e);
				e = en;
			}
		}
		free(m->name);
		m = mn;
	}
	ss_registry = 0;

	f = ss_prefunc_reg;
	while(f) {
		ss_func *fn = f->next;
		free(f);
		f = fn;
	}
	ss_prefunc_reg = 0;

	f = ss_postfunc_reg;
	while(f) {
		ss_func *fn = f->next;
		free(f);
		f = fn;
	}
	ss_postfunc_reg = 0;

	ss_current_tag = 0;
	ss_dump_array = 0;
	ss_dump_size = 0;
}

static ss_module *ss_get_module(const char *name)
{
	int i;
	ss_module **mp = &ss_registry;
	ss_module *m;
	while((m = *mp) != 0) {
		int pos = strcmp(m->name, name);
		if(!pos)
			return m;
		if(pos>0)
			break;
		mp = &((*mp)->next);
	}
	*mp = malloc(sizeof(ss_module));
	if (*mp == NULL) return NULL;
	(*mp)->name = malloc (strlen (name) + 1);
	if ((*mp)->name == NULL) return NULL;
	strcpy ((*mp)->name, name);
	(*mp)->next = m;
	for(i=0; i<MAX_INSTANCES; i++)
		(*mp)->instances[i] = 0;
	return *mp;
}

static ss_entry *ss_register_entry(const char *module, int instance, const char *name, int type, void *data, unsigned size)
{
	ss_module *m = ss_get_module(module);
	ss_entry **ep = &(m->instances[instance]);
	ss_entry *e = *ep;
	while((e = *ep) != 0) {
		int pos = strcmp(e->name, name);
		if(!pos) {
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Duplicate save state registration entry (%s, %d, %s)\n", module, instance, name);
			return NULL;
		}
		if(pos>0)
			break;
		ep = &((*ep)->next);
	}
	*ep = malloc(sizeof(ss_entry));
	if (*ep == NULL) return NULL;
	(*ep)->name = malloc (strlen (name) + 1);
	if ((*ep)->name == NULL) return NULL;
	strcpy ((*ep)->name, name);
	(*ep)->next   = e;
	(*ep)->type   = type;
	(*ep)->data   = data;
	(*ep)->size   = size;
	(*ep)->offset = 0;
	(*ep)->tag	  = ss_current_tag;
	return *ep;
}

void state_save_register_UINT8 (const char *module, int instance,
								const char *name, UINT8 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT8, val, size);
}

void state_save_register_INT8  (const char *module, int instance,
								const char *name, INT8 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT8, val, size);
}

void state_save_register_UINT16(const char *module, int instance,
								const char *name, UINT16 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT16, val, size);
}

void state_save_register_INT16 (const char *module, int instance,
								const char *name, INT16 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT16, val, size);
}

void state_save_register_UINT32(const char *module, int instance,
								const char *name, UINT32 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_UINT32, val, size);
}

void state_save_register_INT32 (const char *module, int instance,
								const char *name, INT32 *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_INT32, val, size);
}

void state_save_register_int   (const char *module, int instance,
								const char *name, int *val)
{
	ss_register_entry(module, instance, name, SS_INT, val, 1);
}

void state_save_register_double(const char *module, int instance,
								const char *name, double *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_DOUBLE, val, size);
}

void state_save_register_float(const char *module, int instance,
							   const char *name, float *val, unsigned size)
{
	ss_register_entry(module, instance, name, SS_FLOAT, val, size);
}



static void ss_register_func(ss_func **root, void (*func)(void))
{
	ss_func *next = *root;
	while (next)
	{
		if (next->func == func && next->tag == ss_current_tag)
		{
			log_cb(RETRO_LOG_DEBUG, LOGPRE "Duplicate save state function (%d, 0x%x)\n", ss_current_tag, (int)func);
			return;
		}
		next = next->next;
	}
	next = *root;
	*root = malloc(sizeof(ss_func));
	if (*root == NULL)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE "malloc failed in ss_register_func\n");
		return;
	}
	(*root)->next = next;
	(*root)->func = func;
	(*root)->tag  = ss_current_tag;
}

void state_save_register_func_presave(void (*func)(void))
{
	ss_register_func(&ss_prefunc_reg, func);
}

void state_save_register_func_postload(void (*func)(void))
{
	ss_register_func(&ss_postfunc_reg, func);
}

void state_save_set_current_tag(int tag)
{
	ss_current_tag = tag;
}

static void ss_c2(unsigned char *data, unsigned size)
{
	unsigned i;
	for(i=0; i<size; i++) {
		unsigned char v;
		v = data[0];
		data[0] = data[1];
		data[1] = v;
		data += 2;
	}
}

static void ss_c4(unsigned char *data, unsigned size)
{
	unsigned i;
	for(i=0; i<size; i++) {
		unsigned char v;
		v = data[0];
		data[0] = data[3];
		data[3] = v;
		v = data[1];
		data[1] = data[2];
		data[2] = v;
		data += 4;
	}
}

static void ss_c8(unsigned char *data, unsigned size)
{
	unsigned i;
	for(i=0; i<size; i++) {
		unsigned char v;
		v = data[0];
		data[0] = data[7];
		data[7] = v;
		v = data[1];
		data[1] = data[6];
		data[6] = v;
		v = data[2];
		data[2] = data[5];
		data[5] = v;
		v = data[3];
		data[3] = data[4];
		data[4] = v;
		data += 8;
	}
}

/* __LIBRETRO__: Serialize helper*/
size_t state_get_dump_size(void)
{
	ss_module *m;
  
  if(Machine->gamedrv->flags & GAME_DOESNT_SERIALIZE)
  {
    log_cb(RETRO_LOG_DEBUG, LOGPRE "Driver flagged GAME_DOESNT_SERIALIZE. Setting state_get_dump_size() to 0.\n");
    return 0;
  }
  
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Beginning save\n");
	ss_dump_size = 0x18;
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next) {
			    if(!e->data)
			    {
			        return 0;
			    }
				e->offset = ss_dump_size;
				ss_dump_size += ss_size[e->type]*e->size;
			}
		}
	}

    return ss_dump_size;
}

void state_save_save_begin(void *array)
{
	ss_module *m;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Beginning save\n");
	ss_dump_size = 0x18;
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next) {
				e->offset = ss_dump_size;
				ss_dump_size += ss_size[e->type]*e->size;
			}
		}
	}

	log_cb(RETRO_LOG_DEBUG, LOGPRE "   total size %u\n", ss_dump_size);
	ss_dump_array = array;
	if (ss_dump_array == NULL)
	{
		log_cb(RETRO_LOG_DEBUG, LOGPRE  "malloc failed in state_save_save_begin\n");
	}
}

int state_save_save_continue(void)
{
	ss_module *m;
	ss_func * f;
	int count = 0;
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Saving tag %d\n", ss_current_tag);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  calling pre-save functions\n");
	f = ss_prefunc_reg;
	while(f) {
		if(f->tag == ss_current_tag) {
			count++;
			(f->func)();
		}
		f = f->next;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "    %d functions called\n", count);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  copying data\n");
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next)
				if(e->tag == ss_current_tag) {
				    if(!e->data) {
				    	ss_dump_array = 0;
                    	ss_dump_size = 0;
                    	return 1;
				    }
					if(e->type == SS_INT) {
						int v = *(int *)(e->data);
						ss_dump_array[e->offset]   = v ;
						ss_dump_array[e->offset+1] = v >> 8;
						ss_dump_array[e->offset+2] = v >> 16;
						ss_dump_array[e->offset+3] = v >> 24;
						log_cb(RETRO_LOG_DEBUG, LOGPRE "    %s.%d.%s: %x..%x\n", m->name, i, e->name, e->offset, e->offset+3);
					} else {
						memcpy(ss_dump_array + e->offset, e->data, ss_size[e->type]*e->size);
						log_cb(RETRO_LOG_DEBUG, LOGPRE "    %s.%d.%s: %x..%x\n", m->name, i, e->name, e->offset, e->offset+ss_size[e->type]*e->size-1);
					}
				}
		}
	}
	
	return 0;
}

void state_save_save_finish(void)
{
	UINT32 signature;
	unsigned char flags = 0;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Finishing save\n");

	signature = ss_get_signature();
	if(!Machine->sample_rate)
		flags |= SS_NO_SOUND;

#ifdef MSB_FIRST
	flags |= SS_MSB_FIRST;
#endif

	memcpy(ss_dump_array, "MAMESAVE", 8);
	ss_dump_array[8] = 1;
	ss_dump_array[9] = flags;
	memset(ss_dump_array+0xa, 0, 10);
	strcpy((char *)ss_dump_array+0xa, Machine->gamedrv->name);

	ss_dump_array[0x14] = signature;
	ss_dump_array[0x15] = signature >> 8;
	ss_dump_array[0x16] = signature >> 16;
	ss_dump_array[0x17] = signature >> 24;

	ss_dump_array = 0;
	ss_dump_size = 0;
}

int state_save_load_begin(void *array, size_t size)
{
	ss_module *m;
	unsigned int offset = 0;
	UINT32 signature, file_sig;

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Beginning load\n");

	signature = ss_get_signature();

	ss_dump_size = size;
	ss_dump_array = array;

	if(memcmp(ss_dump_array, "MAMESAVE", 8)) {
		usrintf_showmessage("Error: This is not a mame save file");
		goto bad;
	}

	if(ss_dump_array[8] != 1) {
		usrintf_showmessage("Error: Wrong version in save file (%d, 1 expected)",
							ss_dump_array[8]);
		goto bad;
	}

	file_sig = ss_dump_array[0x14]
		| (ss_dump_array[0x15] << 8)
		| (ss_dump_array[0x16] << 16)
		| (ss_dump_array[0x17] << 24);

	if(file_sig != signature) {
		usrintf_showmessage("Error: Incompatible save file (signature %08x, expected %08x)",
							file_sig, signature);
		goto bad;
	}

	if(ss_dump_array[9] & SS_NO_SOUND)
	{
		if(Machine->sample_rate)
			usrintf_showmessage("Warning: Game was saved with sound off, but sound is on.  Result may be interesting.");
	}
	else
	{
		if(!Machine->sample_rate)
			usrintf_showmessage("Warning: Game was saved with sound on, but sound is off.  Result may be interesting.");
	}

	offset = 0x18;
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next) {
				e->offset = offset;
				offset += ss_size[e->type]*e->size;
			}
		}
	}
	return 0;

 bad:
	return 1;
}

int state_save_load_continue(void)
{
	ss_module *m;
	ss_func * f;
	int count = 0;
	int need_convert;

#ifdef MSB_FIRST
	need_convert = (ss_dump_array[9] & SS_MSB_FIRST) == 0;
#else
	need_convert = (ss_dump_array[9] & SS_MSB_FIRST) != 0;
#endif

	log_cb(RETRO_LOG_DEBUG, LOGPRE "Loading tag %d\n", ss_current_tag);
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  copying data\n");
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next)
				if(e->tag == ss_current_tag) {
					if(!e->data) {
				    	ss_dump_array = 0;
                    	ss_dump_size = 0;
                    	return 1;
				    }

					if(e->type == SS_INT) {
						int v;
						v = ss_dump_array[e->offset]
							| (ss_dump_array[e->offset+1] << 8)
							| (ss_dump_array[e->offset+2] << 16)
							| (ss_dump_array[e->offset+3] << 24);
						log_cb(RETRO_LOG_DEBUG, LOGPRE "    %s.%d.%s: %x..%x\n", m->name, i, e->name, e->offset, e->offset+3);
						*(int *)(e->data) = v;
					} else {
						memcpy(e->data, ss_dump_array + e->offset, ss_size[e->type]*e->size);
						if (need_convert && ss_conv[e->type])
							ss_conv[e->type](e->data, e->size);
						log_cb(RETRO_LOG_DEBUG, LOGPRE "    %s.%d.%s: %x..%x\n", m->name, i, e->name, e->offset, e->offset+ss_size[e->type]*e->size-1);
					}
				}
		}
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "  calling post-load functions\n");
	f = ss_postfunc_reg;
	while(f) {
		if(f->tag == ss_current_tag) {
			count++;
			(f->func)();
		}
		f = f->next;
	}
	log_cb(RETRO_LOG_DEBUG, LOGPRE "    %d functions called\n", count);
	
	return 0;
}

void state_save_load_finish(void)
{
	log_cb(RETRO_LOG_DEBUG, LOGPRE "Finishing load\n");
	ss_dump_array = 0;
	ss_dump_size = 0;
}

void state_save_dump_registry(void)
{
#ifdef VERBOSE
	ss_module *m;
	for(m = ss_registry; m; m=m->next) {
		int i;
		for(i=0; i<MAX_INSTANCES; i++) {
			ss_entry *e;
			for(e = m->instances[i]; e; e=e->next)
				log_cb(RETRO_LOG_DEBUG, LOGPRE "%d %s.%d.%s: %s, %x\n", e->tag, m->name, i, e->name, ss_type[e->type], e->size);
		}
	}
#endif
}

