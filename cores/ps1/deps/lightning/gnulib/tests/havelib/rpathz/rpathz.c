extern int rpathx_value (void);
extern int rpathy_value (void);
int rpathz_value () { return 1000 * rpathx_value () + 3 * rpathy_value (); }
