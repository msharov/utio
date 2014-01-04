// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#pragma once
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
    inline bool		Empty (void) const		{ return (!Width() | !Height()); }
    inline Size2d	Size (void) const		{ return (at(1) - at(0)); }
    inline void		Expand (coord_t d)		{ at(0) -= d; at(1) += d; }
    inline const Rect&	operator+= (const Point2d& d)	{ at(0) += d; at(1) += d; return (*this); }
    inline const Rect&	operator-= (const Point2d& d)	{ at(0) -= d; at(1) -= d; return (*this); }
    inline const Rect&	operator+= (const Size2d& d)	{ at(0) += d; at(1) += d; return (*this); }
    inline const Rect&	operator-= (const Size2d& d)	{ at(0) -= d; at(1) -= d; return (*this); }
    inline Rect		operator+ (const Point2d& d) const	{ Rect r (*this); r += d; return (r); }
    inline Rect		operator- (const Point2d& d) const	{ Rect r (*this); r -= d; return (r); }
    inline Rect		operator+ (const Size2d& d) const	{ Rect r (*this); r += d; return (r); }
    inline Rect		operator- (const Size2d& d) const	{ Rect r (*this); r -= d; return (r); }
};

} // namespace gdt
} // namespace utio
