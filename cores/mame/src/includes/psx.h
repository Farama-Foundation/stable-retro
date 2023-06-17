/***************************************************************************

  includes/psx.h

***************************************************************************/

#if !defined( PSX_H )

/* vidhrdw */
PALETTE_INIT( psx );
VIDEO_START( psx_type1_1024x1024 );
VIDEO_START( psx_type2_1024x512 );
VIDEO_START( psx_type2_1024x1024 );
VIDEO_UPDATE( psx );
VIDEO_STOP( psx );
INTERRUPT_GEN( psx_vblank );
extern void psx_gpu_reset( void );
extern void psx_gpu_read( UINT32 *p_ram, INT32 n_size );
extern void psx_gpu_write( UINT32 *p_ram, INT32 n_size );
READ32_HANDLER( psx_gpu_r );
WRITE32_HANDLER( psx_gpu_w );

/* machine */
typedef void ( *psx_dma_read_handler )( UINT32, INT32 );
typedef void ( *psx_dma_write_handler )( UINT32, INT32 );
WRITE32_HANDLER( psx_irq_w );
READ32_HANDLER( psx_irq_r );
extern void psx_irq_set( UINT32 );
extern void psx_dma_install_read_handler( int, psx_dma_read_handler );
extern void psx_dma_install_write_handler( int, psx_dma_read_handler );
WRITE32_HANDLER( psx_dma_w );
READ32_HANDLER( psx_dma_r );
WRITE32_HANDLER( psx_counter_w );
READ32_HANDLER( psx_counter_r );
WRITE32_HANDLER( psx_sio_w );
READ32_HANDLER( psx_sio_r );
typedef void ( *psx_sio_write_handler )( data8_t );
extern void psx_sio_install_write_handler( int, psx_sio_write_handler );
extern void psx_sio_send( int, data8_t );
WRITE32_HANDLER( psx_mdec_w );
READ32_HANDLER( psx_mdec_r );
extern void psx_machine_init( void );
extern void psx_driver_init( void );

#define PSX_H ( 1 )
#endif
