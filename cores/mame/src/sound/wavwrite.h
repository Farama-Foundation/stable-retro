void *wav_open(const char *filename, int sample_rate, int channels);
void wav_close(void *wavptr);

void wav_add_data_16(void *wavptr, INT16 *data, int samples);
void wav_add_data_32(void *wavptr, INT32 *data, int samples, int shift);
void wav_add_data_16lr(void *wavptr, INT16 *left, INT16 *right, int samples);
void wav_add_data_32lr(void *wavptr, INT32 *left, INT32 *right, int samples, int shift);
