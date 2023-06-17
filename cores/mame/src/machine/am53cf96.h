/*
 * am53cf96.h
 *
 */

#if !defined( AM53CF96_H )
#define AM53CF96_H ( 1 )

#define AM53CF96_DEVICE_HDD	(0)
#define AM53CF96_DEVICE_CDROM	(1)

struct AM53CF96interface
{
	int device;			/* device type */
	void (*irq_callback)(void);	/* irq callback */
};

extern void am53cf96_init( struct AM53CF96interface *interface );
extern void am53cf96_read_data(int bytes, data8_t *pData);
extern READ32_HANDLER( am53cf96_r );
extern WRITE32_HANDLER( am53cf96_w );

#endif
