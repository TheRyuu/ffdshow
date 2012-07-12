#ifndef _CRECT_H_
#define _CRECT_H_

template <class number_t, class POINT>
struct CPointT : POINT {
    CPointT() {
        x = 0;
        y = 0;
    }
    CPointT(number_t Ix, number_t Iy) {
        x = Ix;
        y = Iy;
    }
    inline void operator-=(const CPointT &pt) {
        x -= pt.x;
        y -= pt.y;
    }

    inline void scale(number_t xnum, number_t xden, number_t ynum, number_t yden) {
        if (xden == 0 || yden == 0) {
            return;
        }
        x = xnum * x / xden;
        y = ynum * y / yden;
    }

    bool operator != (const CPointT &rt) const {
        return !(x == rt.x && y == rt.y);
    }

    bool operator == (const CPointT &rt) const {
        return (x == rt.x && y == rt.y);
    }

    bool operator < (const CPointT &rt) const {
        if (y < rt.y) {
            return true;
        }
        if (y > rt.y) {
            return false;
        }
        if (x < rt.x) {
            return true;
        }
        return false;
    }

    bool operator > (const CPointT &rt) const {
        if (y > rt.y) {
            return true;
        }
        if (y < rt.y) {
            return false;
        }
        if (x > rt.x) {
            return true;
        }
        return false;
    }

    void operator = (const CPointT &rt) {
        x = rt.x;
        y = rt.y;
    }

    void operator = (const POINT &rt) {
        x = rt.x;
        y = rt.y;
    }
};

template <class number_t, class CSize>
struct CSizeT : CSize {
    CSizeT() {}
    CSizeT(number_t Icx, number_t Icy) {
        cx = Icx;
        cy = Icy;
    }
    // Operations
    BOOL operator==(CSize size) {
        return (cx == size.cx && cy == size.cy);
    }
    BOOL operator!=(CSize size) {
        return (cx != size.cx || cy != size.cy);
    }
};

template <class number_t, class RECT, class CPoint, class CSize>
struct CRectT : RECT {
    CRectT() {
        top = 0;
        left = 0;
        right = 0;
        bottom = 0;
    }
    CRectT(const RECT &r) {
        top = r.top;
        left = r.left;
        bottom = r.bottom;
        right = r.right;
    }
    CRectT(number_t Ileft, number_t Itop, number_t Iright, number_t Ibottom) {
        left = Ileft;
        right = Iright;
        top = Itop;
        bottom = Ibottom;
    }
    CRectT(const CPoint &topleft, const CPoint &rightbottom) {
        left = topleft.x;
        top = topleft.y;
        right = rightbottom.x;
        bottom = rightbottom.y;
    }
    CRectT(const CPoint &topleft, const CSize &size) {
        left = topleft.x;
        top = topleft.y;
        right = left + size.cx;
        bottom = top + size.cy;
    }
    inline void operator&=(const CRectT &rect2) {
        RECT rect1 = *this;
        IntersectRect(this, &rect1, &rect2);
    }

    inline void operator += (const CPoint &pt) {
        left += pt.x;
        top += pt.y;
        right += pt.x;
        bottom += pt.y;
    }

    inline void operator -= (const CPoint &pt) {
        left -= pt.x;
        top -= pt.y;
        right -= pt.x;
        bottom -= pt.y;
    }

    inline CRectT operator + (const CRectT &rt) {
        return CRectT(left + rt.left, top + rt.top, right + rt.right, bottom + rt.bottom);
    }

    inline CRectT operator - (const CRectT &rt) {
        return CRectT(left - rt.left, top - rt.top, right - rt.right, bottom - rt.bottom);
    }
    inline bool operator < (const CRectT &rt) const {
        if (top < rt.top) {
            return true;
        }
        if (top > rt.top) {
            return false;
        }
        if (left < rt.left) {
            return true;
        }
        if (left > rt.left) {
            return false;
        }
        if (bottom < rt.bottom) {
            return true;
        }
        if (bottom > rt.bottom) {
            return false;
        }
        if (right < rt.right) {
            return true;
        }
        if (right > rt.right) {
            return false;
        }
        return false;
    }
    inline bool operator > (const CRectT &rt) const {
        if (top < rt.top) {
            return false;
        }
        if (top > rt.top) {
            return true;
        }
        if (left < rt.left) {
            return false;
        }
        if (left > rt.left) {
            return true;
        }
        if (bottom < rt.bottom) {
            return false;
        }
        if (bottom > rt.bottom) {
            return true;
        }
        if (right < rt.right) {
            return false;
        }
        if (right > rt.right) {
            return true;
        }
        return false;
    }
    inline bool operator == (const CRectT &rt) const {
        return
            top == rt.top
            &&   left == rt.left
            && bottom == rt.bottom
            &&  right == rt.right;
    }
    inline bool operator != (const CRectT &rt) const {
        return !(*this == rt);
    }
    inline number_t Width(void) const {
        return right - left;
    }
    inline number_t Height(void) const {
        return bottom - top;
    }
    inline CPoint TopLeft(void) const {
        return CPoint(left, top);
    }
    inline CPoint BottomRight(void) const {
        return CPoint(right, bottom);
    }
    inline CSize Size(void) const {
        return CSize(Width(), Height());
    }
    inline void scale(number_t xnum, number_t xden, number_t ynum, number_t yden) {
        if (xden == 0 || yden == 0) {
            return;
        }
        left = xnum * left / xden;
        top = ynum * top / yden;
        right = xnum * right / xden;
        bottom = ynum * bottom / yden;
    }
    bool checkOverlap(CRectT &rt) const {
        if (top > rt.bottom) {
            return false;
        }
        if (bottom < rt.top) {
            return false;
        }
        if (left > rt.right) {
            return false;
        }
        if (right < rt.left) {
            return false;
        }
        return true;
    }
};

struct POINT_DOUBLE {
    double x, y;
};

struct SIZE_DOUBLE {
    double cx, cy;
};

struct RECT_DOUBLE {
    double left, top, right, bottom;
};

typedef CPointT<int, POINT> CPoint;
typedef CPointT<double, POINT_DOUBLE> CPointDouble;
typedef CSizeT<int, SIZE> CSize;
typedef CSizeT<double, SIZE_DOUBLE> CSizeDouble;
typedef CRectT<int, RECT, CPoint, CSize> CRect;
typedef CRectT<double, RECT_DOUBLE, CPointDouble, CSizeDouble> CRectDouble;

#endif
