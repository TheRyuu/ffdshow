#ifndef _FFGLOBALS_H_
#define _FFGLOBALS_H_

#ifdef WIN64
#define FFDSHOW             _l("ffdshow64")
#define FFDSHOWDECVIDEO     _l("ffdshow64")
#define FFDSHOWDECVIDEORAW  _l("ffdshow64_raw")
#define FFDSHOWDECVIDEOVFW  _l("ffdshow64_vfw")
#define FFDSHOWDECAUDIO     _l("ffdshow64_audio")
#define FFDSHOWDECAUDIORAW  _l("ffdshow64_audio_raw")
#define FFDSHOWENC          _l("ffdshow64_enc")
#define FFDSHOWDECVIDEODXVA _l("ffdshow64_dxva")
#else
#define FFDSHOW             _l("ffdshow")
#define FFDSHOWDECVIDEO     _l("ffdshow")
#define FFDSHOWDECVIDEORAW  _l("ffdshow_raw")
#define FFDSHOWDECVIDEOVFW  _l("ffdshow_vfw")
#define FFDSHOWDECAUDIO     _l("ffdshow_audio")
#define FFDSHOWDECAUDIORAW  _l("ffdshow_audio_raw")
#define FFDSHOWENC          _l("ffdshow_enc")
#define FFDSHOWDECVIDEODXVA _l("ffdshow_dxva")
#endif

const wchar_t* filterMode2regkey(int filtermode);

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
#define ALT_BITSTREAM_READER
#endif

extern const char_t *FFDSHOW_VER;

#undef min
#undef max

#ifndef FFDEFS_STRICT

#if !defined(INFINITY) && defined(HUGE_VAL)
#define INFINITY HUGE_VAL
#endif

#ifdef __GNUC__
#define align16(x) x __attribute__((aligned(16)))
#else
#define align16(x) __declspec(align(16)) x
#endif

#ifdef __GNUC__
typedef const TCHAR *PCTSTR;
#endif

#ifndef __GNUC__
extern "C" LONG  __cdecl _InterlockedIncrement(LONG volatile *Addend);
#pragma intrinsic (_InterlockedIncrement)
#define InterlockedIncrement _InterlockedIncrement

extern "C" LONG  __cdecl _InterlockedDecrement(LONG volatile *Addend);
#pragma intrinsic (_InterlockedDecrement)
#define InterlockedDecrement _InterlockedDecrement

extern "C" LONG  __cdecl _InterlockedExchange(LONG volatile *Target, LONG Value);
#pragma intrinsic (_InterlockedExchange)
#define InterlockedExchange _InterlockedExchange

extern "C" LONG  __cdecl _InterlockedCompareExchange(LONG volatile *Dest, LONG Exchange, LONG Comp);
#pragma intrinsic (_InterlockedCompareExchange)
#define InterlockedCompareExchange _InterlockedCompareExchange
#endif

#define DEG2RAD 0.017453292519943295769236907684886

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.7182818284590452354
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2 0.70710678118654752440
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#define REF_SECOND_MULT 10000000LL
#define REFTIME_INVALID _I64_MIN

#define MAX_SUBTITLE_LENGTH 10000

#define countof(array) (size_t)(sizeof(array)/sizeof(array[0]))

#define SAFE_DELETE(p) { if (p) { delete (p); (p)=NULL; } }
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }
#ifndef SAFE_FREE
#define SAFE_FREE(x) { if (x) free(x); (x) = 0;} /* helper macro */
#endif

#define SAFE_ARRAYDELETE( x )   \
    if ( x )                    \
    {                           \
        delete[] x;             \
        x = NULL;               \
    }

#define STRINGIFY(s) TOSTRING(s)
#define TOSTRING(s) #s

