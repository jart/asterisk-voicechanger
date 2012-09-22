#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *vc_soundtouch_create(int rate, float pitch);
void vc_soundtouch_free(void *st);
void vc_voice_change(void *st_, float *fbuf, int16_t *data,
                     int samples, int datalen);

#ifdef __cplusplus
}
#endif
