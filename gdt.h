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
// gdt.h
//

#ifndef GDT_H_10D99A7949ED8990743247ED3E6897DC
#define GDT_H_10D99A7949ED8990743247ED3E6897DC

#include "ticonst.h"

namespace utio {
/// Contains geometric primitive objects.
namespace gdt {

typedef int16_t			coord_t;	///< A geometric coordinate.
typedef uint16_t		dim_t;		///< A dimension.
typedef tuple<2, dim_t>		Size2d;		///< A geometric size.
typedef tuple<2, coord_t>	Point2d;	///< A geometric point.

/// Represents a geometric rectangle.
class Rect : public tuple<2, Point2d> {
public:
    inline		Rect (const Rect& r)
			    : tuple<2, Point2d> (r) {}
    inline		Rect (coord_t x = 0, coord_t y = 0, dim_t w = 0, dim_t h = 0)
			    { at(0)[0] = x; at(0)[1] = y;
			      at(1)[0] = x + w; at(1)[1] = y + h; }
    inline		Rect (const Point2d& tl, const Point2d& br)
			    { at(0) = tl; at(1) = br; }
    inline		Rect (const Point2d& tl, const Size2d& wh)
			    { at(0) = at(1) = tl; at(1) += wh; }
    inline size_t	Width (void) const		{ return (at(1)[0] - at(0)[0]); }
    inline size_t	Height (void) const		{ return (at(1)[1] - at(0)[1]); }
    inline bool		Empty (void) const		{ return (!Width() || !Height()); }
    inline Size2d	Size (void) const		{ return (at(1) - at(0)); }
    inline const Rect&	operator+= (const Point2d& d)	{ at(0) += d; at(1) += d; return (*this); }
    inline const Rect&	operator-= (const Point2d& d)	{ at(0) -= d; at(1) -= d; return (*this); }
    inline const Rect&	operator+= (const Size2d& d)	{ at(0) += d; at(1) += d; return (*this); }
    inline const Rect&	operator-= (const Size2d& d)	{ at(0) -= d; at(1) -= d; return (*this); }
};

} // namespace gdt
} // namespace usio

#endif