struct H264_SPS {
    int profile_idc;
    int level_idc;
    int chroma_format_idc;
    int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
    int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
    int poc_type;                      ///< pic_order_cnt_type
    int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
    int delta_pic_order_always_zero_flag;
    int offset_for_non_ref_pic;
    int offset_for_top_to_bottom_field;
    int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
    int ref_frame_count;               ///< num_ref_frames
    int gaps_in_frame_num_allowed_flag;
    int mb_width;                      ///< pic_width_in_mbs_minus1 + 1
    int mb_height;                     ///< pic_height_in_map_units_minus1 + 1
    int frame_mbs_only_flag;
    int mb_aff;                        ///<mb_adaptive_frame_field_flag
    int direct_8x8_inference_flag;
    int crop;                          ///< frame_cropping_flag
    unsigned int crop_left;            ///< frame_cropping_rect_left_offset
    unsigned int crop_right;           ///< frame_cropping_rect_right_offset
    unsigned int crop_top;             ///< frame_cropping_rect_top_offset
    unsigned int crop_bottom;          ///< frame_cropping_rect_bottom_offset
    int vui_parameters_present_flag;
    struct {
        int num; ///< numerator
        int den; ///< denominator
    } sar;
    int video_signal_type_present_flag;
    int full_range;
    int colour_description_present_flag;
    unsigned int color_primaries;
    unsigned int color_trc;
    unsigned int colorspace;
    int timing_info_present_flag;
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    int fixed_frame_rate_flag;
    short offset_for_ref_frame[256]; //FIXME dyn aloc?
    int bitstream_restriction_flag;
    int num_reorder_frames;
    int scaling_matrix_present;
    uint8_t scaling_matrix4[6][16];
    uint8_t scaling_matrix8[6][64];
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int pic_struct_present_flag;
    int time_offset_length;
    int cpb_cnt;                       ///< See H.264 E.1.2
    int initial_cpb_removal_delay_length; ///< initial_cpb_removal_delay_length_minus1 +1
    int cpb_removal_delay_length;      ///< cpb_removal_delay_length_minus1 + 1
    int dpb_output_delay_length;       ///< dpb_output_delay_length_minus1 + 1
    int bit_depth_luma;                ///< bit_depth_luma_minus8 + 8
    int bit_depth_chroma;              ///< bit_depth_chroma_minus8 + 8
    int residual_color_transform_flag; ///< residual_colour_transform_flag
    int constraint_set_flags;          ///< constraint_set[0-3]_flag
};

typedef std::vector<HWND> THWNDs;
struct Tstrptrs : std::vector<const char_t*> {};
struct Tstrpart : std::pair<const wchar_t*, size_t> {
    Tstrpart(const wchar_t *p, size_t len): std::pair<const wchar_t*, size_t>(p, len) {}
    Tstrpart(const wchar_t *p): std::pair<const wchar_t*, size_t>(p, strlen(p)) {}
};
typedef std::vector<int> ints;

