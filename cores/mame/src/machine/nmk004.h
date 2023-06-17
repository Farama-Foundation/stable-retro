#ifndef NMK004_H
#define NMK004_H

void NMK004_init(void);
void NMK004_irq(int irq);
READ16_HANDLER( NMK004_r );
WRITE16_HANDLER( NMK004_w );

#endif /* NMK004_H */
