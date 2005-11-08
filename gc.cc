// This file is part of the utio library, a terminal I/O library.
//
// Copyright (C) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.
//
// gc.cc
//

#include "gc.h"
#include "ti.h"

namespace utio {

//----------------------------------------------------------------------

CGC::CGC (void)
: m_Canvas (),
  m_Template (),
  m_Size (0, 0),
  m_TabSize (8)
{
}

void CGC::Resize (Size2d sz)
{
    fill (m_Size, 0);
    m_Canvas.clear();
    m_Canvas.resize (sz[0] * sz[1]);
    m_Size = sz;
}

inline CGC::canvas_t::iterator CGC::CanvasAt (Point2d p)
{
    return (m_Canvas.begin() + p[1] * m_Size[0] + p[0]);
}

inline CGC::canvas_t::const_iterator CGC::CanvasAt (Point2d p) const
{
    return (m_Canvas.begin() + p[1] * m_Size[0] + p[0]);
}

/// Clears the canvas with spaces with current attributes.
void CGC::Clear (wchar_t c)
{
    const CCharCell vlc (c, m_Template);
    fill (m_Canvas, vlc);
}

/// Draws a line art box.
void CGC::Box (Rect r)
{
    Clip (r);
    r[1] -= 1;
    Point2d trCorner (r[1][0], r[0][1]);
    Point2d blCorner (r[0][0], r[1][1]);
    const Rect rr (trCorner, blCorner);
    HLine (r[0], r.Width());
    HLine (rr[1], r.Width());
    VLine (r[0], r.Height());
    VLine (rr[0], r.Height());
    *CanvasAt(rr[0]) = CCharCell (acsv_UpperRightCorner, m_Template);
    *CanvasAt(rr[1]) = CCharCell (acsv_LowerLeftCorner, m_Template);
    *CanvasAt(r[1]) = CCharCell (acsv_LowerRightCorner, m_Template);
    *CanvasAt(r[0]) = CCharCell (acsv_UpperLeftCorner, m_Template);
}

/// Draws a box with character \p c as the border.
void CGC::Bar (Rect r, wchar_t c)
{
    Clip (r);
    const CCharCell vlc (c, m_Template);
    for (dim_t y = 0; y < r.Height(); ++ y)
	fill_n (CanvasAt (Point2d (r[0][0], r[0][1] + y)), r.Width(), vlc);
}

/// Draws a horizontal line from \p p of length \p l.
void CGC::HLine (Point2d p, dim_t l)
{
    if (!Clip (p))
	return;
    if (coord_t(l) > m_Size[0] - p[0])
	l = m_Size[0] - p[0];
    const CCharCell vlc (acsv_HLine, m_Template);
    fill_n (CanvasAt(p), l, vlc);
}

/// Draws a vertical line from \p p of length \p l.
void CGC::VLine (Point2d p, dim_t l)
{
    if (!Clip (p))
	return;
    if (coord_t(l) > m_Size[1] - p[1])
	l = m_Size[1] - p[1];
    const CCharCell vlc (acsv_VLine, m_Template);
    for (dim_t i = 0; i < l; ++ i)
	*CanvasAt (Point2d (p[0], p[1] + i)) = vlc;
}

/// Copies canvas data from \p r into \p cells.
void CGC::GetImage (Rect r, canvas_t& cells) const
{
    const size_t inyskip = Width() - r.Width();
    Clip (r);
    cells.resize (r.Width() * r.Height());
    canvas_t::iterator dout (cells.begin());
    canvas_t::const_iterator din (CanvasAt (r[0]));
    for (dim_t y = 0; y < r.Height(); ++ y, din += inyskip)
	for (dim_t x = 0; x < r.Width(); ++ x, ++ din, ++ dout)
	    if (din->m_Char)
		*dout = *din;
}

/// Copies canvas data from \p cells into \p r.
void CGC::Image (Rect r, const canvas_t& cells)
{
    const size_t outyskip = Width() - r.Width();
    Clip (r);
    canvas_t::const_iterator din (cells.begin());
    canvas_t::iterator dout (CanvasAt (r[0]));
    for (dim_t y = 0; y < r.Height(); ++ y, dout += outyskip)
	for (dim_t x = 0; x < r.Width(); ++ x, ++ din, ++ dout)
	    if (din->m_Char)
		*dout = *din;
}

/// Zeroes out cells which are identical to those in \p src.
void CGC::MakeDiffFrom (const CGC& src)
{
    assert (src.Canvas().size() == m_Canvas.size() && "Diffs can only be made on equally sized canvasses");
    register canvas_t::pointer inew (Canvas().begin());
    register canvas_t::const_pointer iold (src.Canvas().begin());
    const canvas_t::const_pointer iend (src.Canvas().end());
    const CCharCell nullCell (0, color_Preserve, color_Preserve, 0);
    for (; iold < iend; ++ iold, ++ inew)
	if (*iold == *inew)
	    *inew = nullCell;
}

/// Prints character \p c.
void CGC::Char (Point2d p, wchar_t c)
{
    *CanvasAt(p) = CCharCell (c, m_Template);
}

/// Prints string \p str at \p p.
void CGC::Text (Point2d p, const string& str)
{
    if (!Clip (p))
	return;
    const canvas_t::iterator doutstart (CanvasAt (p));
    const canvas_t::iterator doutend = doutstart + (m_Size[0] - p[0]);
    assert (doutend >= doutstart);
    canvas_t::iterator dout (doutstart);
    for (string::utf8_iterator si = str.utf8_begin(); si < str.utf8_end() && dout < doutend; ++ si) {
	if (*si == '\t') {
	    const size_t absX = p[0] + distance (doutstart, dout);
	    size_t toTab = Align (absX + 1, m_TabSize) - absX;
	    for (; toTab && dout < doutend; --toTab, ++dout)
		*dout = CCharCell (' ', m_Template);
	} else
	    *dout++ = CCharCell (*si, m_Template);
    }
}

static const CGC::Point2d c_ZeroPoint (0, 0);

/// Clips point \p pt to the canvas size.
bool CGC::Clip (Point2d& pt) const
{
    const Point2d oldPoint (pt);
    Point2d bottomright (m_Size);
    bottomright -= 1;
    simd::pmax (c_ZeroPoint, pt);
    simd::pmin (bottomright, pt);
    return (oldPoint == pt);
}

/// Clips rectangle \p r to the canvas.
bool CGC::Clip (Rect& r) const
{
    const Rect oldRect (r);
    const Point2d bottomright (m_Size);
    simd::pmax (c_ZeroPoint, r[0]);
    simd::pmin (bottomright, r[0]);
    simd::pmax (c_ZeroPoint, r[1]);
    simd::pmin (bottomright, r[1]);
    if (r[0][1] > r[1][1])
	swap (r[0], r[1]);
    if (r[0][0] > r[1][0])
	swap (r[0][0], r[1][0]);
    return (oldRect == r);
}

//----------------------------------------------------------------------

} // namespace usio