void* memsetd(void *dest, uint32_t c, size_t bytes);
char_t* strncatf(char_t *dst, size_t dstsize, const char_t *fmt, ...);
char_t* strncpyf(char_t *dst, size_t dstsize, const char_t *fmt, ...);
template<class tchar, class TlistElem> void strtok(const tchar *s, const tchar *delim, std::vector<TlistElem> &list, bool add_empty = false, size_t max_parts = std::numeric_limits<size_t>::max());
template<class tchar> void strtok(const tchar *s, const tchar *delim, ints &list, bool add_empty = false, size_t max_parts = std::numeric_limits<size_t>::max());
void mergetok(char_t *dst, size_t dstlen, const char_t *delim, const strings &list);
template<class char_t> const char_t* stristr(const char_t *haystack, const char_t *needle);
template<class char_t> const char_t* strnstr(const char_t *haystack, size_t n, const char_t *needle);
template<class char_t> const char_t* strnistr(const char_t *haystack, size_t n, const char_t *needle);
template<class char_t> const char_t* strnchr(const char_t *s, size_t n, int c);
template<class char_t> char_t* strrmchar(char_t *s, int c);
template<class char_t> const void* memnstr(const void *haystack, size_t n, const char_t *needle);
bool dlgGetDir(HWND owner, char_t *dir, const char_t *capt);
bool dlgGetFile(bool save, HWND owner, const char_t *capt, const char_t *filter, const char_t *defext, char_t *flnm, const char_t *initdir, DWORD flags, DWORD *filterIndex = NULL);
bool dlgOpenFiles(HWND owner, const char_t *capt, const char_t *filter, const char_t *defext, strings &files, const char_t *initdir, DWORD flags);
void findFiles(const char_t *mask, strings &list, bool fullpaths = true);
bool fileexists(const char_t *flnm), directoryexists(const char_t *dir);
inline bool operator !=(const FILETIME &t1, const FILETIME &t2)
{
    return t1.dwHighDateTime != t2.dwHighDateTime || t1.dwLowDateTime != t2.dwLowDateTime;
}
FILETIME fileLastWriteTime(const char_t *flnm);
void extractfilepath(const char_t *flnm, char_t *path);
void extractfilepath(const char_t *flnm, ffstring &path);
void extractfilename(const char_t *flnm, char_t *nameext);
void extractfilename(const char_t *flnm, ffstring &nameext);
void extractfilenameWOext(const char_t *flnm, char_t *name);
void extractfilenameWOext(const char_t *flnm, ffstring &name);
void extractfileext(const char_t *flnm, char_t *ext); //without the .
void extractfileext(const char_t *flnm, ffstring &ext);
void changepathext(const char_t *flnm, const char_t *ext, ffstring &path);
int nCopyAnsiToWideChar(WCHAR *pWCStr, PCTSTR pAnsiIn, int cchAnsi = 0);
char *unicode16toAnsi(const WCHAR *data16, int data16len = -1, char *data8 = NULL);
char *utf8toAnsi(const char *data, int datalen = -1, char *data8 = NULL);
wchar_t *utf8toUnicode(const char *data, int datalen, wchar_t *data16 = NULL);
unsigned int ff_sqrt(unsigned int a);
int av_log2(unsigned int v);
int64_t lavc_gcd(int64_t a, int64_t b);
int lavc_reduce(int *dst_nom, int *dst_den, int64_t nom, int64_t den, int64_t max);
void saveFrame(unsigned int num, const unsigned char *buf, size_t len);
char_t *readTextFile(const char_t *filename);
struct TffPictBase;
bool decodeMPEGsequenceHeader(bool mpeg2, const unsigned char *hdr, size_t len, TffPictBase &pict, bool *isH264);
bool decodeMPEG4pictureHeader(const unsigned char *hdr, size_t len, TffPictBase &pict);
bool decodeH264SPS(const unsigned char *hdr, size_t len, TffPictBase &pict, H264_SPS* sps = NULL);
void getChildWindows(HWND h, THWNDs &lst);
void randomize(void);
int countbits(uint32_t x);
enum {
    FNM_NOESCAPE = 0x01,
    FNM_PATHNAME = 0x02,
    FNM_NOCASE   = 0x08
};
bool fnmatch(const char_t *pattern, const char_t *string, int flags = FNM_NOCASE | FNM_NOESCAPE);
void setThreadName(DWORD dwThreadID, LPCSTR szThreadName);
char_t *guid2str(const GUID &riid, char_t *dest, int bufsize);
inline FOURCC FCCupper(FOURCC fourCC)
{
    return toupper(fourCC & 0xFF) + (toupper((fourCC >> 8) & 0xFF) << 8) + (toupper((fourCC >> 16) & 0xFF) << 16) + (toupper((fourCC >> 24) & 0xFF) << 24);
}
const char_t *fourcc2str(FOURCC fcc, char_t *name, size_t namelength);
FOURCC hdr2fourcc(const BITMAPINFOHEADER *hdr, const GUID *subtype);
void fixMPEGinAVI(FOURCC &fcc);
HWND createInvisibleWindow(HINSTANCE hi, const char_t *classname, const char_t *windowname, WNDPROC wndproc, void* lparam, ATOM *atom);
void getCLSIDname(const CLSID &clsid, char_t *buf, size_t buflen);
void *getAlignedPtr(void *ptr);

// if T is char, ff_abs(0x80) returns -128. Users have to cast the result to unsinged char.
// For this reason, I deleted this template function.
// template<class T> inline T ff_abs(const T &x)
// {
//  return (x<0)?T(-x):T(x);
// }

