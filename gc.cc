// This file is part of the utio library, an terminal I/O library.
// Copyright (C) 2004 by Mike Sharov <msharov@talentg.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the 
// Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
// Boston, MA  02111-1307  USA.
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

/// Returns unicode value for ACS char \p c.
wchar_t	CGC::GraphicChar (EGraphicChar c) const
{
    return (CTerminfo::AcsUnicodeValue (c));
}

/// Clears the canvas with spaces with current attributes.
void CGC::Clear (void)
{
    const CCharCell vlc (' ', m_Template);
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
    *CanvasAt(rr[0]) = CCharCell (GraphicChar (acs_UpperRightCorner), m_Template);
    *CanvasAt(rr[1]) = CCharCell (GraphicChar (acs_LowerLeftCorner), m_Template);
    *CanvasAt(r[1]) = CCharCell (GraphicChar (acs_LowerRightCorner), m_Template);
    *CanvasAt(r[0]) = CCharCell (GraphicChar (acs_UpperLeftCorner), m_Template);
}

/// Draws a box with character \p c as the border.
void CGC::Bar (Rect r, wchar_t c)
{
    Clip (r);
    r[1] -= 1;
    const CCharCell vlc (c, m_Template);
    for (dim_t y = 0; y < r.Height(); ++ y)
	fill_n (CanvasAt (Point2d (r[0][0], r[0][1] + y)), r.Width(), vlc);
}

/// Draws a horizontal line from \p p of length \p l.
void CGC::HLine (Point2d p, dim_t l)
{
    Clip (p);
    if (coord_t(l) > m_Size[0] - p[0])
	l = m_Size[0] - p[0];
    const CCharCell vlc (GraphicChar (acs_HLine), m_Template);
    fill_n (CanvasAt(p), l, vlc);
}

/// Draws a vertical line from \p p of length \p l.
void CGC::VLine (Point2d p, dim_t l)
{
    Clip (p);
    if (coord_t(l) > m_Size[1] - p[1])
	l = m_Size[1] - p[1];
    const CCharCell vlc (GraphicChar (acs_VLine), m_Template);
    for (dim_t i = 0; i < l; ++ i)
	*CanvasAt (Point2d (p[0], p[1] + i)) = vlc;
}

/// Copies canvas data from \p r into \p cells.
void CGC::GetImage (Rect r, canvas_t& cells) const
{
    Clip (r);
    cells.resize (r.Width() * r.Height());
    canvas_t::iterator dout (cells.begin());
    for (dim_t y = 0; y < r.Height(); ++ y, dout += r.Width())
	copy_n (CanvasAt (Point2d (r[0][0], r[0][1] + y)), r.Width(), dout);
}

/// Copies canvas data from \p cells into \p r.
void CGC::Image (Rect r, const canvas_t& cells)
{
    Clip (r);
    canvas_t::const_iterator din (cells.begin());
    for (dim_t y = 0; y < r.Height(); ++ y, din += r.Width())
	copy_n (din, r.Width(), CanvasAt (Point2d (r[0][0], r[0][1] + y)));
}

/// Prints character \p c.
void CGC::Char (Point2d p, wchar_t c)
{
    *CanvasAt(p) = CCharCell (c, m_Template);
}

/// Prints string \p str at \p p.
void CGC::Text (Point2d p, const string& str)
{
    Clip (p);
    canvas_t::iterator dout (CanvasAt (p));
    const canvas_t::iterator doutstart = dout;
    const canvas_t::iterator doutend = dout + (m_Size[0] - p[0]);
    for (int i = 0; i < int(str.size()) && dout < doutend; ++ i, ++ dout) {
	if (str[i] == '\t') {
	    *dout = CCharCell (' ', m_Template);
	    if ((p[0] + distance(doutstart,dout)) % m_TabSize != m_TabSize - 1)
		-- i;
	} else
	    *dout = CCharCell (str[i], m_Template);
    }
}

/// Clips point \p pt to the canvas size.
void CGC::Clip (Point2d& pt) const
{
    for (uoff_t i = 0; i < pt.size(); ++ i) {
	if (pt[i] < 0)
	    pt[i] = 0;
	else if (pt[i] > coord_t(m_Size[i]))
	    pt[i] = m_Size[i];
    }
}

/// Clips rectangle \p r to the canvas.
void CGC::Clip (Rect& r) const
{
    foreach (Rect::iterator, i, r)
	Clip (*i);
    if (r[0][1] > r[1][1])
	swap (r[0], r[1]);
    if (r[0][0] > r[1][0])
	swap (r[0][0], r[1][0]);
}

//----------------------------------------------------------------------

} // namespace usio

