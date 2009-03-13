#ifndef _CRECT_H_
#define _CRECT_H_

struct CPoint :POINT
{
 CPoint(void) {}
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
 CRect(void) {}
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
};

#endif
