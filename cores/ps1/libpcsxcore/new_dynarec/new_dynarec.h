#define NEW_DYNAREC 1

#define MAXBLOCK 2048 // in mips instructions

#define NDHACK_NO_SMC_CHECK	(1<<0)
#define NDHACK_GTE_UNNEEDED	(1<<1)
#define NDHACK_GTE_NO_FLAGS	(1<<2)
#define NDHACK_OVERRIDE_CYCLE_M	(1<<3)
#define NDHACK_NO_STALLS	(1<<4)
#define NDHACK_NO_COMPAT_HACKS	(1<<5)
#define NDHACK_THREAD_FORCE   	(1<<6)
#define NDHACK_THREAD_FORCE_ON	(1<<7)

struct ndrc_globals
{
	int hacks;
	int hacks_pergame;
	int hacks_old;
	int did_compile;
	int cycle_multiplier_old;
	struct {
		void *handle;
		void *lock;
		void *cond;
		void *dirty_start;
		void *dirty_end;
		unsigned int busy_addr; // 0 is valid, ~0 == none
		int exit;
	} thread;
};
extern struct ndrc_globals ndrc_g;

void new_dynarec_init(void);
void new_dynarec_cleanup(void);
void new_dynarec_clear_full(void);
int  new_dynarec_save_blocks(void *save, int size);
void new_dynarec_load_blocks(const void *save, int size);
void new_dynarec_print_stats(void);

int  new_dynarec_quick_check_range(unsigned int start, unsigned int end);
void new_dynarec_invalidate_range(unsigned int start, unsigned int end);
void new_dynarec_invalidate_all_pages(void);
void new_dyna_clear_cache(void *start, void *end);

void new_dyna_start(void *context);
void new_dyna_start_at(void *context, void *compiled_code);

struct ht_entry;
enum ndrc_compile_mode {
	ndrc_cm_no_compile = 0,
	ndrc_cm_compile_live,       // from executing code, vaddr is the current pc
	ndrc_cm_compile_offline,
	ndrc_cm_compile_in_thread,
};
void *ndrc_get_addr_ht_param(struct ht_entry *ht, unsigned int vaddr,
	enum ndrc_compile_mode compile_mode);

extern unsigned int ndrc_smrv_regs[32];
