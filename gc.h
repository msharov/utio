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
// gc.h
//

#ifndef GC_H_2E08A62D1060EA48355708BC7A3C12AF
#define GC_H_2E08A62D1060EA48355708BC7A3C12AF

#include "ticonst.h"
#include "gdt.h"

namespace utio {

/// \brief Graphics context.
///
/// Can be used to draw into a character cell array. Quite useful for
/// internal buffer (Canvas()).
///
class CGC {
public:
    typedef vector<CCharCell>	canvas_t;	///< Type of the output buffer.
    typedef gdt::coord_t	coord_t;
    typedef gdt::dim_t		dim_t;
    typedef gdt::Point2d	Point2d;
    typedef gdt::Size2d		Size2d;
    typedef gdt::Rect		Rect;
public:
				CGC (void);
    void			Clear (void);
    void			Box (Rect r);
    void			Bar (Rect r, wchar_t c);
    void			HLine (Point2d p, dim_t l);
    void			VLine (Point2d p, dim_t l);
    void			GetImage (Rect r, canvas_t& cells) const;
    void			Image (Rect r, const canvas_t& cells);
    void			Char (Point2d p, wchar_t c);
    void			Text (Point2d p, const string& str);
    inline void			Box (coord_t x1, coord_t y1, coord_t x2, coord_t y2)		{ Box (Rect (x1, y1, x2, y2)); }
    inline void			Bar (coord_t x1, coord_t y1, coord_t x2, coord_t y2, wchar_t c)	{ Bar (Rect (x1, y1, x2, y2), c); }
    inline void			HLine (coord_t x, coord_t y, dim_t l)				{ HLine (Point2d (x, y), l); }
    inline void			VLine (coord_t x, coord_t y, dim_t l)				{ VLine (Point2d (x, y), l); }
    inline void			GetImage (coord_t x1, coord_t y1, coord_t x2, coord_t y2, canvas_t& cells)	{ GetImage (Rect (x1, y1, x2, y2), cells); }
    inline void			Image (coord_t x1, coord_t y1, coord_t x2, coord_t y2, const canvas_t& cells)	{ Image (Rect (x1, y1, x2, y2), cells); }
    inline void			Char (coord_t x, coord_t y, wchar_t c)				{ Char (Point2d (x, y), c); }
    inline void			Text (coord_t x, coord_t y, const string& str)			{ Text (Point2d (x, y), str); }
    wchar_t			GraphicChar (EGraphicChar c) const;
    inline void			FgColor (EColor c)	{ m_Template.m_FgColor = c; }
    inline void			BgColor (EColor c)	{ m_Template.m_BgColor = c; }
    inline void			AttrOn (EAttribute a)	{ m_Template.SetAttr (a); }
    inline void			AttrOff (EAttribute a)	{ m_Template.ClearAttr (a); }
    inline void			AllAttrsOff (void)	{ m_Template.m_Attrs = 0; }
    inline const canvas_t&	Canvas (void) const	{ return (m_Canvas); }
    inline canvas_t&		Canvas (void)		{ return (m_Canvas); }
    inline const Size2d&	ScreenSize (void) const	{ return (m_Size); }
    void			Resize (Size2d sz);
    void			Clip (Rect& r) const;
    void			Clip (Point2d& r) const;
    inline void			SetTabSize (size_t nts = 8)	{ assert (nts && "Tab size can not be 0"); m_TabSize = nts; }
private:
    inline canvas_t::iterator		CanvasAt (Point2d p);
    inline canvas_t::const_iterator	CanvasAt (Point2d p) const;
private:
    canvas_t			m_Canvas;	///< The output buffer.
    CCharCell			m_Template;	///< Current drawing values.
    Size2d			m_Size;		///< Size of the output buffer.
    size_t			m_TabSize;	///< Tab size as expanded by Text
};

} // namespace utio

#endif