inline unsigned char ff_abs(char x)
{
    return (unsigned char)((x < 0) ? -x : x);
}

inline unsigned short ff_abs(short x)
{
    return (unsigned short)((x < 0) ? -x : x);
}

inline unsigned int ff_abs(int x)
{
    return (unsigned int)((x < 0) ? -x : x);
}

inline unsigned long ff_abs(long x)
{
    return (unsigned long)((x < 0) ? -x : x);
}

inline uint64_t ff_abs(int64_t x)
{
    return uint64_t((x < 0) ? -x : x);
}

inline float ff_abs(float x)
{
    return (x < 0) ? -x : x;
}

inline double ff_abs(double x)
{
    return (x < 0) ? -x : x;
}

template<class T> inline T odd2even(T x)
{
    return x & 1 ?
           x + 1 :
           x;
}

inline int ff_round(double x)
{
    return int(x + (x > 0.0 ? 0.5 : -0.5));
}
inline int ff_round(float x)
{
    return int(x + (x > 0.0f ? 0.5f : -0.5f));
}

template<class T> inline T roundRshift(const T &x, int s)
{
    return (x + (1 << s) / 2) >> s;
}
template<class T> inline T roundDiv(const T &a, const T &b)
{
    return (a > 0 ? a + (b >> 1) : a - (b >> 1)) / b;
}

template<class T> inline T sqr(const T &a)
{
    return a * a;
}

template <class Tin, class Tout> inline Tout mapRange(Tin in, const std::pair<Tin, Tin> &inrange, const std::pair<Tout, Tout> &outrange)
{
    if (inrange.first != inrange.second && outrange.first != outrange.second) {
        return Tout((outrange.second - outrange.first) * (in - inrange.first) / (inrange.second - inrange.first) + outrange.first);
    } else {
        return Tout(0);
    }
}

template<class T> inline const T& limit(const T& val, const T& min, const T& max)
{
    if (val < min) {
        return min;
    } else if (val > max) {
        return max;
    } else {
        return val;
    }
}
static inline uint8_t limit_uint8(int a)
{
    if (a & (~255)) {
        return uint8_t((-a) >> 31);
    } else {
        return uint8_t(a);
    }
}

template<class T> inline bool isIn(const T& x, const T& min, const T& max)
{
    return (min <= x && x <= max);
}

template<class T> inline bool isIn(const std::vector<T> &v, const T& a)
{
    return std::find(v.begin(), v.end(), a) != v.end();
}

template<class T> inline char sign(const T& x)
{
    if (x < 0) {
        return -1;
    } else if (x > 0) {
        return 1;
    } else {
        return 0;
    }
}

template<class T> inline bool odd(T x)
{
    return x & 1;
}
template<class T> inline bool even(T x)
{
    return (x & 1) == 0;
}

static inline double value2db(double value)
{
    return value > 0 ? log10(value) * 20.0 : 0.0;
}
static inline double db2value(double db)
{
    return pow(10.0, db / 20.0);
}
static inline double db2value(double db, int mul)
{
    return pow(10.0, db / (mul * 20.0));
}

static inline float value2db(float value)
{
    return value > 0 ? log10f(value) * 20.0f : 0.0f;
}
static inline float db2value(float db)
{
    return powf(10.0f, db / 20.0f);
}
static inline float db2value(float db, int mul)
{
    return powf(10.0f, db / (mul * 20.0f));
}
static inline int ffalign(int x, int align)
{
    return (x + align - 1) & ~(align - 1);
}
static inline int ffalign(unsigned int x, int align)
{
    return (x + align - 1) & ~(align - 1);
}

class REFERENCE_TIME_to_hhmmssmmm
{
private:
    wchar_t str[16];
public:
    REFERENCE_TIME_to_hhmmssmmm(REFERENCE_TIME reftime);
    const wchar_t* get_str() {
        return str;
    }
};

template<typename T> inline const T& bswap(T& var)
{
    BYTE* s = (BYTE*)&var;
    for (BYTE* d = s + sizeof(var) - 1; s < d; s++, d--) {
        *s ^= *d, *d ^= *s, *s ^= *d;
    }
    return var;
}

