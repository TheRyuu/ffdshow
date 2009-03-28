#ifndef _CRECT_H_
#define _CRECT_H_

struct CPoint :POINT
{
    CPoint(void) {x=0;y=0;}
    CPoint(int Ix,int Iy) {x=Ix;y=Iy;}
    inline void operator-=(const CPoint &pt)
    {
        x-=pt.x;
        y-=pt.y;
    }

    inline void scale(int xnum,int xden,int ynum,int yden)
    {
        if (xden==0 || yden==0) return;
        x=xnum*x/xden;
        y=ynum*y/yden;
    }

    bool operator != (const CPoint &rt) const
    {
        return !(x == rt.x && y == rt.y);
    }

    bool operator == (const CPoint &rt) const
    {
        return (x == rt.x && y == rt.y);
    }

    bool operator < (const CPoint &rt) const
    {
        if (y < rt.y) return true;
        if (y > rt.y) return false;
        if (x < rt.x) return true;
        return false;
    }

    bool operator > (const CPoint &rt) const
    {
        if (y > rt.y) return true;
        if (y < rt.y) return false;
        if (x > rt.x) return true;
        return false;
    }

    void operator = (const CPoint &rt)
    {
        x = rt.x;
        y = rt.y;
    }
};

struct CSize :SIZE
{
    CSize(void) {}
    CSize(int Icx,int Icy) {cx=Icx;cy=Icy;}
};

struct CRect :RECT
{
    CRect(void) {top=0;left=0;right=0;bottom=0;}
    CRect(const RECT &r) {top=r.top;left=r.left;bottom=r.bottom;right=r.right;}
    CRect(int Ileft,int Itop,int Iright,int Ibottom) {left=Ileft;right=Iright;top=Itop;bottom=Ibottom;}
    CRect(const CPoint &topleft,const CPoint &rightbottom) {left=topleft.x;top=topleft.y;right=rightbottom.x;bottom=rightbottom.y;}
    CRect(const CPoint &topleft,const CSize &size) {left=topleft.x;top=topleft.y;right=left+size.cx;bottom=top+size.cy;}
    inline void operator&=(const CRect &rect2)
    {
        RECT rect1=*this;
        IntersectRect(this,&rect1,&rect2);
    }

    inline void operator += (const CPoint &pt)
    {
        left+=pt.x;top+=pt.y;
        right+=pt.x;bottom+=pt.y;
    }

    inline void operator -= (const CPoint &pt)
    {
        left-=pt.x;top-=pt.y;
        right-=pt.x;bottom-=pt.y;
    }

    inline CRect operator + (const CRect &rt)
    {
        return CRect (left + rt.left, top + rt.top, right + rt.right, bottom + rt.bottom);
    }

    inline CRect operator - (const CRect &rt)
    {
        return CRect (left - rt.left, top - rt.top, right - rt.right, bottom - rt.bottom);
    }
    inline bool operator < (const CRect &rt) const
    {
        if (top < rt.top) return true;
        if (top > rt.top) return false;
        if (left < rt.left) return true;
        if (left > rt.left) return false;
        if (bottom < rt.bottom) return true;
        if (bottom > rt.bottom) return false;
        if (right < rt.right) return true;
        if (right > rt.right) return false;
        return false;
    }
    inline bool operator > (const CRect &rt) const
    {
        if (top < rt.top) return false;
        if (top > rt.top) return true;
        if (left < rt.left) return false;
        if (left > rt.left) return true;
        if (bottom < rt.bottom) return false;
        if (bottom > rt.bottom) return true;
        if (right < rt.right) return false;
        if (right > rt.right) return true;
        return false;
    }
    inline bool operator == (const CRect &rt) const
    {
        return 
               top == rt.top
         &&   left == rt.left
         && bottom == rt.bottom
         &&  right == rt.right;
    }
    inline bool operator != (const CRect &rt) const
    {
        return !(*this == rt);
    }
    inline int Width(void) const {return right-left;}
    inline int Height(void) const {return bottom-top;}
    inline CPoint TopLeft(void) const {return CPoint(left,top);}
    inline CPoint BottomRight(void) const {return CPoint(right,bottom);}
    inline CSize Size(void) const {return CSize(Width(),Height());}
    inline void scale(int xnum,int xden,int ynum,int yden)
    {
        if (xden==0 || yden==0) return;
        left=xnum*left/xden;top=ynum*top/yden;
        right=xnum*right/xden;bottom=ynum*bottom/yden;
    }
    bool checkOverlap(CRect &rt) const
    {
        if (top > rt.bottom) return false;
        if (bottom < rt.top) return false;
        if (left > rt.right) return false;
        if (right < rt.left) return false;
        return true;
    }
};

#endif
