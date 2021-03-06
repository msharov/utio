// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
#include "ticonst.h"
#include "gdt.h"

namespace utio {

/// Graphics context. Can be used to draw into a character cell array.
/// Quite useful for internal buffer (Canvas()).
class CGC {
public:
    using canvas_t	= vector<CCharCell>;	///< Type of the output buffer.
    using coord_t	= gdt::coord_t;
    using dim_t		= gdt::dim_t;
    using Point2d	= gdt::Point2d;
    using Size2d	= gdt::Size2d;
    using Rect		= gdt::Rect;
public:
				CGC (void);
    void			Clear (wchar_t c = ' ');
    void			Box (Rect r);
    void			Bar (Rect r, wchar_t c = ' ');
    void			HLine (Point2d p, dim_t l);
    void			VLine (Point2d p, dim_t l);
    void			GetImage (Rect r, canvas_t& cells) const;
    void			Image (Rect r, const canvas_t& cells);
    void			Char (Point2d p, wchar_t c);
    void			Text (Point2d p, const string& str);
    inline const canvas_t&	Canvas (void) const	{ return _canvas; }
    inline canvas_t&		Canvas (void)		{ return _canvas; }
    inline const Size2d&	Size (void) const	{ return _size; }
    inline dim_t		Width (void) const	{ return _size[0]; }
    inline dim_t		Height (void) const	{ return _size[1]; }
    inline void			Box (coord_t x, coord_t y, dim_t w, dim_t h)				{ Box (Rect (x, y, w, h)); }
    inline void			Bar (coord_t x, coord_t y, dim_t w, dim_t h, wchar_t c = ' ')		{ Bar (Rect (x, y, w, h), c); }
    inline void			HLine (coord_t x, coord_t y, dim_t l)					{ HLine (Point2d (x, y), l); }
    inline void			VLine (coord_t x, coord_t y, dim_t l)					{ VLine (Point2d (x, y), l); }
    inline void			GetImage (coord_t x, coord_t y, dim_t w, dim_t h, canvas_t& cells)	{ GetImage (Rect (x, y, w, h), cells); }
    inline void			Image (coord_t x, coord_t y, dim_t w, dim_t h, const canvas_t& cells)	{ Image (Rect (x, y, w, h), cells); }
    inline void			Image (const CGC& cells)						{ Image (Rect (0, 0, cells.Width(), cells.Height()), cells.Canvas()); }
    inline void			Char (coord_t x, coord_t y, wchar_t c)					{ Char (Point2d (x, y), c); }
    inline void			Text (coord_t x, coord_t y, const string& str)				{ Text (Point2d (x, y), str); }
    inline void			FgColor (EColor c)	{ _template.fg = c; }
    inline void			BgColor (EColor c)	{ _template.bg = c; }
    inline void			Color (EColor fg, EColor bg = color_Preserve)	{ FgColor(fg); BgColor(bg); }
    inline void			AttrOn (EAttribute a)	{ _template.SetAttr (a); }
    inline void			AttrOff (EAttribute a)	{ _template.ClearAttr (a); }
    inline void			AllAttrsOff (void)	{ _template.attrs = 0; }
    inline void			Resize (dim_t x,dim_t y){ Resize (Size2d (x, y)); }
    void			Resize (Size2d sz);
    bool			Clip (Rect& r) const;
    bool			Clip (Point2d& r) const;
    inline void			SetTabSize (size_t nts = 8)	{ assert (nts && "Tab size can not be 0"); _tabSize = nts; }
    bool			MakeDiffFrom (const CGC& src);
private:
    inline canvas_t::iterator		CanvasAt (Point2d p);
    inline canvas_t::const_iterator	CanvasAt (Point2d p) const;
private:
    canvas_t			_canvas;	///< The output buffer.
    CCharCell			_template;	///< Current drawing values.
    Size2d			_size;		///< Size of the output buffer.
    uint32_t			_tabSize;	///< Tab size as expanded by Text
};
} // namespace utio