#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
template<typename T, typename U> struct IsSameType {
    enum {value = false};
};
template<typename T> struct IsSameType<T, T> {
    enum {value = true};
};

template<bool is, typename T, typename U> struct selectType {
    typedef T result;
};
template<typename T, typename U> struct selectType<false, T, U> {
    typedef U result;
};

template<int v> struct intToVal {
    static const int value = v;
};
#endif

void dumpBytes(const char_t *flnm, const unsigned char *buf, size_t len);

inline double psnr(double d)
{
    if (d == 0) {
        return -1;
    }
    return -10.0 * log(d) / log(10.0);
}

template<class Tptr> inline Tptr* lpwAlign(Tptr* pIn)
{
    DWORD_PTR ul;
    ul = (DWORD_PTR) pIn;
    ul += 3;
    ul >>= 2;
    ul <<= 2;
    return (Tptr*)ul;
}

template<template <class> class alloc> struct TbyteBufferBase : public std::vector<unsigned char, alloc<unsigned char> > {
    TbyteBufferBase(void) {}
    TbyteBufferBase(size_t Isize): std::vector<typename TbyteBufferBase<alloc>::value_type, typename TbyteBufferBase<alloc>::allocator_type>(Isize) {}
    TbyteBufferBase(const void *ptr, size_t len) {
        append(ptr, len);
    }
    template<class Ta> void append(const Ta &b) {
        insert(this->end(), (const unsigned char*)&b, (const unsigned char*)&b + sizeof(Ta));
    }
    void append(const void *ptr, size_t len) {
        insert(this->end(), (const unsigned char*)ptr, (const unsigned char*)ptr + len);
    }
};
typedef TbyteBufferBase<std::allocator> TbyteBuffer;
typedef TbyteBufferBase<aligned_allocator> TbyteBufferA;

class Tbuffer
{
private:
    void *buf;
    size_t buflen;
public:
    Tbuffer(void): buf(NULL), buflen(0), free(true) {}
    Tbuffer(size_t Ibuflen): buf(NULL), buflen(0), free(true) {
        alloc(Ibuflen);
    }
    ~Tbuffer() {
        if (free) {
            clear();
        }
    }
    bool free;
    void clear(void) {
        if (buf) {
            aligned_free(buf);
        }
        buf = NULL;
        buflen = 0;
    }
    size_t size(void) const {
        return buflen;
    }
    void* alloc(size_t Ibuflen) {
        if (buflen < Ibuflen) {
            buf = aligned_realloc(buf, buflen = Ibuflen);
        }
        return buf;
    }
    void* allocZ(size_t Ibuflen, int z) { // clear memory with z
        if (buflen < Ibuflen) {
            buf = aligned_realloc(buf, buflen = Ibuflen);
            memset(buf, z, Ibuflen);
        }
        return buf;
    }
    void* resize(size_t Ibuflen) {
        if (buflen < Ibuflen) {
            void *temp = aligned_realloc(NULL, Ibuflen);
            if (temp && buf && buflen) {
                memcpy(temp, buf, buflen);
            }
            if (buf) {
                aligned_free(buf);
            }
            buf = temp;
            buflen = Ibuflen;
        }
        return buf;
    }
    void* resize2(size_t Ibuflen) {
        if (buflen < Ibuflen) {
            void *temp = aligned_realloc(NULL, Ibuflen * 2);
            if (temp && buf && buflen) {
                memcpy(temp, buf, buflen);
            }
            if (buf) {
                aligned_free(buf);
            }
            buf = temp;
            buflen = Ibuflen * 2;
        }
        return buf;
    }
    template<class T> operator T*() const {
        return (T*)buf;
    }
};

