#ifndef _FF_KERNELDEINT_H_
#define _FF_KERNELDEINT_H_

#define FF_CSP_ONLY
#include "../ffImgfmt.h"
#undef FF_CSP_ONLY

DECLARE_INTERFACE(IkernelDeint)
{
 typedef void (Tcopy)(unsigned char *dst, stride_t dstStride, const unsigned char *src, stride_t srcStride, int bytesPerLine, int height);
 STDMETHOD_(void,getFrame)(const unsigned char *cur[3],stride_t srcStride[3],unsigned char *dst[3],stride_t dstStride[3],int bobframe) PURE;
 STDMETHOD_(void,destroy)(void) PURE;
};

#endif
