#pragma once

#include "Tsubtitles.h"

DECLARE_INTERFACE(IcheckSubtitle)
{
    STDMETHOD(checkSubtitle)(const char_t * subflnm) PURE; // S_OK on accept
};

#define SUBFILES_HEURISTIC_LIMIT 6

class TsubtitlesFile : public Tsubtitles
{
private:
    double fps;
    HANDLE hwatch;
    FILETIME lastwritetime;
    static void findPossibleSubtitles(const char_t *dir, strings &files);
    static const char_t *exts[];
    FILE *f;
    TstreamFile fs;
protected:
    virtual void checkChange(const TsubtitlesSettings *cfg, bool *forceChange);
public:
    /* Possible matching rules :
    - SUBFILES_ALL : find for all the subtitle files inside the search directory
    - SUBFILES_VIDEO_FILE_MATCH : find subtitles files where name matches exactly or partially (<video file>.<suffix>.<sub extension>)
    - SUBFILES_HEURISTIC : find subtitles files with heuristic algorithm (based on a percentage of similarity)
    - SUBFILES_VIDEO_FILE_HEURISTIC : use SUBFILES_HEURISTIC and if no results are returned use SUBFILES_HEURISTIC
    */
    enum subtitleFilesSearchMode { SUBFILES_ALL, SUBFILES_VIDEO_FILE_MATCH, SUBFILES_HEURISTIC, SUBFILES_VIDEO_FILE_HEURISTIC };
    static const char_t *mask;
    static bool extMatch(const char_t *flnm);
    static void findPossibleSubtitles(const char_t *aviFlnm, const char_t *sdir, strings &files, subtitleFilesSearchMode searchMode = SUBFILES_ALL);
    static void findSubtitlesFile(const char_t *aviFlnm, const char_t *sdir, const char_t *sext, char_t *subFlnm, size_t buflen, int heuristic, IcheckSubtitle *checkSubtitle);
    virtual Tsubtitle* getSubtitle(const TsubtitlesSettings *cfg, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, bool *forceChange);

    TsubtitlesFile(IffdshowBase *Ideci);
    virtual ~TsubtitlesFile();
    bool init(const TsubtitlesSettings *cfg, const char_t *subFlnm, double Ifps, bool watch, int checkOnly);
    virtual void done(void);
    char_t subFlnm[MAX_PATH];
};
