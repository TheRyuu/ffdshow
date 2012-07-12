#pragma once

#include "Tfont.h"

struct Tsubtitle {
    virtual ~Tsubtitle() {}
    REFERENCE_TIME start, stop;
    Tsubtitle() {
        start = stop = REFTIME_INVALID;
    }
    virtual void print(
        REFERENCE_TIME time,
        bool wasseek,
        Tfont &f,
        bool forceChange,
        TprintPrefs &prefs,
        unsigned char **dst,
        const stride_t *stride) = 0;
    virtual size_t numlines(void) const {
        return 1;
    }
    virtual size_t numchars(void) const {
        return 1;
    }
    virtual void append(const unsigned char *data, unsigned int datalen) {}
    virtual void addEmpty(void) {}
    virtual bool isText(void) const {
        return false;
    }
    virtual size_t dropRenderedLines(void) {
        return 0;   // return size of released memory
    }
    virtual size_t getRenderedMemorySize() const {
        return 0;
    }
};
