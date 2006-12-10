#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "ffImgfmt.h"

void xvid_colorspace_init(void);

typedef struct
{
    uint8_t * y;
    uint8_t * u;
    uint8_t * v;
} IMAGE;

int image_input(IMAGE * image,
                uint32_t width,
                int height,
                stride_t edged_width,stride_t edged_width2,
                const uint8_t * src,
                stride_t src_stride,
                int csp,
                int interlaced,int jpeg);

int image_output(IMAGE * image,
                 uint32_t width,
                 int height,
                 stride_t edged_width[4],
                 uint8_t * dst[4],
                 stride_t dst_stride[4],
                 int csp,
                 int interlaced,int jpeg);


#endif /* _IMAGE_H_ */
