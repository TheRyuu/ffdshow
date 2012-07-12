#pragma once

class TfontManager
{
private:
    boost::mutex mutex;
    struct THFONT {
        THFONT():
            hf(NULL),
            count(0)
        {}

        THFONT(const LOGFONT &Ilf):
            lf(Ilf) {
            hf = CreateFontIndirect(&lf);
            ASSERT(hf);
        }

        LOGFONT lf;
        HFONT hf;
        unsigned int count;

        bool operator ==(const LOGFONT &lf1) const {
            return memcmp(&lf, &lf1, sizeof(LOGFONT)) == 0;
        }

        bool operator <(const THFONT &f1) const {
            return count < f1.count;
        }
    };

    typedef std::list<THFONT> THFONTs;
    THFONTs fonts;

public:
    ~TfontManager() {
        boost::unique_lock<boost::mutex> lock(mutex);
        for (THFONTs::iterator f = fonts.begin(); f != fonts.end(); f++) {
            DeleteObject(f->hf);
        }
    }

    HFONT getFont(const LOGFONT &font) {
        boost::unique_lock<boost::mutex> lock(mutex);
        THFONTs::iterator f = std::find(fonts.begin(), fonts.end(), font);
        if (f != fonts.end()) {
            f->count++;
            return f->hf;
        }
        if (fonts.size() == 32) {
            THFONTs::iterator f = std::min_element(fonts.begin(), fonts.end());
            DeleteObject(f->hf);
            fonts.erase(f);
        }
        fonts.push_back(font);
        return fonts.back().hf;
    }
};
