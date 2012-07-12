#ifndef _TSWSCALE_H_
#define _TSWSCALE_H_

#include "ffImgfmt.h"
#include "TrgbPrimaries.h"

struct Tlibavcodec;
struct SwsContext;
struct Tswscale {
private:
    Tlibavcodec *libavcodec;
    unsigned int dx, dy;
    int sws_flags;
public:
    Tswscale(Tlibavcodec *Ilibavcodec);
    ~Tswscale();
    SwsContext *swsc;
    static bool getVersion(char *ver);
    bool init(unsigned int dx, unsigned int dy, uint64_t incsp, uint64_t outcsp);
    bool convert(const uint8_t* src[], const stride_t srcStride[], uint8_t* dst[], stride_t dstStride[], const TrgbPrimaries &rgbPrimaries);
    void done(void);
};

#endif
