// This file is part of the utio library, a terminal I/O library.
//
// Copyright (c) 2004 by Mike Sharov <msharov@users.sourceforge.net>
// This file is free software, distributed under the MIT License.

#include "stdmain.h"

//----------------------------------------------------------------------

class CHelloWorld {
private:
    inline	CHelloWorld (void) :_ti() {}
public:
		DECLARE_SINGLETON (CHelloWorld)		// See stdmain.h
    void	Run (void);
private:
    CTerminfo	_ti;
};

//----------------------------------------------------------------------

/// Prints a bunch of multicolored lines.
void CHelloWorld::Run (void)
{
    _ti.Load();		// Just loads the terminfo (using $TERM)

    cout << ios::hex;
    for (int y = black; y < color_Last; ++ y) {
	for (int x = black; x < color_Last; ++ x)
	    cout << _ti.Color (EColor(x), EColor(y)) << x;
	cout << _ti.Color (green, black) << "\tHello World!" << endl;
    }

    cout << _ti.AllAttrsOff();	// Not using Reset() to keep the output onscreen.
}

//----------------------------------------------------------------------

StdDemoMain (CHelloWorld)

//----------------------------------------------------------------------