template<class T, class alloc = std::allocator<T> > struct vectorEx : public std::vector<T, alloc> {
private:
    void add(const T &t1, va_list marker) {
        push_back(t1);
        while (T t = va_arg(marker, T)) {
            push_back(t);
        }
    }
public:
    vectorEx(void) {}
    vectorEx(T t) {
        push_back(t);
    }
    vectorEx(T t1, T t2, ...) {
        if (!t1) {
            return;
        }
        push_back(t1);
        va_list marker;
        va_start(marker, t2);
        if (t2) {
            add(t2, marker);
        }
        va_end(marker);
    }
    void add(T t) {
        push_back(t);
    }
    void add(T t1, T t2, ...) {
        push_back(t1);
        va_list marker;
        va_start(marker, t2);
        if (t2) {
            add(t2, marker);
        }
        va_end(marker);
    }
    void addEnd(T tend, T t1, T t2, ...) {
        push_back(t1);
        push_back(t2);
        va_list marker;
        va_start(marker, t2);
        T t;
        while ((t = va_arg(marker, T)) != tend) {
            push_back(t);
        }
        va_end(marker);
    }
};

struct YUVcolor {
    uint32_t r, g, b;
    unsigned char Y;
    char U, V;
    YUVcolor(void) {
        Y = U = V = 0;
        r = g = b = 0;
    }
    YUVcolor(COLORREF rgb);
};


struct YUVcolorA {
    uint32_t r, g, b;
    uint32_t Y, U, V, A;

    COLORREF m_rgb;   // duplicated data, just for convenience 0x00bbggrr
    uint32_t m_aaa64; // duplicated data, just for convenience. DVD's contrast 0-15, this: 0-64. 0x00404040, at max
    YUVcolorA();
    YUVcolorA(YUVcolor yuv, unsigned int alpha = 256);
    YUVcolorA(COLORREF rgb, unsigned int alpha = 256);
    struct vobsubWeirdCsp_t {};
    YUVcolorA(COLORREF rgb, vobsubWeirdCsp_t);
    bool operator !=(const YUVcolorA &rt) const {
        return !(r == rt.r && g == rt.g && b == rt.b && A == rt.A);
    }
    bool operator ==(const YUVcolorA &rt) const {
        return (r == rt.r && g == rt.g && b == rt.b && A == rt.A);
    }
    bool operator ==(const _AM_DVD_YUV &rt) const {
        return (Y == rt.Y && U == rt.U && V == rt.V && A == rt.Reserved);
    }
    bool operator !=(const _AM_DVD_YUV &rt) const {
        return !(Y == rt.Y && U == rt.U && V == rt.V && A == rt.Reserved);
    }
    YUVcolorA& operator =(const _AM_DVD_YUV &rt);
    bool isGray() {
        return (r == g && r == b);
    }
    void setAlpha(uint32_t alpha);
};

enum {
    rfReplaceAll = 1,
    rfIgnoreCase = 2
};
template<class replstring, class ffstring> static inline replstring stringreplace0(const replstring &s0, const ffstring &oldstr, const ffstring &newstr, int flags)
{
    replstring s = s0;
    size_t pos = replstring::npos + 1, oldstrsize = oldstr.size(), newstrsize = newstr.size();
    for (;;) {
        pos = s.find(oldstr, pos);
        if (pos == replstring::npos) {
            break;
        }
        s.replace(pos, oldstrsize, newstr);
        if ((flags & rfReplaceAll) == 0) {
            break;
        }
        pos += newstrsize;
    }
    return s;
}
#if defined(__INTEL_COMPILER) || defined(__GNUC__) || (_MSC_VER>=1300)
template<template<class char_t> class replstring> replstring<char> stringreplace(const replstring<char> &s0, const DwString<char> &oldstr, const DwString<char> &newstr, int flags = 0)
{
    return stringreplace0< replstring<char>, DwString<char> >(s0, oldstr, newstr, flags);
}
template<template<class char_t> class replstring> replstring<wchar_t> stringreplace(const replstring<wchar_t> &s0, const DwString<wchar_t> &oldstr, const DwString<wchar_t> &newstr, int flags = 0)
{
    return stringreplace0< replstring<wchar_t>, DwString<wchar_t> >(s0, oldstr, newstr, flags);
}
#endif

#ifndef HKEY_PERFORMANCE_TEXT
#define HKEY_PERFORMANCE_TEXT       (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#endif

