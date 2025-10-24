#ifndef OURCONVERSIONLIB_H
#define OURCONVERSIONLIB_H

#include <stddef.h>

int convertToJPG(const char *input, int quality);
int convertToPNG(const char *input, int quality);
int convertToTIFF(const char *input, int quality);
int convertToWEBP(const char *input, int quality);

int makeGIF(const char **frames, size_t count,
            const char *out_gif,
            int delay_cs, int loop,
            size_t target_w, size_t target_h);

#endif
