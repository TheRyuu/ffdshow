#pragma once

// global stuffs for text subtitles.
namespace nmTextSubtitles
{
    typedef enum {
        SSA = 4, ASS = 5, ASS2 = 6
    } TSSAversion;
    void strToInt(const ffstring &str, int *i);
    void strToDouble(const ffstring &str, double *d);
    void strToIntMargin(const ffstring &str, int *i);
};