#ifndef HKEY_PERFORMANCE_NLSTEXT
#define HKEY_PERFORMANCE_NLSTEXT    (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )
#endif

#ifndef ListView_SetExtendedListViewStyleEx
#define ListView_SetExtendedListViewStyleEx(hwndLV, dwMask, dw)\
        (DWORD)SNDMSG((hwndLV), LVM_SETEXTENDEDLISTVIEWSTYLE, dwMask, dw)
#endif

#ifndef ListView_SetItemCountEx
#define ListView_SetItemCountEx(hwndLV, cItems, dwFlags) \
  SNDMSG((hwndLV), LVM_SETITEMCOUNT, (WPARAM)(cItems), (LPARAM)(dwFlags))
#endif

#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck) \
  ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#endif

#ifndef ListView_GetCheckState
#define ListView_GetCheckState(hwndLV, i) \
   ((((UINT)(SNDMSG((hwndLV), LVM_GETITEMSTATE, (WPARAM)(i), LVIS_STATEIMAGEMASK))) >> 12) -1)
#endif

#ifndef OFN_ENABLESIZING
#define OFN_ENABLESIZING 0x00800000
#endif

#ifndef TV_SORTCB
typedef struct tagTVSORTCB {
    HTREEITEM       hParent;
    PFNTVCOMPARE    lpfnCompare;
    LPARAM          lParam;
} TVSORTCB, *LPTVSORTCB;
#endif

#ifdef __GNUC__
typedef CONST DLGTEMPLATE *LPCDLGTEMPLATEA;
#endif

#ifndef TVN_GETINFOTIP
#define TVN_GETINFOTIPA         (TVN_FIRST-13)
#define TVN_GETINFOTIP          TVN_GETINFOTIPA
#endif

