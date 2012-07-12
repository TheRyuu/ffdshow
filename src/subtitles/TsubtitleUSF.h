#pragma once

#include "Tsubtitle.h"

struct TsubreaderUSF2;
struct TsubtitleUSF2 : public Tsubtitle {
private:
    TsubreaderUSF2 *subs;
    int idx;
    mutable bool ok, first;
public:
    TsubtitleUSF2(TsubreaderUSF2 *Isubs, int Iidx, REFERENCE_TIME start, REFERENCE_TIME stop);
    virtual void print(
        REFERENCE_TIME time,
        bool wasseek,
        Tfont &f,
        bool forceChange,
        TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride);
    virtual Tsubtitle* create(void) {
        return new TsubtitleUSF2(subs, idx, start, stop);
    }
    virtual Tsubtitle* copy(void) {
        return new TsubtitleUSF2(subs, idx, start, stop);
    }
};
