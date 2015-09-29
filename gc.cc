// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "gc.h"
#include "ti.h"

namespace utio {

//----------------------------------------------------------------------

CGC::CGC (void)
:_canvas()
,_template()
,_size (0, 0)
,_tabSize (8)
{
}

void CGC::Resize (Size2d sz)
{
    fill (_size, 0);
    _canvas.clear();
    _canvas.resize (sz[0] * sz[1]);
    _size = sz;
}

inline CGC::canvas_t::iterator CGC::CanvasAt (Point2d p)
{
    return _canvas.begin() + p[1] * _size[0] + p[0];
}

inline CGC::canvas_t::const_iterator CGC::CanvasAt (Point2d p) const
{
    return _canvas.begin() + p[1] * _size[0] + p[0];
}

/// Clears the canvas with spaces with current attributes.
void CGC::Clear (wchar_t c)
{
    const CCharCell vlc (c, _template);
    fill (_canvas, vlc);
}

/// Draws a line art box.
void CGC::Box (Rect r)
{
    Clip (r);
    if ((r.Width() < 2) | (r.Height() < 2))
	return;
    r[1] -= 1;
    Point2d trCorner (r[1][0], r[0][1]);
    Point2d blCorner (r[0][0], r[1][1]);
    const Rect rr (trCorner, blCorner);
    HLine (r[0], r.Width());
    HLine (rr[1], r.Width());
    VLine (r[0], r.Height());
    VLine (rr[0], r.Height());
    *CanvasAt(rr[0]) = CCharCell (acsv_UpperRightCorner, _template);
    *CanvasAt(rr[1]) = CCharCell (acsv_LowerLeftCorner, _template);
    *CanvasAt(r[1]) = CCharCell (acsv_LowerRightCorner, _template);
    *CanvasAt(r[0]) = CCharCell (acsv_UpperLeftCorner, _template);
}

/// Draws a box with character \p c as the border.
void CGC::Bar (Rect r, wchar_t c)
{
    Clip (r);
    const CCharCell vlc (c, _template);
    for (dim_t y = 0; y < r.Height(); ++ y)
	fill_n (CanvasAt (Point2d (r[0][0], r[0][1] + y)), r.Width(), vlc);
}

/// Draws a horizontal line from \p p of length \p l.
void CGC::HLine (Point2d p, dim_t l)
{
    if (!Clip (p))
	return;
    if (coord_t(l) > _size[0] - p[0])
	l = _size[0] - p[0];
    fill_n (CanvasAt(p), l, CCharCell (acsv_HLine, _template));
}

/// Draws a vertical line from \p p of length \p l.
void CGC::VLine (Point2d p, dim_t l)
{
    if (!Clip (p))
	return;
    if (coord_t(l) > _size[1] - p[1])
	l = _size[1] - p[1];
    const CCharCell vlc (acsv_VLine, _template);
    for (dim_t i = 0; i < l; ++i)
	*CanvasAt (Point2d (p[0], p[1] + i)) = vlc;
}

/// Copies canvas data from \p r into \p cells.
void CGC::GetImage (Rect r, canvas_t& cells) const
{
    const auto inyskip = Width() - r.Width();
    Clip (r);
    cells.resize (r.Width() * r.Height());
    auto dout (cells.begin());
    auto din (CanvasAt (r[0]));
    for (auto y = 0u; y < r.Height(); ++y, din += inyskip)
	for (auto x = 0u; x < r.Width(); ++x, ++din, ++dout)
	    if (din->c)
		*dout = *din;
}

/// Copies canvas data from \p cells into \p r.
void CGC::Image (Rect r, const canvas_t& cells)
{
    const auto outyskip = Width() - r.Width();
    Clip (r);
    auto din (cells.begin());
    auto dout (CanvasAt (r[0]));
    for (auto y = 0u; y < r.Height(); ++ y, dout += outyskip)
	for (auto x = 0u; x < r.Width(); ++ x, ++ din, ++ dout)
	    if (din->c)
		*dout = *din;
}

/// Zeroes out cells which are identical to those in \p src.
bool CGC::MakeDiffFrom (const CGC& src)
{
    assert (src.Canvas().size() == _canvas.size() && "Diffs can only be made on equally sized canvasses");
    auto inew (Canvas().begin());
    auto iold (src.Canvas().begin());
    const auto iend (src.Canvas().end());
    const CCharCell nullCell (0, color_Preserve, color_Preserve, 0);
    bool bHaveChanges = false;
    for (; iold < iend; ++iold, ++inew) {
	const bool bSameCell (*iold == *inew);
	if (bSameCell)
	    *inew = nullCell;
	bHaveChanges |= !bSameCell;
    }
    return bHaveChanges;
}

/// Prints character \p c.
void CGC::Char (Point2d p, wchar_t c)
{
    if (Clip (p))
	*CanvasAt(p) = CCharCell (c, _template);
}

/// Prints string \p str at \p p.
void CGC::Text (Point2d p, const string& str)
{
    if (!Clip (p))
	return;
    auto doutstart (CanvasAt (p));
    auto doutend = doutstart + (_size[0] - p[0]);
    assert (doutend >= doutstart);
    auto dout (doutstart);
    for (auto si = str.utf8_begin(); si < str.utf8_end() && dout < doutend; ++si) {
	if (*si == '\t') {
	    const size_t absX = p[0] + distance (doutstart, dout);
	    size_t toTab = Align (absX + 1, _tabSize) - absX;
	    toTab = min (toTab, size_t(distance (dout, doutend)));
	    dout = fill_n (dout, toTab, CCharCell (' ', _template));
	} else
	    *dout++ = CCharCell (*si, _template);
    }
}

static const CGC::Point2d c_ZeroPoint (0, 0);

/// Clips point \p pt to the canvas size.
bool CGC::Clip (Point2d& pt) const
{
    const Point2d oldPoint (pt);
    Point2d bottomright (_size);
    bottomright -= 1;
    simd::pmax (c_ZeroPoint, pt);
    simd::pmin (bottomright, pt);
    return (oldPoint == pt) & (_size[0] > 0) & (_size[1] > 0);
}

/// Clips rectangle \p r to the canvas.
bool CGC::Clip (Rect& r) const
{
    const Rect oldRect (r);
    const Point2d bottomright (_size);
    simd::pmax (c_ZeroPoint, r[0]);
    simd::pmin (bottomright, r[0]);
    simd::pmax (c_ZeroPoint, r[1]);
    simd::pmin (bottomright, r[1]);
    if (r[0][1] > r[1][1])
	swap (r[0], r[1]);
    if (r[0][0] > r[1][0])
	swap (r[0][0], r[1][0]);
    simd::pmax (r[0], r[1]);
    return oldRect == r;
}

//----------------------------------------------------------------------

} // namespace utio