#ifndef NMTVGETINFOTIP
#define TVN_GETINFOTIP          TVN_GETINFOTIPA
#define NMTVGETINFOTIP          NMTVGETINFOTIPA
#define LPNMTVGETINFOTIP        LPNMTVGETINFOTIPA
typedef struct tagNMTVGETINFOTIPA {
    NMHDR hdr;
    LPSTR pszText;
    int cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPA, *LPNMTVGETINFOTIPA;
#endif

#ifndef LPTV_HITTESTINFO
#define LPTV_HITTESTINFO   LPTVHITTESTINFO
#define   TV_HITTESTINFO     TVHITTESTINFO
typedef struct tagTVHITTESTINFO {
    POINT       pt;
    UINT        flags;
    HTREEITEM   hItem;
} TVHITTESTINFO, *LPTVHITTESTINFO;
#endif

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

#ifndef TVS_NOHSCROLL
#define TVS_NOHSCROLL 0x8000
#endif

#ifndef LVN_GETINFOTIP
typedef struct tagNMLVGETINFOTIPA {
    NMHDR hdr;
    DWORD dwFlags;
    LPSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPA, *LPNMLVGETINFOTIPA;

typedef struct tagNMLVGETINFOTIPW {
    NMHDR hdr;
    DWORD dwFlags;
    LPWSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPW, *LPNMLVGETINFOTIPW;

// NMLVGETINFOTIPA.dwFlag values

#define LVGIT_UNFOLDED  0x0001

#define LVN_GETINFOTIPA          (LVN_FIRST-57)
#define LVN_GETINFOTIPW          (LVN_FIRST-58)

#ifdef UNICODE
#define LVN_GETINFOTIP          LVN_GETINFOTIPW
#define NMLVGETINFOTIP          NMLVGETINFOTIPW
#define LPNMLVGETINFOTIP        LPNMLVGETINFOTIPW
#else
#define LVN_GETINFOTIP          LVN_GETINFOTIPA
#define NMLVGETINFOTIP          NMLVGETINFOTIPA
#define LPNMLVGETINFOTIP        LPNMLVGETINFOTIPA
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#endif

#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#endif

#ifndef DWLP_MSGRESULT
#define DWLP_MSGRESULT DWL_MSGRESULT
#endif

#endif

#ifndef __IPropertyPageSite_INTERFACE_DEFINED__
#define PROPPAGESTATUS_CLEAN 0x4
#endif

// encoding
struct ENC_MODE {
    enum {
        UNKNOWN      = -1,
        CBR          = 0,
        VBR_QUAL     = 1,
        VBR_QUANT    = 2,
    };
};

extern const char_t *encQuantTypes[];

struct Taspect {
    const char_t *caption;
    float x, y;
};
extern const Taspect sampleAspects[], displayAspects[];

struct FFSTATS {
    enum {
        UNUSED = 0,
        WRITE  = 1,
        READ   = 2,
        RW = WRITE | READ
    };
};

struct CREDITS_MODE {
    enum {
        PERCENT = 0,
        QUANT   = 1,
        SIZE    = 2
    };
};

struct CREDITS_POS {
    enum {
        NONE  = 0,
        START = 1,
        END   = 2
    };
};

struct QUANT {
    enum {
        H263    = 0,
        MPEG    = 1,
        MOD     = 2,
        MOD_NEW = 3,
        CUSTOM  = 4,
        JVT     = 5
    };
};

struct TmultipleInstances {
    int id;
    const char_t *name;
};
extern const TmultipleInstances multipleInstances[];

// unlocks a critical section, and locks it automatically
// when the lock goes out of scope
class CAutoUnlock
{

    // make copy constructor and assignment operator inaccessible

    CAutoUnlock(const CAutoUnlock &refAutoLock);
    CAutoUnlock &operator=(const CAutoUnlock &refAutoLock);

protected:
    CCritSec * m_pLock;

public:
    CAutoUnlock(CCritSec * plock) {
        m_pLock = plock;
        m_pLock->Unlock();
    };

    ~CAutoUnlock() {
        m_pLock->Lock();
    };
};

// defferred lock
// at the time of construction, you may not know the address of Lockable.
// explicitly call lock to lock.
// unlocked on destruction.
template <typename Lockable> class deferred_lock:
    boost::noncopyable
{
    Lockable *lockptr;
public:
    deferred_lock(): lockptr(NULL) {}
    void lock(Lockable *l) {
        lockptr = l;
        l->lock();
    }
    ~deferred_lock() {
        if (lockptr) {
            lockptr->unlock();
        }
    }
};

struct defer_priority_set_t
{};

class TthreadPriority
{
    HANDLE thread;
    bool changed;
    int priority_on_destruction;
public:
    TthreadPriority(HANDLE Ithread, int Ipriority_on_destruction, defer_priority_set_t) :
        thread(Ithread),
        changed(false),
        priority_on_destruction(Ipriority_on_destruction) {
    }

    TthreadPriority(HANDLE Ithread, int Ipriority_on_construction, int Ipriority_on_destruction) :
        thread(Ithread),
        changed(true),
        priority_on_destruction(Ipriority_on_destruction) {
        SetThreadPriority(thread, Ipriority_on_construction);
    }

    void set_priority(int priority) {
        SetThreadPriority(thread, priority);
        changed = true;
    }

    ~TthreadPriority() {
        if (changed) {
            SetThreadPriority(thread, priority_on_destruction);
        }
    }
};

struct Trt2str:
    public ffstring {
    Trt2str(REFERENCE_TIME rt0) {
        uint64_t rt = ff_abs(rt0);
        int below_ms = rt % 10000;
        rt -= below_ms;
        rt /= 10000;
        int ms = rt % 1000;
        rt -= ms;
        rt /= 1000;
        int s = rt % 60;
        rt -= s;
        rt /= 60;
        int m = rt % 60;
        rt -= m;
        rt /= 60;
        wchar_t buf[32];
        _snwprintf_s(buf, countof(buf), _TRUNCATE, L"%02d:%02d:%02d %03d %04d", int(rt), m, s, ms, below_ms);
        if (rt0 < 0) {
            ffstring::operator = (L"-");
        } else {
            ffstring::operator = (L"");
        }
        ffstring::operator += (buf);
    }
};
#endif //FFDEFS_STRICT

#endif
