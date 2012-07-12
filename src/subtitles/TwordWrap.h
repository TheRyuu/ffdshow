#pragma once

class TwordWrap
{
private:
    int mode;
    int *pwidths;
    ffstring str;
    int splitdxMax, splitdxMin;
    std::deque<int> rightOfTheLines;
    int smart(void);
    int smartReverse(void);
    int nextChar(int x);
    int prevChar(int x);
    int pwidthsLeft(int x);
    bool assCompatibleMode;
public:
    TwordWrap(int Imode, const wchar_t *Istr, int *Ipwidths, int IsplitdxMax, bool assCompatibleMode);
    //WrapStyle: Defines the default wrapping style.
    //0: smart wrapping, lines are evenly broken
    //1: end-of-line word wrapping
    //2: no word wrapping
    //3: same as 0, but lower line gets wider.
    void debugprint();
    int getRightOfTheLine(int n);
    int getLineCount(void) {
        return (int)rightOfTheLines.size();
    }
    static int iswspace(wchar_t ch); // custom iswspace that return false for 0xa0.
};
