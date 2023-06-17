#define CPUTAG_T5182 "T5182"

MACHINE_DRIVER_EXTERN( t5182_audio );

extern WRITE_HANDLER(t5182_sound_irq_w );
extern READ_HANDLER(t5182_sharedram_semaphore_snd_r);
extern READ_HANDLER(t5182shared_r);
extern WRITE_HANDLER(t5182_sharedram_semaphore_main_acquire_w);
extern WRITE_HANDLER(t5182_sharedram_semaphore_main_release_w);
extern WRITE_HANDLER(t5182shared_w);

extern UINT8 *t5182_sharedram;
